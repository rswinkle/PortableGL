#include "c_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/** Useful utility function since strdup isn't in standard C.*/
char* mystrdup(const char* str)
{
	if (!str)
		return NULL;

	size_t len = strlen(str);
	char* temp = (char*)calloc(len+1, sizeof(char));
	if (!temp) {
		return NULL;
	}

	return (char*)memcpy(temp, str, len);  /* memcpy returns to, and calloc already nulled last char */
}

c_array init_c_array(byte* data, size_t elem_size, size_t len)
{
	c_array a = { NULL, elem_size, 0 };
	a.data = (byte*)malloc(len * elem_size + 1);
	if (!a.data)
		return a;

	a.len = len;

	if (!data)
		return a;

	memcpy(a.data, data, len*elem_size);
	a.data[len*elem_size] = 0;

	return a;
}

c_array copy_c_array(c_array src)
{
	c_array a = { NULL, src.elem_size, 0 };
	a.data = (byte*)malloc(src.len * src.elem_size + 1);
	if (!a.data)
		return a;

	a.len = src.len;
	memcpy(a.data, src.data, src.len*src.elem_size+1); /*copy over the null byte too*/
	return a;
}


int file_open_read(const char* filename, const char* mode, c_array* out)
{
	FILE *file = fopen(filename, mode);
	if (!file)
		return 0;

	return file_read(file, out);
}



int file_read(FILE* file, c_array* out)
{
	byte* data;
	long size;
	out->data = NULL;
	out->len = 0;
	out->elem_size = 1;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	if (size <= 0) {
		if (size == -1)
			perror("ftell failure");
		fclose(file);
		return 0;
	}

	data = (byte*)malloc(size+1);
	if (!data) {
		fclose(file);
		return 0;
	}

	rewind(file);
	if (!fread(data, size, 1, file)) {
		perror("fread failure");
		fclose(file);
		free(data);
		return 0;
	}

	data[size] = 0; /* null terminate in all cases even if reading binary data */

	out->data = data;
	out->len = size;
	out->elem_size = 1;

	fclose(file);
	return size;
}



int file_open_write(const char* filename, const char* mode, c_array* out)
{
	FILE* file = fopen(filename, mode);
	if (!file)
		return 0;

	return file_write(file, out);
}

int file_write(FILE* file, c_array* out)
{
	int ret = fwrite(out->data, out->elem_size, out->len, file);
	fclose(file);
	return (ret == out->len);
}









/*
 * builds an array of pointers to the strings in file_contents.  file_contents->data
 * is output from file_read or file_open_read or similar block of text/data.  The pointers
 * in lines are not allocated but point into file_contents->data ie file_contents->data
 * is modified to turn '\n' to '\0'
 */
int file_open_readlines(const char* filename, c_array* lines, c_array* file_contents)
{
	int i, pos, len;
	char** char_ptr = NULL;
	char* nl = NULL;

	lines->data = NULL;
	lines->len = 0;
	lines->elem_size = 1;

	if (!file_open_read(filename, "r", file_contents)) {
		return 0;
	}

	len = file_contents->len / 60 + 1; /* start with conservative estimate if # of lines */
	lines->data = (byte*)malloc(len * sizeof(char*) + 1);
	if (!lines->data)
		return 0;

	char_ptr = (char**)lines->data;
	i = 0, pos = 0;
	while (1) {
		char_ptr[i] = (char*)&file_contents->data[pos];
		nl = strchr((char*)&file_contents->data[pos], '\n');
		if (nl) {
			*nl = '\0';
			pos = nl - (char*)file_contents->data + 1;
			i++;
			if (i == len) {
				len *= 2;
				if (!(char_ptr = (char**)realloc(lines->data, len * sizeof(char*) + 1))) {
					free(lines->data);
					lines->len = 0;
					return 0;
				}
				lines->data = (byte*)char_ptr;
			}
		} else {
			break;
		}
	}

	lines->data = (byte*)realloc(char_ptr, i*sizeof(char*)+1);
	lines->len = i;
	lines->elem_size = sizeof(char*);

	return 1;
}


int freadline_into_str(FILE* input, char* str, size_t len)
{
	return freadstring_into_str(input, '\n', str, len);
}

int freadstring_into_str(FILE* input, int delim, char* str, size_t len)
{
	int temp;
	int i=0;

	if (feof(input))
		return 0;

	while (i < len-1) {
		temp = getc(input);

		if (temp == EOF || temp == delim) {
			if (!i && temp != delim) {
				return 0;
			}
			break;
		}

		str[i] = temp;
		i++;
	}
	str[i] = '\0';


	return 1;
}

char* freadline(FILE* input)
{
	return freadstring(input, '\n', 0);
}

char* freadstring(FILE* input, int delim, size_t max_len)
{
	char* string = NULL, *tmp_str = NULL;
	int temp;
	int i = 0;
	int inf = 0;

	if (feof(input))
		return NULL;

	if (!max_len) {
		inf = 1;
		max_len = 4096;
	}

	if (!(string = (char*)malloc(max_len+1)))
		return NULL;

	while (1) {
		temp = fgetc(input);

		if (temp == EOF || temp == delim) {
			if (!i && temp != delim) { //allow for delim == EOF
				free(string);
				return NULL;
			}
			tmp_str = (char*)realloc(string, i+1);
			if (!tmp_str) {
				free(string);
				return NULL;
			}
			string = tmp_str;
			break;
		}

		if (i == max_len) {
			if (inf) {
				tmp_str = (char*)realloc(string, max_len*2+1);
				if (!tmp_str) {
					free(string);
					return NULL;
				}
				string = tmp_str;
				max_len *= 2;
			} else {
				break;
			}
		}

		string[i] = temp;
		i++;
	}
	string[i] = '\0';


	return string;
}


int fpeek(FILE* input)
{
	int tmp = getc(input);
	ungetc(tmp, input);
	return tmp;
}

int readline_into_str(c_array* input, char* str, size_t len)
{
	return readstring_into_str(input, '\n', str, len);
}

int readstring_into_str(c_array* input, char delim, char* str, size_t len)
{
	char temp;
	int i=0;
	char* p = (char*) input->data;

	if (!(input->len * input->elem_size))
		return 0;

	while (*p && i < len-1) {
		temp = *p++;

		if (temp == delim) {
			break;
		}

		str[i] = temp;
		i++;
	}
	str[i] = '\0';

	return 1;
}

char* readline(c_array* input)
{
	return readstring(input, '\n', 0);
}

char* readstring(c_array* input, char delim, size_t max_len)
{
	char* string = NULL, *tmp_str = NULL;
	char temp;
	int i=0;
	int inf = 0;
	char* p = (char*) input->data;

	if (!(input->len * input->elem_size))
		return NULL;

	if (!max_len) {
		inf = 1;
		max_len = 4096;
	}

	if (!(string = (char*)malloc(max_len+1)))
		return NULL;

	while (*p) {
		temp = *p++;

		if (temp == delim) {
			if (!i) {
				free(string);
				return NULL;
			}
			tmp_str = (char*)realloc(string, i+1);
			if (!tmp_str) {
				free(string);
				return NULL;
			}
			string = tmp_str;
			break;
		}

		if (i == max_len) {
			if (inf) {
				tmp_str = (char*)realloc(string, max_len*2+1);
				if (!tmp_str) {
					free(string);
					return NULL;
				}
				string = tmp_str;
				max_len *= 2;
			} else {
				break;
			}
		}

		string[i] = temp;
		i++;
	}
	string[i] = '\0';


	return string;
}



c_array slice_c_array(c_array array, long start, long end)
{
	c_array a = { NULL, array.elem_size, 0 };
	int len;

	if (start < 0)
		start = array.len + start;

	if (end < 0)
		end = array.len + end;

	if (start < 0)
		start = 0;

	if (end < 0)
		end = 0;

	if (end <= start)
		return a;

	len = end - start;
	if (!(a.data = (byte*)malloc(len * array.elem_size + 1)))
		return a;

	a.data[len * array.elem_size] = 0;  /* as with file read functions always null terminate */

	memcpy(a.data, &array.data[start], len*array.elem_size);
	a.len = len;
	return a;
}

/* TODO make skip_chars byte* or u8*? */
int read_char(FILE* input, const char* skip_chars, int complement, int clear_line)
{
	int c, ret;
	byte tmp;
	c_array skip;
	const char* tmp_skip = (skip_chars) ? skip_chars : "";

	SET_C_ARRAY(skip, (byte*)skip_chars, 1, strlen(tmp_skip));

	do {
		ret = getc(input);
		if (ret == EOF)
			return ret;
		tmp = ret;
		c = is_any(&skip, &tmp, are_equal_uchar);
	} while ((!complement && c) || (complement && !c));

	if (clear_line && ret != '\n')
		do { c = getc(input); } while (c != '\n' && c != EOF);

	return ret;
}

char* read_string(FILE* file, const char* skip_chars, int delim, size_t max_len)
{
	int tmp;
	byte tmp2;
	char* str = NULL;
	c_array skip;
	const char* tmp_skip = (skip_chars) ? skip_chars : "";

	SET_C_ARRAY(skip, (byte*)skip_chars, 1, strlen(tmp_skip));

	do {
		tmp = getc(file);
		if (tmp == EOF)
			return NULL;
		tmp2 = tmp;
	} while (is_any(&skip, &tmp2, are_equal_uchar));
	ungetc(tmp, file);
	do {
		str = freadstring(file, delim, max_len);
	} while (!str);

	return str;
}


/* fills out.data with c_array's whose data members point at the split
 * out strings in array.data, iow you don't free anything in out.
 * see example usage in main.c ... for now I don't NULL out delimiter
 * so you can't just print ((*c_array)&out.data[i])->data as a string */
int split(c_array* array, byte* delim, size_t delim_len, c_array* out)
{
	size_t pos = 0, max_len = 1000;
	out->elem_size = sizeof(c_array);
	out->len = 0;
	byte* match;
	c_array* results;

	out->data = (byte*)malloc(max_len*sizeof(c_array)+1);
	if (!out->data)
		return 0;

	results = (c_array*)out->data;

	while ((match = (byte*)memchr(&array->data[pos], delim[0], array->len*array->elem_size - pos))) {
		if (!memcmp(match, delim, delim_len)) {
			results[out->len].data = &array->data[pos];
			results[out->len].elem_size = 1;
			results[out->len].len = match - &array->data[pos];

			out->len++;
			if (out->len == max_len) {
				max_len *= 2;
				out->data = (byte*)realloc(results, max_len*out->elem_size + 1);
				if (!out->data) {
					free(results);
					out->data = NULL;
					out->len = 0;
					return 0;
				}
				results = (c_array*)out->data;
			}
			pos = match - array->data + delim_len;
		} else {
			pos = match - array->data + 1;
		}
	}
	/* get remaining data if necessary */
	if (array->len - pos) {
		results[out->len].data = &array->data[pos];
		results[out->len].elem_size = 1;
		results[out->len].len = array->len - pos;
		out->len++;
	}

	results = (c_array*)realloc(out->data, out->len*out->elem_size+1);
	if (!results) {
		free(out->data);
		out->data = NULL;
		out->len = 0;
		return 0;
	}
	out->data = (byte*)results;
	out->data[out->len*out->elem_size] = 0;

	return 1;
}


char* ltrim(char* str)
{
	int i = 0;
	int len = strlen(str);
	while (isspace(str[i]))
		i++;
	memmove(str, &str[i], len-i);
	str[len-i] = 0;
	return str;
}

char* rtrim(char* str)
{
	int i;
	int len = strlen(str);
	i = len - 1;
	while (isspace(str[i]))
		i--;
	str[++i] = 0;
	return str;
}

char* trim(char* str)
{
	return ltrim(rtrim(str));
}

// like strtok but takes a single character delim and doesn't
// skip empty fields (ie 2 delims next to each other returns "")
char* mystrtok(char* str, int delim)
{
	static char* p;
	if (str) {
		p = str;
	}
	char* ret = p;

	while (*p && *p != delim) {
		p++;
	}
	if (*p) {
		*p = 0;
		p++;
	} else if (p == ret) {
		return NULL;
	}

	return ret;
}

// like strtok but takes a single character delim and doesn't
// skip empty fields (ie 2 delims next to each other returns "")
// and does not modify str but return allocated copies of tokens
char* mystrtok_alloc(const char* str, int delim)
{
	static const char* p;
	if (str) {
		p = str;
	}
	const char* s = p;

	while (*p && *p != delim) {
		p++;
	}
	char* ret = NULL;
	if (*p || p != s) {
		ret = (char*)calloc(p-s+1, 1);
		memcpy(ret, s, p-s);
		p += !!*p;  // only ++ if we're not at '\0'
	}

	return ret;
}

size_t find(c_array haystack, c_array needle)
{
	byte* result = haystack.data;
	byte* end = haystack.data + haystack.len*haystack.elem_size;
	while ((result = (byte*)memchr(result, needle.data[0], end-result))) {
		if (!memcmp(result, needle.data, needle.len*needle.elem_size)) {
			return result - haystack.data;
		} else {
			++result;
		}
	}

	return -1; /* make a macro or static const size_t npos = -1 ? */
}

/*

void find_all(c_array haystack, c_array needle, vector_int* vec)
{
	byte* result = haystack.data;
	byte* end = haystack.data + haystack.len*haystack.elem_size;
	while(result = memchr(result, needle.data[0], end-result)) {
		if (!memcmp(result, needle.data, needle.len*needle.elem_size)) {
			push_i(vec, result - haystack.data);
			result += needle.len * needle.elem_size;
		} else {
			++result;
		}
	}
}
*/

void* mybsearch(const void *key, const void *buf, size_t num, size_t size, int (*compare)(const void *, const void *))
{
	size_t min = 0, max = num-1;
	size_t cursor;

	while (min <= max) {
		cursor = min + ((max - min) / 2);
		int ret = compare(&key, (const char*)buf+cursor*size);
		if (!ret) {
			return (char*)buf + cursor*size;
		} else if (ret < 0) {
			max = cursor - 1;  //overflow possibilities here and below
		} else {
			min = cursor + 1;
		}
	}
	return NULL;
}



int compare_char(const void* a, const void* b)
{
	char a_ = *(char*)a;
	char b_ = *(char*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}

int compare_uchar(const void* a, const void* b)
{
	unsigned char a_ = *(unsigned char*)a;
	unsigned char b_ = *(unsigned char*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}

int compare_short(const void* a, const void* b)
{
	short a_ = *(short*)a;
	short b_ = *(short*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}

int compare_ushort(const void* a, const void* b)
{
	unsigned short a_ = *(unsigned short*)a;
	unsigned short b_ = *(unsigned short*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}


int compare_int(const void* a, const void* b)
{
	int a_ = *(int*)a;
	int b_ = *(int*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}

int compare_uint(const void* a, const void* b)
{
	unsigned int a_ = *(unsigned int*)a;
	unsigned int b_ = *(unsigned int*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}

int compare_long(const void* a, const void* b)
{
	long a_ = *(long*)a;
	long b_ = *(long*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}

int compare_ulong(const void* a, const void* b)
{
	unsigned long a_ = *(unsigned long*)a;
	unsigned long b_ = *(unsigned long*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}

int compare_float(const void* a, const void* b)
{
	float a_ = *(float*)a;
	float b_ = *(float*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}

int compare_double(const void* a, const void* b)
{
	double a_ = *(double*)a;
	double b_ = *(double*)b;

	if (a_ < b_)
		return -1;
	else if (a_ > b_)
		return 1;

	return 0;
}

int compare_string(const void* a, const void* b)
{
	return strcmp(*(const char**)a, *(const char**)b);
}







int are_equal_char(const void* a, const void* b)
{
	return *(char*)a == *(char*)b;
}

int are_equal_uchar(const void* a, const void* b)
{
	return *(unsigned char*)a == *(unsigned char*)b;
}

int are_equal_short(const void* a, const void* b)
{
	return *(short*)a == *(short*)b;
}

int are_equal_ushort(const void* a, const void* b)
{
	return *(unsigned short*)a == *(unsigned short*)b;
}

int are_equal_int(const void* a, const void* b)
{
	return *(int*)a == *(int*)b;
}

int are_equal_uint(const void* a, const void* b)
{
	return *(unsigned int*)a == *(unsigned int*)b;
}

int are_equal_long(const void* a, const void* b)
{
	return *(long*)a == *(long*)b;
}

int are_equal_ulong(const void* a, const void* b)
{
	return *(unsigned long*)a == *(unsigned long*)b;
}

int are_equal_float(const void* a, const void* b)
{
	return *(float*)a == *(float*)b;
}

int are_equal_double(const void* a, const void* b)
{
	return *(double*)a == *(double*)b;
}

int are_equal_string(const void* a, const void* b)
{
	return !strcmp(*(char**)a, *(char**)b);
}


//int is_any_array


int is_any(c_array* array, const void* the_one, int (*are_equal)(const void*, const void*))
{
	size_t i;
	for (i=0; i<array->len; ++i) {
		if (are_equal(the_one, &array->data[i*array->elem_size]))
			return 1;
	}
	return 0;
}




int any(c_array* array, int (*is_true)(const void*))
{
	size_t i;
	for (i=0; i<array->len; ++i) {
		if (is_true(&array->data[i*array->elem_size])) {
			return 1;
		}
	}
	return 0;
}


int all(c_array* array, int (*is_true)(const void*))
{
	size_t i;
	for (i=0; i<array->len; ++i) {
		if (!is_true(&array->data[i*array->elem_size])) {
			return 0;
		}
	}
	return 1;
}


void map(c_array* array, void (*func)(const void*))
{
	size_t i;
	for (i=0; i<array->len; ++i) {
		func(&array->data[i*array->elem_size]);
	}
}


char* int_to_str(int num, int base)
{
	static char buf[INT_MAX_LEN+1];
	static char digits[] = "0123456789ABCDEF";

	char* ret;
	char *pos = buf + INT_MAX_LEN-1;
	int tmp = (num < 0) ? -num : num;

	do {
		*pos-- = digits[tmp % base];
		tmp /= base;
	} while (tmp != 0);

	if (num < 0) {
		*pos = '-';
	} else {
		pos++;
	}

	ret = (char*)calloc(buf + INT_MAX_LEN+1 - pos, sizeof(char));
	if (!ret) {
		fprintf(stderr, "Failed to allocate memory!\n");
		return NULL;
	}

	return (char*)memcpy(ret, pos, buf + INT_MAX_LEN - pos);  /* memcpy returns to, and calloc already nulled last char */
}

float rand_float(float min, float max)
{
	return ((float)rand()/(float)(RAND_MAX-1))*(max-min) + min;
}

double rand_double(double min, double max)
{
	return ((double)rand()/(double)(RAND_MAX-1))*(max-min) + min;
}



