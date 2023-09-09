#ifndef CVECTOR_float_H
#define CVECTOR_float_H

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

/** Data structure for float vector. */
typedef struct cvector_float
{
	float* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_float;



extern cvec_sz CVEC_float_SZ;

int cvec_float(cvector_float* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_float(cvector_float* vec, float* vals, cvec_sz num);

cvector_float* cvec_float_heap(cvec_sz size, cvec_sz capacity);
cvector_float* cvec_init_float_heap(float* vals, cvec_sz num);
int cvec_copyc_float(void* dest, void* src);
int cvec_copy_float(cvector_float* dest, cvector_float* src);

int cvec_push_float(cvector_float* vec, float a);
float cvec_pop_float(cvector_float* vec);

int cvec_extend_float(cvector_float* vec, cvec_sz num);
int cvec_insert_float(cvector_float* vec, cvec_sz i, float a);
int cvec_insert_array_float(cvector_float* vec, cvec_sz i, float* a, cvec_sz num);
float cvec_replace_float(cvector_float* vec, cvec_sz i, float a);
void cvec_erase_float(cvector_float* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_float(cvector_float* vec, cvec_sz size);
int cvec_set_cap_float(cvector_float* vec, cvec_sz size);
void cvec_set_val_sz_float(cvector_float* vec, float val);
void cvec_set_val_cap_float(cvector_float* vec, float val);

float* cvec_back_float(cvector_float* vec);

void cvec_clear_float(cvector_float* vec);
void cvec_free_float_heap(void* vec);
void cvec_free_float(void* vec);

#ifdef __cplusplus
}
#endif

/* CVECTOR_float_H */
#endif


#ifdef CVECTOR_float_IMPLEMENTATION

cvec_sz CVEC_float_SZ = 50;

#define CVEC_float_ALLOCATOR(x) ((x+1) * 2)

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

cvector_float* cvec_float_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_float* vec;
	if (!(vec = (cvector_float*)CVEC_MALLOC(sizeof(cvector_float)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_float_SZ;

	if (!(vec->a = (float*)CVEC_MALLOC(vec->capacity*sizeof(float)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_float* cvec_init_float_heap(float* vals, cvec_sz num)
{
	cvector_float* vec;
	
	if (!(vec = (cvector_float*)CVEC_MALLOC(sizeof(cvector_float)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_float_SZ;
	vec->size = num;
	if (!(vec->a = (float*)CVEC_MALLOC(vec->capacity*sizeof(float)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(float)*num);

	return vec;
}

int cvec_float(cvector_float* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_float_SZ;

	if (!(vec->a = (float*)CVEC_MALLOC(vec->capacity*sizeof(float)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_float(cvector_float* vec, float* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_float_SZ;
	vec->size = num;
	if (!(vec->a = (float*)CVEC_MALLOC(vec->capacity*sizeof(float)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(float)*num);

	return 1;
}

int cvec_copyc_float(void* dest, void* src)
{
	cvector_float* vec1 = (cvector_float*)dest;
	cvector_float* vec2 = (cvector_float*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_float(vec1, vec2);
}

int cvec_copy_float(cvector_float* dest, cvector_float* src)
{
	float* tmp = NULL;
	if (!(tmp = (float*)CVEC_REALLOC(dest->a, src->capacity*sizeof(float)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(float));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_float(cvector_float* vec, float a)
{
	float* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_float_ALLOCATOR(vec->capacity);
		if (!(tmp = (float*)CVEC_REALLOC(vec->a, sizeof(float)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

float cvec_pop_float(cvector_float* vec)
{
	return vec->a[--vec->size];
}

float* cvec_back_float(cvector_float* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_float(cvector_float* vec, cvec_sz num)
{
	float* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_float_SZ;
		if (!(tmp = (float*)CVEC_REALLOC(vec->a, sizeof(float)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_float(cvector_float* vec, cvec_sz i, float a)
{
	float* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(float));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_float_ALLOCATOR(vec->capacity);
		if (!(tmp = (float*)CVEC_REALLOC(vec->a, sizeof(float)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(float));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_float(cvector_float* vec, cvec_sz i, float* a, cvec_sz num)
{
	float* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_float_SZ;
		if (!(tmp = (float*)CVEC_REALLOC(vec->a, sizeof(float)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(float));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(float));
	vec->size += num;
	return 1;
}

float cvec_replace_float(cvector_float* vec, cvec_sz i, float a)
{
	float tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_float(cvector_float* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(float));
	vec->size -= d;
}


int cvec_reserve_float(cvector_float* vec, cvec_sz size)
{
	float* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (float*)CVEC_REALLOC(vec->a, sizeof(float)*(size+CVEC_float_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_float_SZ;
	}
	return 1;
}

int cvec_set_cap_float(cvector_float* vec, cvec_sz size)
{
	float* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (float*)CVEC_REALLOC(vec->a, sizeof(float)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_float(cvector_float* vec, float val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_float(cvector_float* vec, float val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_float(cvector_float* vec) { vec->size = 0; }

void cvec_free_float_heap(void* vec)
{
	cvector_float* tmp = (cvector_float*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_float(void* vec)
{
	cvector_float* tmp = (cvector_float*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}

#endif
