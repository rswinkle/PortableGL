/*

CVector 4.1.1 MIT Licensed vector (dynamic array) library in strict C89
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

Copyright (c) 2011-2023 Robert Winkler

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

#ifndef CVECTOR_MACRO_H
#define CVECTOR_MACRO_H

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

#ifndef CVEC_SIZE_TYPE
#define CVEC_SIZE_TYPE size_t
#endif

#ifndef CVEC_SZ
#define CVEC_SZ
typedef CVEC_SIZE_TYPE cvec_sz;
#endif


/*
 User can optionally wrap CVEC_NEW_DECLS(2) with extern "C"
 since it's not part of the macro
*/

#define CVEC_NEW_DECLS(TYPE)                                                          \
  typedef struct cvector_##TYPE {                                                     \
    TYPE* a;                                                                          \
    cvec_sz size;                                                                     \
    cvec_sz capacity;                                                                 \
  } cvector_##TYPE;                                                                   \
                                                                                      \
  extern cvec_sz CVEC_##TYPE##_SZ;                                                    \
                                                                                      \
  int cvec_##TYPE(cvector_##TYPE* vec, cvec_sz size, cvec_sz capacity);               \
  int cvec_init_##TYPE(cvector_##TYPE* vec, TYPE* vals, cvec_sz num);                 \
                                                                                      \
  cvector_##TYPE* cvec_##TYPE##_heap(cvec_sz size, cvec_sz capacity);                 \
  cvector_##TYPE* cvec_init_##TYPE##_heap(TYPE* vals, cvec_sz num);                   \
                                                                                      \
  int cvec_copyc_##TYPE(void* dest, void* src);                                       \
  int cvec_copy_##TYPE(cvector_##TYPE* dest, cvector_##TYPE* src);                    \
                                                                                      \
  int cvec_push_##TYPE(cvector_##TYPE* vec, TYPE a);                                  \
  TYPE cvec_pop_##TYPE(cvector_##TYPE* vec);                                          \
                                                                                      \
  int cvec_extend_##TYPE(cvector_##TYPE* vec, cvec_sz num);                           \
  int cvec_insert_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE a);                     \
  int cvec_insert_array_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num); \
  TYPE cvec_replace_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE a);                   \
  void cvec_erase_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end);            \
  int cvec_reserve_##TYPE(cvector_##TYPE* vec, cvec_sz size);                         \
  int cvec_set_cap_##TYPE(cvector_##TYPE* vec, cvec_sz size);                         \
  void cvec_set_val_sz_##TYPE(cvector_##TYPE* vec, TYPE val);                         \
  void cvec_set_val_cap_##TYPE(cvector_##TYPE* vec, TYPE val);                        \
                                                                                      \
  TYPE* cvec_back_##TYPE(cvector_##TYPE* vec);                                        \
                                                                                      \
  void cvec_clear_##TYPE(cvector_##TYPE* vec);                                        \
  void cvec_free_##TYPE##_heap(void* vec);                                            \
  void cvec_free_##TYPE(void* vec);

#define CVEC_NEW_DEFS(TYPE, RESIZE_MACRO)                                                   \
  cvec_sz CVEC_##TYPE##_SZ = 50;                                                            \
                                                                                            \
  cvector_##TYPE* cvec_##TYPE##_heap(cvec_sz size, cvec_sz capacity)                        \
  {                                                                                         \
    cvector_##TYPE* vec;                                                                    \
    if (!(vec = (cvector_##TYPE*)CVEC_MALLOC(sizeof(cvector_##TYPE)))) {                    \
      CVEC_ASSERT(vec != NULL);                                                             \
      return NULL;                                                                          \
    }                                                                                       \
                                                                                            \
    vec->size     = size;                                                                   \
    vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size))          \
                        ? capacity                                                          \
                        : vec->size + CVEC_##TYPE##_SZ;                                     \
                                                                                            \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                     \
      CVEC_ASSERT(vec->a != NULL);                                                          \
      CVEC_FREE(vec);                                                                       \
      return NULL;                                                                          \
    }                                                                                       \
                                                                                            \
    return vec;                                                                             \
  }                                                                                         \
                                                                                            \
  cvector_##TYPE* cvec_init_##TYPE##_heap(TYPE* vals, cvec_sz num)                          \
  {                                                                                         \
    cvector_##TYPE* vec;                                                                    \
                                                                                            \
    if (!(vec = (cvector_##TYPE*)CVEC_MALLOC(sizeof(cvector_##TYPE)))) {                    \
      CVEC_ASSERT(vec != NULL);                                                             \
      return NULL;                                                                          \
    }                                                                                       \
                                                                                            \
    vec->capacity = num + CVEC_##TYPE##_SZ;                                                 \
    vec->size     = num;                                                                    \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                     \
      CVEC_ASSERT(vec->a != NULL);                                                          \
      CVEC_FREE(vec);                                                                       \
      return NULL;                                                                          \
    }                                                                                       \
                                                                                            \
    CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE) * num);                                         \
                                                                                            \
    return vec;                                                                             \
  }                                                                                         \
                                                                                            \
  int cvec_##TYPE(cvector_##TYPE* vec, cvec_sz size, cvec_sz capacity)                      \
  {                                                                                         \
    vec->size     = size;                                                                   \
    vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size))          \
                        ? capacity                                                          \
                        : vec->size + CVEC_##TYPE##_SZ;                                     \
                                                                                            \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                     \
      CVEC_ASSERT(vec->a != NULL);                                                          \
      vec->size = vec->capacity = 0;                                                        \
      return 0;                                                                             \
    }                                                                                       \
                                                                                            \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_init_##TYPE(cvector_##TYPE* vec, TYPE* vals, cvec_sz num)                        \
  {                                                                                         \
    vec->capacity = num + CVEC_##TYPE##_SZ;                                                 \
    vec->size     = num;                                                                    \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                     \
      CVEC_ASSERT(vec->a != NULL);                                                          \
      vec->size = vec->capacity = 0;                                                        \
      return 0;                                                                             \
    }                                                                                       \
                                                                                            \
    CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE) * num);                                         \
                                                                                            \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_copyc_##TYPE(void* dest, void* src)                                              \
  {                                                                                         \
    cvector_##TYPE* vec1 = (cvector_##TYPE*)dest;                                           \
    cvector_##TYPE* vec2 = (cvector_##TYPE*)src;                                            \
                                                                                            \
    vec1->a = NULL;                                                                         \
    vec1->size = 0;                                                                         \
    vec1->capacity = 0;                                                                     \
                                                                                            \
    return cvec_copy_##TYPE(vec1, vec2);                                                    \
  }                                                                                         \
                                                                                            \
  int cvec_copy_##TYPE(cvector_##TYPE* dest, cvector_##TYPE* src)                           \
  {                                                                                         \
    TYPE* tmp = NULL;                                                                       \
    if (!(tmp = (TYPE*)CVEC_REALLOC(dest->a, src->capacity*sizeof(TYPE)))) {                \
      CVEC_ASSERT(tmp != NULL);                                                             \
      return 0;                                                                             \
    }                                                                                       \
    dest->a = tmp;                                                                          \
                                                                                            \
    CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(TYPE));                                  \
    dest->size = src->size;                                                                 \
    dest->capacity = src->capacity;                                                         \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_push_##TYPE(cvector_##TYPE* vec, TYPE a)                                         \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    cvec_sz tmp_sz;                                                                         \
    if (vec->capacity > vec->size) {                                                        \
      vec->a[vec->size++] = a;                                                              \
    } else {                                                                                \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                 \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                    \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a              = tmp;                                                            \
      vec->a[vec->size++] = a;                                                              \
      vec->capacity       = tmp_sz;                                                         \
    }                                                                                       \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  TYPE cvec_pop_##TYPE(cvector_##TYPE* vec) { return vec->a[--vec->size]; }                 \
                                                                                            \
  TYPE* cvec_back_##TYPE(cvector_##TYPE* vec) { return &vec->a[vec->size - 1]; }            \
                                                                                            \
  int cvec_extend_##TYPE(cvector_##TYPE* vec, cvec_sz num)                                  \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    cvec_sz tmp_sz;                                                                         \
    if (vec->capacity < vec->size + num) {                                                  \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                    \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a        = tmp;                                                                  \
      vec->capacity = tmp_sz;                                                               \
    }                                                                                       \
                                                                                            \
    vec->size += num;                                                                       \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_insert_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE a)                            \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    cvec_sz tmp_sz;                                                                         \
    if (vec->capacity > vec->size) {                                                        \
      CVEC_MEMMOVE(&vec->a[i + 1], &vec->a[i], (vec->size - i) * sizeof(TYPE));             \
      vec->a[i] = a;                                                                        \
    } else {                                                                                \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                 \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                    \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a = tmp;                                                                         \
      CVEC_MEMMOVE(&vec->a[i + 1], &vec->a[i], (vec->size - i) * sizeof(TYPE));             \
      vec->a[i]     = a;                                                                    \
      vec->capacity = tmp_sz;                                                               \
    }                                                                                       \
                                                                                            \
    vec->size++;                                                                            \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_insert_array_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num)        \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    cvec_sz tmp_sz;                                                                         \
    if (vec->capacity < vec->size + num) {                                                  \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                    \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a        = tmp;                                                                  \
      vec->capacity = tmp_sz;                                                               \
    }                                                                                       \
                                                                                            \
    CVEC_MEMMOVE(&vec->a[i + num], &vec->a[i], (vec->size - i) * sizeof(TYPE));             \
    CVEC_MEMMOVE(&vec->a[i], a, num * sizeof(TYPE));                                        \
    vec->size += num;                                                                       \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  TYPE cvec_replace_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE a)                          \
  {                                                                                         \
    TYPE tmp  = vec->a[i];                                                                  \
    vec->a[i] = a;                                                                          \
    return tmp;                                                                             \
  }                                                                                         \
                                                                                            \
  void cvec_erase_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end)                   \
  {                                                                                         \
    cvec_sz d = end - start + 1;                                                            \
    CVEC_MEMMOVE(&vec->a[start], &vec->a[end + 1], (vec->size - 1 - end) * sizeof(TYPE));   \
    vec->size -= d;                                                                         \
  }                                                                                         \
                                                                                            \
  int cvec_reserve_##TYPE(cvector_##TYPE* vec, cvec_sz size)                                \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    if (vec->capacity < size) {                                                             \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * (size + CVEC_##TYPE##_SZ)))) { \
        CVEC_ASSERT(tmp != NULL);                                                           \
        return 0;                                                                           \
      }                                                                                     \
      vec->a        = tmp;                                                                  \
      vec->capacity = size + CVEC_##TYPE##_SZ;                                              \
    }                                                                                       \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  int cvec_set_cap_##TYPE(cvector_##TYPE* vec, cvec_sz size)                                \
  {                                                                                         \
    TYPE* tmp;                                                                              \
    if (size < vec->size) {                                                                 \
      vec->size = size;                                                                     \
    }                                                                                       \
                                                                                            \
    if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * size))) {                        \
      CVEC_ASSERT(tmp != NULL);                                                             \
      return 0;                                                                             \
    }                                                                                       \
    vec->a        = tmp;                                                                    \
    vec->capacity = size;                                                                   \
    return 1;                                                                               \
  }                                                                                         \
                                                                                            \
  void cvec_set_val_sz_##TYPE(cvector_##TYPE* vec, TYPE val)                                \
  {                                                                                         \
    cvec_sz i;                                                                              \
    for (i = 0; i < vec->size; i++) {                                                       \
      vec->a[i] = val;                                                                      \
    }                                                                                       \
  }                                                                                         \
                                                                                            \
  void cvec_set_val_cap_##TYPE(cvector_##TYPE* vec, TYPE val)                               \
  {                                                                                         \
    cvec_sz i;                                                                              \
    for (i = 0; i < vec->capacity; i++) {                                                   \
      vec->a[i] = val;                                                                      \
    }                                                                                       \
  }                                                                                         \
                                                                                            \
  void cvec_clear_##TYPE(cvector_##TYPE* vec) { vec->size = 0; }                            \
                                                                                            \
  void cvec_free_##TYPE##_heap(void* vec)                                                   \
  {                                                                                         \
    cvector_##TYPE* tmp = (cvector_##TYPE*)vec;                                             \
    if (!tmp) return;                                                                       \
    CVEC_FREE(tmp->a);                                                                      \
    CVEC_FREE(tmp);                                                                         \
  }                                                                                         \
                                                                                            \
  void cvec_free_##TYPE(void* vec)                                                          \
  {                                                                                         \
    cvector_##TYPE* tmp = (cvector_##TYPE*)vec;                                             \
    CVEC_FREE(tmp->a);                                                                      \
    tmp->size     = 0;                                                                      \
    tmp->capacity = 0;                                                                      \
  }

#define CVEC_NEW_DECLS2(TYPE)                                                                    \
  typedef struct cvector_##TYPE {                                                                \
    TYPE* a;                                                                                     \
    cvec_sz size;                                                                                \
    cvec_sz capacity;                                                                            \
    void (*elem_free)(void*);                                                                    \
    int (*elem_init)(void*, void*);                                                              \
  } cvector_##TYPE;                                                                              \
                                                                                                 \
  extern cvec_sz CVEC_##TYPE##_SZ;                                                               \
                                                                                                 \
  int cvec_##TYPE(cvector_##TYPE* vec, cvec_sz size, cvec_sz capacity, void (*elem_free)(void*), \
                  int (*elem_init)(void*, void*));                                               \
  int cvec_init_##TYPE(cvector_##TYPE* vec, TYPE* vals, cvec_sz num, void (*elem_free)(void*),   \
                       int (*elem_init)(void*, void*));                                          \
                                                                                                 \
  cvector_##TYPE* cvec_##TYPE##_heap(cvec_sz size, cvec_sz capacity, void (*elem_free)(void*),   \
                                     int (*elem_init)(void*, void*));                            \
  cvector_##TYPE* cvec_init_##TYPE##_heap(TYPE* vals, cvec_sz num, void (*elem_free)(void*),     \
                                          int (*elem_init)(void*, void*));                       \
                                                                                                 \
  int cvec_copyc_##TYPE(void* dest, void* src);                                                  \
  int cvec_copy_##TYPE(cvector_##TYPE* dest, cvector_##TYPE* src);                               \
                                                                                                 \
  int cvec_push_##TYPE(cvector_##TYPE* vec, TYPE* val);                                          \
  void cvec_pop_##TYPE(cvector_##TYPE* vec, TYPE* ret);                                          \
                                                                                                 \
  int cvec_pushm_##TYPE(cvector_##TYPE* vec, TYPE* a);                                           \
  void cvec_popm_##TYPE(cvector_##TYPE* vec, TYPE* ret);                                         \
  int cvec_insertm_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a);                              \
  int cvec_insert_arraym_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num);           \
  void cvec_replacem_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, TYPE* ret);                 \
                                                                                                 \
  int cvec_extend_##TYPE(cvector_##TYPE* vec, cvec_sz num);                                      \
  int cvec_insert_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a);                               \
  int cvec_insert_array_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num);            \
  int cvec_replace_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, TYPE* ret);                   \
  void cvec_erase_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end);                       \
  void cvec_remove_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end);                      \
  int cvec_reserve_##TYPE(cvector_##TYPE* vec, cvec_sz size);                                    \
  int cvec_set_cap_##TYPE(cvector_##TYPE* vec, cvec_sz size);                                    \
  int cvec_set_val_sz_##TYPE(cvector_##TYPE* vec, TYPE* val);                                    \
  int cvec_set_val_cap_##TYPE(cvector_##TYPE* vec, TYPE* val);                                   \
                                                                                                 \
  TYPE* cvec_back_##TYPE(cvector_##TYPE* vec);                                                   \
                                                                                                 \
  void cvec_clear_##TYPE(cvector_##TYPE* vec);                                                   \
  void cvec_free_##TYPE##_heap(void* vec);                                                       \
  void cvec_free_##TYPE(void* vec);

#define CVEC_NEW_DEFS2(TYPE, RESIZE_MACRO)                                                       \
  cvec_sz CVEC_##TYPE##_SZ = 20;                                                                 \
                                                                                                 \
  cvector_##TYPE* cvec_##TYPE##_heap(cvec_sz size, cvec_sz capacity, void (*elem_free)(void*),   \
                                     int (*elem_init)(void*, void*))                             \
  {                                                                                              \
    cvector_##TYPE* vec;                                                                         \
    if (!(vec = (cvector_##TYPE*)CVEC_MALLOC(sizeof(cvector_##TYPE)))) {                         \
      CVEC_ASSERT(vec != NULL);                                                                  \
      return NULL;                                                                               \
    }                                                                                            \
                                                                                                 \
    vec->size     = size;                                                                        \
    vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size))               \
                        ? capacity                                                               \
                        : vec->size + CVEC_##TYPE##_SZ;                                          \
                                                                                                 \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                          \
      CVEC_ASSERT(vec->a != NULL);                                                               \
      CVEC_FREE(vec);                                                                            \
      return NULL;                                                                               \
    }                                                                                            \
                                                                                                 \
    vec->elem_free = elem_free;                                                                  \
    vec->elem_init = elem_init;                                                                  \
                                                                                                 \
    return vec;                                                                                  \
  }                                                                                              \
                                                                                                 \
  cvector_##TYPE* cvec_init_##TYPE##_heap(TYPE* vals, cvec_sz num, void (*elem_free)(void*),     \
                                          int (*elem_init)(void*, void*))                        \
  {                                                                                              \
    cvector_##TYPE* vec;                                                                         \
    cvec_sz i;                                                                                   \
                                                                                                 \
    if (!(vec = (cvector_##TYPE*)CVEC_MALLOC(sizeof(cvector_##TYPE)))) {                         \
      CVEC_ASSERT(vec != NULL);                                                                  \
      return NULL;                                                                               \
    }                                                                                            \
                                                                                                 \
    vec->capacity = num + CVEC_##TYPE##_SZ;                                                      \
    vec->size     = num;                                                                         \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                          \
      CVEC_ASSERT(vec->a != NULL);                                                               \
      CVEC_FREE(vec);                                                                            \
      return NULL;                                                                               \
    }                                                                                            \
                                                                                                 \
    if (elem_init) {                                                                             \
      for (i = 0; i < num; ++i) {                                                                \
        if (!elem_init(&vec->a[i], &vals[i])) {                                                  \
          CVEC_ASSERT(0);                                                                        \
          CVEC_FREE(vec->a);                                                                     \
          CVEC_FREE(vec);                                                                        \
          return NULL;                                                                           \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE) * num);                                            \
    }                                                                                            \
                                                                                                 \
    vec->elem_free = elem_free;                                                                  \
    vec->elem_init = elem_init;                                                                  \
                                                                                                 \
    return vec;                                                                                  \
  }                                                                                              \
                                                                                                 \
  int cvec_##TYPE(cvector_##TYPE* vec, cvec_sz size, cvec_sz capacity, void (*elem_free)(void*), \
                  int (*elem_init)(void*, void*))                                                \
  {                                                                                              \
    vec->size     = size;                                                                        \
    vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size))               \
                        ? capacity                                                               \
                        : vec->size + CVEC_##TYPE##_SZ;                                          \
                                                                                                 \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                          \
      CVEC_ASSERT(vec->a != NULL);                                                               \
      vec->size = vec->capacity = 0;                                                             \
      return 0;                                                                                  \
    }                                                                                            \
                                                                                                 \
    vec->elem_free = elem_free;                                                                  \
    vec->elem_init = elem_init;                                                                  \
                                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_init_##TYPE(cvector_##TYPE* vec, TYPE* vals, cvec_sz num, void (*elem_free)(void*),   \
                       int (*elem_init)(void*, void*))                                           \
  {                                                                                              \
    cvec_sz i;                                                                                   \
                                                                                                 \
    vec->capacity = num + CVEC_##TYPE##_SZ;                                                      \
    vec->size     = num;                                                                         \
    if (!(vec->a = (TYPE*)CVEC_MALLOC(vec->capacity * sizeof(TYPE)))) {                          \
      CVEC_ASSERT(vec->a != NULL);                                                               \
      vec->size = vec->capacity = 0;                                                             \
      return 0;                                                                                  \
    }                                                                                            \
                                                                                                 \
    if (elem_init) {                                                                             \
      for (i = 0; i < num; ++i) {                                                                \
        if (!elem_init(&vec->a[i], &vals[i])) {                                                  \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(vec->a, vals, sizeof(TYPE) * num);                                            \
    }                                                                                            \
                                                                                                 \
    vec->elem_free = elem_free;                                                                  \
    vec->elem_init = elem_init;                                                                  \
                                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_copyc_##TYPE(void* dest, void* src)                                                   \
  {                                                                                              \
    cvector_##TYPE* vec1 = (cvector_##TYPE*)dest;                                                \
    cvector_##TYPE* vec2 = (cvector_##TYPE*)src;                                                 \
                                                                                                 \
    vec1->a = NULL;                                                                              \
    vec1->size = 0;                                                                              \
    vec1->capacity = 0;                                                                          \
                                                                                                 \
    return cvec_copy_##TYPE(vec1, vec2);                                                         \
  }                                                                                              \
                                                                                                 \
  int cvec_copy_##TYPE(cvector_##TYPE* dest, cvector_##TYPE* src)                                \
  {                                                                                              \
    int i;                                                                                       \
    TYPE* tmp = NULL;                                                                            \
    if (!(tmp = (TYPE*)CVEC_REALLOC(dest->a, src->capacity*sizeof(TYPE)))) {                     \
      CVEC_ASSERT(tmp != NULL);                                                                  \
      return 0;                                                                                  \
    }                                                                                            \
    dest->a = tmp;                                                                               \
                                                                                                 \
    if (src->elem_init) {                                                                        \
      for (i=0; i<src->size; ++i) {                                                              \
        if (!src->elem_init(&dest->a[i], &src->a[i])) {                                          \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(TYPE));                                     \
    }                                                                                            \
                                                                                                 \
    dest->size = src->size;                                                                      \
    dest->capacity = src->capacity;                                                              \
    dest->elem_free = src->elem_free;                                                            \
    dest->elem_init = src->elem_init;                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_push_##TYPE(cvector_##TYPE* vec, TYPE* a)                                             \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity == vec->size) {                                                            \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
    if (vec->elem_init) {                                                                        \
      if (!vec->elem_init(&vec->a[vec->size], a)) {                                              \
        CVEC_ASSERT(0);                                                                          \
        return 0;                                                                                \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(&vec->a[vec->size], a, sizeof(TYPE));                                         \
    }                                                                                            \
                                                                                                 \
    vec->size++;                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_pushm_##TYPE(cvector_##TYPE* vec, TYPE* a)                                            \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity == vec->size) {                                                            \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
    CVEC_MEMMOVE(&vec->a[vec->size], a, sizeof(TYPE));                                           \
                                                                                                 \
    vec->size++;                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  void cvec_pop_##TYPE(cvector_##TYPE* vec, TYPE* ret)                                           \
  {                                                                                              \
    if (ret) {                                                                                   \
      CVEC_MEMMOVE(ret, &vec->a[--vec->size], sizeof(TYPE));                                     \
    } else {                                                                                     \
      vec->size--;                                                                               \
    }                                                                                            \
                                                                                                 \
    if (vec->elem_free) {                                                                        \
      vec->elem_free(&vec->a[vec->size]);                                                        \
    }                                                                                            \
  }                                                                                              \
                                                                                                 \
  void cvec_popm_##TYPE(cvector_##TYPE* vec, TYPE* ret)                                          \
  {                                                                                              \
    vec->size--;                                                                                 \
    if (ret) {                                                                                   \
      CVEC_MEMMOVE(ret, &vec->a[vec->size], sizeof(TYPE));                                       \
    }                                                                                            \
  }                                                                                              \
                                                                                                 \
  TYPE* cvec_back_##TYPE(cvector_##TYPE* vec) { return &vec->a[vec->size - 1]; }                 \
                                                                                                 \
  int cvec_extend_##TYPE(cvector_##TYPE* vec, cvec_sz num)                                       \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity < vec->size + num) {                                                       \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                           \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
                                                                                                 \
    vec->size += num;                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_insert_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a)                                \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity == vec->size) {                                                            \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
                                                                                                 \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
    CVEC_MEMMOVE(&vec->a[i + 1], &vec->a[i], (vec->size - i) * sizeof(TYPE));                    \
                                                                                                 \
    if (vec->elem_init) {                                                                        \
      if (!vec->elem_init(&vec->a[i], a)) {                                                      \
        CVEC_ASSERT(0);                                                                          \
        return 0;                                                                                \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(&vec->a[i], a, sizeof(TYPE));                                                 \
    }                                                                                            \
                                                                                                 \
    vec->size++;                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_insertm_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a)                               \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity == vec->size) {                                                            \
      tmp_sz = RESIZE_MACRO(vec->capacity);                                                      \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
                                                                                                 \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
    CVEC_MEMMOVE(&vec->a[i + 1], &vec->a[i], (vec->size - i) * sizeof(TYPE));                    \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i], a, sizeof(TYPE));                                                   \
                                                                                                 \
    vec->size++;                                                                                 \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_insert_array_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num)             \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz, j;                                                                           \
    if (vec->capacity < vec->size + num) {                                                       \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                           \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i + num], &vec->a[i], (vec->size - i) * sizeof(TYPE));                  \
    if (vec->elem_init) {                                                                        \
      for (j = 0; j < num; ++j) {                                                                \
        if (!vec->elem_init(&vec->a[j + i], &a[j])) {                                            \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(&vec->a[i], a, num * sizeof(TYPE));                                           \
    }                                                                                            \
    vec->size += num;                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_insert_arraym_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, cvec_sz num)            \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    cvec_sz tmp_sz;                                                                              \
    if (vec->capacity < vec->size + num) {                                                       \
      tmp_sz = vec->capacity + num + CVEC_##TYPE##_SZ;                                           \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * tmp_sz))) {                         \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = tmp_sz;                                                                    \
    }                                                                                            \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i + num], &vec->a[i], (vec->size - i) * sizeof(TYPE));                  \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i], a, num * sizeof(TYPE));                                             \
    vec->size += num;                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_replace_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, TYPE* ret)                    \
  {                                                                                              \
    if (ret) {                                                                                   \
      CVEC_MEMMOVE(ret, &vec->a[i], sizeof(TYPE));                                               \
    } else if (vec->elem_free) {                                                                 \
      vec->elem_free(&vec->a[i]);                                                                \
    }                                                                                            \
                                                                                                 \
    if (vec->elem_init) {                                                                        \
      if (!vec->elem_init(&vec->a[i], a)) {                                                      \
        CVEC_ASSERT(0);                                                                          \
        return 0;                                                                                \
      }                                                                                          \
    } else {                                                                                     \
      CVEC_MEMMOVE(&vec->a[i], a, sizeof(TYPE));                                                 \
    }                                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  void cvec_replacem_##TYPE(cvector_##TYPE* vec, cvec_sz i, TYPE* a, TYPE* ret)                  \
  {                                                                                              \
    if (ret) {                                                                                   \
      CVEC_MEMMOVE(ret, &vec->a[i], sizeof(TYPE));                                               \
    }                                                                                            \
                                                                                                 \
    CVEC_MEMMOVE(&vec->a[i], a, sizeof(TYPE));                                                   \
  }                                                                                              \
                                                                                                 \
  void cvec_erase_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end)                        \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    cvec_sz d = end - start + 1;                                                                 \
    if (vec->elem_free) {                                                                        \
      for (i = start; i <= end; i++) {                                                           \
        vec->elem_free(&vec->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
    CVEC_MEMMOVE(&vec->a[start], &vec->a[end + 1], (vec->size - 1 - end) * sizeof(TYPE));        \
    vec->size -= d;                                                                              \
  }                                                                                              \
                                                                                                 \
  void cvec_remove_##TYPE(cvector_##TYPE* vec, cvec_sz start, cvec_sz end)                       \
  {                                                                                              \
    cvec_sz d = end - start + 1;                                                                 \
    CVEC_MEMMOVE(&vec->a[start], &vec->a[end + 1], (vec->size - 1 - end) * sizeof(TYPE));        \
    vec->size -= d;                                                                              \
  }                                                                                              \
                                                                                                 \
  int cvec_reserve_##TYPE(cvector_##TYPE* vec, cvec_sz size)                                     \
  {                                                                                              \
    TYPE* tmp;                                                                                   \
    if (vec->capacity < size) {                                                                  \
      if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * (size + CVEC_##TYPE##_SZ)))) {      \
        CVEC_ASSERT(tmp != NULL);                                                                \
        return 0;                                                                                \
      }                                                                                          \
      vec->a        = tmp;                                                                       \
      vec->capacity = size + CVEC_##TYPE##_SZ;                                                   \
    }                                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_set_cap_##TYPE(cvector_##TYPE* vec, cvec_sz size)                                     \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    TYPE* tmp;                                                                                   \
    if (size < vec->size) {                                                                      \
      if (vec->elem_free) {                                                                      \
        for (i = vec->size - 1; i >= size; i--) {                                                \
          vec->elem_free(&vec->a[i]);                                                            \
        }                                                                                        \
      }                                                                                          \
      vec->size = size;                                                                          \
    }                                                                                            \
                                                                                                 \
    vec->capacity = size;                                                                        \
                                                                                                 \
    if (!(tmp = (TYPE*)CVEC_REALLOC(vec->a, sizeof(TYPE) * size))) {                             \
      CVEC_ASSERT(tmp != NULL);                                                                  \
      return 0;                                                                                  \
    }                                                                                            \
    vec->a = tmp;                                                                                \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_set_val_sz_##TYPE(cvector_##TYPE* vec, TYPE* val)                                     \
  {                                                                                              \
    cvec_sz i;                                                                                   \
                                                                                                 \
    if (vec->elem_free) {                                                                        \
      for (i = 0; i < vec->size; i++) {                                                          \
        vec->elem_free(&vec->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
                                                                                                 \
    if (vec->elem_init) {                                                                        \
      for (i = 0; i < vec->size; i++) {                                                          \
        if (!vec->elem_init(&vec->a[i], val)) {                                                  \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      for (i = 0; i < vec->size; i++) {                                                          \
        CVEC_MEMMOVE(&vec->a[i], val, sizeof(TYPE));                                             \
      }                                                                                          \
    }                                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  int cvec_set_val_cap_##TYPE(cvector_##TYPE* vec, TYPE* val)                                    \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    if (vec->elem_free) {                                                                        \
      for (i = 0; i < vec->size; i++) {                                                          \
        vec->elem_free(&vec->a[i]);                                                              \
      }                                                                                          \
      vec->size = vec->capacity;                                                                 \
    }                                                                                            \
                                                                                                 \
    if (vec->elem_init) {                                                                        \
      for (i = 0; i < vec->capacity; i++) {                                                      \
        if (!vec->elem_init(&vec->a[i], val)) {                                                  \
          CVEC_ASSERT(0);                                                                        \
          return 0;                                                                              \
        }                                                                                        \
      }                                                                                          \
    } else {                                                                                     \
      for (i = 0; i < vec->capacity; i++) {                                                      \
        CVEC_MEMMOVE(&vec->a[i], val, sizeof(TYPE));                                             \
      }                                                                                          \
    }                                                                                            \
    return 1;                                                                                    \
  }                                                                                              \
                                                                                                 \
  void cvec_clear_##TYPE(cvector_##TYPE* vec)                                                    \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    if (vec->elem_free) {                                                                        \
      for (i = 0; i < vec->size; ++i) {                                                          \
        vec->elem_free(&vec->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
    vec->size = 0;                                                                               \
  }                                                                                              \
                                                                                                 \
  void cvec_free_##TYPE##_heap(void* vec)                                                        \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    cvector_##TYPE* tmp = (cvector_##TYPE*)vec;                                                  \
    if (!tmp) return;                                                                            \
    if (tmp->elem_free) {                                                                        \
      for (i = 0; i < tmp->size; i++) {                                                          \
        tmp->elem_free(&tmp->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
    CVEC_FREE(tmp->a);                                                                           \
    CVEC_FREE(tmp);                                                                              \
  }                                                                                              \
                                                                                                 \
  void cvec_free_##TYPE(void* vec)                                                               \
  {                                                                                              \
    cvec_sz i;                                                                                   \
    cvector_##TYPE* tmp = (cvector_##TYPE*)vec;                                                  \
    if (tmp->elem_free) {                                                                        \
      for (i = 0; i < tmp->size; i++) {                                                          \
        tmp->elem_free(&tmp->a[i]);                                                              \
      }                                                                                          \
    }                                                                                            \
                                                                                                 \
    CVEC_FREE(tmp->a);                                                                           \
                                                                                                 \
    tmp->size     = 0;                                                                           \
    tmp->capacity = 0;                                                                           \
  }

#endif
