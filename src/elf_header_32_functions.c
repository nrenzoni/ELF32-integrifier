#include <elf_common.h>
#include <stdlib.h>     /* malloc, free */
#include <stdio.h>      /* printf 	*/
#include <unistd.h>	/* read, exit	*/

#include "elf_header_32_functions.h"
#include "byte_functions.h"
#include "misc_functions.h"
#include "elf_common.h"
#include "elf32.h"

// file seek should already be at 0
int verifyElfHeader() {
	Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;

	if( file_in_mem_size < sizeof(Elf32_Ehdr) )
        fatal("file is smaller than elf 32 header size");

	if( !IS_ELF(*elf_header_32) )
        fatal("not a ELF file");

	if( elf_header_32->e_ident[EI_CLASS] !=  ELFCLASS32 )
        fatal("support is only for 32 bit ELFs");

	if(!( elf_header_32->e_type == ET_REL || elf_header_32->e_type == ET_EXEC || elf_header_32->e_type == ET_DYN ))
        fatal("ELF is not an executable");

	// all tests pass
	return 1;
}

void printElfHeader(Elf32_Ehdr *header) {
	printHex(header, sizeof(Elf32_Ehdr));
}

// finds section containing a given offset
// returns pointer to section and size of section in section_size; otherwise NULL if not found
// offset_in_section must be relative to 0, not to p_file_in_mem
uint8_t* getSectionFromElf(uint32_t offset_in_section, uint32_t *p_section_size) {
    Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;

	uint32_t  e_shoff = elf_header_32->e_shoff; 	 /* section header offset */
	if(e_shoff == 0)
        fatal("file does not contain section header table");

	Elf32_Half e_shnum = elf_header_32->e_shnum;		 /* number of section header entries */
	uint32_t e_shentsize = elf_header_32->e_shentsize; /* Size of section header entry. */

	// loop over all sections, find section header of section containing offset_in_section
	Elf32_Shdr *cur_section;
	for(uint32_t i=0; i<e_shnum; i++) {
        cur_section = (Elf32_Shdr*)( p_file_in_mem + e_shoff + (i * e_shentsize));

        // if sh_addr == 0, section does not lie in virtual memory
        if(cur_section->sh_addr != 0)
            // entry point lies in current section's virtual memory range
            if(offset_in_section >= cur_section->sh_addr
               && offset_in_section <= cur_section->sh_addr + cur_section->sh_entsize)
                break;

        if(i == e_shnum - 1) // didn't find section containing offset address
            fatal("didn't find elf section containing offset address");
	}

	*p_section_size = cur_section->sh_size;

	// pointer to section containing offset address
	return p_file_in_mem + cur_section->sh_addr;
}

// returns pointer to txt_section, or NULL if not found, and section's size returned in p_txt_section_size param
uint8_t* getTxtSectionFromElf(uint32_t *p_txt_section_size, uint32_t *p_offset_to_entry) {
	// prog entry offset (virtual address)
	Elf32_Ehdr *elf_header_32 = (Elf32_Ehdr*) p_file_in_mem;
	Elf32_Addr entry_point = elf_header_32->e_entry;

	if(entry_point == 0)
        fatal("elf file does not contain entry point"); // and therefore, no executable section

    *p_offset_to_entry = entry_point;
    return getSectionFromElf(entry_point, p_txt_section_size);
}


