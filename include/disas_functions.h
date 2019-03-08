#ifndef DISAS_FUNCTIONS_H
#define DISAS_FUNCTIONS_H

#include "main.h"
#include "capstone.h"

#define FOREACH_BRANCH_TYPE(MACRO) \
    MACRO(NONE   ) \
    MACRO(ABS_IMM) \
    MACRO(REL_IMM) \
    MACRO(ABS_REG) \
    MACRO(REL_REG) \
    MACRO(MEM    )

#define GEN_ENUM(VAL) VAL,
#define GEN_STRING(INPUT) #INPUT,

typedef enum {
    FOREACH_BRANCH_TYPE(GEN_ENUM)
} BranchTypeEnum;

static const char* BRANCH_TYPE_STRING[] = {
    FOREACH_BRANCH_TYPE(GEN_STRING)
};

#define BRANCH_INSTRN_FMT \
    "offset val: %ld, offset byte len: %hhu, distance to offset: %u, " \
    "distance to offset dest: %lu, branch type: %s"

#define BRANCH_INSTRN_MEMS(s) \
    (s).offset_val, (s).offset_byte_length, (s).distance_to_offset_from_buf_start, \
    (s).distance_to_offset_destination_from_buf_start, BRANCH_TYPE_STRING[(s).branch_type]

typedef struct {
    int64_t offset_val; // actual offset value can be negative
    uint8_t offset_byte_length;
    uint32_t distance_to_offset_from_buf_start;                 // relative to section
    uint64_t distance_to_offset_destination_from_buf_start;     // relative to section
    BranchTypeEnum branch_type;
} branch_instrn;

void printDisas(uint8_t *data, uint32_t data_len, uint32_t base_address);
void printJMPinstrns(uint8_t *data, uint32_t length, uint32_t base_address);
uint32_t getBranchInstrns(uint8_t *p_section, uint32_t section_length, branch_instrn **pp_branch_instrns);
uint8_t checkBranchRELorABS(cs_insn *insn);

#endif // DISAS_FUNCTIONS_H
