
#ifndef CVEC_SIZE_T
#include <stdlib.h>
#define CVEC_SIZE_T size_t
#endif

#ifndef CVEC_SZ
#define CVEC_SZ
typedef CVEC_SIZE_T cvec_sz;
#endif


/** Data structure for glVertex_Array vector. */
typedef struct cvector_glVertex_Array
{
	glVertex_Array* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glVertex_Array;



extern cvec_sz CVEC_glVertex_Array_SZ;

int cvec_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array* vals, cvec_sz num);

cvector_glVertex_Array* cvec_glVertex_Array_heap(cvec_sz size, cvec_sz capacity);
cvector_glVertex_Array* cvec_init_glVertex_Array_heap(glVertex_Array* vals, cvec_sz num);
int cvec_copyc_glVertex_Array(void* dest, void* src);
int cvec_copy_glVertex_Array(cvector_glVertex_Array* dest, cvector_glVertex_Array* src);

int cvec_push_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array a);
glVertex_Array cvec_pop_glVertex_Array(cvector_glVertex_Array* vec);

int cvec_extend_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz num);
int cvec_insert_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz i, glVertex_Array a);
int cvec_insert_array_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz i, glVertex_Array* a, cvec_sz num);
glVertex_Array cvec_replace_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz i, glVertex_Array a);
void cvec_erase_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz size);
#define cvec_shrink_to_fit_glVertex_Array(vec) cvec_set_cap_glVertex_Array((vec), (vec)->size)
int cvec_set_cap_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz size);
void cvec_set_val_sz_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val);
void cvec_set_val_cap_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val);

glVertex_Array* cvec_back_glVertex_Array(cvector_glVertex_Array* vec);

void cvec_clear_glVertex_Array(cvector_glVertex_Array* vec);
void cvec_free_glVertex_Array_heap(void* vec);
void cvec_free_glVertex_Array(void* vec);



/** Data structure for glBuffer vector. */
typedef struct cvector_glBuffer
{
	glBuffer* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glBuffer;



extern cvec_sz CVEC_glBuffer_SZ;

int cvec_glBuffer(cvector_glBuffer* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glBuffer(cvector_glBuffer* vec, glBuffer* vals, cvec_sz num);

cvector_glBuffer* cvec_glBuffer_heap(cvec_sz size, cvec_sz capacity);
cvector_glBuffer* cvec_init_glBuffer_heap(glBuffer* vals, cvec_sz num);
int cvec_copyc_glBuffer(void* dest, void* src);
int cvec_copy_glBuffer(cvector_glBuffer* dest, cvector_glBuffer* src);

int cvec_push_glBuffer(cvector_glBuffer* vec, glBuffer a);
glBuffer cvec_pop_glBuffer(cvector_glBuffer* vec);

int cvec_extend_glBuffer(cvector_glBuffer* vec, cvec_sz num);
int cvec_insert_glBuffer(cvector_glBuffer* vec, cvec_sz i, glBuffer a);
int cvec_insert_array_glBuffer(cvector_glBuffer* vec, cvec_sz i, glBuffer* a, cvec_sz num);
glBuffer cvec_replace_glBuffer(cvector_glBuffer* vec, cvec_sz i, glBuffer a);
void cvec_erase_glBuffer(cvector_glBuffer* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glBuffer(cvector_glBuffer* vec, cvec_sz size);
#define cvec_shrink_to_fit_glBuffer(vec) cvec_set_cap_glBuffer((vec), (vec)->size)
int cvec_set_cap_glBuffer(cvector_glBuffer* vec, cvec_sz size);
void cvec_set_val_sz_glBuffer(cvector_glBuffer* vec, glBuffer val);
void cvec_set_val_cap_glBuffer(cvector_glBuffer* vec, glBuffer val);

glBuffer* cvec_back_glBuffer(cvector_glBuffer* vec);

void cvec_clear_glBuffer(cvector_glBuffer* vec);
void cvec_free_glBuffer_heap(void* vec);
void cvec_free_glBuffer(void* vec);



/** Data structure for glTexture vector. */
typedef struct cvector_glTexture
{
	glTexture* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glTexture;



extern cvec_sz CVEC_glTexture_SZ;

int cvec_glTexture(cvector_glTexture* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glTexture(cvector_glTexture* vec, glTexture* vals, cvec_sz num);

cvector_glTexture* cvec_glTexture_heap(cvec_sz size, cvec_sz capacity);
cvector_glTexture* cvec_init_glTexture_heap(glTexture* vals, cvec_sz num);
int cvec_copyc_glTexture(void* dest, void* src);
int cvec_copy_glTexture(cvector_glTexture* dest, cvector_glTexture* src);

int cvec_push_glTexture(cvector_glTexture* vec, glTexture a);
glTexture cvec_pop_glTexture(cvector_glTexture* vec);

int cvec_extend_glTexture(cvector_glTexture* vec, cvec_sz num);
int cvec_insert_glTexture(cvector_glTexture* vec, cvec_sz i, glTexture a);
int cvec_insert_array_glTexture(cvector_glTexture* vec, cvec_sz i, glTexture* a, cvec_sz num);
glTexture cvec_replace_glTexture(cvector_glTexture* vec, cvec_sz i, glTexture a);
void cvec_erase_glTexture(cvector_glTexture* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glTexture(cvector_glTexture* vec, cvec_sz size);
#define cvec_shrink_to_fit_glTexture(vec) cvec_set_cap_glTexture((vec), (vec)->size)
int cvec_set_cap_glTexture(cvector_glTexture* vec, cvec_sz size);
void cvec_set_val_sz_glTexture(cvector_glTexture* vec, glTexture val);
void cvec_set_val_cap_glTexture(cvector_glTexture* vec, glTexture val);

glTexture* cvec_back_glTexture(cvector_glTexture* vec);

void cvec_clear_glTexture(cvector_glTexture* vec);
void cvec_free_glTexture_heap(void* vec);
void cvec_free_glTexture(void* vec);



/** Data structure for glProgram vector. */
typedef struct cvector_glProgram
{
	glProgram* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glProgram;



extern cvec_sz CVEC_glProgram_SZ;

int cvec_glProgram(cvector_glProgram* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glProgram(cvector_glProgram* vec, glProgram* vals, cvec_sz num);

cvector_glProgram* cvec_glProgram_heap(cvec_sz size, cvec_sz capacity);
cvector_glProgram* cvec_init_glProgram_heap(glProgram* vals, cvec_sz num);
int cvec_copyc_glProgram(void* dest, void* src);
int cvec_copy_glProgram(cvector_glProgram* dest, cvector_glProgram* src);

int cvec_push_glProgram(cvector_glProgram* vec, glProgram a);
glProgram cvec_pop_glProgram(cvector_glProgram* vec);

int cvec_extend_glProgram(cvector_glProgram* vec, cvec_sz num);
int cvec_insert_glProgram(cvector_glProgram* vec, cvec_sz i, glProgram a);
int cvec_insert_array_glProgram(cvector_glProgram* vec, cvec_sz i, glProgram* a, cvec_sz num);
glProgram cvec_replace_glProgram(cvector_glProgram* vec, cvec_sz i, glProgram a);
void cvec_erase_glProgram(cvector_glProgram* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glProgram(cvector_glProgram* vec, cvec_sz size);
#define cvec_shrink_to_fit_glProgram(vec) cvec_set_cap_glProgram((vec), (vec)->size)
int cvec_set_cap_glProgram(cvector_glProgram* vec, cvec_sz size);
void cvec_set_val_sz_glProgram(cvector_glProgram* vec, glProgram val);
void cvec_set_val_cap_glProgram(cvector_glProgram* vec, glProgram val);

glProgram* cvec_back_glProgram(cvector_glProgram* vec);

void cvec_clear_glProgram(cvector_glProgram* vec);
void cvec_free_glProgram_heap(void* vec);
void cvec_free_glProgram(void* vec);



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

