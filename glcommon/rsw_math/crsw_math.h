#ifndef CRSW_MATH_H
#define CRSW_MATH_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

#ifndef RSW_INLINE
#ifdef _WIN32
	#define RSW_INLINE __attribute__((always_inline)) inline
#else
	#define RSW_INLINE inline
#endif
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

#define SET_V2(v, _x, _y) \
	do {\
	(v).x = _x;\
	(v).y = _y;\
	} while (0)

RSW_INLINE vec2 make_v2(float x, float y)
{
	vec2 v = { x, y };
	return v;
}

RSW_INLINE vec2 neg_v2(vec2 v)
{
	vec2 r = { -v.x, -v.y };
	return r;
}

RSW_INLINE void fprint_v2(FILE* f, vec2 v, const char* append)
{
	fprintf(f, "(%f, %f)%s", v.x, v.y, append);
}

RSW_INLINE void print_v2(vec2 v, const char* append)
{
	printf("(%f, %f)%s", v.x, v.y, append);
}

RSW_INLINE int fread_v2(FILE* f, vec2* v)
{
	int tmp = fscanf(f, " (%f, %f)", &v->x, &v->y);
	return (tmp == 2);
}

RSW_INLINE float len_v2(vec2 a)
{
	return sqrt(a.x * a.x + a.y * a.y);
}

RSW_INLINE vec2 norm_v2(vec2 a)
{
	float l = len_v2(a);
	vec2 c = { a.x/l, a.y/l };
	return c;
}

RSW_INLINE void normalize_v2(vec2* a)
{
	float l = len_v2(*a);
	a->x /= l;
	a->y /= l;
}

RSW_INLINE vec2 add_v2s(vec2 a, vec2 b)
{
	vec2 c = { a.x + b.x, a.y + b.y };
	return c;
}

RSW_INLINE vec2 sub_v2s(vec2 a, vec2 b)
{
	vec2 c = { a.x - b.x, a.y - b.y };
	return c;
}

RSW_INLINE vec2 mult_v2s(vec2 a, vec2 b)
{
	vec2 c = { a.x * b.x, a.y * b.y };
	return c;
}

RSW_INLINE vec2 div_v2s(vec2 a, vec2 b)
{
	vec2 c = { a.x / b.x, a.y / b.y };
	return c;
}

RSW_INLINE float dot_v2s(vec2 a, vec2 b)
{
	return a.x*b.x + a.y*b.y;
}

RSW_INLINE vec2 add_v2(vec2 a, float s)
{
	vec2 b = { a.x + s, a.y + s };
	return b;
}

RSW_INLINE vec2 scale_v2(vec2 a, float s)
{
	vec2 b = { a.x * s, a.y * s };
	return b;
}

RSW_INLINE int equal_v2s(vec2 a, vec2 b)
{
	return (a.x == b.x && a.y == b.y);
}

RSW_INLINE int equal_epsilon_v2s(vec2 a, vec2 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon);
}

RSW_INLINE float cross_v2s(vec2 a, vec2 b)
{
	return a.x * b.y - a.y * b.x;
}

RSW_INLINE float angle_v2s(vec2 a, vec2 b)
{
	return acos(dot_v2s(a, b) / (len_v2(a) * len_v2(b)));
}


typedef struct vec3
{
	float x;
	float y;
	float z;
} vec3;

#define SET_V3(v, _x, _y, _z) \
	do {\
	(v).x = _x;\
	(v).y = _y;\
	(v).z = _z;\
	} while (0)

RSW_INLINE vec3 make_v3(float x, float y, float z)
{
	vec3 v = { x, y, z };
	return v;
}

RSW_INLINE vec3 neg_v3(vec3 v)
{
	vec3 r = { -v.x, -v.y, -v.z };
	return r;
}

RSW_INLINE void fprint_v3(FILE* f, vec3 v, const char* append)
{
	fprintf(f, "(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

RSW_INLINE void print_v3(vec3 v, const char* append)
{
	printf("(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

RSW_INLINE int fread_v3(FILE* f, vec3* v)
{
	int tmp = fscanf(f, " (%f, %f, %f)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}

RSW_INLINE float len_v3(vec3 a)
{
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

RSW_INLINE vec3 norm_v3(vec3 a)
{
	float l = len_v3(a);
	vec3 c = { a.x/l, a.y/l, a.z/l };
	return c;
}

RSW_INLINE void normalize_v3(vec3* a)
{
	float l = len_v3(*a);
	a->x /= l;
	a->y /= l;
	a->z /= l;
}

RSW_INLINE vec3 add_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x + b.x, a.y + b.y, a.z + b.z };
	return c;
}

RSW_INLINE vec3 sub_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x - b.x, a.y - b.y, a.z - b.z };
	return c;
}

RSW_INLINE vec3 mult_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x * b.x, a.y * b.y, a.z * b.z };
	return c;
}

RSW_INLINE vec3 div_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x / b.x, a.y / b.y, a.z / b.z };
	return c;
}

RSW_INLINE float dot_v3s(vec3 a, vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

RSW_INLINE vec3 add_v3(vec3 a, float s)
{
	vec3 b = { a.x + s, a.y + s, a.z + s };
	return b;
}

RSW_INLINE vec3 scale_v3(vec3 a, float s)
{
	vec3 b = { a.x * s, a.y * s, a.z * s };
	return b;
}

RSW_INLINE int equal_v3s(vec3 a, vec3 b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}

RSW_INLINE int equal_epsilon_v3s(vec3 a, vec3 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
			fabs(a.z - b.z) < epsilon);
}

RSW_INLINE vec3 cross_v3s(const vec3 u, const vec3 v)
{
	vec3 result;
	result.x = u.y*v.z - v.y*u.z;
	result.y = -u.x*v.z + v.x*u.z;
	result.z = u.x*v.y - v.x*u.y;
	return result;
}

RSW_INLINE float angle_v3s(const vec3 u, const vec3 v)
{
	return acos(dot_v3s(u, v));
}


typedef struct vec4
{
	float x;
	float y;
	float z;
	float w;
} vec4;

#define SET_V4(v, _x, _y, _z, _w) \
	do {\
	(v).x = _x;\
	(v).y = _y;\
	(v).z = _z;\
	(v).w = _w;\
	} while (0)

RSW_INLINE vec4 make_v4(float x, float y, float z, float w)
{
	vec4 v = { x, y, z, w };
	return v;
}

RSW_INLINE vec4 neg_v4(vec4 v)
{
	vec4 r = { -v.x, -v.y, -v.z, -v.w };
	return r;
}

RSW_INLINE void fprint_v4(FILE* f, vec4 v, const char* append)
{
	fprintf(f, "(%f, %f, %f, %f)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE void print_v4(vec4 v, const char* append)
{
	printf("(%f, %f, %f, %f)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE int fread_v4(FILE* f, vec4* v)
{
	int tmp = fscanf(f, " (%f, %f, %f, %f)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}

RSW_INLINE float len_v4(vec4 a)
{
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

RSW_INLINE vec4 norm_v4(vec4 a)
{
	float l = len_v4(a);
	vec4 c = { a.x/l, a.y/l, a.z/l, a.w/l };
	return c;
}

RSW_INLINE void normalize_v4(vec4* a)
{
	float l = len_v4(*a);
	a->x /= l;
	a->y /= l;
	a->z /= l;
	a->w /= l;
}

RSW_INLINE vec4 add_v4s(vec4 a, vec4 b)
{
	vec4 c = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
	return c;
}

RSW_INLINE vec4 sub_v4s(vec4 a, vec4 b)
{
	vec4 c = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
	return c;
}

RSW_INLINE vec4 mult_v4s(vec4 a, vec4 b)
{
	vec4 c = { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
	return c;
}

RSW_INLINE vec4 div_v4s(vec4 a, vec4 b)
{
	vec4 c = { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
	return c;
}

RSW_INLINE float dot_v4s(vec4 a, vec4 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

RSW_INLINE vec4 add_v4(vec4 a, float s)
{
	vec4 b = { a.x + s, a.y + s, a.z + s, a.w + s };
	return b;
}

RSW_INLINE vec4 scale_v4(vec4 a, float s)
{
	vec4 b = { a.x * s, a.y * s, a.z * s, a.w * s };
	return b;
}

RSW_INLINE int equal_v4s(vec4 a, vec4 b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}

RSW_INLINE int equal_epsilon_v4s(vec4 a, vec4 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
	        fabs(a.z - b.z) < epsilon && fabs(a.w - b.w) < epsilon);
}


typedef struct ivec2
{
	int x;
	int y;
} ivec2;

RSW_INLINE ivec2 make_iv2(int x, int y)
{
	ivec2 v = { x, y };
	return v;
}

RSW_INLINE void fprint_iv2(FILE* f, ivec2 v, const char* append)
{
	fprintf(f, "(%d, %d)%s", v.x, v.y, append);
}

RSW_INLINE int fread_iv2(FILE* f, ivec2* v)
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

RSW_INLINE ivec3 make_iv3(int x, int y, int z)
{
	ivec3 v = { x, y, z };
	return v;
}

RSW_INLINE void fprint_iv3(FILE* f, ivec3 v, const char* append)
{
	fprintf(f, "(%d, %d, %d)%s", v.x, v.y, v.z, append);
}

RSW_INLINE int fread_iv3(FILE* f, ivec3* v)
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

RSW_INLINE ivec4 make_iv4(int x, int y, int z, int w)
{
	ivec4 v = { x, y, z, w };
	return v;
}

RSW_INLINE void fprint_iv4(FILE* f, ivec4 v, const char* append)
{
	fprintf(f, "(%d, %d, %d, %d)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE int fread_iv4(FILE* f, ivec4* v)
{
	int tmp = fscanf(f, " (%d, %d, %d, %d)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}



typedef struct uvec2
{
	unsigned int x;
	unsigned int y;
} uvec2;

RSW_INLINE uvec2 make_uv2(unsigned int x, unsigned int y)
{
	uvec2 v = { x, y };
	return v;
}

RSW_INLINE void fprint_uv2(FILE* f, uvec2 v, const char* append)
{
	fprintf(f, "(%u, %u)%s", v.x, v.y, append);
}

RSW_INLINE int fread_uv2(FILE* f, uvec2* v)
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

RSW_INLINE uvec3 make_uv3(unsigned int x, unsigned int y, unsigned int z)
{
	uvec3 v = { x, y, z };
	return v;
}

RSW_INLINE void fprint_uv3(FILE* f, uvec3 v, const char* append)
{
	fprintf(f, "(%u, %u, %u)%s", v.x, v.y, v.z, append);
}

RSW_INLINE int fread_uv3(FILE* f, uvec3* v)
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

RSW_INLINE uvec4 make_uv4(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
{
	uvec4 v = { x, y, z, w };
	return v;
}

RSW_INLINE void fprint_uv4(FILE* f, uvec4 v, const char* append)
{
	fprintf(f, "(%u, %u, %u, %u)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE int fread_uv4(FILE* f, uvec4* v)
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
RSW_INLINE bvec2 make_bv2(int x, int y)
{
	bvec2 v = { !!x, !!y };
	return v;
}

RSW_INLINE void fprint_bv2(FILE* f, bvec2 v, const char* append)
{
	fprintf(f, "(%u, %u)%s", v.x, v.y, append);
}

// Should technically use SCNu8 macro not hhu
RSW_INLINE int fread_bv2(FILE* f, bvec2* v)
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

RSW_INLINE bvec3 make_bv3(int x, int y, int z)
{
	bvec3 v = { !!x, !!y, !!z };
	return v;
}

RSW_INLINE void fprint_bv3(FILE* f, bvec3 v, const char* append)
{
	fprintf(f, "(%u, %u, %u)%s", v.x, v.y, v.z, append);
}

RSW_INLINE int fread_bv3(FILE* f, bvec3* v)
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

RSW_INLINE bvec4 make_bv4(int x, int y, int z, int w)
{
	bvec4 v = { !!x, !!y, !!z, !!w };
	return v;
}

RSW_INLINE void fprint_bv4(FILE* f, bvec4 v, const char* append)
{
	fprintf(f, "(%u, %u, %u, %u)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE int fread_bv4(FILE* f, bvec4* v)
{
	int tmp = fscanf(f, " (%hhu, %hhu, %hhu, %hhu)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}


typedef struct dvec2
{
	double x;
	double y;
} dvec2;

RSW_INLINE void fprint_dv2(FILE* f, dvec2 v, const char* append)
{
	fprintf(f, "(%f, %f)%s", v.x, v.y, append);
}

RSW_INLINE int fread_dv2(FILE* f, dvec2* v)
{
	int tmp = fscanf(f, " (%lf, %lf)", &v->x, &v->y);
	return (tmp == 2);
}


typedef struct dvec3
{
	double x;
	double y;
	double z;
} dvec3;

RSW_INLINE void fprint_dv3(FILE* f, dvec3 v, const char* append)
{
	fprintf(f, "(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

RSW_INLINE int fread_dv3(FILE* f, dvec3* v)
{
	int tmp = fscanf(f, " (%lf, %lf, %lf)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}


typedef struct dvec4
{
	double x;
	double y;
	double z;
	double w;
} dvec4;

RSW_INLINE void fprint_dv4(FILE* f, dvec4 v, const char* append)
{
	fprintf(f, "(%f, %f, %f, %f)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE int fread_dv4(FILE* f, dvec4* v)
{
	int tmp = fscanf(f, " (%lf, %lf, %lf, %lf)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}


RSW_INLINE vec2 v3_to_v2(vec3 a)
{
	vec2 v = { a.x, a.y };
	return v;
}

RSW_INLINE vec2 v4_to_v2(vec4 a)
{
	vec2 v = { a.x, a.y };
	return v;
}

RSW_INLINE vec3 v4_to_v3(vec4 a)
{
	vec3 v = { a.x, a.y, a.z };
	return v;
}

RSW_INLINE vec2 v4_to_v2h(vec4 a)
{
	vec2 v = { a.x/a.w, a.y/a.w };
	return v;
}

RSW_INLINE vec3 v4_to_v3h(vec4 a)
{
	vec3 v = { a.x/a.w, a.y/a.w, a.z/a.w };
	return v;
}


/* matrices **************/

typedef float mat2[4];
typedef float mat3[9];
typedef float mat4[16];

#define IDENTITY_M2() { 1, 0, 0, 1 }
#define IDENTITY_M3() { 1, 0, 0, 0, 1, 0, 0, 0, 1 }
#define IDENTITY_M4() { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }
#define SET_IDENTITY_M2(m) \
	do { \
	m[1] = m[2] = 0; \
	m[0] = m[3] = 1; \
	} while (0)

#define SET_IDENTITY_M3(m) \
	do { \
	m[1] = m[2] = m[3] = 0; \
	m[5] = m[6] = m[7] = 0; \
	m[0] = m[4] = m[8] = 1; \
	} while (0)

#define SET_IDENTITY_M4(m) \
	do { \
	m[1] = m[2] = m[3] = m[4] = 0; \
	m[6] = m[7] = m[8] = m[9] = 0; \
	m[11] = m[12] = m[13] = m[14] = 0; \
	m[0] = m[5] = m[10] = m[15] = 1; \
	} while (0)

#ifndef ROW_MAJOR
RSW_INLINE vec2 x_m2(mat2 m) {  return make_v2(m[0], m[2]); }
RSW_INLINE vec2 y_m2(mat2 m) {  return make_v2(m[1], m[3]); }
RSW_INLINE vec2 c1_m2(mat2 m) { return make_v2(m[0], m[1]); }
RSW_INLINE vec2 c2_m2(mat2 m) { return make_v2(m[2], m[3]); }

RSW_INLINE void setc1_m2(mat2 m, vec2 v) { m[0]=v.x, m[1]=v.y; }
RSW_INLINE void setc2_m2(mat2 m, vec2 v) { m[2]=v.x, m[3]=v.y; }

RSW_INLINE void setx_m2(mat2 m, vec2 v) { m[0]=v.x, m[2]=v.y; }
RSW_INLINE void sety_m2(mat2 m, vec2 v) { m[1]=v.x, m[3]=v.y; }
#else
RSW_INLINE vec2 x_m2(mat2 m) {  return make_v2(m[0], m[1]); }
RSW_INLINE vec2 y_m2(mat2 m) {  return make_v2(m[2], m[3]); }
RSW_INLINE vec2 c1_m2(mat2 m) { return make_v2(m[0], m[2]); }
RSW_INLINE vec2 c2_m2(mat2 m) { return make_v2(m[1], m[3]); }

RSW_INLINE void setc1_m2(mat2 m, vec2 v) { m[0]=v.x, m[2]=v.y; }
RSW_INLINE void setc2_m2(mat2 m, vec2 v) { m[1]=v.x, m[3]=v.y; }

RSW_INLINE void setx_m2(mat2 m, vec2 v) { m[0]=v.x, m[1]=v.y; }
RSW_INLINE void sety_m2(mat2 m, vec2 v) { m[2]=v.x, m[3]=v.y; }
#endif


#ifndef ROW_MAJOR
RSW_INLINE vec3 x_m3(mat3 m) {  return make_v3(m[0], m[3], m[6]); }
RSW_INLINE vec3 y_m3(mat3 m) {  return make_v3(m[1], m[4], m[7]); }
RSW_INLINE vec3 z_m3(mat3 m) {  return make_v3(m[2], m[5], m[8]); }
RSW_INLINE vec3 c1_m3(mat3 m) { return make_v3(m[0], m[1], m[2]); }
RSW_INLINE vec3 c2_m3(mat3 m) { return make_v3(m[3], m[4], m[5]); }
RSW_INLINE vec3 c3_m3(mat3 m) { return make_v3(m[6], m[7], m[8]); }

RSW_INLINE void setc1_m3(mat3 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z; }
RSW_INLINE void setc2_m3(mat3 m, vec3 v) { m[3]=v.x, m[4]=v.y, m[5]=v.z; }
RSW_INLINE void setc3_m3(mat3 m, vec3 v) { m[6]=v.x, m[7]=v.y, m[8]=v.z; }

RSW_INLINE void setx_m3(mat3 m, vec3 v) { m[0]=v.x, m[3]=v.y, m[6]=v.z; }
RSW_INLINE void sety_m3(mat3 m, vec3 v) { m[1]=v.x, m[4]=v.y, m[7]=v.z; }
RSW_INLINE void setz_m3(mat3 m, vec3 v) { m[2]=v.x, m[5]=v.y, m[8]=v.z; }
#else
RSW_INLINE vec3 x_m3(mat3 m) {  return make_v3(m[0], m[1], m[2]); }
RSW_INLINE vec3 y_m3(mat3 m) {  return make_v3(m[3], m[4], m[5]); }
RSW_INLINE vec3 z_m3(mat3 m) {  return make_v3(m[6], m[7], m[8]); }
RSW_INLINE vec3 c1_m3(mat3 m) { return make_v3(m[0], m[3], m[6]); }
RSW_INLINE vec3 c2_m3(mat3 m) { return make_v3(m[1], m[4], m[7]); }
RSW_INLINE vec3 c3_m3(mat3 m) { return make_v3(m[2], m[5], m[8]); }

RSW_INLINE void setc1_m3(mat3 m, vec3 v) { m[0]=v.x, m[3]=v.y, m[6]=v.z; }
RSW_INLINE void setc2_m3(mat3 m, vec3 v) { m[1]=v.x, m[4]=v.y, m[7]=v.z; }
RSW_INLINE void setc3_m3(mat3 m, vec3 v) { m[2]=v.x, m[5]=v.y, m[8]=v.z; }

RSW_INLINE void setx_m3(mat3 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z; }
RSW_INLINE void sety_m3(mat3 m, vec3 v) { m[3]=v.x, m[4]=v.y, m[5]=v.z; }
RSW_INLINE void setz_m3(mat3 m, vec3 v) { m[6]=v.x, m[7]=v.y, m[8]=v.z; }
#endif


#ifndef ROW_MAJOR
RSW_INLINE vec4 c1_m4(mat4 m) { return make_v4(m[ 0], m[ 1], m[ 2], m[ 3]); }
RSW_INLINE vec4 c2_m4(mat4 m) { return make_v4(m[ 4], m[ 5], m[ 6], m[ 7]); }
RSW_INLINE vec4 c3_m4(mat4 m) { return make_v4(m[ 8], m[ 9], m[10], m[11]); }
RSW_INLINE vec4 c4_m4(mat4 m) { return make_v4(m[12], m[13], m[14], m[15]); }

RSW_INLINE vec4 x_m4(mat4 m) { return make_v4(m[0], m[4], m[8], m[12]); }
RSW_INLINE vec4 y_m4(mat4 m) { return make_v4(m[1], m[5], m[9], m[13]); }
RSW_INLINE vec4 z_m4(mat4 m) { return make_v4(m[2], m[6], m[10], m[14]); }
RSW_INLINE vec4 w_m4(mat4 m) { return make_v4(m[3], m[7], m[11], m[15]); }

//sets 4th row to 0 0 0 1
RSW_INLINE void setc1_m4v3(mat4 m, vec3 v) { m[ 0]=v.x, m[ 1]=v.y, m[ 2]=v.z, m[ 3]=0; }
RSW_INLINE void setc2_m4v3(mat4 m, vec3 v) { m[ 4]=v.x, m[ 5]=v.y, m[ 6]=v.z, m[ 7]=0; }
RSW_INLINE void setc3_m4v3(mat4 m, vec3 v) { m[ 8]=v.x, m[ 9]=v.y, m[10]=v.z, m[11]=0; }
RSW_INLINE void setc4_m4v3(mat4 m, vec3 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=1; }

RSW_INLINE void setc1_m4v4(mat4 m, vec4 v) { m[ 0]=v.x, m[ 1]=v.y, m[ 2]=v.z, m[ 3]=v.w; }
RSW_INLINE void setc2_m4v4(mat4 m, vec4 v) { m[ 4]=v.x, m[ 5]=v.y, m[ 6]=v.z, m[ 7]=v.w; }
RSW_INLINE void setc3_m4v4(mat4 m, vec4 v) { m[ 8]=v.x, m[ 9]=v.y, m[10]=v.z, m[11]=v.w; }
RSW_INLINE void setc4_m4v4(mat4 m, vec4 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=v.w; }

//sets 4th column to 0 0 0 1
RSW_INLINE void setx_m4v3(mat4 m, vec3 v) { m[0]=v.x, m[4]=v.y, m[ 8]=v.z, m[12]=0; }
RSW_INLINE void sety_m4v3(mat4 m, vec3 v) { m[1]=v.x, m[5]=v.y, m[ 9]=v.z, m[13]=0; }
RSW_INLINE void setz_m4v3(mat4 m, vec3 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=0; }
RSW_INLINE void setw_m4v3(mat4 m, vec3 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=1; }

RSW_INLINE void setx_m4v4(mat4 m, vec4 v) { m[0]=v.x, m[4]=v.y, m[ 8]=v.z, m[12]=v.w; }
RSW_INLINE void sety_m4v4(mat4 m, vec4 v) { m[1]=v.x, m[5]=v.y, m[ 9]=v.z, m[13]=v.w; }
RSW_INLINE void setz_m4v4(mat4 m, vec4 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=v.w; }
RSW_INLINE void setw_m4v4(mat4 m, vec4 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=v.w; }
#else
RSW_INLINE vec4 c1_m4(mat4 m) { return make_v4(m[0], m[4], m[8], m[12]); }
RSW_INLINE vec4 c2_m4(mat4 m) { return make_v4(m[1], m[5], m[9], m[13]); }
RSW_INLINE vec4 c3_m4(mat4 m) { return make_v4(m[2], m[6], m[10], m[14]); }
RSW_INLINE vec4 c4_m4(mat4 m) { return make_v4(m[3], m[7], m[11], m[15]); }

RSW_INLINE vec4 x_m4(mat4 m) { return make_v4(m[0], m[1], m[2], m[3]); }
RSW_INLINE vec4 y_m4(mat4 m) { return make_v4(m[4], m[5], m[6], m[7]); }
RSW_INLINE vec4 z_m4(mat4 m) { return make_v4(m[8], m[9], m[10], m[11]); }
RSW_INLINE vec4 w_m4(mat4 m) { return make_v4(m[12], m[13], m[14], m[15]); }

//sets 4th row to 0 0 0 1
RSW_INLINE void setc1_m4v3(mat4 m, vec3 v) { m[0]=v.x, m[4]=v.y, m[8]=v.z, m[12]=0; }
RSW_INLINE void setc2_m4v3(mat4 m, vec3 v) { m[1]=v.x, m[5]=v.y, m[9]=v.z, m[13]=0; }
RSW_INLINE void setc3_m4v3(mat4 m, vec3 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=0; }
RSW_INLINE void setc4_m4v3(mat4 m, vec3 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=1; }

RSW_INLINE void setc1_m4v4(mat4 m, vec4 v) { m[0]=v.x, m[4]=v.y, m[8]=v.z, m[12]=v.w; }
RSW_INLINE void setc2_m4v4(mat4 m, vec4 v) { m[1]=v.x, m[5]=v.y, m[9]=v.z, m[13]=v.w; }
RSW_INLINE void setc3_m4v4(mat4 m, vec4 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=v.w; }
RSW_INLINE void setc4_m4v4(mat4 m, vec4 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=v.w; }

//sets 4th column to 0 0 0 1
RSW_INLINE void setx_m4v3(mat4 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z, m[3]=0; }
RSW_INLINE void sety_m4v3(mat4 m, vec3 v) { m[4]=v.x, m[5]=v.y, m[6]=v.z, m[7]=0; }
RSW_INLINE void setz_m4v3(mat4 m, vec3 v) { m[8]=v.x, m[9]=v.y, m[10]=v.z, m[11]=0; }
RSW_INLINE void setw_m4v3(mat4 m, vec3 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=1; }

RSW_INLINE void setx_m4v4(mat4 m, vec4 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z, m[3]=v.w; }
RSW_INLINE void sety_m4v4(mat4 m, vec4 v) { m[4]=v.x, m[5]=v.y, m[6]=v.z, m[7]=v.w; }
RSW_INLINE void setz_m4v4(mat4 m, vec4 v) { m[8]=v.x, m[9]=v.y, m[10]=v.z, m[11]=v.w; }
RSW_INLINE void setw_m4v4(mat4 m, vec4 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=v.w; }
#endif


RSW_INLINE void fprint_m2(FILE* f, mat2 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f)\n (%f, %f)]%s",
	        m[0], m[2], m[1], m[3], append);
#else
	fprintf(f, "[(%f, %f)\n (%f, %f)]%s",
	        m[0], m[1], m[2], m[3], append);
#endif
}


RSW_INLINE void fprint_m3(FILE* f, mat3 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f, %f)\n (%f, %f, %f)\n (%f, %f, %f)]%s",
	        m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8], append);
#else
	fprintf(f, "[(%f, %f, %f)\n (%f, %f, %f)\n (%f, %f, %f)]%s",
	        m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], append);
#endif
}

RSW_INLINE void fprint_m4(FILE* f, mat4 m, const char* append)
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
RSW_INLINE void print_m2(mat2 m, const char* append)
{
	fprint_m2(stdout, m, append);
}

RSW_INLINE void print_m3(mat3 m, const char* append)
{
	fprint_m3(stdout, m, append);
}

RSW_INLINE void print_m4(mat4 m, const char* append)
{
	fprint_m4(stdout, m, append);
}

//TODO define macros for doing array version
RSW_INLINE vec2 mult_m2_v2(mat2 m, vec2 v)
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


RSW_INLINE vec3 mult_m3_v3(mat3 m, vec3 v)
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

RSW_INLINE vec4 mult_m4_v4(mat4 m, vec4 v)
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

void mult_m2_m2(mat2 c, mat2 a, mat2 b);

void mult_m3_m3(mat3 c, mat3 a, mat3 b);

void mult_m4_m4(mat4 c, mat4 a, mat4 b);

RSW_INLINE void load_rotation_m2(mat2 mat, float angle)
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

void load_rotation_m3(mat3 mat, vec3 v, float angle);

void load_rotation_m4(mat4 mat, vec3 vec, float angle);

//void invert_m4(mat4 mInverse, const mat4 m);

void make_perspective_m4(mat4 mat, float fFov, float aspect, float near, float far);
void make_pers_m4(mat4 mat, float z_near, float z_far);

void make_perspective_proj_m4(mat4 mat, float left, float right, float bottom, float top, float near, float far);

void make_orthographic_m4(mat4 mat, float left, float right, float bottom, float top, float near, float far);

void make_viewport_m4(mat4 mat, int x, int y, unsigned int width, unsigned int height, int opengl);

void lookAt(mat4 mat, vec3 eye, vec3 center, vec3 up);


///////////Matrix transformation functions
RSW_INLINE void scale_m3(mat3 m, float x, float y, float z)
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

RSW_INLINE void scale_m4(mat4 m, float x, float y, float z)
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
RSW_INLINE void translation_m4(mat4 m, float x, float y, float z)
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
RSW_INLINE void extract_rotation_m4(mat3 dst, mat4 src, int normalize)
{
	vec3 tmp;
	if (normalize) {
		tmp.x = M44(src, 0, 0);
		tmp.y = M44(src, 1, 0);
		tmp.z = M44(src, 2, 0);
		normalize_v3(&tmp);

		M33(dst, 0, 0) = tmp.x;
		M33(dst, 1, 0) = tmp.y;
		M33(dst, 2, 0) = tmp.z;

		tmp.x = M44(src, 0, 1);
		tmp.y = M44(src, 1, 1);
		tmp.z = M44(src, 2, 1);
		normalize_v3(&tmp);

		M33(dst, 0, 1) = tmp.x;
		M33(dst, 1, 1) = tmp.y;
		M33(dst, 2, 1) = tmp.z;

		tmp.x = M44(src, 0, 2);
		tmp.y = M44(src, 1, 2);
		tmp.z = M44(src, 2, 2);
		normalize_v3(&tmp);

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


// returns float [0,1)
RSW_INLINE float rsw_randf(void)
{
	return rand() / ((float)RAND_MAX + 1.0f);
}

RSW_INLINE float rsw_randf_range(float min, float max)
{
	return min + (max-min) * rsw_randf();
}

RSW_INLINE double rsw_map(double x, double a, double b, double c, double d)
{
	return (x-a)/(b-a) * (d-c) + c;
}

RSW_INLINE float rsw_mapf(float x, float a, float b, float c, float d)
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
Color make_Color(void)
{
	r = g = b = 0;
	a = 255;
}
*/

RSW_INLINE Color make_Color(u8 red, u8 green, u8 blue, u8 alpha)
{
	Color c = { red, green, blue, alpha };
	return c;
}

RSW_INLINE void print_Color(Color c, const char* append)
{
	printf("(%d, %d, %d, %d)%s", c.r, c.g, c.b, c.a, append);
}

RSW_INLINE Color v4_to_Color(vec4 v)
{
	//assume all in the range of [0, 1]
	//NOTE(rswinkle): There are other ways of doing the conversion:
	//
	// round like HH: (u8)(v.x * 255.0f + 0.5f)
	// so 0 and 255 get half sized buckets, the rest get [(n-1).5, n.5)
	//
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

RSW_INLINE vec4 Color_to_v4(Color c)
{
	vec4 v = { (float)c.r/255.0f, (float)c.g/255.0f, (float)c.b/255.0f, (float)c.a/255.0f };
	return v;
}

typedef struct Line
{
	float A, B, C;
} Line;

RSW_INLINE Line make_Line(float x1, float y1, float x2, float y2)
{
	Line l;
	l.A = y1 - y2;
	l.B = x2 - x1;
	l.C = x1*y2 - x2*y1;
	return l;
}

RSW_INLINE void normalize_line(Line* line)
{
	// TODO could enforce that n always points toward +y or +x...should I?
	vec2 n = { line->A, line->B };
	float len = len_v2(n);
	line->A /= len;
	line->B /= len;
	line->C /= len;
}

RSW_INLINE float line_func(Line* line, float x, float y)
{
	return line->A*x + line->B*y + line->C;
}
RSW_INLINE float line_findy(Line* line, float x)
{
	return -(line->A*x + line->C)/line->B;
}

RSW_INLINE float line_findx(Line* line, float y)
{
	return -(line->B*y + line->C)/line->A;
}

// return squared distance from c to line segment between a and b
RSW_INLINE float sq_dist_pt_segment2d(vec2 a, vec2 b, vec2 c)
{
	vec2 ab = sub_v2s(b, a);
	vec2 ac = sub_v2s(c, a);
	vec2 bc = sub_v2s(c, b);
	float e = dot_v2s(ac, ab);

	// cases where c projects outside ab
	if (e <= 0.0f) return dot_v2s(ac, ac);
	float f = dot_v2s(ab, ab);
	if (e >= f) return dot_v2s(bc, bc);

	// handle cases where c projects onto ab
	return dot_v2s(ac, ac) - e * e / f;
}

// return t and closest pt on segment ab to c
RSW_INLINE void closest_pt_pt_segment(vec2 c, vec2 a, vec2 b, float* t, vec2* d)
{
	vec2 ab = sub_v2s(b, a);

	// project c onto ab, compute t
	float t_ = dot_v2s(sub_v2s(c, a), ab) / dot_v2s(ab, ab);

	// clamp if outside segment
	if (t_ < 0.0f) t_ = 0.0f;
	if (t_ > 1.0f) t_ = 1.0f;

	// compute projected position
	*d = add_v2s(a, scale_v2(ab, t_));
	*t = t_;
}

RSW_INLINE float closest_pt_pt_segment_t(vec2 c, vec2 a, vec2 b)
{
	vec2 ab = sub_v2s(b, a);

	// project c onto ab, compute t
	float t = dot_v2s(sub_v2s(c, a), ab) / dot_v2s(ab, ab);
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	return t;
}

typedef struct Plane
{
	vec3 n;	//normal points x on plane satisfy n dot x = d
	float d; //d = n dot p

} Plane;

/*
Plane(void) {}
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
RSW_INLINE vec2 operator*(vec2 v, float a) { return scale_v2(v, a); }
RSW_INLINE vec2 operator*(float a, vec2 v) { return scale_v2(v, a); }
RSW_INLINE vec3 operator*(vec3 v, float a) { return scale_v3(v, a); }
RSW_INLINE vec3 operator*(float a, vec3 v) { return scale_v3(v, a); }
RSW_INLINE vec4 operator*(vec4 v, float a) { return scale_v4(v, a); }
RSW_INLINE vec4 operator*(float a, vec4 v) { return scale_v4(v, a); }

RSW_INLINE vec2 operator+(vec2 v1, vec2 v2) { return add_v2s(v1, v2); }
RSW_INLINE vec3 operator+(vec3 v1, vec3 v2) { return add_v3s(v1, v2); }
RSW_INLINE vec4 operator+(vec4 v1, vec4 v2) { return add_v4s(v1, v2); }

RSW_INLINE vec2 operator-(vec2 v1, vec2 v2) { return sub_v2s(v1, v2); }
RSW_INLINE vec3 operator-(vec3 v1, vec3 v2) { return sub_v3s(v1, v2); }
RSW_INLINE vec4 operator-(vec4 v1, vec4 v2) { return sub_v4s(v1, v2); }

RSW_INLINE int operator==(vec2 v1, vec2 v2) { return equal_v2s(v1, v2); }
RSW_INLINE int operator==(vec3 v1, vec3 v2) { return equal_v3s(v1, v2); }
RSW_INLINE int operator==(vec4 v1, vec4 v2) { return equal_v4s(v1, v2); }

RSW_INLINE vec2 operator-(vec2 v) { return neg_v2(v); }
RSW_INLINE vec3 operator-(vec3 v) { return neg_v3(v); }
RSW_INLINE vec4 operator-(vec4 v) { return neg_v4(v); }

RSW_INLINE vec2 operator*(mat2 m, vec2 v) { return mult_m2_v2(m, v); }
RSW_INLINE vec3 operator*(mat3 m, vec3 v) { return mult_m3_v3(m, v); }
RSW_INLINE vec4 operator*(mat4 m, vec4 v) { return mult_m4_v4(m, v); }

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
RSW_INLINE vec2 func##_v2(vec2 v) \
{ \
	return make_v2(func(v.x), func(v.y)); \
}
#define PGL_VECTORIZE_VEC3(func) \
RSW_INLINE vec3 func##_v3(vec3 v) \
{ \
	return make_v3(func(v.x), func(v.y), func(v.z)); \
}
#define PGL_VECTORIZE_VEC4(func) \
RSW_INLINE vec4 func##_v4(vec4 v) \
{ \
	return make_v4(func(v.x), func(v.y), func(v.z), func(v.w)); \
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
RSW_INLINE vec2 func##_v2(vec2 a, vec2 b) \
{ \
	return make_v2(func(a.x, b.x), func(a.y, b.y)); \
}
#define PGL_VECTORIZE2_VEC3(func) \
RSW_INLINE vec3 func##_v3(vec3 a, vec3 b) \
{ \
	return make_v3(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z)); \
}
#define PGL_VECTORIZE2_VEC4(func) \
RSW_INLINE vec4 func##_v4(vec4 a, vec4 b) \
{ \
	return make_v4(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z), func(a.w, b.w)); \
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
RSW_INLINE vec2 func##_v2(vec2 a, vec2 b, float c) \
{ \
	return make_v2(func(a.x, b.x, c), func(a.y, b.y, c)); \
}
#define PGL_VECTORIZE2_1_VEC3(func) \
RSW_INLINE vec3 func##_v3(vec3 a, vec3 b, float c) \
{ \
	return make_v3(func(a.x, b.x, c), func(a.y, b.y, c), func(a.z, b.z, c)); \
}
#define PGL_VECTORIZE2_1_VEC4(func) \
RSW_INLINE vec4 func##_v4(vec4 a, vec4 b, float c) \
{ \
	return make_v4(func(a.x, b.x, c), func(a.y, b.y, c), func(a.z, b.z, c), func(a.w, b.w, c)); \
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
RSW_INLINE vec2 func##_v2(vec2 v, float a, float b) \
{ \
	return make_v2(func(v.x, a, b), func(v.y, a, b)); \
}
#define PGL_VECTORIZE_2_VEC3(func) \
RSW_INLINE vec3 func##_v3(vec3 v, float a, float b) \
{ \
	return make_v3(func(v.x, a, b), func(v.y, a, b), func(v.z, a, b)); \
}
#define PGL_VECTORIZE_2_VEC4(func) \
RSW_INLINE vec4 func##_v4(vec4 v, float a, float b) \
{ \
	return make_v4(func(v.x, a, b), func(v.y, a, b), func(v.z, a, b), func(v.w, a, b)); \
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
RSW_INLINE ivec2 func##_iv2(ivec2 v) \
{ \
	return make_iv2(func(v.x), func(v.y)); \
}
#define PGL_VECTORIZE_IVEC3(func) \
RSW_INLINE ivec3 func##_iv3(ivec3 v) \
{ \
	return make_iv3(func(v.x), func(v.y), func(v.z)); \
}
#define PGL_VECTORIZE_IVEC4(func) \
RSW_INLINE ivec4 func##_iv4(ivec4 v) \
{ \
	return make_iv4(func(v.x), func(v.y), func(v.z), func(v.w)); \
}

#define PGL_VECTORIZE_IVEC(func) \
	PGL_VECTORIZE_IVEC2(func) \
	PGL_VECTORIZE_IVEC3(func) \
	PGL_VECTORIZE_IVEC4(func)

#define PGL_VECTORIZE_BVEC2(func) \
RSW_INLINE bvec2 func##_bv2(bvec2 v) \
{ \
	return make_bv2(func(v.x), func(v.y)); \
}
#define PGL_VECTORIZE_BVEC3(func) \
RSW_INLINE bvec3 func##_bv3(bvec3 v) \
{ \
	return make_bv3(func(v.x), func(v.y), func(v.z)); \
}
#define PGL_VECTORIZE_BVEC4(func) \
RSW_INLINE bvec4 func##_bv4(bvec4 v) \
{ \
	return make_bv4(func(v.x), func(v.y), func(v.z), func(v.w)); \
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
RSW_INLINE bvec2 func##_v2(vec2 a, vec2 b) \
{ \
	return make_bv2(func(a.x, b.x), func(a.y, b.y)); \
}
#define PGL_VECTORIZE2_BVEC3(func) \
RSW_INLINE bvec3 func##_v3(vec3 a, vec3 b) \
{ \
	return make_bv3(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z)); \
}
#define PGL_VECTORIZE2_BVEC4(func) \
RSW_INLINE bvec4 func##_v4(vec4 a, vec4 b) \
{ \
	return make_bv4(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z), func(a.w, b.w)); \
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

static inline float mixf(float x, float y, float a)
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
PGL_STATIC_VECTORIZE2_1_VEC(mixf)

PGL_VECTORIZE_VEC(isnan)
PGL_VECTORIZE_VEC(isinf)


// 8.4 Geometric Functions
// Most of these are elsewhere in the the file
// TODO Where should these go?

static inline float distance_v2(vec2 a, vec2 b)
{
	return len_v2(sub_v2s(a, b));
}
static inline float distance_v3(vec3 a, vec3 b)
{
	return len_v3(sub_v3s(a, b));
}

static inline vec3 reflect_v3(vec3 i, vec3 n)
{
	return sub_v3s(i, scale_v3(n, 2 * dot_v3s(i, n)));
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

#endif
