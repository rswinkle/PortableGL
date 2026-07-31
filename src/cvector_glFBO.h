#ifndef CVECTOR_glFBO_H
#define CVECTOR_glFBO_H

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

/** Data structure for glFBO vector. */
typedef struct cvector_glFBO
{
	glFBO* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glFBO;



extern cvec_sz CVEC_glFBO_SZ;

int cvec_glFBO(cvector_glFBO* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glFBO(cvector_glFBO* vec, glFBO* vals, cvec_sz num);

cvector_glFBO* cvec_glFBO_heap(cvec_sz size, cvec_sz capacity);
cvector_glFBO* cvec_init_glFBO_heap(glFBO* vals, cvec_sz num);
int cvec_copyc_glFBO(void* dest, void* src);
int cvec_copy_glFBO(cvector_glFBO* dest, cvector_glFBO* src);

int cvec_push_glFBO(cvector_glFBO* vec, glFBO a);
glFBO cvec_pop_glFBO(cvector_glFBO* vec);

int cvec_extend_glFBO(cvector_glFBO* vec, cvec_sz num);
int cvec_insert_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO a);
int cvec_insert_array_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO* a, cvec_sz num);
glFBO cvec_replace_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO a);
void cvec_erase_glFBO(cvector_glFBO* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glFBO(cvector_glFBO* vec, cvec_sz size);
#define cvec_shrink_to_fit_glFBO(vec) cvec_set_cap_glFBO((vec), (vec)->size)
int cvec_set_cap_glFBO(cvector_glFBO* vec, cvec_sz size);
void cvec_set_val_sz_glFBO(cvector_glFBO* vec, glFBO val);
void cvec_set_val_cap_glFBO(cvector_glFBO* vec, glFBO val);

glFBO* cvec_back_glFBO(cvector_glFBO* vec);

void cvec_clear_glFBO(cvector_glFBO* vec);
void cvec_free_glFBO_heap(void* vec);
void cvec_free_glFBO(void* vec);

#ifdef __cplusplus
}
#endif

/* CVECTOR_glFBO_H */
#endif


#ifdef CVECTOR_glFBO_IMPLEMENTATION

cvec_sz CVEC_glFBO_SZ = 50;

#define CVEC_glFBO_ALLOCATOR(x) ((x+1) * 2)

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

cvector_glFBO* cvec_glFBO_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glFBO* vec;
	if (!(vec = (cvector_glFBO*)CVEC_MALLOC(sizeof(cvector_glFBO)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glFBO_SZ;

	if (!(vec->a = (glFBO*)CVEC_MALLOC(vec->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glFBO* cvec_init_glFBO_heap(glFBO* vals, cvec_sz num)
{
	cvector_glFBO* vec;
	
	if (!(vec = (cvector_glFBO*)CVEC_MALLOC(sizeof(cvector_glFBO)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glFBO_SZ;
	vec->size = num;
	if (!(vec->a = (glFBO*)CVEC_MALLOC(vec->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glFBO)*num);

	return vec;
}

int cvec_glFBO(cvector_glFBO* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glFBO_SZ;

	if (!(vec->a = (glFBO*)CVEC_MALLOC(vec->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glFBO(cvector_glFBO* vec, glFBO* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glFBO_SZ;
	vec->size = num;
	if (!(vec->a = (glFBO*)CVEC_MALLOC(vec->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glFBO)*num);

	return 1;
}

int cvec_copyc_glFBO(void* dest, void* src)
{
	cvector_glFBO* vec1 = (cvector_glFBO*)dest;
	cvector_glFBO* vec2 = (cvector_glFBO*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glFBO(vec1, vec2);
}

int cvec_copy_glFBO(cvector_glFBO* dest, cvector_glFBO* src)
{
	glFBO* tmp = NULL;
	if (!(tmp = (glFBO*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glFBO));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glFBO(cvector_glFBO* vec, glFBO a)
{
	glFBO* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glFBO_ALLOCATOR(vec->capacity);
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glFBO cvec_pop_glFBO(cvector_glFBO* vec)
{
	return vec->a[--vec->size];
}

glFBO* cvec_back_glFBO(cvector_glFBO* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glFBO(cvector_glFBO* vec, cvec_sz num)
{
	glFBO* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glFBO_SZ;
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO a)
{
	glFBO* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glFBO));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glFBO_ALLOCATOR(vec->capacity);
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glFBO));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO* a, cvec_sz num)
{
	glFBO* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glFBO_SZ;
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glFBO));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glFBO));
	vec->size += num;
	return 1;
}

glFBO cvec_replace_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO a)
{
	glFBO tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glFBO(cvector_glFBO* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glFBO));
	vec->size -= d;
}


int cvec_reserve_glFBO(cvector_glFBO* vec, cvec_sz size)
{
	glFBO* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*(size+CVEC_glFBO_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glFBO_SZ;
	}
	return 1;
}

int cvec_set_cap_glFBO(cvector_glFBO* vec, cvec_sz size)
{
	glFBO* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glFBO(cvector_glFBO* vec, glFBO val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glFBO(cvector_glFBO* vec, glFBO val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glFBO(cvector_glFBO* vec) { vec->size = 0; }

void cvec_free_glFBO_heap(void* vec)
{
	cvector_glFBO* tmp = (cvector_glFBO*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glFBO(void* vec)
{
	cvector_glFBO* tmp = (cvector_glFBO*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}

#endif
