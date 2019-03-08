#ifndef MISC_FUNCTIONS_H
#define MISC_FUNCTIONS_H

void fatal(char *msg);

// opens p_file_to_open, and if successful, allocates space for it in memory, and assigns p_file_in_mem and file_in_mem_size
int loadFileToMem(char *p_file_to_open);

int saveFileFromMem(char *p_filename);

// unused in project
uint32_t getIndexAndLenOfLastNumInStr(char *p_str, uint32_t str_len, uint32_t *p_num_len);

int isDigit(char chr);

#endif // MISC_FUNCTIONS_H
