#ifndef ELF_INJECTION_32_H
#define ELF_INJECTION_32_H

// type to store offset and length of each injection
typedef struct {
    uint32_t offset;
    uint32_t length;
    uint8_t  *p_inject_buf;
} injection_stat;

// used in fix* functions
// contains offset to element to correct, and its value
typedef struct {
    uint32_t offset_to_elem;
    uint32_t value;
} elem_to_correct;

// used in fixProgramHeaders
typedef struct {
    uint32_t i_segment;              // index of segment
    uint32_t *p_sections_in_segment; // array of length of number of sections. for section i, contains 1 if in segment, 0 otherwise
    uint32_t num_of_sections;        // total number of sections in i_segment
} section_to_segment_mapping;

section_to_segment_mapping* populateSectionSegmentMapping(void);
void freeSectionToSegmentMapping(section_to_segment_mapping **pp_mapping);

uint32_t injectBytesInFile(injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count);

void fixBranchInstrns        (injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count);
void fixElfHeader            (injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count);
void fixRelocs               (injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count);
void fixSectionHeaders       (injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count);
void fixProgramHeaders       (injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count, section_to_segment_mapping*);
void fixDynamicSection       (injection_stat inject_stat_arr[], uint32_t inject_stat_arr_count);

void fixBranchInstrnsSingleInjection (uint32_t offset_to_injection, uint32_t injection_bytes_size);

int injectHashCheckBeginningOfTxtSection(uint32_t offset_to_start_of_txt_section, uint32_t txt_section_size, uint32_t hash_of_txt_section);

int inject_stat_cmp_helper(const void *right, const void *left);

#endif // ELF_INJECTION_32_H
