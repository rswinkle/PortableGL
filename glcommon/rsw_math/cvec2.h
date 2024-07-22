#ifndef CVEC2_H
#define CVEC2_H

#include <stdio.h>

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

#endif

