#ifndef CRSW_MATH_H
#define CRSW_MATH_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//#define PGL_PREFIX_TYPES

#ifdef PREFIX_TYPES
#define vec2 glinternal_vec2
#define vec3 glinternal_vec3
#define vec4 glinternal_vec4
#define dvec2 glinternal_dvec2
#define dvec3 glinternal_dvec3
#define dvec4 glinternal_dvec4
#define ivec2 glinternal_ivec2
#define ivec3 glinternal_ivec3
#define ivec4 glinternal_ivec4
#define uvec2 glinternal_uvec2
#define uvec3 glinternal_uvec3
#define uvec4 glinternal_uvec4
#define mat2 glinternal_mat2
#define mat3 glinternal_mat3
#define mat4 glinternal_mat4
#define Color glinternal_Color
#define Line glinternal_Line
#define Plane glinternal_Plane
#endif

#define RM_PI (3.14159265358979323846)
#define RM_2PI (2.0 * RM_PI)
#define PI_DIV_180 (0.017453292519943296)
#define INV_PI_DIV_180 (57.2957795130823229)

#define DEG_TO_RAD(x)   ((x)*PI_DIV_180)
#define RAD_TO_DEG(x)   ((x)*INV_PI_DIV_180)

/* Hour angles */
#define HR_TO_DEG(x)    ((x) * (1.0 / 15.0))
#define HR_TO_RAD(x)    DEG_TO_RAD(HR_TO_DEG(x))

#define DEG_TO_HR(x)    ((x) * 15.0)
#define RAD_TO_HR(x)    DEG_TO_HR(RAD_TO_DEG(x))

// TODO rename RM_MAX/RSW_MAX?  make proper inline functions?
#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;


typedef struct vec2
{
	float x;
	float y;
} vec2;

#define SET_VEC2(v, _x, _y) \
	do {\
	(v).x = _x;\
	(v).y = _y;\
	} while (0)

inline vec2 make_vec2(float x, float y)
{
	vec2 v = { x, y };
	return v;
}

inline vec2 negate_vec2(vec2 v)
{
	vec2 r = { -v.x, -v.y };
	return r;
}

inline void fprint_vec2(FILE* f, vec2 v, const char* append)
{
	fprintf(f, "(%f, %f)%s", v.x, v.y, append);
}

inline void print_vec2(vec2 v, const char* append)
{
	printf("(%f, %f)%s", v.x, v.y, append);
}

inline int fread_vec2(FILE* f, vec2* v)
{
	int tmp = fscanf(f, " (%f, %f)", &v->x, &v->y);
	return (tmp == 2);
}

inline float length_vec2(vec2 a)
{
	return sqrt(a.x * a.x + a.y * a.y);
}

inline vec2 norm_vec2(vec2 a)
{
	float l = length_vec2(a);
	vec2 c = { a.x/l, a.y/l };
	return c;
}

inline void normalize_vec2(vec2* a)
{
	float l = length_vec2(*a);
	a->x /= l;
	a->y /= l;
}

inline vec2 add_vec2s(vec2 a, vec2 b)
{
	vec2 c = { a.x + b.x, a.y + b.y };
	return c;
}

inline vec2 sub_vec2s(vec2 a, vec2 b)
{
	vec2 c = { a.x - b.x, a.y - b.y };
	return c;
}

inline vec2 mult_vec2s(vec2 a, vec2 b)
{
	vec2 c = { a.x * b.x, a.y * b.y };
	return c;
}

inline vec2 div_vec2s(vec2 a, vec2 b)
{
	vec2 c = { a.x / b.x, a.y / b.y };
	return c;
}

inline float dot_vec2s(vec2 a, vec2 b)
{
	return a.x*b.x + a.y*b.y;
}

inline vec2 scale_vec2(vec2 a, float s)
{
	vec2 b = { a.x * s, a.y * s };
	return b;
}

inline int equal_vec2s(vec2 a, vec2 b)
{
	return (a.x == b.x && a.y == b.y);
}

inline int equal_epsilon_vec2s(vec2 a, vec2 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon);
}

/*
inline vec2 vec4_to_vec2(vec4 a)
{
	vec2 v = { a.x, a.y };
	return v;
}

inline vec3 vec4_to_vec3(vec4 a)
{
	vec3 v = { a.x, a.y, a.z };
	return v;
}

inline vec2 vec4_to_vec2h(vec4 a)
{
	vec2 v = { a.x/a.w, a.y/a.w };
	return v;
}

inline vec3 vec4_to_vec3h(vec4 a)
{
	vec3 v = { a.x/a.w, a.y/a.w, a.z/a.w };
	return v;
}
*/



typedef struct vec3
{
	float x;
	float y;
	float z;
} vec3;

#define SET_VEC3(v, _x, _y, _z) \
	do {\
	(v).x = _x;\
	(v).y = _y;\
	(v).z = _z;\
	} while (0)

inline vec3 make_vec3(float x, float y, float z)
{
	vec3 v = { x, y, z };
	return v;
}

inline vec3 negate_vec3(vec3 v)
{
	vec3 r = { -v.x, -v.y, -v.z };
	return r;
}

inline void fprint_vec3(FILE* f, vec3 v, const char* append)
{
	fprintf(f, "(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

inline void print_vec3(vec3 v, const char* append)
{
	printf("(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

inline int fread_vec3(FILE* f, vec3* v)
{
	int tmp = fscanf(f, " (%f, %f, %f)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}

inline float length_vec3(vec3 a)
{
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}


inline vec3 norm_vec3(vec3 a)
{
	float l = length_vec3(a);
	vec3 c = { a.x/l, a.y/l, a.z/l };
	return c;
}

inline void normalize_vec3(vec3* a)
{
	float l = length_vec3(*a);
	a->x /= l;
	a->y /= l;
	a->z /= l;
}

inline vec3 add_vec3s(vec3 a, vec3 b)
{
	vec3 c = { a.x + b.x, a.y + b.y, a.z + b.z };
	return c;
}

inline vec3 sub_vec3s(vec3 a, vec3 b)
{
	vec3 c = { a.x - b.x, a.y - b.y, a.z - b.z };
	return c;
}

inline vec3 mult_vec3s(vec3 a, vec3 b)
{
	vec3 c = { a.x * b.x, a.y * b.y, a.z * b.z };
	return c;
}

inline vec3 div_vec3s(vec3 a, vec3 b)
{
	vec3 c = { a.x / b.x, a.y / b.y, a.z / b.z };
	return c;
}

inline float dot_vec3s(vec3 a, vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 scale_vec3(vec3 a, float s)
{
	vec3 b = { a.x * s, a.y * s, a.z * s };
	return b;
}

inline int equal_vec3s(vec3 a, vec3 b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}

inline int equal_epsilon_vec3s(vec3 a, vec3 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
			fabs(a.z - b.z) < epsilon);
}

inline vec3 cross_product(const vec3 u, const vec3 v)
{
	vec3 result;
	result.x = u.y*v.z - v.y*u.z;
	result.y = -u.x*v.z + v.x*u.z;
	result.z = u.x*v.y - v.x*u.y;
	return result;
}

inline float angle_between_vec3(const vec3 u, const vec3 v)
{
	return acos(dot_vec3s(u, v));
}




typedef struct vec4
{
	float x;
	float y;
	float z;
	float w;
} vec4;

#define SET_VEC4(v, _x, _y, _z, _w) \
	do {\
	(v).x = _x;\
	(v).y = _y;\
	(v).z = _z;\
	(v).w = _w;\
	} while (0)

inline vec4 make_vec4(float x, float y, float z, float w)
{
	vec4 v = { x, y, z, w };
	return v;
}

inline vec4 negate_vec4(vec4 v)
{
	vec4 r = { -v.x, -v.y, -v.z, -v.w };
	return r;
}

inline void fprint_vec4(FILE* f, vec4 v, const char* append)
{
	fprintf(f, "(%f, %f, %f, %f)%s", v.x, v.y, v.z, v.w, append);
}

inline void print_vec4(vec4 v, const char* append)
{
	printf("(%f, %f, %f, %f)%s", v.x, v.y, v.z, v.w, append);
}

inline int fread_vec4(FILE* f, vec4* v)
{
	int tmp = fscanf(f, " (%f, %f, %f, %f)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}

inline float length_vec4(vec4 a)
{
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

inline vec4 norm_vec4(vec4 a)
{
	float l = length_vec4(a);
	vec4 c = { a.x/l, a.y/l, a.z/l, a.w/l };
	return c;
}

inline void normalize_vec4(vec4* a)
{
	float l = length_vec4(*a);
	a->x /= l;
	a->y /= l;
	a->z /= l;
	a->w /= l;
}

inline vec4 add_vec4s(vec4 a, vec4 b)
{
	vec4 c = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
	return c;
}

inline vec4 sub_vec4s(vec4 a, vec4 b)
{
	vec4 c = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
	return c;
}

inline vec4 mult_vec4s(vec4 a, vec4 b)
{
	vec4 c = { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
	return c;
}

inline vec4 div_vec4s(vec4 a, vec4 b)
{
	vec4 c = { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
	return c;
}

inline float dot_vec4s(vec4 a, vec4 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline vec4 scale_vec4(vec4 a, float s)
{
	vec4 b = { a.x * s, a.y * s, a.z * s, a.w * s };
	return b;
}

inline int equal_vec4s(vec4 a, vec4 b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}

inline int equal_epsilon_vec4s(vec4 a, vec4 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
	        fabs(a.z - b.z) < epsilon && fabs(a.w - b.w) < epsilon);
}





typedef struct ivec2
{
	int x;
	int y;
} ivec2;

inline ivec2 make_ivec2(int x, int y)
{
	ivec2 v = { x, y };
	return v;
}

inline void fprint_ivec2(FILE* f, ivec2 v, const char* append)
{
	fprintf(f, "(%d, %d)%s", v.x, v.y, append);
}

inline int fread_ivec2(FILE* f, ivec2* v)
{
	int tmp = fscanf(f, " (%d, %d)", &v->x, &v->y);
	return (tmp == 2);
}


typedef struct ivec3
{
	int x;
	int y;
	int z;
} ivec3;

inline ivec3 make_ivec3(int x, int y, int z)
{
	ivec3 v = { x, y, z };
	return v;
}

inline void fprint_ivec3(FILE* f, ivec3 v, const char* append)
{
	fprintf(f, "(%d, %d, %d)%s", v.x, v.y, v.z, append);
}

inline int fread_ivec3(FILE* f, ivec3* v)
{
	int tmp = fscanf(f, " (%d, %d, %d)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}



typedef struct ivec4
{
	int x;
	int y;
	int z;
	int w;
} ivec4;

inline ivec4 make_ivec4(int x, int y, int z, int w)
{
	ivec4 v = { x, y, z, w };
	return v;
}

inline void fprint_ivec4(FILE* f, ivec4 v, const char* append)
{
	fprintf(f, "(%d, %d, %d, %d)%s", v.x, v.y, v.z, v.w, append);
}

inline int fread_ivec4(FILE* f, ivec4* v)
{
	int tmp = fscanf(f, " (%d, %d, %d, %d)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}



typedef struct uvec2
{
	unsigned int x;
	unsigned int y;
} uvec2;

inline uvec2 make_uvec2(unsigned int x, unsigned int y)
{
	uvec2 v = { x, y };
	return v;
}

inline void fprint_uvec2(FILE* f, uvec2 v, const char* append)
{
	fprintf(f, "(%u, %u)%s", v.x, v.y, append);
}

inline int fread_uvec2(FILE* f, uvec2* v)
{
	int tmp = fscanf(f, " (%u, %u)", &v->x, &v->y);
	return (tmp == 2);
}



typedef struct uvec3
{
	unsigned int x;
	unsigned int y;
	unsigned int z;
} uvec3;

inline uvec3 make_uvec3(unsigned int x, unsigned int y, unsigned int z)
{
	uvec3 v = { x, y, z };
	return v;
}

inline void fprint_uvec3(FILE* f, uvec3 v, const char* append)
{
	fprintf(f, "(%u, %u, %u)%s", v.x, v.y, v.z, append);
}

inline int fread_uvec3(FILE* f, uvec3* v)
{
	int tmp = fscanf(f, " (%u, %u, %u)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}



typedef struct uvec4
{
	unsigned int x;
	unsigned int y;
	unsigned int z;
	unsigned int w;
} uvec4;

inline uvec4 make_uvec4(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
{
	uvec4 v = { x, y, z, w };
	return v;
}

inline void fprint_uvec4(FILE* f, uvec4 v, const char* append)
{
	fprintf(f, "(%u, %u, %u, %u)%s", v.x, v.y, v.z, v.w, append);
}

inline int fread_uvec4(FILE* f, uvec4* v)
{
	int tmp = fscanf(f, " (%u, %u, %u, %u)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}




typedef struct bvec2
{
	u8 x;
	u8 y;
} bvec2;

// TODO What to do here? param type?  enforce 0 or 1?
inline bvec2 make_bvec2(int x, int y)
{
	bvec2 v = { !!x, !!y };
	return v;
}

inline void fprint_bvec2(FILE* f, bvec2 v, const char* append)
{
	fprintf(f, "(%u, %u)%s", v.x, v.y, append);
}

// Should technically use SCNu8 macro not hhu
inline int fread_bvec2(FILE* f, bvec2* v)
{
	int tmp = fscanf(f, " (%hhu, %hhu)", &v->x, &v->y);
	return (tmp == 2);
}




typedef struct bvec3
{
	u8 x;
	u8 y;
	u8 z;
} bvec3;

inline bvec3 make_bvec3(int x, int y, int z)
{
	bvec3 v = { !!x, !!y, !!z };
	return v;
}

inline void fprint_bvec3(FILE* f, bvec3 v, const char* append)
{
	fprintf(f, "(%u, %u, %u)%s", v.x, v.y, v.z, append);
}

inline int fread_bvec3(FILE* f, bvec3* v)
{
	int tmp = fscanf(f, " (%hhu, %hhu, %hhu)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}






typedef struct bvec4
{
	u8 x;
	u8 y;
	u8 z;
	u8 w;
} bvec4;

inline bvec4 make_bvec4(int x, int y, int z, int w)
{
	bvec4 v = { !!x, !!y, !!z, !!w };
	return v;
}

inline void fprint_bvec4(FILE* f, bvec4 v, const char* append)
{
	fprintf(f, "(%u, %u, %u, %u)%s", v.x, v.y, v.z, v.w, append);
}

inline int fread_bvec4(FILE* f, bvec4* v)
{
	int tmp = fscanf(f, " (%hhu, %hhu, %hhu, %hhu)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}





inline vec2 vec4_to_vec2(vec4 a)
{
	vec2 v = { a.x, a.y };
	return v;
}

inline vec3 vec4_to_vec3(vec4 a)
{
	vec3 v = { a.x, a.y, a.z };
	return v;
}

inline vec2 vec4_to_vec2h(vec4 a)
{
	vec2 v = { a.x/a.w, a.y/a.w };
	return v;
}

inline vec3 vec4_to_vec3h(vec4 a)
{
	vec3 v = { a.x/a.w, a.y/a.w, a.z/a.w };
	return v;
}

#ifndef CMAT234_H
#define CMAT234_H

#include <stdio.h>
/* matrices **************/

typedef float mat2[4];
typedef float mat3[9];
typedef float mat4[16];

#define IDENTITY_MAT2() { 1, 0, 0, 1 }
#define IDENTITY_MAT3() { 1, 0, 0, 0, 1, 0, 0, 0, 1 }
#define IDENTITY_MAT4() { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }
#define SET_IDENTITY_MAT2(m) \
	do { \
	m[1] = m[2] = 0; \
	m[0] = m[3] = 1; \
	} while (0)

#define SET_IDENTITY_MAT3(m) \
	do { \
	m[1] = m[2] = m[3] = 0; \
	m[5] = m[6] = m[7] = 0; \
	m[0] = m[4] = m[8] = 1; \
	} while (0)

#define SET_IDENTITY_MAT4(m) \
	do { \
	m[1] = m[2] = m[3] = m[4] = 0; \
	m[6] = m[7] = m[8] = m[9] = 0; \
	m[11] = m[12] = m[13] = m[14] = 0; \
	m[0] = m[5] = m[10] = m[15] = 1; \
	} while (0)

#ifndef ROW_MAJOR
inline vec2 x_mat2(mat2 m) {  return make_vec2(m[0], m[2]); }
inline vec2 y_mat2(mat2 m) {  return make_vec2(m[1], m[3]); }
inline vec2 c1_mat2(mat2 m) { return make_vec2(m[0], m[1]); }
inline vec2 c2_mat2(mat2 m) { return make_vec2(m[2], m[3]); }

inline void setc1_mat2(mat2 m, vec2 v) { m[0]=v.x, m[1]=v.y; }
inline void setc2_mat2(mat2 m, vec2 v) { m[2]=v.x, m[3]=v.y; }

inline void setx_mat2(mat2 m, vec2 v) { m[0]=v.x, m[2]=v.y; }
inline void sety_mat2(mat2 m, vec2 v) { m[1]=v.x, m[3]=v.y; }
#else
inline vec2 x_mat2(mat2 m) {  return make_vec2(m[0], m[1]); }
inline vec2 y_mat2(mat2 m) {  return make_vec2(m[2], m[3]); }
inline vec2 c1_mat2(mat2 m) { return make_vec2(m[0], m[2]); }
inline vec2 c2_mat2(mat2 m) { return make_vec2(m[1], m[3]); }

inline void setc1_mat2(mat2 m, vec2 v) { m[0]=v.x, m[2]=v.y; }
inline void setc2_mat2(mat2 m, vec2 v) { m[1]=v.x, m[3]=v.y; }

inline void setx_mat2(mat2 m, vec2 v) { m[0]=v.x, m[1]=v.y; }
inline void sety_mat2(mat2 m, vec2 v) { m[2]=v.x, m[3]=v.y; }
#endif


#ifndef ROW_MAJOR
inline vec3 x_mat3(mat3 m) {  return make_vec3(m[0], m[3], m[6]); }
inline vec3 y_mat3(mat3 m) {  return make_vec3(m[1], m[4], m[7]); }
inline vec3 z_mat3(mat3 m) {  return make_vec3(m[2], m[5], m[8]); }
inline vec3 c1_mat3(mat3 m) { return make_vec3(m[0], m[1], m[2]); }
inline vec3 c2_mat3(mat3 m) { return make_vec3(m[3], m[4], m[5]); }
inline vec3 c3_mat3(mat3 m) { return make_vec3(m[6], m[7], m[8]); }

inline void setc1_mat3(mat3 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z; }
inline void setc2_mat3(mat3 m, vec3 v) { m[3]=v.x, m[4]=v.y, m[5]=v.z; }
inline void setc3_mat3(mat3 m, vec3 v) { m[6]=v.x, m[7]=v.y, m[8]=v.z; }

inline void setx_mat3(mat3 m, vec3 v) { m[0]=v.x, m[3]=v.y, m[6]=v.z; }
inline void sety_mat3(mat3 m, vec3 v) { m[1]=v.x, m[4]=v.y, m[7]=v.z; }
inline void setz_mat3(mat3 m, vec3 v) { m[2]=v.x, m[5]=v.y, m[8]=v.z; }
#else
inline vec3 x_mat3(mat3 m) {  return make_vec3(m[0], m[1], m[2]); }
inline vec3 y_mat3(mat3 m) {  return make_vec3(m[3], m[4], m[5]); }
inline vec3 z_mat3(mat3 m) {  return make_vec3(m[6], m[7], m[8]); }
inline vec3 c1_mat3(mat3 m) { return make_vec3(m[0], m[3], m[6]); }
inline vec3 c2_mat3(mat3 m) { return make_vec3(m[1], m[4], m[7]); }
inline vec3 c3_mat3(mat3 m) { return make_vec3(m[2], m[5], m[8]); }

inline void setc1_mat3(mat3 m, vec3 v) { m[0]=v.x, m[3]=v.y, m[6]=v.z; }
inline void setc2_mat3(mat3 m, vec3 v) { m[1]=v.x, m[4]=v.y, m[7]=v.z; }
inline void setc3_mat3(mat3 m, vec3 v) { m[2]=v.x, m[5]=v.y, m[8]=v.z; }

inline void setx_mat3(mat3 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z; }
inline void sety_mat3(mat3 m, vec3 v) { m[3]=v.x, m[4]=v.y, m[5]=v.z; }
inline void setz_mat3(mat3 m, vec3 v) { m[6]=v.x, m[7]=v.y, m[8]=v.z; }
#endif


#ifndef ROW_MAJOR
inline vec4 c1_mat4(mat4 m) { return make_vec4(m[ 0], m[ 1], m[ 2], m[ 3]); }
inline vec4 c2_mat4(mat4 m) { return make_vec4(m[ 4], m[ 5], m[ 6], m[ 7]); }
inline vec4 c3_mat4(mat4 m) { return make_vec4(m[ 8], m[ 9], m[10], m[11]); }
inline vec4 c4_mat4(mat4 m) { return make_vec4(m[12], m[13], m[14], m[15]); }

inline vec4 x_mat4(mat4 m) { return make_vec4(m[0], m[4], m[8], m[12]); }
inline vec4 y_mat4(mat4 m) { return make_vec4(m[1], m[5], m[9], m[13]); }
inline vec4 z_mat4(mat4 m) { return make_vec4(m[2], m[6], m[10], m[14]); }
inline vec4 w_mat4(mat4 m) { return make_vec4(m[3], m[7], m[11], m[15]); }

//sets 4th row to 0 0 0 1
inline void setc1_mat4v3(mat4 m, vec3 v) { m[ 0]=v.x, m[ 1]=v.y, m[ 2]=v.z, m[ 3]=0; }
inline void setc2_mat4v3(mat4 m, vec3 v) { m[ 4]=v.x, m[ 5]=v.y, m[ 6]=v.z, m[ 7]=0; }
inline void setc3_mat4v3(mat4 m, vec3 v) { m[ 8]=v.x, m[ 9]=v.y, m[10]=v.z, m[11]=0; }
inline void setc4_mat4v3(mat4 m, vec3 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=1; }

inline void setc1_mat4v4(mat4 m, vec4 v) { m[ 0]=v.x, m[ 1]=v.y, m[ 2]=v.z, m[ 3]=v.w; }
inline void setc2_mat4v4(mat4 m, vec4 v) { m[ 4]=v.x, m[ 5]=v.y, m[ 6]=v.z, m[ 7]=v.w; }
inline void setc3_mat4v4(mat4 m, vec4 v) { m[ 8]=v.x, m[ 9]=v.y, m[10]=v.z, m[11]=v.w; }
inline void setc4_mat4v4(mat4 m, vec4 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=v.w; }

//sets 4th column to 0 0 0 1
inline void setx_mat4v3(mat4 m, vec3 v) { m[0]=v.x, m[4]=v.y, m[ 8]=v.z, m[12]=0; }
inline void sety_mat4v3(mat4 m, vec3 v) { m[1]=v.x, m[5]=v.y, m[ 9]=v.z, m[13]=0; }
inline void setz_mat4v3(mat4 m, vec3 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=0; }
inline void setw_mat4v3(mat4 m, vec3 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=1; }

inline void setx_mat4v4(mat4 m, vec4 v) { m[0]=v.x, m[4]=v.y, m[ 8]=v.z, m[12]=v.w; }
inline void sety_mat4v4(mat4 m, vec4 v) { m[1]=v.x, m[5]=v.y, m[ 9]=v.z, m[13]=v.w; }
inline void setz_mat4v4(mat4 m, vec4 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=v.w; }
inline void setw_mat4v4(mat4 m, vec4 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=v.w; }
#else
inline vec4 c1_mat4(mat4 m) { return make_vec4(m[0], m[4], m[8], m[12]); }
inline vec4 c2_mat4(mat4 m) { return make_vec4(m[1], m[5], m[9], m[13]); }
inline vec4 c3_mat4(mat4 m) { return make_vec4(m[2], m[6], m[10], m[14]); }
inline vec4 c4_mat4(mat4 m) { return make_vec4(m[3], m[7], m[11], m[15]); }

inline vec4 x_mat4(mat4 m) { return make_vec4(m[0], m[1], m[2], m[3]); }
inline vec4 y_mat4(mat4 m) { return make_vec4(m[4], m[5], m[6], m[7]); }
inline vec4 z_mat4(mat4 m) { return make_vec4(m[8], m[9], m[10], m[11]); }
inline vec4 w_mat4(mat4 m) { return make_vec4(m[12], m[13], m[14], m[15]); }

//sets 4th row to 0 0 0 1
inline void setc1_mat4v3(mat4 m, vec3 v) { m[0]=v.x, m[4]=v.y, m[8]=v.z, m[12]=0; }
inline void setc2_mat4v3(mat4 m, vec3 v) { m[1]=v.x, m[5]=v.y, m[9]=v.z, m[13]=0; }
inline void setc3_mat4v3(mat4 m, vec3 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=0; }
inline void setc4_mat4v3(mat4 m, vec3 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=1; }

inline void setc1_mat4v4(mat4 m, vec4 v) { m[0]=v.x, m[4]=v.y, m[8]=v.z, m[12]=v.w; }
inline void setc2_mat4v4(mat4 m, vec4 v) { m[1]=v.x, m[5]=v.y, m[9]=v.z, m[13]=v.w; }
inline void setc3_mat4v4(mat4 m, vec4 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=v.w; }
inline void setc4_mat4v4(mat4 m, vec4 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=v.w; }

//sets 4th column to 0 0 0 1
inline void setx_mat4v3(mat4 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z, m[3]=0; }
inline void sety_mat4v3(mat4 m, vec3 v) { m[4]=v.x, m[5]=v.y, m[6]=v.z, m[7]=0; }
inline void setz_mat4v3(mat4 m, vec3 v) { m[8]=v.x, m[9]=v.y, m[10]=v.z, m[11]=0; }
inline void setw_mat4v3(mat4 m, vec3 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=1; }

inline void setx_mat4v4(mat4 m, vec4 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z, m[3]=v.w; }
inline void sety_mat4v4(mat4 m, vec4 v) { m[4]=v.x, m[5]=v.y, m[6]=v.z, m[7]=v.w; }
inline void setz_mat4v4(mat4 m, vec4 v) { m[8]=v.x, m[9]=v.y, m[10]=v.z, m[11]=v.w; }
inline void setw_mat4v4(mat4 m, vec4 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=v.w; }
#endif


inline void fprint_mat2(FILE* f, mat2 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f)\n (%f, %f)]%s",
	        m[0], m[2], m[1], m[3], append);
#else
	fprintf(f, "[(%f, %f)\n (%f, %f)]%s",
	        m[0], m[1], m[2], m[3], append);
#endif
}


inline void fprint_mat3(FILE* f, mat3 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f, %f)\n (%f, %f, %f)\n (%f, %f, %f)]%s",
	        m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8], append);
#else
	fprintf(f, "[(%f, %f, %f)\n (%f, %f, %f)\n (%f, %f, %f)]%s",
	        m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], append);
#endif
}

inline void fprint_mat4(FILE* f, mat4 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)]%s",
	        m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14],
	        m[3], m[7], m[11], m[15], append);
#else
	fprintf(f, "[(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)]%s",
	        m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11],
	        m[12], m[13], m[14], m[15], append);
#endif
}

// macros?
inline void print_mat2(mat2 m, const char* append)
{
	fprint_mat2(stdout, m, append);
}

inline void print_mat3(mat3 m, const char* append)
{
	fprint_mat3(stdout, m, append);
}

inline void print_mat4(mat4 m, const char* append)
{
	fprint_mat4(stdout, m, append);
}

//TODO define macros for doing array version
inline vec2 mult_mat2_vec2(mat2 m, vec2 v)
{
	vec2 r;
#ifndef ROW_MAJOR
	r.x = m[0]*v.x + m[2]*v.y;
	r.y = m[1]*v.x + m[3]*v.y;
#else
	r.x = m[0]*v.x + m[1]*v.y;
	r.y = m[3]*v.x + m[3]*v.y;
#endif
	return r;
}


inline vec3 mult_mat3_vec3(mat3 m, vec3 v)
{
	vec3 r;
#ifndef ROW_MAJOR
	r.x = m[0]*v.x + m[3]*v.y + m[6]*v.z;
	r.y = m[1]*v.x + m[4]*v.y + m[7]*v.z;
	r.z = m[2]*v.x + m[5]*v.y + m[8]*v.z;
#else
	r.x = m[0]*v.x + m[1]*v.y + m[2]*v.z;
	r.y = m[3]*v.x + m[4]*v.y + m[5]*v.z;
	r.z = m[6]*v.x + m[7]*v.y + m[8]*v.z;
#endif
	return r;
}

inline vec4 mult_mat4_vec4(mat4 m, vec4 v)
{
	vec4 r;
#ifndef ROW_MAJOR
	r.x = m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12]*v.w;
	r.y = m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13]*v.w;
	r.z = m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w;
	r.w = m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w;
#else
	r.x = m[0]*v.x + m[1]*v.y + m[2]*v.z + m[3]*v.w;
	r.y = m[4]*v.x + m[5]*v.y + m[6]*v.z + m[7]*v.w;
	r.z = m[8]*v.x + m[9]*v.y + m[10]*v.z + m[11]*v.w;
	r.w = m[12]*v.x + m[13]*v.y + m[14]*v.z + m[15]*v.w;
#endif
	return r;
}

void mult_mat2_mat2(mat2 c, mat2 a, mat2 b);

void mult_mat3_mat3(mat3 c, mat3 a, mat3 b);

void mult_mat4_mat4(mat4 c, mat4 a, mat4 b);

inline void load_rotation_mat2(mat2 mat, float angle)
{
#ifndef ROW_MAJOR
	mat[0] = cos(angle);
	mat[2] = -sin(angle);

	mat[1] = sin(angle);
	mat[3] = cos(angle);
#else
	mat[0] = cos(angle);
	mat[1] = -sin(angle);

	mat[2] = sin(angle);
	mat[3] = cos(angle);
#endif
}

void load_rotation_mat3(mat3 mat, vec3 v, float angle);

void load_rotation_mat4(mat4 mat, vec3 vec, float angle);

//void invert_mat4(mat4 mInverse, const mat4 m);

void make_perspective_matrix(mat4 mat, float fFov, float aspect, float near, float far);
void make_pers_matrix(mat4 mat, float z_near, float z_far);

void make_perspective_proj_matrix(mat4 mat, float left, float right, float bottom, float top, float near, float far);

void make_orthographic_matrix(mat4 mat, float left, float right, float bottom, float top, float near, float far);

void make_viewport_matrix(mat4 mat, int x, int y, unsigned int width, unsigned int height, int opengl);

void lookAt(mat4 mat, vec3 eye, vec3 center, vec3 up);



///////////Matrix transformation functions
inline void scale_mat3(mat3 m, float x, float y, float z)
{
#ifndef ROW_MAJOR
	m[0] = x; m[3] = 0; m[6] = 0;
	m[1] = 0; m[4] = y; m[7] = 0;
	m[2] = 0; m[5] = 0; m[8] = z;
#else
	m[0] = x; m[1] = 0; m[2] = 0;
	m[3] = 0; m[4] = y; m[5] = 0;
	m[6] = 0; m[7] = 0; m[8] = z;
#endif
}

inline void scale_mat4(mat4 m, float x, float y, float z)
{
#ifndef ROW_MAJOR
	m[ 0] = x; m[ 4] = 0; m[ 8] = 0; m[12] = 0;
	m[ 1] = 0; m[ 5] = y; m[ 9] = 0; m[13] = 0;
	m[ 2] = 0; m[ 6] = 0; m[10] = z; m[14] = 0;
	m[ 3] = 0; m[ 7] = 0; m[11] = 0; m[15] = 1;
#else
	m[ 0] = x; m[ 1] = 0; m[ 2] = 0; m[ 3] = 0;
	m[ 4] = 0; m[ 5] = y; m[ 6] = 0; m[ 7] = 0;
	m[ 8] = 0; m[ 9] = 0; m[10] = z; m[11] = 0;
	m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
#endif
}

// Create a Translation matrix. Only 4x4 matrices have translation components
inline void translation_mat4(mat4 m, float x, float y, float z)
{
#ifndef ROW_MAJOR
	m[ 0] = 1; m[ 4] = 0; m[ 8] = 0; m[12] = x;
	m[ 1] = 0; m[ 5] = 1; m[ 9] = 0; m[13] = y;
	m[ 2] = 0; m[ 6] = 0; m[10] = 1; m[14] = z;
	m[ 3] = 0; m[ 7] = 0; m[11] = 0; m[15] = 1;
#else
	m[ 0] = 1; m[ 1] = 0; m[ 2] = 0; m[ 3] = x;
	m[ 4] = 0; m[ 5] = 1; m[ 6] = 0; m[ 7] = y;
	m[ 8] = 0; m[ 9] = 0; m[10] = 1; m[11] = z;
	m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
#endif
}





// Extract a rotation matrix from a 4x4 matrix
// Extracts the rotation matrix (3x3) from a 4x4 matrix
//
#ifndef ROW_MAJOR
#define M44(m, row, col) m[col*4 + row]
#define M33(m, row, col) m[col*3 + row]
#else
#define M44(m, row, col) m[row*4 + col]
#define M33(m, row, col) m[row*3 + col]
#endif
inline void extract_rotation_mat4(mat3 dst, mat4 src, int normalize)
{
	vec3 tmp;
	if (normalize) {
		tmp.x = M44(src, 0, 0);
		tmp.y = M44(src, 1, 0);
		tmp.z = M44(src, 2, 0);
		normalize_vec3(&tmp);

		M33(dst, 0, 0) = tmp.x;
		M33(dst, 1, 0) = tmp.y;
		M33(dst, 2, 0) = tmp.z;

		tmp.x = M44(src, 0, 1);
		tmp.y = M44(src, 1, 1);
		tmp.z = M44(src, 2, 1);
		normalize_vec3(&tmp);

		M33(dst, 0, 1) = tmp.x;
		M33(dst, 1, 1) = tmp.y;
		M33(dst, 2, 1) = tmp.z;

		tmp.x = M44(src, 0, 2);
		tmp.y = M44(src, 1, 2);
		tmp.z = M44(src, 2, 2);
		normalize_vec3(&tmp);

		M33(dst, 0, 2) = tmp.x;
		M33(dst, 1, 2) = tmp.y;
		M33(dst, 2, 2) = tmp.z;
	} else {
		M33(dst, 0, 0) = M44(src, 0, 0);
		M33(dst, 1, 0) = M44(src, 1, 0);
		M33(dst, 2, 0) = M44(src, 2, 0);

		M33(dst, 0, 1) = M44(src, 0, 1);
		M33(dst, 1, 1) = M44(src, 1, 1);
		M33(dst, 2, 1) = M44(src, 2, 1);

		M33(dst, 0, 2) = M44(src, 0, 2);
		M33(dst, 1, 2) = M44(src, 1, 2);
		M33(dst, 2, 2) = M44(src, 2, 2);
	}
}
#undef M33
#undef M44

#endif

// returns float [0,1)
inline float rsw_randf(void)
{
	return rand() / (RAND_MAX + 1.0f);
}

inline float rsw_randf_range(float min, float max)
{
	return min + (max-min) * rsw_randf();
}

inline double rsw_map(double x, double a, double b, double c, double d)
{
	return (x-a)/(b-a) * (d-c) + c;
}

inline float rsw_mapf(float x, float a, float b, float c, float d)
{
	return (x-a)/(b-a) * (d-c) + c;
}


typedef struct Color
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
} Color;

/*
Color make_Color()
{
	r = g = b = 0;
	a = 255;
}
*/

inline Color make_Color(u8 red, u8 green, u8 blue, u8 alpha)
{
	Color c = { red, green, blue, alpha };
	return c;
}

inline void print_Color(Color c, const char* append)
{
	printf("(%d, %d, %d, %d)%s", c.r, c.g, c.b, c.a, append);
}

inline Color vec4_to_Color(vec4 v)
{
	//assume all in the range of [0, 1]
	//NOTE(rswinkle): There are other ways of doing the conversion
	//
	// round like HH: (u8)(v.x * 255.0f + 0.5f)
	// allocate equal sized buckets: (u8)(v.x * 256.0f - EPSILON) (where epsilon is eg 0.000001f)
	//
	// But as far as I can tell the spec does it this way
	Color c;
	c.r = v.x * 255.0f;
	c.g = v.y * 255.0f;
	c.b = v.z * 255.0f;
	c.a = v.w * 255.0f;
	return c;
}

inline vec4 Color_to_vec4(Color c)
{
	vec4 v = { (float)c.r/255.0f, (float)c.g/255.0f, (float)c.b/255.0f, (float)c.a/255.0f };
	return v;
}

typedef struct Line
{
	float A, B, C;
} Line;

inline Line make_Line(float x1, float y1, float x2, float y2)
{
	Line l;
	l.A = y1 - y2;
	l.B = x2 - x1;
	l.C = x1*y2 - x2*y1;
	return l;
}

inline void normalize_line(Line* line)
{
	// TODO could enforce that n always points toward +y or +x...should I?
	vec2 n = { line->A, line->B };
	float len = length_vec2(n);
	line->A /= len;
	line->B /= len;
	line->C /= len;
}

inline float line_func(Line* line, float x, float y)
{
	return line->A*x + line->B*y + line->C;
}
inline float line_findy(Line* line, float x)
{
	return -(line->A*x + line->C)/line->B;
}

inline float line_findx(Line* line, float y)
{
	return -(line->B*y + line->C)/line->A;
}

// return squared distance from c to line segment between a and b
inline float sq_dist_pt_segment2d(vec2 a, vec2 b, vec2 c)
{
	vec2 ab = sub_vec2s(b, a);
	vec2 ac = sub_vec2s(c, a);
	vec2 bc = sub_vec2s(c, b);
	float e = dot_vec2s(ac, ab);

	// cases where c projects outside ab
	if (e <= 0.0f) return dot_vec2s(ac, ac);
	float f = dot_vec2s(ab, ab);
	if (e >= f) return dot_vec2s(bc, bc);

	// handle cases where c projects onto ab
	return dot_vec2s(ac, ac) - e * e / f;
}


typedef struct Plane
{
	vec3 n;	//normal points x on plane satisfy n dot x = d
	float d; //d = n dot p

} Plane;

/*
Plane() {}
Plane(vec3 a, vec3 b, vec3 c)	//ccw winding
{
	n = cross_product(b-a, c-a).norm();
	d = n * a;
}
*/

//int intersect_segment_plane(vec3 a, vec3 b, Plane p, float* t, vec3* q);


// TODO hmm would have to change mat3 and mat4 to proper
// structures to have operators return them since our
// current mat*mat functions take the output mat as a parameter


// For some reason g++ chokes on these operator overloads but they work just
// fine with clang++.  Commented till I figure out what's going on.
/*
#ifdef __cplusplus
inline vec2 operator*(vec2 v, float a) { return scale_vec2(v, a); }
inline vec2 operator*(float a, vec2 v) { return scale_vec2(v, a); }
inline vec3 operator*(vec3 v, float a) { return scale_vec3(v, a); }
inline vec3 operator*(float a, vec3 v) { return scale_vec3(v, a); }
inline vec4 operator*(vec4 v, float a) { return scale_vec4(v, a); }
inline vec4 operator*(float a, vec4 v) { return scale_vec4(v, a); }

inline vec2 operator+(vec2 v1, vec2 v2) { return add_vec2s(v1, v2); }
inline vec3 operator+(vec3 v1, vec3 v2) { return add_vec3s(v1, v2); }
inline vec4 operator+(vec4 v1, vec4 v2) { return add_vec4s(v1, v2); }

inline vec2 operator-(vec2 v1, vec2 v2) { return sub_vec2s(v1, v2); }
inline vec3 operator-(vec3 v1, vec3 v2) { return sub_vec3s(v1, v2); }
inline vec4 operator-(vec4 v1, vec4 v2) { return sub_vec4s(v1, v2); }

inline int operator==(vec2 v1, vec2 v2) { return equal_vec2s(v1, v2); }
inline int operator==(vec3 v1, vec3 v2) { return equal_vec3s(v1, v2); }
inline int operator==(vec4 v1, vec4 v2) { return equal_vec4s(v1, v2); }

inline vec2 operator-(vec2 v) { return negate_vec2(v); }
inline vec3 operator-(vec3 v) { return negate_vec3(v); }
inline vec4 operator-(vec4 v) { return negate_vec4(v); }

inline vec2 operator*(mat2 m, vec2 v) { return mult_mat2_vec2(m, v); }
inline vec3 operator*(mat3 m, vec3 v) { return mult_mat3_vec3(m, v); }
inline vec4 operator*(mat4 m, vec4 v) { return mult_mat4_vec4(m, v); }

#include <iostream>
static inline std::ostream& operator<<(std::ostream& stream, const vec2& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<")";
}
static inline std::ostream& operator<<(std::ostream& stream, const vec3& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<")";
}

static inline std::ostream& operator<<(std::ostream& stream, const vec4& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<", "<<a.w<<")";
}

#endif
*/




// Built-in GLSL functions from Chapter 8 of the GLSLangSpec.3.30.pdf
// Some functionality is included elsewhere in crsw_math (especially
// the geometric functions) and texture lookup functions are in
// gl_glsl.c but this is for the rest of them.  May be moved eventually

// For functions that take 1 float input
#define PGL_VECTORIZE_VEC2(func) \
inline vec2 func##_vec2(vec2 v) \
{ \
	return make_vec2(func(v.x), func(v.y)); \
}
#define PGL_VECTORIZE_VEC3(func) \
inline vec3 func##_vec3(vec3 v) \
{ \
	return make_vec3(func(v.x), func(v.y), func(v.z)); \
}
#define PGL_VECTORIZE_VEC4(func) \
inline vec4 func##_vec4(vec4 v) \
{ \
	return make_vec4(func(v.x), func(v.y), func(v.z), func(v.w)); \
}

#define PGL_VECTORIZE_VEC(func) \
	PGL_VECTORIZE_VEC2(func) \
	PGL_VECTORIZE_VEC3(func) \
	PGL_VECTORIZE_VEC4(func)

#define PGL_STATIC_VECTORIZE_VEC(func) \
static PGL_VECTORIZE_VEC2(func) \
static PGL_VECTORIZE_VEC3(func) \
static PGL_VECTORIZE_VEC4(func)

// for functions that take 2 float inputs and return a float
#define PGL_VECTORIZE2_VEC2(func) \
inline vec2 func##_vec2(vec2 a, vec2 b) \
{ \
	return make_vec2(func(a.x, b.x), func(a.y, b.y)); \
}
#define PGL_VECTORIZE2_VEC3(func) \
inline vec3 func##_vec3(vec3 a, vec3 b) \
{ \
	return make_vec3(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z)); \
}
#define PGL_VECTORIZE2_VEC4(func) \
inline vec4 func##_vec4(vec4 a, vec4 b) \
{ \
	return make_vec4(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z), func(a.w, b.w)); \
}

#define PGL_VECTORIZE2_VEC(func) \
	PGL_VECTORIZE2_VEC2(func) \
	PGL_VECTORIZE2_VEC3(func) \
	PGL_VECTORIZE2_VEC4(func)

#define PGL_STATIC_VECTORIZE2_VEC(func) \
static PGL_VECTORIZE2_VEC2(func) \
static PGL_VECTORIZE2_VEC3(func) \
static PGL_VECTORIZE2_VEC4(func)

// For functions that take 2 float inputs and 1 float control
//  and return a float like mix
#define PGL_VECTORIZE2_1_VEC2(func) \
inline vec2 func##_vec2(vec2 a, vec2 b, float c) \
{ \
	return make_vec2(func(a.x, b.x, c), func(a.y, b.y, c)); \
}
#define PGL_VECTORIZE2_1_VEC3(func) \
inline vec3 func##_vec3(vec3 a, vec3 b, float c) \
{ \
	return make_vec3(func(a.x, b.x, c), func(a.y, b.y, c), func(a.z, b.z, c)); \
}
#define PGL_VECTORIZE2_1_VEC4(func) \
inline vec4 func##_vec4(vec4 a, vec4 b, float c) \
{ \
	return make_vec4(func(a.x, b.x, c), func(a.y, b.y, c), func(a.z, b.z, c), func(a.w, b.w, c)); \
}

#define PGL_VECTORIZE2_1_VEC(func) \
	PGL_VECTORIZE2_1_VEC2(func) \
	PGL_VECTORIZE2_1_VEC3(func) \
	PGL_VECTORIZE2_1_VEC4(func)

#define PGL_STATIC_VECTORIZE2_1_VEC(func) \
static PGL_VECTORIZE2_1_VEC2(func) \
static PGL_VECTORIZE2_1_VEC3(func) \
static PGL_VECTORIZE2_1_VEC4(func)

// for functions that take 1 input and 2 control floats
// and return a float like clamp
#define PGL_VECTORIZE_2_VEC2(func) \
inline vec2 func##_vec2(vec2 v, float a, float b) \
{ \
	return make_vec2(func(v.x, a, b), func(v.y, a, b)); \
}
#define PGL_VECTORIZE_2_VEC3(func) \
inline vec3 func##_vec3(vec3 v, float a, float b) \
{ \
	return make_vec3(func(v.x, a, b), func(v.y, a, b), func(v.z, a, b)); \
}
#define PGL_VECTORIZE_2_VEC4(func) \
inline vec4 func##_vec4(vec4 v, float a, float b) \
{ \
	return make_vec4(func(v.x, a, b), func(v.y, a, b), func(v.z, a, b), func(v.w, a, b)); \
}

#define PGL_VECTORIZE_2_VEC(func) \
	PGL_VECTORIZE_2_VEC2(func) \
	PGL_VECTORIZE_2_VEC3(func) \
	PGL_VECTORIZE_2_VEC4(func)

#define PGL_STATIC_VECTORIZE_2_VEC(func) \
static PGL_VECTORIZE_2_VEC2(func) \
static PGL_VECTORIZE_2_VEC3(func) \
static PGL_VECTORIZE_2_VEC4(func)

// hmm name VECTORIZEI_IVEC2?  suffix is return type?
#define PGL_VECTORIZE_IVEC2(func) \
inline ivec2 func##_ivec2(ivec2 v) \
{ \
	return make_ivec2(func(v.x), func(v.y)); \
}
#define PGL_VECTORIZE_IVEC3(func) \
inline ivec3 func##_ivec3(ivec3 v) \
{ \
	return make_ivec3(func(v.x), func(v.y), func(v.z)); \
}
#define PGL_VECTORIZE_IVEC4(func) \
inline ivec4 func##_ivec4(ivec4 v) \
{ \
	return make_ivec4(func(v.x), func(v.y), func(v.z), func(v.w)); \
}

#define PGL_VECTORIZE_IVEC(func) \
	PGL_VECTORIZE_IVEC2(func) \
	PGL_VECTORIZE_IVEC3(func) \
	PGL_VECTORIZE_IVEC4(func)

#define PGL_VECTORIZE_BVEC2(func) \
inline bvec2 func##_bvec2(bvec2 v) \
{ \
	return make_bvec2(func(v.x), func(v.y)); \
}
#define PGL_VECTORIZE_BVEC3(func) \
inline bvec3 func##_bvec3(bvec3 v) \
{ \
	return make_bvec3(func(v.x), func(v.y), func(v.z)); \
}
#define PGL_VECTORIZE_BVEC4(func) \
inline bvec4 func##_bvec4(bvec4 v) \
{ \
	return make_bvec4(func(v.x), func(v.y), func(v.z), func(v.w)); \
}

#define PGL_VECTORIZE_BVEC(func) \
	PGL_VECTORIZE_BVEC2(func) \
	PGL_VECTORIZE_BVEC3(func) \
	PGL_VECTORIZE_BVEC4(func)

#define PGL_STATIC_VECTORIZE_BVEC(func) \
static PGL_VECTORIZE_BVEC2(func) \
static PGL_VECTORIZE_BVEC3(func) \
static PGL_VECTORIZE_BVEC4(func)

// for functions that take 2 float inputs and return a bool
#define PGL_VECTORIZE2_BVEC2(func) \
inline bvec2 func##_vec2(vec2 a, vec2 b) \
{ \
	return make_bvec2(func(a.x, b.x), func(a.y, b.y)); \
}
#define PGL_VECTORIZE2_BVEC3(func) \
inline bvec3 func##_vec3(vec3 a, vec3 b) \
{ \
	return make_bvec3(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z)); \
}
#define PGL_VECTORIZE2_BVEC4(func) \
inline bvec4 func##_vec4(vec4 a, vec4 b) \
{ \
	return make_bvec4(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z), func(a.w, b.w)); \
}

#define PGL_VECTORIZE2_BVEC(func) \
	PGL_VECTORIZE2_BVEC2(func) \
	PGL_VECTORIZE2_BVEC3(func) \
	PGL_VECTORIZE2_BVEC4(func)

#define PGL_STATIC_VECTORIZE2_BVEC(func) \
static PGL_VECTORIZE2_BVEC2(func) \
static PGL_VECTORIZE2_BVEC3(func) \
static PGL_VECTORIZE2_BVEC4(func)



// 8.1 Angle and Trig Functions
static inline float radiansf(float degrees) { return DEG_TO_RAD(degrees); }
static inline float degreesf(float radians) { return RAD_TO_DEG(radians); }

static inline double radians(double degrees) { return DEG_TO_RAD(degrees); }
static inline double degrees(double radians) { return RAD_TO_DEG(radians); }

PGL_STATIC_VECTORIZE_VEC(radiansf)
PGL_STATIC_VECTORIZE_VEC(degreesf)
PGL_VECTORIZE_VEC(sinf)
PGL_VECTORIZE_VEC(cosf)
PGL_VECTORIZE_VEC(tanf)
PGL_VECTORIZE_VEC(asinf)
PGL_VECTORIZE_VEC(acosf)
PGL_VECTORIZE_VEC(atanf)
PGL_VECTORIZE2_VEC(atan2f)
PGL_VECTORIZE_VEC(sinhf)
PGL_VECTORIZE_VEC(coshf)
PGL_VECTORIZE_VEC(tanhf)
PGL_VECTORIZE_VEC(asinhf)
PGL_VECTORIZE_VEC(acoshf)
PGL_VECTORIZE_VEC(atanhf)

// 8.2 Exponential Functions

static inline float inversesqrtf(float x)
{
	return 1/sqrtf(x);
}

PGL_VECTORIZE2_VEC(powf)
PGL_VECTORIZE_VEC(expf)
PGL_VECTORIZE_VEC(exp2f)
PGL_VECTORIZE_VEC(logf)
PGL_VECTORIZE_VEC(log2f)
PGL_VECTORIZE_VEC(sqrtf)
PGL_STATIC_VECTORIZE_VEC(inversesqrtf)

// 8.3 Common Functions
//
static inline float signf(float x)
{
	if (x > 0.0f) return 1.0f;
	if (x < 0.0f) return -1.0f;
	return 0.0f;
}

static inline float fractf(float x) { return x - floorf(x); }

// GLSL mod() function, can't do modf for float because
// modf is a different standard C function for doubles
// TODO final name?
static inline float modulusf(float x, float y)
{
	return x - y * floorf(x/y);
}

static inline float minf(float x, float y)
{
	return (x < y) ? x : y;
}
static inline float maxf(float x, float y)
{
	return (x > y) ? x : y;
}

static inline float clamp_01(float f)
{
	if (f < 0.0f) return 0.0f;
	if (f > 1.0f) return 1.0f;
	return f;
}

static inline float clamp(float x, float minVal, float maxVal)
{
	if (x < minVal) return minVal;
	if (x > maxVal) return maxVal;
	return x;
}

static inline int clampi(int i, int min, int max)
{
	if (i < min) return min;
	if (i > max) return max;
	return i;
}

static inline float mix(float x, float y, float a)
{
	return x*(1-a) + y*a;
}

PGL_VECTORIZE_IVEC(abs)
PGL_VECTORIZE_VEC(fabsf)
PGL_STATIC_VECTORIZE_VEC(signf)
PGL_VECTORIZE_VEC(floorf)
PGL_VECTORIZE_VEC(truncf)
PGL_VECTORIZE_VEC(roundf)

// assumes current rounding direction (fegetround/fesetround)
// is nearest in which case nearbyintf rounds to nearest even
#define roundEvenf nearbyintf
PGL_VECTORIZE_VEC(nearbyintf)

PGL_VECTORIZE_VEC(ceilf)
PGL_STATIC_VECTORIZE_VEC(fractf)

PGL_STATIC_VECTORIZE2_VEC(modulusf)
PGL_STATIC_VECTORIZE2_VEC(minf)
PGL_STATIC_VECTORIZE2_VEC(maxf)

PGL_STATIC_VECTORIZE_VEC(clamp_01)
PGL_STATIC_VECTORIZE_2_VEC(clamp)
PGL_STATIC_VECTORIZE2_1_VEC(mix)

PGL_VECTORIZE_VEC(isnan)
PGL_VECTORIZE_VEC(isinf)


// 8.4 Geometric Functions
// Most of these are elsewhere in the the file
// TODO Where should these go?

static inline float distance_vec2(vec2 a, vec2 b)
{
	return length_vec2(sub_vec2s(a, b));
}
static inline float distance_vec3(vec3 a, vec3 b)
{
	return length_vec3(sub_vec3s(a, b));
}

static inline vec3 reflect_vec3(vec3 i, vec3 n)
{
	return sub_vec3s(i, scale_vec3(n, 2 * dot_vec3s(i, n)));
}

static inline float smoothstep(float edge0, float edge1, float x)
{
	float t = clamp_01((x-edge0)/(edge1-edge0));
	return t*t*(3 - 2*t);
}

// 8.5 Matrix Functions
// Again the ones that exist are currently elsewhere

// 8.6 Vector Relational functions

static inline u8 lessThan(float x, float y) { return x < y; }
static inline u8 lessThanEqual(float x, float y) { return x <= y; }
static inline u8 greaterThan(float x, float y) { return x > y; }
static inline u8 greaterThanEqual(float x, float y) { return x >= y; }
static inline u8 equal(float x, float y) { return x == y; }
static inline u8 notEqual(float x, float y) { return x != y; }

//TODO any, all, not

PGL_STATIC_VECTORIZE2_BVEC(lessThan)
PGL_STATIC_VECTORIZE2_BVEC(lessThanEqual)
PGL_STATIC_VECTORIZE2_BVEC(greaterThan)
PGL_STATIC_VECTORIZE2_BVEC(greaterThanEqual)
PGL_STATIC_VECTORIZE2_BVEC(equal)
PGL_STATIC_VECTORIZE2_BVEC(notEqual)

// 8.7 Texture Lookup Functions
// currently in gl_glsl.h/c
