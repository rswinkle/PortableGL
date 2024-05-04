#ifndef CVECTOR_glVertex_H
#define CVECTOR_glVertex_H

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

/** Data structure for glVertex vector. */
typedef struct cvector_glVertex
{
	glVertex* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glVertex;



extern cvec_sz CVEC_glVertex_SZ;

int cvec_glVertex(cvector_glVertex* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glVertex(cvector_glVertex* vec, glVertex* vals, cvec_sz num);

cvector_glVertex* cvec_glVertex_heap(cvec_sz size, cvec_sz capacity);
cvector_glVertex* cvec_init_glVertex_heap(glVertex* vals, cvec_sz num);
int cvec_copyc_glVertex(void* dest, void* src);
int cvec_copy_glVertex(cvector_glVertex* dest, cvector_glVertex* src);

int cvec_push_glVertex(cvector_glVertex* vec, glVertex a);
glVertex cvec_pop_glVertex(cvector_glVertex* vec);

int cvec_extend_glVertex(cvector_glVertex* vec, cvec_sz num);
int cvec_insert_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex a);
int cvec_insert_array_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex* a, cvec_sz num);
glVertex cvec_replace_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex a);
void cvec_erase_glVertex(cvector_glVertex* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glVertex(cvector_glVertex* vec, cvec_sz size);
#define cvec_shrink_to_fit_glVertex(vec) cvec_set_cap_glVertex((vec), (vec)->size)
int cvec_set_cap_glVertex(cvector_glVertex* vec, cvec_sz size);
void cvec_set_val_sz_glVertex(cvector_glVertex* vec, glVertex val);
void cvec_set_val_cap_glVertex(cvector_glVertex* vec, glVertex val);

glVertex* cvec_back_glVertex(cvector_glVertex* vec);

void cvec_clear_glVertex(cvector_glVertex* vec);
void cvec_free_glVertex_heap(void* vec);
void cvec_free_glVertex(void* vec);

#ifdef __cplusplus
}
#endif

/* CVECTOR_glVertex_H */
#endif


#ifdef CVECTOR_glVertex_IMPLEMENTATION

cvec_sz CVEC_glVertex_SZ = 50;

#define CVEC_glVertex_ALLOCATOR(x) ((x+1) * 2)

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

cvector_glVertex* cvec_glVertex_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glVertex* vec;
	if (!(vec = (cvector_glVertex*)CVEC_MALLOC(sizeof(cvector_glVertex)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glVertex_SZ;

	if (!(vec->a = (glVertex*)CVEC_MALLOC(vec->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glVertex* cvec_init_glVertex_heap(glVertex* vals, cvec_sz num)
{
	cvector_glVertex* vec;
	
	if (!(vec = (cvector_glVertex*)CVEC_MALLOC(sizeof(cvector_glVertex)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glVertex_SZ;
	vec->size = num;
	if (!(vec->a = (glVertex*)CVEC_MALLOC(vec->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glVertex)*num);

	return vec;
}

int cvec_glVertex(cvector_glVertex* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glVertex_SZ;

	if (!(vec->a = (glVertex*)CVEC_MALLOC(vec->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glVertex(cvector_glVertex* vec, glVertex* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glVertex_SZ;
	vec->size = num;
	if (!(vec->a = (glVertex*)CVEC_MALLOC(vec->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glVertex)*num);

	return 1;
}

int cvec_copyc_glVertex(void* dest, void* src)
{
	cvector_glVertex* vec1 = (cvector_glVertex*)dest;
	cvector_glVertex* vec2 = (cvector_glVertex*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glVertex(vec1, vec2);
}

int cvec_copy_glVertex(cvector_glVertex* dest, cvector_glVertex* src)
{
	glVertex* tmp = NULL;
	if (!(tmp = (glVertex*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glVertex));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glVertex(cvector_glVertex* vec, glVertex a)
{
	glVertex* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glVertex_ALLOCATOR(vec->capacity);
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glVertex cvec_pop_glVertex(cvector_glVertex* vec)
{
	return vec->a[--vec->size];
}

glVertex* cvec_back_glVertex(cvector_glVertex* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glVertex(cvector_glVertex* vec, cvec_sz num)
{
	glVertex* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glVertex_SZ;
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex a)
{
	glVertex* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glVertex));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glVertex_ALLOCATOR(vec->capacity);
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glVertex));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex* a, cvec_sz num)
{
	glVertex* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glVertex_SZ;
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glVertex));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glVertex));
	vec->size += num;
	return 1;
}

glVertex cvec_replace_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex a)
{
	glVertex tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glVertex(cvector_glVertex* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glVertex));
	vec->size -= d;
}


int cvec_reserve_glVertex(cvector_glVertex* vec, cvec_sz size)
{
	glVertex* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*(size+CVEC_glVertex_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glVertex_SZ;
	}
	return 1;
}

int cvec_set_cap_glVertex(cvector_glVertex* vec, cvec_sz size)
{
	glVertex* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glVertex(cvector_glVertex* vec, glVertex val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glVertex(cvector_glVertex* vec, glVertex val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glVertex(cvector_glVertex* vec) { vec->size = 0; }

void cvec_free_glVertex_heap(void* vec)
{
	cvector_glVertex* tmp = (cvector_glVertex*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glVertex(void* vec)
{
	cvector_glVertex* tmp = (cvector_glVertex*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}

#endif
