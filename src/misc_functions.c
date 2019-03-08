#include <stdio.h>      /* fopen, fpintf    */
#include <stdlib.h>     /* malloc, atoi     */
#include <inttypes.h> /* uint...            */
#include <math.h>       // exp
#include <unistd.h>     // F_OK
#include <string.h>     // memcpy

#include "misc_functions.h"
#include "main.h"

void fatal(char *msg) {
    fprintf(stderr, "error");
    if(msg != NULL)
        fprintf(stderr, ": %s\n", msg);
    else
        fprintf(stderr, "\n");
    exit(1);
}

int loadFileToMem(char *p_file_to_open) {
    FILE *file;
    uint32_t file_size;
    uint8_t *p_file_loaded;

    if((file = fopen(p_file_to_open, "r")) == NULL) {
        fprintf(stderr, "error opening %s\n", p_file_to_open);
        exit(1);
	}

	// get file size
	if(fseek(file, 0, SEEK_END) != 0)
        fatal("fseek");

	file_size = ftell(file);
	rewind(file);

	// load file to memory
	p_file_loaded = malloc( file_size );
	if(p_file_loaded == NULL)
        fatal("allocating on heap for file");

	if(fread(p_file_loaded, 1, file_size, file) != file_size) {
        perror("error on fread");
        exit(1);
	}

	p_file_in_mem = p_file_loaded;
	file_in_mem_size = file_size;

	fclose(file);

	return 1;
}

int saveFileFromMem(char *p_filename) {
    FILE *p_file;
    uint32_t len_filename = strlen(p_filename); // excludes null terminator
    char *p_new_filename = NULL;

#define INTEGRIFIED "_integrified"

    p_new_filename = malloc( len_filename + sizeof(INTEGRIFIED) ); // sizeof includes null
    strncpy(p_new_filename, p_filename, len_filename); // max of len_filename chars copied
    strncpy(p_new_filename + len_filename, INTEGRIFIED, sizeof(INTEGRIFIED));
    p_filename = p_new_filename;

//    // filename with appended "_integrified" exists already
//    if (0 == access(p_filename, F_OK)) {}

    // check if can open p_filename for writing
    if((p_file = fopen(p_filename, "w+")) == NULL) {
        fprintf(stderr, "error opening %s for writing\n", p_filename);
        exit(1);
	}

	if(fwrite(p_file_in_mem, 1, file_in_mem_size, p_file) != file_in_mem_size) {
        fatal("error writing to file");
	}

	printf("integrified ELF file output: %s\n", p_filename);

	if(NULL != p_new_filename)
        free(p_new_filename);

    fclose(p_file);


    return 1; // success
}

//int saveFileFromMemOld(char *p_filename, uint32_t filename_len) {
//    FILE *p_file;
//    int need_to_free_p_new_filename = 0;
//    int extracted_num_from_filename = -1;
//    int append_num_for_filename = 1;
//
//    // if file exists already
//    if (0 == access(p_filename, F_OK)) {
//        // extract number after last '_' in filename if exists. increment number for new filename
//
//        // find last '_' in p_filename
//        uint32_t i_last_underscore;
//        for (i_last_underscore = filename_len-1; i_last_underscore >= 0; --i_last_underscore) {
//            if ('_' == p_filename[i_last_underscore])
//                break;
//        }
//
//        // no underscore found; append _01 to end of filename for creating new filename
//        if (-1 == i_last_underscore) {
//
//            char *p_new_filename = malloc(filename_len + 3); // + 2 for _01 (null already included)
//            need_to_free_p_new_filename = 1;
//
//            memcpy(p_new_filename, p_filename, filename_len);
//            strcpy(p_new_filename + filename_len, "_01"); // overwrites old '\x00' with "_01" '\x00'
//
//            p_filename = p_new_filename;
//
//        }
//
//        // underscore found; check that only numerical digits after found '_'
//        else {
//
//            uint32_t i_first_non_digit_after_underscore;
//
//            for (i_first_non_digit_after_underscore = i_last_underscore + 1; i_first_non_digit_after_underscore < filename_len;
//                                                                                        ++i_first_non_digit_after_underscore) {
//                if (!isDigit(p_file[i_first_non_digit_after_underscore]))
//                    break;
//            }
//
//            // only numerical digits after last '_' in filename
//            if (filename_len == i_first_non_digit_after_underscore) {
//                extracted_num_from_filename = atoi(p_filename + i_last_underscore + 1);
//                append_num_for_filename = extracted_num_from_filename + 1;
//            }
//
//            // filename doesn't contain only digits after last '_' in filename; append _01 to filename
//            else {
//
//            }
//        } // end else case; underscore found in filename
//
//
//        else { // ending number in filename already
//            int suffix_num = atoi(p_filename + i_of_suffix_number);
//            suffix_num++;
//            if (suffix_num >= 100)
//                fatal("new file number will be above 100");
//
//            // should check that not overflowing p_filename buffer by appending number
//            sprintf(p_filename + i_of_suffix_number, "%-2d", suffix_num);
//        }
//    } // end if file exists already
//
//    // file doesn't already exist
//    else {
//
//    }
//
//    // check if can open p_filename
//    if((p_file = fopen(p_filename, "a")) == NULL) {
//        fprintf(stderr, "error opening %s\n", p_filename);
//        exit(1);
//	}
//
//	if(fwrite(p_file_in_mem, 1, file_in_mem_size, p_file) != file_in_mem_size) {
//        fatal("error writing to file");
//	}
//
//	if(need_to_free_p_new_filename)
//        free(p_filename);
//
//    fclose(p_file);
//
//    return 1; // success
//}

// returns starting index of last number in str if exists, otherwise undefined.
// sets *p_num_len to length of found number in characters in str if exists. otherwise = 0
uint32_t getIndexAndLenOfLastNumInStr(char *p_str, uint32_t str_len, uint32_t *p_num_len) {
    uint32_t i, num_len=0;

    // set i to last digit in string if exists
    for (i=str_len-1; i >= 0; --i) {
        if (isDigit(p_str[i]))
            break;
    }

    // no numerical digits found in str
    if (-1 == i) {
        *p_num_len = 0;
        return 0; // undefined, doesn't matter
    }

    // find length of number and beginning index (i+1 at completion of loop)
    for(; i >= 0; --i) {
        if (isDigit(p_str[i]))
            num_len++;
        else
            break;
    }

    *p_num_len = num_len;
    return i + 1; // index of beginning of last number in str
}

int isDigit(char chr) {
    int temp = chr - '0';
    return (temp >= 0 && temp <= 9) ? 1 : 0;
}





