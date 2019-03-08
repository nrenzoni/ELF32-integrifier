#ifndef ELF_HEADER_32_H
#define ELF_HEADER_32_H

#include <stdint.h>
#include "main.h" // for externs

int verifyElfHeader();
uint8_t* getSectionFromElf(uint32_t offset_in_section, uint32_t *p_section_size);
uint8_t* getTxtSectionFromElf(uint32_t *p_txt_section_size, uint32_t *p_offset_to_entry);

#endif
