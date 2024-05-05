#ifndef CVECTOR_TYPE_H
#define CVECTOR_TYPE_H

#ifndef CVEC_SIZE_T
#include <stdlib.h>
#define CVEC_SIZE_T size_t
#endif

#ifndef CVEC_SZ
#define CVEC_SZ
typedef CVEC_SIZE_T cvec_sz;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Data structure for TYPE vector. */
typedef struct cvector_TYPE
{
	TYPE* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_TYPE;



extern cvec_sz CVEC_TYPE_SZ;

int cvec_TYPE(cvector_TYPE* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_TYPE(cvector_TYPE* vec, TYPE* vals, cvec_sz num);

cvector_TYPE* cvec_TYPE_heap(cvec_sz size, cvec_sz capacity);
cvector_TYPE* cvec_init_TYPE_heap(TYPE* vals, cvec_sz num);
int cvec_copyc_TYPE(void* dest, void* src);
int cvec_copy_TYPE(cvector_TYPE* dest, cvector_TYPE* src);

int cvec_push_TYPE(cvector_TYPE* vec, TYPE a);
TYPE cvec_pop_TYPE(cvector_TYPE* vec);

int cvec_extend_TYPE(cvector_TYPE* vec, cvec_sz num);
int cvec_insert_TYPE(cvector_TYPE* vec, cvec_sz i, TYPE a);
int cvec_insert_array_TYPE(cvector_TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num);
TYPE cvec_replace_TYPE(cvector_TYPE* vec, cvec_sz i, TYPE a);
void cvec_erase_TYPE(cvector_TYPE* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_TYPE(cvector_TYPE* vec, cvec_sz size);
#define cvec_shrink_to_fit_TYPE(vec) cvec_set_cap_TYPE((vec), (vec)->size)
int cvec_set_cap_TYPE(cvector_TYPE* vec, cvec_sz size);
void cvec_set_val_sz_TYPE(cvector_TYPE* vec, TYPE val);
void cvec_set_val_cap_TYPE(cvector_TYPE* vec, TYPE val);

TYPE* cvec_back_TYPE(cvector_TYPE* vec);

void cvec_clear_TYPE(cvector_TYPE* vec);
void cvec_free_TYPE_heap(void* vec);
void cvec_free_TYPE(void* vec);

#ifdef __cplusplus
}
#endif

/* CVECTOR_TYPE_H */
#endif


#ifdef CVECTOR_TYPE_IMPLEMENTATION

cvec_sz CVEC_TYPE_SZ = 50;

#define CVEC_TYPE_ALLOCATOR(x) ((x+1) * 2)

#if defined(CVEC_MALLOC) && defined(CVEC_FREE) && defined(CVEC_REALLOC)
/* ok */
#elif !defined(CVEC_MALLOC) && !defined(CVEC_FREE) && !defined(CVEC_REALLOC)
/* ok */
#else
#error "Must define all or none of CVEC_MALLOC, CVEC_FREE, and CVEC_REALLOC."
#endif

#ifndef CVEC_MALLOC
#include <stdlib.h>
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

cvector_TYPE* cvec_TYPE_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_TYPE* vec;
	if (!(vec = (cvector_TYPE*)CVEC_MALLOC(sizeof(cvector_TYPE)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_TYPE_SZ;

	if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity*sizeof(TYPE)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_TYPE* cvec_init_TYPE_heap(TYPE* vals, cvec_sz num)
{
	cvector_TYPE* vec;
	
	if (!(vec = (cvector_TYPE*)CVEC_MALLOC(sizeof(cvector_TYPE)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_TYPE_SZ;
	vec->size = num;
	if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity*sizeof(TYPE)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE)*num);

	return vec;
}

int cvec_TYPE(cvector_TYPE* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_TYPE_SZ;

	if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity*sizeof(TYPE)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_TYPE(cvector_TYPE* vec, TYPE* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_TYPE_SZ;
	vec->size = num;
	if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity*sizeof(TYPE)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE)*num);

	return 1;
}

int cvec_copyc_TYPE(void* dest, void* src)
{
	cvector_TYPE* vec1 = (cvector_TYPE*)dest;
	cvector_TYPE* vec2 = (cvector_TYPE*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_TYPE(vec1, vec2);
}

int cvec_copy_TYPE(cvector_TYPE* dest, cvector_TYPE* src)
{
	TYPE* tmp = NULL;
	if (!(tmp = (TYPE*)CVEC_REALLOC(dest->a, src->capacity*sizeof(TYPE)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(TYPE));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_TYPE(cvector_TYPE* vec, TYPE a)
{
	TYPE* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_TYPE_ALLOCATOR(vec->capacity);
		if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

TYPE cvec_pop_TYPE(cvector_TYPE* vec)
{
	return vec->a[--vec->size];
}

TYPE* cvec_back_TYPE(cvector_TYPE* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_TYPE(cvector_TYPE* vec, cvec_sz num)
{
	TYPE* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_TYPE_SZ;
		if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_TYPE(cvector_TYPE* vec, cvec_sz i, TYPE a)
{
	TYPE* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(TYPE));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_TYPE_ALLOCATOR(vec->capacity);
		if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(TYPE));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_TYPE(cvector_TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num)
{
	TYPE* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_TYPE_SZ;
		if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(TYPE));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(TYPE));
	vec->size += num;
	return 1;
}

TYPE cvec_replace_TYPE(cvector_TYPE* vec, cvec_sz i, TYPE a)
{
	TYPE tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_TYPE(cvector_TYPE* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(TYPE));
	vec->size -= d;
}


int cvec_reserve_TYPE(cvector_TYPE* vec, cvec_sz size)
{
	TYPE* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE)*(size+CVEC_TYPE_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_TYPE_SZ;
	}
	return 1;
}

int cvec_set_cap_TYPE(cvector_TYPE* vec, cvec_sz size)
{
	TYPE* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_TYPE(cvector_TYPE* vec, TYPE val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_TYPE(cvector_TYPE* vec, TYPE val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_TYPE(cvector_TYPE* vec) { vec->size = 0; }

void cvec_free_TYPE_heap(void* vec)
{
	cvector_TYPE* tmp = (cvector_TYPE*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_TYPE(void* vec)
{
	cvector_TYPE* tmp = (cvector_TYPE*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}

#endif
