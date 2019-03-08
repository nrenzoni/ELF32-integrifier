#include <stdint.h>

// define hash function prototype: function that accepts (uint8_t* data, uint32_t len) and returns uint32_t (calculated hash)
typedef uint32_t (*hf)(const uint8_t* data, uint32_t len);

// returns 1 if match; otherwise, 0
int hashVerify(const uint8_t *data, uint32_t length, const uint32_t pre_computed_hash, hf hash_func) {
    uint32_t hash_res = hash_func(data, length);
    if (pre_computed_hash == hash_res)
        return 1;
    else
        return 0;
}
