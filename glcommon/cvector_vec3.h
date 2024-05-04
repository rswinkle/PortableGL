/*

CVector 4.2.0 MIT Licensed vector (dynamic array) library in strict C89
http://www.robertwinkler.com/projects/cvector.html
http://www.robertwinkler.com/projects/cvector/

Besides the docs and all the Doxygen comments, see cvector_tests.c for
examples of how to use it or look at any of these other projects for
more practical examples:

https://github.com/rswinkle/C_Interpreter
https://github.com/rswinkle/CPIM2
https://github.com/rswinkle/spelling_game
https://github.com/rswinkle/c_bigint
http://portablegl.com/

The MIT License (MIT)

Copyright (c) 2011-2024 Robert Winkler

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#ifndef CVECTOR_vec3_H
#define CVECTOR_vec3_H

#include <stdlib.h>

#ifndef CVEC_SIZE_T
#define CVEC_SIZE_T size_t
#endif

#ifndef CVEC_SZ
#define CVEC_SZ
typedef CVEC_SIZE_T cvec_sz;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Data structure for vec3 vector. */
typedef struct cvector_vec3
{
	vec3* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_vec3;



extern cvec_sz CVEC_vec3_SZ;

int cvec_vec3(cvector_vec3* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_vec3(cvector_vec3* vec, vec3* vals, cvec_sz num);

cvector_vec3* cvec_vec3_heap(cvec_sz size, cvec_sz capacity);
cvector_vec3* cvec_init_vec3_heap(vec3* vals, cvec_sz num);
int cvec_copyc_vec3(void* dest, void* src);
int cvec_copy_vec3(cvector_vec3* dest, cvector_vec3* src);

int cvec_push_vec3(cvector_vec3* vec, vec3 a);
vec3 cvec_pop_vec3(cvector_vec3* vec);

int cvec_extend_vec3(cvector_vec3* vec, cvec_sz num);
int cvec_insert_vec3(cvector_vec3* vec, cvec_sz i, vec3 a);
int cvec_insert_array_vec3(cvector_vec3* vec, cvec_sz i, vec3* a, cvec_sz num);
vec3 cvec_replace_vec3(cvector_vec3* vec, cvec_sz i, vec3 a);
void cvec_erase_vec3(cvector_vec3* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_vec3(cvector_vec3* vec, cvec_sz size);
#define cvec_shrink_to_fit_vec3(vec) cvec_set_cap_vec3((vec), (vec)->size)
int cvec_set_cap_vec3(cvector_vec3* vec, cvec_sz size);
void cvec_set_val_sz_vec3(cvector_vec3* vec, vec3 val);
void cvec_set_val_cap_vec3(cvector_vec3* vec, vec3 val);

vec3* cvec_back_vec3(cvector_vec3* vec);

void cvec_clear_vec3(cvector_vec3* vec);
void cvec_free_vec3_heap(void* vec);
void cvec_free_vec3(void* vec);

#ifdef __cplusplus
}
#endif

/* CVECTOR_vec3_H */
#endif


#ifdef CVECTOR_vec3_IMPLEMENTATION

cvec_sz CVEC_vec3_SZ = 50;

#define CVEC_vec3_ALLOCATOR(x) ((x+1) * 2)

#if defined(CVEC_MALLOC) && defined(CVEC_FREE) && defined(CVEC_REALLOC)
/* ok */
#elif !defined(CVEC_MALLOC) && !defined(CVEC_FREE) && !defined(CVEC_REALLOC)
/* ok */
#else
#error "Must define all or none of CVEC_MALLOC, CVEC_FREE, and CVEC_REALLOC."
#endif

#ifndef CVEC_MALLOC
#define CVEC_MALLOC(sz)      malloc(sz)
#define CVEC_REALLOC(p, sz)  realloc(p, sz)
#define CVEC_FREE(p)         free(p)
#endif

#ifndef CVEC_MEMMOVE
#include <string.h>
#define CVEC_MEMMOVE(dst, src, sz)  memmove(dst, src, sz)
#endif

#ifndef CVEC_ASSERT
#include <assert.h>
#define CVEC_ASSERT(x)       assert(x)
#endif

cvector_vec3* cvec_vec3_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_vec3* vec;
	if (!(vec = (cvector_vec3*)CVEC_MALLOC(sizeof(cvector_vec3)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_vec3_SZ;

	if (!(vec->a = (vec3*)CVEC_MALLOC(vec->capacity*sizeof(vec3)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_vec3* cvec_init_vec3_heap(vec3* vals, cvec_sz num)
{
	cvector_vec3* vec;
	
	if (!(vec = (cvector_vec3*)CVEC_MALLOC(sizeof(cvector_vec3)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_vec3_SZ;
	vec->size = num;
	if (!(vec->a = (vec3*)CVEC_MALLOC(vec->capacity*sizeof(vec3)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(vec3)*num);

	return vec;
}

int cvec_vec3(cvector_vec3* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_vec3_SZ;

	if (!(vec->a = (vec3*)CVEC_MALLOC(vec->capacity*sizeof(vec3)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_vec3(cvector_vec3* vec, vec3* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_vec3_SZ;
	vec->size = num;
	if (!(vec->a = (vec3*)CVEC_MALLOC(vec->capacity*sizeof(vec3)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(vec3)*num);

	return 1;
}

int cvec_copyc_vec3(void* dest, void* src)
{
	cvector_vec3* vec1 = (cvector_vec3*)dest;
	cvector_vec3* vec2 = (cvector_vec3*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_vec3(vec1, vec2);
}

int cvec_copy_vec3(cvector_vec3* dest, cvector_vec3* src)
{
	vec3* tmp = NULL;
	if (!(tmp = (vec3*)CVEC_REALLOC(dest->a, src->capacity*sizeof(vec3)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(vec3));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_vec3(cvector_vec3* vec, vec3 a)
{
	vec3* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_vec3_ALLOCATOR(vec->capacity);
		if (!(tmp = (vec3*)CVEC_REALLOC(vec->a, sizeof(vec3)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

vec3 cvec_pop_vec3(cvector_vec3* vec)
{
	return vec->a[--vec->size];
}

vec3* cvec_back_vec3(cvector_vec3* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_vec3(cvector_vec3* vec, cvec_sz num)
{
	vec3* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_vec3_SZ;
		if (!(tmp = (vec3*)CVEC_REALLOC(vec->a, sizeof(vec3)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_vec3(cvector_vec3* vec, cvec_sz i, vec3 a)
{
	vec3* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(vec3));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_vec3_ALLOCATOR(vec->capacity);
		if (!(tmp = (vec3*)CVEC_REALLOC(vec->a, sizeof(vec3)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(vec3));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_vec3(cvector_vec3* vec, cvec_sz i, vec3* a, cvec_sz num)
{
	vec3* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_vec3_SZ;
		if (!(tmp = (vec3*)CVEC_REALLOC(vec->a, sizeof(vec3)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(vec3));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(vec3));
	vec->size += num;
	return 1;
}

vec3 cvec_replace_vec3(cvector_vec3* vec, cvec_sz i, vec3 a)
{
	vec3 tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_vec3(cvector_vec3* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(vec3));
	vec->size -= d;
}


int cvec_reserve_vec3(cvector_vec3* vec, cvec_sz size)
{
	vec3* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (vec3*)CVEC_REALLOC(vec->a, sizeof(vec3)*(size+CVEC_vec3_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_vec3_SZ;
	}
	return 1;
}

int cvec_set_cap_vec3(cvector_vec3* vec, cvec_sz size)
{
	vec3* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (vec3*)CVEC_REALLOC(vec->a, sizeof(vec3)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_vec3(cvector_vec3* vec, vec3 val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_vec3(cvector_vec3* vec, vec3 val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_vec3(cvector_vec3* vec) { vec->size = 0; }

void cvec_free_vec3_heap(void* vec)
{
	cvector_vec3* tmp = (cvector_vec3*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_vec3(void* vec)
{
	cvector_vec3* tmp = (cvector_vec3*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}

#endif
