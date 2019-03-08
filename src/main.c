#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check_elf_32.h"
#include "elf_header_32_functions.h"
#include "misc_functions.h"
#include "byte_functions.h"
#include "disas_functions.h"
#include "elf_injection_32.h"
#include "integrity_check_functions.h"

uint8_t *p_file_in_mem;
uint32_t file_in_mem_size;

int main(int argc, char *argv[]) {
    uint32_t txt_section_size;
	uint8_t *p_txt_section;
	uint32_t offset_to_txt_section;
	int8_t output_filename_defined;

	if(argc < 2)
        fatal("usage: <ELF32 file to integrify> [output filename]");

	if(loadFileToMem(argv[1]) < 0)
        fatal("loading file to memory");

    output_filename_defined = (3 == argc) ? 1 : 0;

	// function exits() if not valid ELF 32 file
	verifyElfHeader();

	p_txt_section = getTxtSectionFromElf(&txt_section_size, &offset_to_txt_section);

    if (offset_to_txt_section < 0 || offset_to_txt_section >= file_in_mem_size)
        fatal("offset to .text section out of range");

    // implement: allow user to choose hashing function
    uint32_t txt_section_hashdigest = fastHash(p_txt_section, txt_section_size);

    injectHashCheckBeginningOfTxtSection(offset_to_txt_section, txt_section_size, txt_section_hashdigest);

    // output filename defined as 3rd argument
    if (output_filename_defined) {
        saveFileFromMem(argv[2]);
    }
    else {
        saveFileFromMem(argv[1]);
    }
}
