#ifndef INTEGRITY_CHECK_FUNCTIONS_H
#define INTEGRITY_CHECK_FUNCTIONS_H

#include <stdint.h>

// define hash function prototype: function that accepts (uint8_t* data, uint32_t len) and returns uint32_t (calculated hash)
typedef uint32_t (*hf)(const uint8_t* data, uint32_t len);

// calculates hash using hash_function from *data to *(data+length) and compares against pre_computed_hash
// if match, return 1; otherwise, 0
int hashVerify(const uint8_t *data, uint32_t length, uint32_t pre_computed_hash, hf hash_func);

uint32_t fastHash (const uint8_t * data, uint32_t len);

#endif // INTEGRITY_CHECK_FUNCTIONS_H
