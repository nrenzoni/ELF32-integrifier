#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "check_elf_32.h"
#include "byte_functions.h"

uint32_t getProgramEntry32(uint8_t fd) {
	return get4BytesAsWordFromFile(fd, 24);
}

//uint32_t getSectionTableAdrs32(uint8_t fd) {
//	return get4BytesAsWordFromFile(fd,
//}

void getBytesFromFile(uint8_t fd, unsigned int file_offset, unsigned int byte_count, void *return_buffer) {
	lseek(fd, file_offset, SEEK_SET);
	read(fd, return_buffer, byte_count);
}

// little-endianness
uint32_t get4BytesAsWordFromFile(uint8_t fd, unsigned int file_offset) {
	uint8_t temp_buf[4];
	getBytesFromFile(fd, 24, sizeof(temp_buf), temp_buf);

	// reverse for endianness conversion
	reverseBuf(temp_buf, sizeof(temp_buf), temp_buf);
	return byteArrToWord(temp_buf);
}
