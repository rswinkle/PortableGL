#ifndef CVECTOR_vec2_H
#define CVECTOR_vec2_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Data structure for vec2 vector. */
typedef struct cvector_vec2
{
	vec2* a;           /**< Array. */
	size_t size;       /**< Current size (amount you use when manipulating array directly). */
	size_t capacity;   /**< Allocated size of array; always >= size. */
} cvector_vec2;



extern size_t CVEC_vec2_SZ;

int cvec_vec2(cvector_vec2* vec, size_t size, size_t capacity);
int cvec_init_vec2(cvector_vec2* vec, vec2* vals, size_t num);

cvector_vec2* cvec_vec2_heap(size_t size, size_t capacity);
cvector_vec2* cvec_init_vec2_heap(vec2* vals, size_t num);
int cvec_copyc_vec2(void* dest, void* src);
int cvec_copy_vec2(cvector_vec2* dest, cvector_vec2* src);

int cvec_push_vec2(cvector_vec2* vec, vec2 a);
vec2 cvec_pop_vec2(cvector_vec2* vec);

int cvec_extend_vec2(cvector_vec2* vec, size_t num);
int cvec_insert_vec2(cvector_vec2* vec, size_t i, vec2 a);
int cvec_insert_array_vec2(cvector_vec2* vec, size_t i, vec2* a, size_t num);
vec2 cvec_replace_vec2(cvector_vec2* vec, size_t i, vec2 a);
void cvec_erase_vec2(cvector_vec2* vec, size_t start, size_t end);
int cvec_reserve_vec2(cvector_vec2* vec, size_t size);
int cvec_set_cap_vec2(cvector_vec2* vec, size_t size);
void cvec_set_val_sz_vec2(cvector_vec2* vec, vec2 val);
void cvec_set_val_cap_vec2(cvector_vec2* vec, vec2 val);

vec2* cvec_back_vec2(cvector_vec2* vec);

void cvec_clear_vec2(cvector_vec2* vec);
void cvec_free_vec2_heap(void* vec);
void cvec_free_vec2(void* vec);

#ifdef __cplusplus
}
#endif

/* CVECTOR_vec2_H */
#endif


#ifdef CVECTOR_vec2_IMPLEMENTATION

size_t CVEC_vec2_SZ = 50;

#define CVEC_vec2_ALLOCATOR(x) ((x+1) * 2)


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

cvector_vec2* cvec_vec2_heap(size_t size, size_t capacity)
{
	cvector_vec2* vec;
	if (!(vec = (cvector_vec2*)CVEC_MALLOC(sizeof(cvector_vec2)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_vec2_SZ;

	if (!(vec->a = (vec2*)CVEC_MALLOC(vec->capacity*sizeof(vec2)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_vec2* cvec_init_vec2_heap(vec2* vals, size_t num)
{
	cvector_vec2* vec;
	
	if (!(vec = (cvector_vec2*)CVEC_MALLOC(sizeof(cvector_vec2)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_vec2_SZ;
	vec->size = num;
	if (!(vec->a = (vec2*)CVEC_MALLOC(vec->capacity*sizeof(vec2)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(vec2)*num);

	return vec;
}

int cvec_vec2(cvector_vec2* vec, size_t size, size_t capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_vec2_SZ;

	if (!(vec->a = (vec2*)CVEC_MALLOC(vec->capacity*sizeof(vec2)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_vec2(cvector_vec2* vec, vec2* vals, size_t num)
{
	vec->capacity = num + CVEC_vec2_SZ;
	vec->size = num;
	if (!(vec->a = (vec2*)CVEC_MALLOC(vec->capacity*sizeof(vec2)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(vec2)*num);

	return 1;
}

int cvec_copyc_vec2(void* dest, void* src)
{
	cvector_vec2* vec1 = (cvector_vec2*)dest;
	cvector_vec2* vec2 = (cvector_vec2*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_vec2(vec1, vec2);
}

int cvec_copy_vec2(cvector_vec2* dest, cvector_vec2* src)
{
	vec2* tmp = NULL;
	if (!(tmp = (vec2*)CVEC_REALLOC(dest->a, src->capacity*sizeof(vec2)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(vec2));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_vec2(cvector_vec2* vec, vec2 a)
{
	vec2* tmp;
	size_t tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_vec2_ALLOCATOR(vec->capacity);
		if (!(tmp = (vec2*)CVEC_REALLOC(vec->a, sizeof(vec2)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

vec2 cvec_pop_vec2(cvector_vec2* vec)
{
	return vec->a[--vec->size];
}

vec2* cvec_back_vec2(cvector_vec2* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_vec2(cvector_vec2* vec, size_t num)
{
	vec2* tmp;
	size_t tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_vec2_SZ;
		if (!(tmp = (vec2*)CVEC_REALLOC(vec->a, sizeof(vec2)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_vec2(cvector_vec2* vec, size_t i, vec2 a)
{
	vec2* tmp;
	size_t tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(vec2));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_vec2_ALLOCATOR(vec->capacity);
		if (!(tmp = (vec2*)CVEC_REALLOC(vec->a, sizeof(vec2)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(vec2));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_vec2(cvector_vec2* vec, size_t i, vec2* a, size_t num)
{
	vec2* tmp;
	size_t tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_vec2_SZ;
		if (!(tmp = (vec2*)CVEC_REALLOC(vec->a, sizeof(vec2)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(vec2));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(vec2));
	vec->size += num;
	return 1;
}

vec2 cvec_replace_vec2(cvector_vec2* vec, size_t i, vec2 a)
{
	vec2 tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_vec2(cvector_vec2* vec, size_t start, size_t end)
{
	size_t d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(vec2));
	vec->size -= d;
}


int cvec_reserve_vec2(cvector_vec2* vec, size_t size)
{
	vec2* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (vec2*)CVEC_REALLOC(vec->a, sizeof(vec2)*(size+CVEC_vec2_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_vec2_SZ;
	}
	return 1;
}

int cvec_set_cap_vec2(cvector_vec2* vec, size_t size)
{
	vec2* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (vec2*)CVEC_REALLOC(vec->a, sizeof(vec2)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_vec2(cvector_vec2* vec, vec2 val)
{
	size_t i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_vec2(cvector_vec2* vec, vec2 val)
{
	size_t i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_vec2(cvector_vec2* vec) { vec->size = 0; }

void cvec_free_vec2_heap(void* vec)
{
	cvector_vec2* tmp = (cvector_vec2*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_vec2(void* vec)
{
	cvector_vec2* tmp = (cvector_vec2*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}

#endif
