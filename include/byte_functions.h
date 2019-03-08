#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <stdio.h>
#include <stdint.h>

void printHex(void *in_buf, int buf_size);
void printByteAsBits(char *val);
void printBytes(void *in_buf, size_t in_buf_size, size_t element_size);

// useful for endian conversion
void reverseBuf(uint8_t *in_buf, int in_buf_size, uint8_t *out_buf);

int32_t byteArrToWord(uint8_t *byte_arr);

char* genHexStr(uint8_t *p_bytes, uint32_t byte_count);
void freeHexStr(char* p_hexStr);

uint8_t* findByteSequence(uint8_t *in_buf, uint32_t len_in_buf, uint8_t *byte_seq, uint32_t len_byte_seq);

int writeBytesFromDW(void *p_dst, uint32_t src_DW, uint8_t bytes_to_write);

int getIndexOfSubstring(char *str, uint32_t str_len, char *substr, uint32_t substr_len);

#endif
