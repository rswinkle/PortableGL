
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

