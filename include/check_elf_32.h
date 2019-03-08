#ifndef CHECK_ELF_32_H
#define CHECK_ELF_32_H

#include <stdint.h>

// exits if fd not 32 bit ELF executable
void checkElf32Bit(uint8_t fd, char *filename);
uint32_t getProgramEntry32(uint8_t fd);
uint32_t getSectionTableAdrs32(uint8_t fd);
void getBytesFromFile(uint8_t fd, unsigned int file_offset, unsigned int byte_count, void *return_buffer);
uint32_t get4BytesAsWordFromFile(uint8_t fd, unsigned int file_offset);

#endif
