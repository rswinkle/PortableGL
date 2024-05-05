
#ifndef CVEC_SIZE_T
#include <stdlib.h>
#define CVEC_SIZE_T size_t
#endif

#ifndef CVEC_SZ
#define CVEC_SZ
typedef CVEC_SIZE_T cvec_sz;
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
#define cvec_shrink_to_fit_float(vec) cvec_set_cap_float((vec), (vec)->size)
int cvec_set_cap_float(cvector_float* vec, cvec_sz size);
void cvec_set_val_sz_float(cvector_float* vec, float val);
void cvec_set_val_cap_float(cvector_float* vec, float val);

float* cvec_back_float(cvector_float* vec);

void cvec_clear_float(cvector_float* vec);
void cvec_free_float_heap(void* vec);
void cvec_free_float(void* vec);

