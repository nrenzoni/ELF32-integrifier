#include <stdlib.h>     // exit

// template for compiled bytecodes to inject at beginning of .text section of ELF executable file


typedef uint32_t (*hf)(const uint8_t* data, uint32_t len);

static inline uint32_t fastHash (const uint8_t * data, uint32_t len);

void hashVerify(void) {
    hf hash_function = fastHash;

    uint32_t hash_res = hash_function(0, /* offset to beginning of original .text section */
                                      0  /* length of original .text section */);

    if (pre_computed_hash == hash_res)
        return; // jmp to instruct ion was originally at injection point
    else
        exit(1);
}


static inline uint32_t fastHash (const uint8_t * data, uint32_t len) {
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
