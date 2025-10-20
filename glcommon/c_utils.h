#ifndef C_UTILS
#define C_UTILS

#include "basic_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CUTILS_SIZE_T
#define CUTILS_SIZE_T size_t
#endif

#ifndef CU_SIZE_T
#define CU_SIZE_T
typedef CUTILS_SIZE_T cu_size_t;
#endif

typedef struct c_array
{
	byte* data;
	cu_size_t elem_size;
	cu_size_t len;
} c_array;

#define SET_C_ARRAY(array, data_, elem_size_, len_) \
	(array).data = (data_); \
	(array).elem_size = (elem_size_); \
	(array).len = (len_)

c_array init_c_array(byte* data, cu_size_t elem_size, cu_size_t len);
c_array copy_c_array(c_array src);


char* mystrdup(const char* str);

int file_open_read(const char* filename, const char* mode, c_array* out);
int file_read(FILE* file, c_array* out);

int file_open_write(const char* filename, const char* mode, c_array* out);
int file_write(FILE* file, c_array* out);
int file_open_readlines(const char* filename, c_array* lines, c_array* file_contents);
int file_readlines(FILE* file, c_array* lines, c_array* file_contents);

int freadstring_into_str(FILE* input, int delim, char* str, cu_size_t len);
int freadline_into_str(FILE* input, char* str, cu_size_t len);
char* freadline(FILE* input);
char* freadstring(FILE* input, int delim, cu_size_t max_len);
int fpeek(FILE* input);

int readstring_into_str(c_array* input, char delim, char* str, cu_size_t len);
int readline_into_str(c_array* input, char* str, cu_size_t len);
char* readline(c_array* input);
char* readstring(c_array* input, char delim, cu_size_t max_len);

int read_char(FILE* input, const char* skip_chars, int complement, int clear_line);

/* define GET_STRING macro */
#define READ_STRING(STR, CHAR) \
do { \
	do { CHAR = getchar(); } while (isspace(CHAR)); \
	ungetc(CHAR, stdin); \
	do { \
		STR = freadline(stdin); \
	} while (!STR); \
} while(0)

char* read_string(FILE* file, const char* skip_chars, int delim, cu_size_t max_len);

c_array slice_c_array(c_array array, long start, long end);


int split(c_array* in, byte* delim, cu_size_t delim_len, c_array* out);

char* ltrim(char* str);
char* rtrim(char* str);
char* trim(char* str);
char* mystrtok(char* str, int delim);
char* mystrtok_alloc(const char* str, int delim);


#define CLEAR_SCREEN "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" \
                     "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" \
                     "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"

#define SPACE_SET " \t\v\f\r\n"
#define SPACE_SET_NO_NEWLINE " \t\v\f\r"

int are_equal_char(const void* a, const void* b);
int are_equal_uchar(const void* a, const void* b);
int are_equal_short(const void* a, const void* b);
int are_equal_ushort(const void* a, const void* b);
int are_equal_int(const void* a, const void* b);
int are_equal_uint(const void* a, const void* b);
int are_equal_long(const void* a, const void* b);
int are_equal_ulong(const void* a, const void* b);
int are_equal_float(const void* a, const void* b);
int are_equal_double(const void* a, const void* b);
int are_equal_string(const void* a, const void* b);

int cmp_char_lt(const void* a, const void* b);
int cmp_uchar_lt(const void* a, const void* b);
int cmp_short_lt(const void* a, const void* b);
int cmp_ushort_lt(const void* a, const void* b);
int cmp_int_lt(const void* a, const void* b);
int cmp_uint_lt(const void* a, const void* b);
int cmp_long_lt(const void* a, const void* b);
int cmp_ulong_lt(const void* a, const void* b);
int cmp_float_lt(const void* a, const void* b);
int cmp_double_lt(const void* a, const void* b);
int cmp_string_lt(const void* a, const void* b);

int cmp_char_gt(const void* a, const void* b);
int cmp_uchar_gt(const void* a, const void* b);
int cmp_short_gt(const void* a, const void* b);
int cmp_ushort_gt(const void* a, const void* b);
int cmp_int_gt(const void* a, const void* b);
int cmp_uint_gt(const void* a, const void* b);
int cmp_long_gt(const void* a, const void* b);
int cmp_ulong_gt(const void* a, const void* b);
int cmp_float_gt(const void* a, const void* b);
int cmp_double_gt(const void* a, const void* b);
int cmp_string_gt(const void* a, const void* b);


int any(c_array* array, int (*is_true)(const void*));
int all(c_array* array, int (*is_true)(const void*));
int is_any(c_array* array, const void* the_one, int (*are_equal)(const void*, const void*));

void map(c_array* array, void (*func)(const void*));


cu_size_t find(c_array haystack, c_array needle);


#define INT_MAX_LEN sizeof(int)*CHAR_BIT

char* int_to_str(int num, int base);
char* long_to_str(long num, int base);

float rand_float(float min, float max);
double rand_double(double min, double max);


#ifdef __cplusplus
}
#endif

#endif

