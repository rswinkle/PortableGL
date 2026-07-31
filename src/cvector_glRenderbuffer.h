#ifndef CVECTOR_glRenderbuffer_H
#define CVECTOR_glRenderbuffer_H

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

/** Data structure for glRenderbuffer vector. */
typedef struct cvector_glRenderbuffer
{
	glRenderbuffer* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glRenderbuffer;



extern cvec_sz CVEC_glRenderbuffer_SZ;

int cvec_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer* vals, cvec_sz num);

cvector_glRenderbuffer* cvec_glRenderbuffer_heap(cvec_sz size, cvec_sz capacity);
cvector_glRenderbuffer* cvec_init_glRenderbuffer_heap(glRenderbuffer* vals, cvec_sz num);
int cvec_copyc_glRenderbuffer(void* dest, void* src);
int cvec_copy_glRenderbuffer(cvector_glRenderbuffer* dest, cvector_glRenderbuffer* src);

int cvec_push_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer a);
glRenderbuffer cvec_pop_glRenderbuffer(cvector_glRenderbuffer* vec);

int cvec_extend_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz num);
int cvec_insert_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer a);
int cvec_insert_array_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer* a, cvec_sz num);
glRenderbuffer cvec_replace_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer a);
void cvec_erase_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size);
#define cvec_shrink_to_fit_glRenderbuffer(vec) cvec_set_cap_glRenderbuffer((vec), (vec)->size)
int cvec_set_cap_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size);
void cvec_set_val_sz_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer val);
void cvec_set_val_cap_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer val);

glRenderbuffer* cvec_back_glRenderbuffer(cvector_glRenderbuffer* vec);

void cvec_clear_glRenderbuffer(cvector_glRenderbuffer* vec);
void cvec_free_glRenderbuffer_heap(void* vec);
void cvec_free_glRenderbuffer(void* vec);

#ifdef __cplusplus
}
#endif

/* CVECTOR_glRenderbuffer_H */
#endif


#ifdef CVECTOR_glRenderbuffer_IMPLEMENTATION

cvec_sz CVEC_glRenderbuffer_SZ = 50;

#define CVEC_glRenderbuffer_ALLOCATOR(x) ((x+1) * 2)

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

cvector_glRenderbuffer* cvec_glRenderbuffer_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glRenderbuffer* vec;
	if (!(vec = (cvector_glRenderbuffer*)CVEC_MALLOC(sizeof(cvector_glRenderbuffer)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glRenderbuffer_SZ;

	if (!(vec->a = (glRenderbuffer*)CVEC_MALLOC(vec->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glRenderbuffer* cvec_init_glRenderbuffer_heap(glRenderbuffer* vals, cvec_sz num)
{
	cvector_glRenderbuffer* vec;
	
	if (!(vec = (cvector_glRenderbuffer*)CVEC_MALLOC(sizeof(cvector_glRenderbuffer)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glRenderbuffer_SZ;
	vec->size = num;
	if (!(vec->a = (glRenderbuffer*)CVEC_MALLOC(vec->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glRenderbuffer)*num);

	return vec;
}

int cvec_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glRenderbuffer_SZ;

	if (!(vec->a = (glRenderbuffer*)CVEC_MALLOC(vec->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glRenderbuffer_SZ;
	vec->size = num;
	if (!(vec->a = (glRenderbuffer*)CVEC_MALLOC(vec->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glRenderbuffer)*num);

	return 1;
}

int cvec_copyc_glRenderbuffer(void* dest, void* src)
{
	cvector_glRenderbuffer* vec1 = (cvector_glRenderbuffer*)dest;
	cvector_glRenderbuffer* vec2 = (cvector_glRenderbuffer*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glRenderbuffer(vec1, vec2);
}

int cvec_copy_glRenderbuffer(cvector_glRenderbuffer* dest, cvector_glRenderbuffer* src)
{
	glRenderbuffer* tmp = NULL;
	if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glRenderbuffer));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer a)
{
	glRenderbuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glRenderbuffer_ALLOCATOR(vec->capacity);
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glRenderbuffer cvec_pop_glRenderbuffer(cvector_glRenderbuffer* vec)
{
	return vec->a[--vec->size];
}

glRenderbuffer* cvec_back_glRenderbuffer(cvector_glRenderbuffer* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz num)
{
	glRenderbuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glRenderbuffer_SZ;
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer a)
{
	glRenderbuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glRenderbuffer));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glRenderbuffer_ALLOCATOR(vec->capacity);
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glRenderbuffer));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer* a, cvec_sz num)
{
	glRenderbuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glRenderbuffer_SZ;
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glRenderbuffer));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glRenderbuffer));
	vec->size += num;
	return 1;
}

glRenderbuffer cvec_replace_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer a)
{
	glRenderbuffer tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glRenderbuffer));
	vec->size -= d;
}


int cvec_reserve_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size)
{
	glRenderbuffer* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*(size+CVEC_glRenderbuffer_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glRenderbuffer_SZ;
	}
	return 1;
}

int cvec_set_cap_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size)
{
	glRenderbuffer* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glRenderbuffer(cvector_glRenderbuffer* vec) { vec->size = 0; }

void cvec_free_glRenderbuffer_heap(void* vec)
{
	cvector_glRenderbuffer* tmp = (cvector_glRenderbuffer*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glRenderbuffer(void* vec)
{
	cvector_glRenderbuffer* tmp = (cvector_glRenderbuffer*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}

#endif
