
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

