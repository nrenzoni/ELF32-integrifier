#include <stdint.h>
#include <stdlib.h>     // qsort
#include <string.h>     // memcpy

#include "elf_injection_32.h"
#include "elf_header_32_functions.h"
#include "disas_functions.h"
#include "misc_functions.h"
#include "main.h"
#include "capstone.h"
#include "byte_functions.h"
#include "elf_common.h"
#include "elf32.h"

// returns new buffer size on success, 0 on failure
// on success, p_file_in_mem will point to new buffer containing injected bytes
uint32_t injectBytesInFile(injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count) {

    // sum of all injection byte sizes
    uint32_t file_size_addend = 0;

    for (uint32_t i=0; i<inject_stat_arr_count; ++i) {
        if(inject_stat_arr[i].offset <= 0 || inject_stat_arr[i].offset >= file_in_mem_size)
            fatal("offset to inject point is not within the currently loaded file's range");
        file_size_addend += inject_stat_arr[i].length;
    }

    // find which sections exist in which segments (segments are extracted from program headers)
    section_to_segment_mapping *p_segment_section_mapping = populateSectionSegmentMapping();

    // find jmp and call instrns for correction of destination/offset (ideally should be done in all executable sections):
    fixBranchInstrns (inject_stat_arr, inject_stat_arr_count);

    // fix reloc and dyn-reloc tables
    fixRelocs        (inject_stat_arr, inject_stat_arr_count);

    fixDynamicSection(inject_stat_arr, inject_stat_arr_count);

    // fix offsets and section sizes in section headers
    fixSectionHeaders(inject_stat_arr, inject_stat_arr_count);

    // adjust program headers to re-sized sections which exist in program headers' segments
    fixProgramHeaders(inject_stat_arr, inject_stat_arr_count, p_segment_section_mapping);

    freeSectionToSegmentMapping(&p_segment_section_mapping);

    // fix pointers in elf header
    fixElfHeader     (inject_stat_arr, inject_stat_arr_count);


    uint8_t *p_file_in_mem_new = malloc( file_in_mem_size + file_size_addend );
    if (NULL == p_file_in_mem_new)
        fatal("allocating new memory");

    // sort injection_stat_arr by offset
    qsort(inject_stat_arr, inject_stat_arr_count, sizeof(injection_stat), inject_stat_cmp_helper);

    uint32_t cur_stat_inject_offset, cur_stat_inject_len, old_file_cp_len, old_file_cur_offset = 0;
    uint8_t *p_old_file_seek = p_file_in_mem,
            *p_new_file_seek = p_file_in_mem_new;

    for (uint32_t i=0; i<inject_stat_arr_count; ++i) {

        // relative to old file's buffer
        cur_stat_inject_offset  = inject_stat_arr[i].offset;
        cur_stat_inject_len     = inject_stat_arr[i].length;
        old_file_cp_len         = cur_stat_inject_offset - old_file_cur_offset;

        // copy from old file up to inject offset into new buffer
        memcpy(p_new_file_seek, p_old_file_seek, old_file_cp_len);
        old_file_cur_offset += old_file_cp_len;
        p_old_file_seek     += old_file_cp_len;
        p_new_file_seek     += old_file_cp_len;

        // copy inject bytes into new buffer
        // possible that a injection buffer will overwrite another. the one with the larger offset will be written lastly
        memcpy(p_new_file_seek, inject_stat_arr[i].p_inject_buf, cur_stat_inject_len);
        p_new_file_seek += cur_stat_inject_len;

    }


    // copy remaining bytes from old file
    memcpy(p_new_file_seek, p_old_file_seek, file_in_mem_size - old_file_cur_offset);

    free(p_file_in_mem);
    p_file_in_mem = p_file_in_mem_new;

    file_in_mem_size += file_size_addend;

    return file_in_mem_size;
}

int inject_stat_cmp_helper(const void *right, const void *left) {
    return  ((injection_stat*)right)->offset - ((injection_stat*)left)->offset;
}

// calls fixBranchInstrnsSingleInjection for each injection
void fixBranchInstrns(injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count) {
    for (uint32_t i=0; i<inject_stat_arr_count; ++i) {
        fixBranchInstrnsSingleInjection(inject_stat_arr[i].offset, inject_stat_arr[i].length);
    }
}

// recalculates offsets/addresses in branches that need to be fixed before performing byte injection
// and writes to p_file_in_mem
void fixBranchInstrnsSingleInjection(uint32_t offset_to_injection, uint32_t injection_bytes_size) {
    uint32_t      inject_section_size;
    uint8_t       *p_section_of_injection;
    uint32_t      number_of_branch_instrns;
    branch_instrn *p_branch_instrns;

    // improve: go through all sections that are executable and fix jumps/calls if necessary
    if((p_section_of_injection = getSectionFromElf(offset_to_injection, &inject_section_size)) == NULL)
        fatal("finding section containing offset to injection point");


    number_of_branch_instrns = getBranchInstrns(p_section_of_injection, inject_section_size, &p_branch_instrns);

    // only if current section is executable:
    // if(isExecutableSection(p_section) == NOT_EXECUTABLE)
    //      fatal("injecting code in non-executable section in ELF file");


    // loop over all branch instrns
    for (uint32_t i=0; i < number_of_branch_instrns; i++) {

        // if write thru is required
        uint8_t write_thru = 0;

        // if offset is relative, need to check that dist_to_beg_offset_loc +/- offset doesn't cross inject point boundary

        // if branch has absolute address offset stored encoded as immediate
        if (ABS_IMM == p_branch_instrns[i].branch_type) {
            // distance to operand containing offset is below inject location, and actual offset location is above inject point
            if (p_branch_instrns[i].distance_to_offset_from_buf_start < offset_to_injection
                && p_branch_instrns[i].offset_val > offset_to_injection) {

                p_branch_instrns[i].offset_val += injection_bytes_size;
                write_thru = 1;
            }

            // distance to operand containing offset is above or equal to inject location, and actual offset location is below inject point
            else if (p_branch_instrns[i].distance_to_offset_from_buf_start >= offset_to_injection
                && p_branch_instrns[i].offset_val < offset_to_injection) {

                p_branch_instrns[i].offset_val -= injection_bytes_size;
                write_thru = 1;
            }
        } // end if current branch has absolute offset encoded as ABS IMM

        else if (REL_IMM == p_branch_instrns[i].branch_type) {

            uint32_t dist_to_offset_dest = p_branch_instrns[i].distance_to_offset_from_buf_start + p_branch_instrns[i].offset_val;

            // offset destination is before injection point and encoded offset is after injection point
            if (dist_to_offset_dest < offset_to_injection
                && p_branch_instrns[i].distance_to_offset_from_buf_start > offset_to_injection) {

                p_branch_instrns[i].offset_val -= injection_bytes_size;
                write_thru = 1;
            }

            // offset destination is after injection point and encoded offset is before injection point
            else if (dist_to_offset_dest >= offset_to_injection
                     && p_branch_instrns[i].distance_to_offset_from_buf_start < offset_to_injection) {

                p_branch_instrns[i].offset_val += injection_bytes_size;
                write_thru = 1;
            }

        } // end if cur branch instrn offset encoded as REL IMM

        else {
            printf("skipped branch insn not encoded with IMM in fixBranchInstrns()\n");
        }


        // write re-calculated offsets to p_file_in_mem
        if (write_thru) {

            void* p_dst = p_section_of_injection + p_branch_instrns[i].distance_to_offset_from_buf_start;

            if(0 == writeBytesFromDW(p_dst, p_branch_instrns[i].offset_val, p_branch_instrns[i].offset_byte_length))
                fatal("writing bytes from DW to p_file_in_mem");

        } // end write-thru

    } // end loop over all branch instrns

    free(p_branch_instrns);
}

// fixes the following fields in elf header if needed:
// entry point address, start of program headers, start of section headers
//
// needs to be fixed after injection in elf file since program headers, sections, and section headers can come in any order in elf file.
// and pointers from elf header to different sections may need to be re-adjusted
// writes corrections to p_file_in_mem
void fixElfHeader(injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count) {

    Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;

    uint32_t entry_point            = elf_header_32->e_entry;
    uint32_t section_headers_offset = elf_header_32->e_shoff;
    uint32_t program_headers_offset = elf_header_32->e_phoff;

    uint32_t entry_p_additive   = 0;
    uint32_t section_h_additive = 0;
    uint32_t program_h_additive = 0;

    for (uint32_t i=0; i<inject_stat_arr_count; ++i) {

        uint32_t cur_offset = inject_stat_arr[i].offset;
        uint32_t cur_len    = inject_stat_arr[i].length;

        // correct entry point
        if (cur_offset < entry_point) {
            entry_p_additive += cur_len;
        }

        // correct offset to section headers
        if (cur_offset < section_headers_offset) {
            section_h_additive += cur_len;
        }

        // correct offset to program headers
        if (cur_offset < program_headers_offset) {
            program_h_additive += cur_len;
        }

    } // end for loop  over inject_stat_array

    elf_header_32->e_entry += entry_p_additive;
    elf_header_32->e_shoff += section_h_additive;
    elf_header_32->e_phoff += program_h_additive;
}

// fix relocs and dynamic-relocs to point at corrected location
void fixRelocs(injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count) {

#define SHT_RELA 4
#define SHT_REL  9
#define RELOC_ENTRY_SIZE  sizeof(Elf32_Rel)
#define RELOCA_ENTRY_SIZE sizeof(Elf32_Rela)

    Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;
    uint32_t offset_to_section_headers = elf_header_32->e_shoff;
    uint32_t num_of_sections = elf_header_32->e_shnum;
    uint32_t section_header_size = elf_header_32->e_shentsize;
    uint8_t is_section_rela;
    uint32_t offset_to_cur_section;
    uint32_t num_relocs;

    // find reloc section headers
    for (uint32_t i_section=0; i_section < num_of_sections; ++i_section) {

        Elf32_Shdr *p_cur_section_header = (Elf32_Shdr*) ( p_file_in_mem + offset_to_section_headers + i_section * section_header_size );
        uint32_t cur_section_type = p_cur_section_header->sh_type;

        // section contains relocation entries
        if (SHT_REL == cur_section_type || SHT_RELA == cur_section_type) {

            is_section_rela = (SHT_RELA == cur_section_type) ? 1 : 0;

            offset_to_cur_section = p_cur_section_header->sh_offset; // offset relative to file

            // uint32_t cur_section_alignment = p_cur_section_header->sh_addralign;

            // section alignment is for loading program to memory, not in file on hdd storage
//            uint32_t modulo_val;
//            // align offset to section
//            if ((modulo_val = offset_to_cur_section % cur_section_alignment) != cur_section_alignment) {
//                offset_to_cur_section += modulo_val;
//            }

            num_relocs = p_cur_section_header->sh_size /
                ((is_section_rela) ? RELOCA_ENTRY_SIZE : RELOC_ENTRY_SIZE);

            // loop over relocs in section
            for (uint32_t i_reloc=0; i_reloc < num_relocs; ++i_reloc) {

                // disregard addend even if applicable, since no change necessary to addend

                Elf32_Rel *p_cur_reloc_entry = (Elf32_Rel*) ( p_file_in_mem + offset_to_cur_section +
                    i_reloc * ((is_section_rela) ? RELOCA_ENTRY_SIZE : RELOC_ENTRY_SIZE) );

                uint32_t cur_reloc_entry_offset_addend = 0;

                // loop over inject stat arr
                for (uint32_t i_inject_stat_arr = 0; i_inject_stat_arr < inject_stat_arr_count; ++i_inject_stat_arr) {

                    // injection is before offset to relocation destination
                    if (inject_stat_arr[i_inject_stat_arr].offset < p_cur_reloc_entry->r_offset) {
                        cur_reloc_entry_offset_addend += inject_stat_arr[i_inject_stat_arr].length;
                    }
                }

                p_cur_reloc_entry->r_offset = cur_reloc_entry_offset_addend;

            } // end loop over relocs in current section

        } // end current section is reloc(a)

    } // end loop over all sections

}

void fixDynamicSection(injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count) {

    Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;

    uint32_t section_headers_offset = elf_header_32->e_shoff;
    uint32_t num_section_headers    = elf_header_32->e_shnum;
    uint32_t section_header_size    = elf_header_32->e_shentsize;

    Elf32_Shdr *p_dyn_shdr;

    // first section is null
    // find .dynamic section
    uint32_t i_shdr;
    for (i_shdr=1; i_shdr < num_section_headers; ++i_shdr) {

        p_dyn_shdr = (Elf32_Shdr*)(p_file_in_mem + section_headers_offset + i_shdr * section_header_size);

        if (SHT_DYNAMIC == p_dyn_shdr->sh_type)
            break;
    }
    // i_shdr should now contain index of .dynamic section, as well as p_dyn_shdr points to .dynamic section header

    if (i_shdr == num_section_headers)
        fatal(".dynamic section not found in elf file");

    uint32_t dyn_entry_size = sizeof(Elf32_Dyn);
    uint32_t dyn_arr_count = p_dyn_shdr->sh_size / dyn_entry_size;

    Elf32_Dyn *p_cur_dyn_etry = (Elf32_Dyn*)(p_file_in_mem + p_dyn_shdr->sh_offset);

    // loop over dynamic entries
    for (uint32_t i=0; i < dyn_arr_count; ++i, p_cur_dyn_etry += 1) {

        Elf32_Sword tag = p_cur_dyn_etry->d_tag;

        // d_un in entries of these types is d_ptr; needs correction
        if (DT_PLTGOT == tag || DT_HASH == tag || DT_STRTAB == tag || DT_SYMTAB == tag || DT_RELA == tag ||
            DT_INIT == tag || DT_FINI == tag || DT_REL == tag || DT_DEBUG == tag || DT_JMPREL == tag ||
            DT_INIT_ARRAY == tag || DT_FINI_ARRAY == tag || DT_PREINIT_ARRAY == tag) {

            Elf32_Addr *p_d_ptr = &(p_cur_dyn_etry->d_un.d_ptr);

            uint32_t cur_pnt_addend = 0;

            // loop over inject stat array
            for (uint32_t i_inject_stat=0; i_inject_stat < inject_stat_arr_count; ++i_inject_stat) {

                uint32_t cur_inject_stat_offset = inject_stat_arr[i_inject_stat].offset;
                uint32_t cur_inject_stat_length = inject_stat_arr[i_inject_stat].length;

                // injection is before current dyn pointer
                if (cur_inject_stat_offset < *p_d_ptr) {
                    cur_pnt_addend += cur_inject_stat_length;
                }

            } // end loop over inject stat arr

            *p_d_ptr += cur_pnt_addend;

        } // end dyn entry contains d_ptr

    } // end loop over dynamic entries

}

void fixSectionHeaders(injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count) {

    Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;

    uint32_t section_headers_offset = elf_header_32->e_shoff;
    uint32_t num_section_headers    = elf_header_32->e_shnum;
    uint32_t section_header_size    = elf_header_32->e_shentsize;

    // loop over section headers
    // skip first section header since it's null
    for (uint32_t i_shdr=1; i_shdr < num_section_headers; ++i_shdr) {

        Elf32_Shdr *p_cur_shdr = (Elf32_Shdr*)(p_file_in_mem + section_headers_offset + i_shdr * section_header_size);

        uint32_t cur_shr_offset_addend = 0;
        uint32_t cur_shr_size_addend   = 0;

        // loop over inject stat array
        for (uint32_t i_inject_stat=0; i_inject_stat < inject_stat_arr_count; ++i_inject_stat) {

            uint32_t cur_inject_stat_offset = inject_stat_arr[i_inject_stat].offset;
            uint32_t cur_inject_stat_length = inject_stat_arr[i_inject_stat].length;

            // injection is before current section
            if (cur_inject_stat_offset < p_cur_shdr->sh_offset) {
                cur_shr_offset_addend += cur_inject_stat_length;
            }

            uint32_t begin_cur_section = p_cur_shdr[i_inject_stat].sh_offset;
            uint32_t end_cur_section   = begin_cur_section + p_cur_shdr[i_inject_stat].sh_size;

            // injection is within current section
            if (cur_inject_stat_offset >= begin_cur_section &&
                cur_inject_stat_offset + cur_inject_stat_length <= end_cur_section) {

                cur_shr_size_addend += cur_inject_stat_length;
            }

        } // end loop over inject stat arr

        p_cur_shdr->sh_offset += cur_shr_offset_addend;

        // if section virtual address == 0, not mapped into memory
        if (0 == p_cur_shdr->sh_addr)
            continue;

        // calculate virtual address for section base

        // first section, previous one to compare with
        if (0 == i_shdr) {
            p_cur_shdr->sh_addr = p_cur_shdr->sh_offset;
        }
        else { // exists previous section; sh_addr = max(sh_offset, prev_shr->sh_addr + prev_shr->sh_size)

            Elf32_Shdr *p_prev_shdr = p_cur_shdr - 1; // 1 == sizeof(Elf32_Shdr)

            uint32_t prev_shdr_last_addrs = p_prev_shdr->sh_addr + p_prev_shdr->sh_size;

            if (p_cur_shdr->sh_offset >= prev_shdr_last_addrs) {
                p_cur_shdr->sh_addr = p_cur_shdr->sh_offset;
            }
            else {
                p_cur_shdr->sh_addr = prev_shdr_last_addrs;
            }
        }

        // make address congruent to 0 (mod sh_addralign)
        if (p_cur_shdr->sh_addralign != 0 && p_cur_shdr->sh_addralign != 1)
            p_cur_shdr->sh_addr = (uint32_t)(p_cur_shdr->sh_addr * 1.0 / p_cur_shdr->sh_addralign + 0.5) * p_cur_shdr->sh_addralign;

        p_cur_shdr->sh_size  += cur_shr_size_addend;

    } // end loop over section headers
}

// must be called after fixSectionHeaders
void fixProgramHeaders(injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count, section_to_segment_mapping *p_segment_section_mappings) {

    Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;

    uint32_t prog_headers_offset = elf_header_32->e_phoff;
    uint32_t num_prog_headers    = elf_header_32->e_phnum;
    uint32_t prog_header_size    = elf_header_32->e_phentsize;

    uint32_t section_headers_offset = elf_header_32->e_shoff;
    uint32_t num_of_sections        = elf_header_32->e_shnum;
    uint32_t section_header_size    = elf_header_32->e_shentsize;

    // loop over program headers
    for (uint32_t i_phdr=0; i_phdr < num_prog_headers; ++i_phdr) {

        // if no sections mapped to current segment
        if ( 0 == p_segment_section_mappings[i_phdr].num_of_sections )
            continue;

        Elf32_Phdr *p_cur_phdr = (Elf32_Phdr*)(p_file_in_mem + prog_headers_offset + i_phdr * prog_header_size);

        uint32_t *p_cur_segment_sections = p_segment_section_mappings[i_phdr].p_sections_in_segment;

        uint32_t min_offset_cur_segment  = UINT32_MAX;
        uint32_t max_offset_cur_segment  = 0;
        uint32_t min_address_cur_segment = UINT32_MAX;
        uint32_t max_address_cur_segment = 0;

        // loop over all sections to find sections that exist in current program's segment to calculate min and max address in segment
        for (uint32_t i_section=1, discovered=0;
             i_section < num_of_sections && discovered < p_segment_section_mappings[i_phdr].num_of_sections;
             ++i_section) {

            Elf32_Shdr *p_cur_shdr = (Elf32_Shdr*)(p_file_in_mem + section_headers_offset + i_section * section_header_size);

            // current section exists in segment
            if ( 1 == p_cur_segment_sections[i_section] ) {

                discovered++;

                // min offset
                if (p_cur_shdr->sh_offset < min_offset_cur_segment)
                    min_offset_cur_segment = p_cur_shdr->sh_offset;

                // min address
                if (p_cur_shdr->sh_addr < min_address_cur_segment)
                    min_address_cur_segment = p_cur_shdr->sh_addr;

                // max offset
                if (p_cur_shdr->sh_offset + p_cur_shdr->sh_size > max_offset_cur_segment)
                    max_offset_cur_segment = p_cur_shdr->sh_offset + p_cur_shdr->sh_size;

                // max address
                if (p_cur_shdr->sh_addr + p_cur_shdr->sh_size > max_address_cur_segment)
                    max_address_cur_segment = p_cur_shdr->sh_addr + p_cur_shdr->sh_size;

            }

        } // end loop over all sections to find sections that exist in current program's segment


        // virtual address = max(offset, prev_segment's virtual address + prev_segment's mem_size)
        // if virt address = prev_segment's virtual address + prev_segment's mem_size, need to make congruent with offset (mod align)

        // if p_offset == 0, don't change offset or virt address
        if (0 == p_cur_phdr->p_offset) {
            min_offset_cur_segment = 0;
            min_address_cur_segment = 0;
        }
        else {
            // correct current segment's offset and address
            p_cur_phdr->p_offset = min_offset_cur_segment;
            p_cur_phdr->p_vaddr  = min_address_cur_segment;
        }

        // needed for adding back on to mem size after adjusting
        uint32_t mem_size_file_size_delta = p_cur_phdr->p_memsz - p_cur_phdr->p_filesz;

        uint32_t cur_segment_offset_delta = max_offset_cur_segment - min_offset_cur_segment;
        uint32_t cur_segment_address_delta = max_address_cur_segment - min_address_cur_segment;

        if (cur_segment_offset_delta < p_cur_phdr->p_filesz)
            fatal("segment size shrank in size");

        // correct current segment's filesize, memsize, and physical address
        p_cur_phdr->p_filesz = cur_segment_address_delta;
        p_cur_phdr->p_memsz  = cur_segment_address_delta + mem_size_file_size_delta;

        // unused field in elf according to: http://www.sco.com/developers/gabi/2003-12-17/ch5.pheader.html
        p_cur_phdr->p_paddr = p_cur_phdr->p_vaddr;

    } // end loop over program headers
}

// returns arr of section_to_segment_mapping of number of program headers
// called before any fixes are made
section_to_segment_mapping* populateSectionSegmentMapping(void) {
    Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;

    uint32_t prog_headers_offset = elf_header_32->e_phoff;
    uint32_t num_prog_headers    = elf_header_32->e_phnum;
    uint32_t prog_header_size    = elf_header_32->e_phentsize;

    uint32_t section_headers_offset = elf_header_32->e_shoff;
    uint32_t num_section_headers    = elf_header_32->e_shnum;
    uint32_t section_header_size    = elf_header_32->e_shentsize;

    section_to_segment_mapping *p_mappings = malloc( num_prog_headers * sizeof(section_to_segment_mapping) );

    // loop over program headers
    for (uint32_t i_phdr=0; i_phdr < num_prog_headers; ++i_phdr) {

        Elf32_Phdr *p_cur_phdr = (Elf32_Phdr*)(p_file_in_mem + prog_headers_offset + i_phdr * prog_header_size);

        p_mappings[i_phdr].i_segment = i_phdr;
        p_mappings[i_phdr].num_of_sections = 0;
        p_mappings[i_phdr].p_sections_in_segment = calloc( 1, num_section_headers * sizeof(uint32_t) ); // initialize all to \x00 with calloc

        // loop over section headers; 1st one is null
        for (uint32_t i_section=1; i_section < num_section_headers; ++i_section) {

            Elf32_Shdr *p_cur_section_header = (Elf32_Shdr*) ( p_file_in_mem + section_headers_offset + i_section * section_header_size );

            // segment contains current section
            if ((p_cur_section_header->sh_offset >= p_cur_phdr->p_offset) &&
                (p_cur_section_header->sh_offset + p_cur_section_header->sh_size <= p_cur_phdr->p_offset + p_cur_phdr->p_memsz)) {

                p_mappings[i_phdr].p_sections_in_segment[i_section] = 1; // current section is in current segment
                p_mappings[i_phdr].num_of_sections += 1;
            } // otherwise, p_mappings[i_phdr].p_sections_in_segment[i_section] = 0 from calloc initialization

        } // end loop over section headers

    } // end loop over program headers

    return p_mappings;
}

// double pointer for setting *pp_mapping to NULL
void freeSectionToSegmentMapping(section_to_segment_mapping **pp_mapping) {

    section_to_segment_mapping *p_mapping = *pp_mapping;
    Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;
    uint32_t num_segments     = elf_header_32->e_phnum;

    for (uint32_t i=0; i<num_segments; ++i) {
        free( p_mapping[i].p_sections_in_segment );
    }

    free(p_mapping);
    *pp_mapping = NULL;
}



int injectHashCheckBeginningOfTxtSection(uint32_t offset_to_start_of_txt_section, uint32_t txt_section_size, uint32_t hash_of_txt_section) {
    // generated from file: pre_hash_function_code.asm using nasm
    // idea: use keystone to generate assembly during runtime
    char before_hash_func_bytecode[] =
    "\x66\xe8\x00\x00\x58\x83\xc0\x23\x05\x10\x32\x00\x00\x68\x34\x12\x00\x00\x50\x66\xe8\x16\x00\x83\xc4\x08\x3d\x76\x98\x00\x00\x0f"
    "\x84\x26\x01\x00\x00\x31\xc0\xb0\x01\x31\xdb\xcd\x80";

    uint8_t before_hash_bytecode_len = sizeof(before_hash_func_bytecode) / sizeof(before_hash_func_bytecode[0]);

    // fasthash from above. compiled into bytecode for x86-32 using scripts in bash_scripts directory (gcc -m32 -O3)
    // length = 272 bytes
    char hash_func_bytecodes[] =
    "\x57\x56\x53\x8b\x44\x24\x14\x8b\x54\x24\x10\x85\xc0\x0f\x84\x8d\x00\x00\x00\x85\xd2\x0f\x84\x85\x00\x00"
    "\x00\x89\xc1\x89\xc3\xc1\xe9\x02\x83\xe3\x03\x85\xc9\x0f\x84\x9b\x00\x00\x00\x8d\x0c\x8a\x0f\xb7\x32\x83\xc2\x04\x01\xc6\x0f\xb7"
    "\x42\xfe\x89\xf7\xc1\xe7\x10\xc1\xe0\x0b\x31\xf8\x31\xf0\x89\xc6\xc1\xee\x0b\x01\xf0\x39\xca\x75\xdd\x83\xfb\x02\x0f\x84\x9c\x00"
    "\x00\x00\x83\xfb\x03\x74\x6f\x83\xfb\x01\x74\x4a\x8d\x14\xc5\x00\x00\x00\x00\x5b\x31\xd0\x89\xc2\xc1\xea\x05\x01\xd0\x89\xc2\xc1"
    "\xe2\x04\x31\xd0\x89\xc2\xc1\xea\x11\x01\xc2\x89\xd0\xc1\xe0\x19\x31\xc2\x89\xd0\xc1\xe8\x06\x01\xd0\x5e\x5f\xc3\x8d\x76\x00\x8d"
    "\xbc\x27\x00\x00\x00\x00\x5b\x31\xc0\x5e\x5f\xc3\x8d\x76\x00\x8d\xbc\x27\x00\x00\x00\x00\x0f\xbe\x11\x01\xc2\x89\xd0\xc1\xe0\x0a"
    "\x31\xd0\x89\xc2\xd1\xea\x01\xd0\xeb\xa2\x8d\x74\x26\x00\x89\xd1\xeb\x87\x8d\x74\x26\x00\x0f\xb7\x11\x01\xd0\x89\xc2\xc1\xe2\x10"
    "\x31\xc2\x0f\xbe\x41\x02\xc1\xe0\x12\x31\xd0\x89\xc2\xc1\xea\x0b\x01\xd0\xe9\x75\xff\xff\xff\x8d\xb4\x26\x00\x00\x00\x00\x0f\xb7"
    "\x11\x01\xc2\x89\xd0\xc1\xe0\x0b\x31\xd0\x89\xc2\xc1\xea\x11\x01\xd0\xe9\x56\xff\xff\xff";

    uint32_t hash_func_bytecode_len = sizeof(hash_func_bytecodes) / sizeof(hash_func_bytecodes[0]);

    /*

    necessary corrections to perform on before_hash_func_bytecode:

    replace 0x3210 in add eax, 0x3210   -> sizeof(hash function bytecodes)
    replace 0x1234 in push 0x1234       -> sizeof(.text section)
    replace 0x9876 cmp eax, 0x9876      -> hash digest of .text section
    replace $+300 in je $+300           -> remaining bytes in before_hash_func_bytecode after je +  hash_func_bytecode_len

    */

    int i_of_0x3210 = getIndexOfSubstring(before_hash_func_bytecode, before_hash_bytecode_len, "\x05\x10\x32\x00\x00", 5);
    if(-1 == i_of_0x3210)
        fatal("");
    i_of_0x3210 += 2; // offset to imm 0x3210
    before_hash_func_bytecode[i_of_0x3210] = (uint32_t)hash_func_bytecode_len; // 32 bit little endian

    int i_of_0x1234 = getIndexOfSubstring(before_hash_func_bytecode, before_hash_bytecode_len, "\x68\x34\x12\x00\x00", 5);
    if (-1 == i_of_0x1234)
        fatal("");
    i_of_0x1234 += 1;
    before_hash_func_bytecode[i_of_0x1234] = (uint32_t)txt_section_size; // 32 bit little endian

    int i_of_0x9876 = getIndexOfSubstring(before_hash_func_bytecode, before_hash_bytecode_len, "\x3d\x76\x98\x00\x00", 5);
    if (-1 == i_of_0x9876)
        fatal("");
    i_of_0x9876 += 1;
    before_hash_func_bytecode[i_of_0x9876] = (uint32_t)hash_of_txt_section; // 32 bit little endian


    int i_of_je_to_entry_point = getIndexOfSubstring(before_hash_func_bytecode, before_hash_bytecode_len, "\x0f\x84\x26\x01\x00\x00", 6);
    if (-1 == i_of_je_to_entry_point)
        fatal("");
    uint32_t je_to_end_of_pre_hash_bytecode_dist = before_hash_bytecode_len - (i_of_je_to_entry_point + 6);
                                                // 2 bytes for 0f 84 for JZ REL32 opcode
    before_hash_func_bytecode[i_of_je_to_entry_point + 2] =
        (uint32_t) ( je_to_end_of_pre_hash_bytecode_dist + hash_func_bytecode_len ); // offset to original entry point
        // JZ REL32 is relative to following instruction after JZ

    uint32_t inject_buf_len = before_hash_bytecode_len + hash_func_bytecode_len;
    uint8_t *p_inject_buffer = malloc(inject_buf_len);
    if (NULL == p_inject_buffer)
        fatal("allocating buffer");

    memcpy(p_inject_buffer, before_hash_func_bytecode, before_hash_bytecode_len);
    memcpy(p_inject_buffer + before_hash_bytecode_len, hash_func_bytecodes, hash_func_bytecode_len);

    injection_stat inject_stat;
    inject_stat.offset       = offset_to_start_of_txt_section;
    inject_stat.length       = inject_buf_len;
    inject_stat.p_inject_buf = p_inject_buffer;

    injectBytesInFile(&inject_stat, 1);

    free(p_inject_buffer);
    return 1;
}



