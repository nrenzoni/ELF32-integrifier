#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "byte_functions.h"
#include "misc_functions.h"

void printHex(void *in_buf, int buf_size) {
	for(int i=0; i<buf_size; i++) {
		fprintf(stdout, "%02x%c", ((uint8_t*)in_buf)[i],
			(i+1) % 16 == 0 ? '\n' : ' '); // newline after printing 16 bytes, otherwise space
	}
	fprintf(stdout, "\n");
}

void printByteAsBits(char *val) {
	for(int i=7; i>=0; i--) {
		// !! is either 0 or 1
		printf("%c", !!((uint16_t)(*val) & (1 << i)));
	}
}

// element_size in bytes
void printBytes(void *in_buf, size_t in_buf_size, size_t element_size) {
	// loop over bytes in in_buf from offset 0 to in_buf_size * element_size
	for(size_t i=0; i<in_buf_size * element_size; i++) {
		printByteAsBits((char*)in_buf + i);
		printf("%c", (i+1)%5 == 2 ? '\n' : ' ');
	}
	printf("\n");
}

// safe to use same in_buf as out_buf, only if indices match
void reverseBuf(uint8_t *in_buf, int in_buf_size, uint8_t *out_buf) {
	int middle = in_buf_size / 2; // rounded down if in_buf_size is odd
	char temp;

	for(int i=0, j=in_buf_size-1; i<middle; i++, j--) {
		temp = in_buf[i];
		out_buf[i] = in_buf[j];
		out_buf[j] = temp;
	}
	// if in_buf is of odd size, copy over middle element
	if(in_buf_size % 2 == 1) {
		out_buf[middle] = in_buf[middle];
	}
}

// byte_arr must contain 4 elements
// byte value returned is of format: byte_arr[0] : byte_arr[1] : byte_arr[2] :byte_arr[3]
// using uint8_t (not int8_t) since signed values get auto casted to signed 32 bit word in bit shifting; undisirable behavior
int32_t byteArrToWord(uint8_t *byte_arr) {
	int32_t final = (byte_arr[0] << 24) + (byte_arr[1] << 16) + (byte_arr[2] << 8) + byte_arr[3];
	return final;
}

uint32_t readFileToDynBuf(uint8_t fd) {
	return 0;
}

char* genHexStr(uint8_t *p_bytes, uint32_t byte_count) {
    char *p_byteStr = malloc(byte_count * 2); // each byte takes up 2 chars in hex output
    if(p_byteStr == NULL)
        fatal("allocating memory for hex string");

    for(uint32_t i=0; i<byte_count; i++) {
		sprintf(p_byteStr+i*2, "%02x", ((uint8_t*)p_bytes)[i]);
    }

    return p_byteStr;
}

void freeHexStr(char* p_hexStr) {
    free(p_hexStr);
}

// to implement: find NEXT found occurrence
// returns pointer to beginning of first matching byte-seq in in_buf if found. otherwise NULL
uint8_t* findByteSequence(uint8_t *in_buf, uint32_t len_in_buf, uint8_t *byte_seq, uint32_t len_byte_seq) {
    uint8_t *p_buf_offset = in_buf;

    // iterate over all bytes in in_buf with enough bytes left to compare with byte_seq
    for (; p_buf_offset < in_buf + len_in_buf - len_byte_seq; p_buf_offset++) {

        uint8_t match = 1;

        // compare byte_seq
        for (uint32_t i=0; i< len_byte_seq; i++) {
            if (p_buf_offset[i] != byte_seq[i]) {
                match = 0;
                break;
            }
        } // end compare byte_seq

        if (match) {
            return p_buf_offset;
        }
    } // end iterate over all bytes in in_buf

    return NULL; // no match found
}

// writes a uint32_t to p_dst; bytes to write can be 1,2, or 4; endian conversion is performed if necessary depending on arch
// return status; 1 on success, 0 on failure
int writeBytesFromDW(void *p_dst, uint32_t src_DW, uint8_t bytes_to_write) {
    switch(bytes_to_write) {
    case 1:
        *(uint8_t*)p_dst = (uint8_t)(src_DW);
        break;

    case 2:
        *(uint16_t*)p_dst = (uint16_t)(src_DW);
        break;

    case 4:
        *(uint32_t*)p_dst = (uint32_t)(src_DW);
        break;

    default:
        fatal("replacing DW of branch instrn; offset not of size 1,2, or 4");
        return 0;
    }

    // success
    return 1;
}

// returns index in str of matching substring with str if exists. otherwise returns -1
int getIndexOfSubstring(char *str, uint32_t str_len, char *substr, uint32_t substr_len) {
    if (substr_len > str_len)
        return -1;

    for (uint32_t i=0; i< str_len - substr_len; i++ ) {

        if (str[i] == substr[0]) { // first char of substr matches

            uint32_t j = 1;

            for (; j < substr_len; j++) {

                if (str[i+j] != substr[j])
                    break;
            } // a match if all j's looped over

            if (j == substr_len) { // all chars in substr match
                return i; // index of first char in str that contains match with substr
            }
        }
    }

    return -1; // no match
}





