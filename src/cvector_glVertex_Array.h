#ifndef CVECTOR_glVertex_Array_H
#define CVECTOR_glVertex_Array_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Data structure for glVertex_Array vector. */
typedef struct cvector_glVertex_Array
{
	glVertex_Array* a;           /**< Array. */
	size_t size;       /**< Current size (amount you use when manipulating array directly). */
	size_t capacity;   /**< Allocated size of array; always >= size. */
} cvector_glVertex_Array;



extern size_t CVEC_glVertex_Array_SZ;

int cvec_glVertex_Array(cvector_glVertex_Array* vec, size_t size, size_t capacity);
int cvec_init_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array* vals, size_t num);

cvector_glVertex_Array* cvec_glVertex_Array_heap(size_t size, size_t capacity);
cvector_glVertex_Array* cvec_init_glVertex_Array_heap(glVertex_Array* vals, size_t num);
int cvec_copyc_glVertex_Array(void* dest, void* src);
int cvec_copy_glVertex_Array(cvector_glVertex_Array* dest, cvector_glVertex_Array* src);

int cvec_push_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array a);
glVertex_Array cvec_pop_glVertex_Array(cvector_glVertex_Array* vec);

int cvec_extend_glVertex_Array(cvector_glVertex_Array* vec, size_t num);
int cvec_insert_glVertex_Array(cvector_glVertex_Array* vec, size_t i, glVertex_Array a);
int cvec_insert_array_glVertex_Array(cvector_glVertex_Array* vec, size_t i, glVertex_Array* a, size_t num);
glVertex_Array cvec_replace_glVertex_Array(cvector_glVertex_Array* vec, size_t i, glVertex_Array a);
void cvec_erase_glVertex_Array(cvector_glVertex_Array* vec, size_t start, size_t end);
int cvec_reserve_glVertex_Array(cvector_glVertex_Array* vec, size_t size);
int cvec_set_cap_glVertex_Array(cvector_glVertex_Array* vec, size_t size);
void cvec_set_val_sz_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val);
void cvec_set_val_cap_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val);

glVertex_Array* cvec_back_glVertex_Array(cvector_glVertex_Array* vec);

void cvec_clear_glVertex_Array(cvector_glVertex_Array* vec);
void cvec_free_glVertex_Array_heap(void* vec);
void cvec_free_glVertex_Array(void* vec);

#ifdef __cplusplus
}
#endif

/* CVECTOR_glVertex_Array_H */
#endif


#ifdef CVECTOR_glVertex_Array_IMPLEMENTATION

size_t CVEC_glVertex_Array_SZ = 50;

#define CVEC_glVertex_Array_ALLOCATOR(x) ((x+1) * 2)


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

cvector_glVertex_Array* cvec_glVertex_Array_heap(size_t size, size_t capacity)
{
	cvector_glVertex_Array* vec;
	if (!(vec = (cvector_glVertex_Array*)CVEC_MALLOC(sizeof(cvector_glVertex_Array)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glVertex_Array_SZ;

	if (!(vec->a = (glVertex_Array*)CVEC_MALLOC(vec->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glVertex_Array* cvec_init_glVertex_Array_heap(glVertex_Array* vals, size_t num)
{
	cvector_glVertex_Array* vec;
	
	if (!(vec = (cvector_glVertex_Array*)CVEC_MALLOC(sizeof(cvector_glVertex_Array)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glVertex_Array_SZ;
	vec->size = num;
	if (!(vec->a = (glVertex_Array*)CVEC_MALLOC(vec->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glVertex_Array)*num);

	return vec;
}

int cvec_glVertex_Array(cvector_glVertex_Array* vec, size_t size, size_t capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glVertex_Array_SZ;

	if (!(vec->a = (glVertex_Array*)CVEC_MALLOC(vec->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array* vals, size_t num)
{
	vec->capacity = num + CVEC_glVertex_Array_SZ;
	vec->size = num;
	if (!(vec->a = (glVertex_Array*)CVEC_MALLOC(vec->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glVertex_Array)*num);

	return 1;
}

int cvec_copyc_glVertex_Array(void* dest, void* src)
{
	cvector_glVertex_Array* vec1 = (cvector_glVertex_Array*)dest;
	cvector_glVertex_Array* vec2 = (cvector_glVertex_Array*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glVertex_Array(vec1, vec2);
}

int cvec_copy_glVertex_Array(cvector_glVertex_Array* dest, cvector_glVertex_Array* src)
{
	glVertex_Array* tmp = NULL;
	if (!(tmp = (glVertex_Array*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glVertex_Array));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array a)
{
	glVertex_Array* tmp;
	size_t tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glVertex_Array_ALLOCATOR(vec->capacity);
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glVertex_Array cvec_pop_glVertex_Array(cvector_glVertex_Array* vec)
{
	return vec->a[--vec->size];
}

glVertex_Array* cvec_back_glVertex_Array(cvector_glVertex_Array* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glVertex_Array(cvector_glVertex_Array* vec, size_t num)
{
	glVertex_Array* tmp;
	size_t tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glVertex_Array_SZ;
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glVertex_Array(cvector_glVertex_Array* vec, size_t i, glVertex_Array a)
{
	glVertex_Array* tmp;
	size_t tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glVertex_Array));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glVertex_Array_ALLOCATOR(vec->capacity);
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glVertex_Array));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glVertex_Array(cvector_glVertex_Array* vec, size_t i, glVertex_Array* a, size_t num)
{
	glVertex_Array* tmp;
	size_t tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glVertex_Array_SZ;
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glVertex_Array));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glVertex_Array));
	vec->size += num;
	return 1;
}

glVertex_Array cvec_replace_glVertex_Array(cvector_glVertex_Array* vec, size_t i, glVertex_Array a)
{
	glVertex_Array tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glVertex_Array(cvector_glVertex_Array* vec, size_t start, size_t end)
{
	size_t d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glVertex_Array));
	vec->size -= d;
}


int cvec_reserve_glVertex_Array(cvector_glVertex_Array* vec, size_t size)
{
	glVertex_Array* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*(size+CVEC_glVertex_Array_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glVertex_Array_SZ;
	}
	return 1;
}

int cvec_set_cap_glVertex_Array(cvector_glVertex_Array* vec, size_t size)
{
	glVertex_Array* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val)
{
	size_t i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val)
{
	size_t i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glVertex_Array(cvector_glVertex_Array* vec) { vec->size = 0; }

void cvec_free_glVertex_Array_heap(void* vec)
{
	cvector_glVertex_Array* tmp = (cvector_glVertex_Array*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glVertex_Array(void* vec)
{
	cvector_glVertex_Array* tmp = (cvector_glVertex_Array*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}

#endif
