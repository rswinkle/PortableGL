
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

inline vec3 make_v3(float x, float y, float z)
{
	vec3 v = { x, y, z };
	return v;
}

inline vec3 neg_v3(vec3 v)
{
	vec3 r = { -v.x, -v.y, -v.z };
	return r;
}

inline void fprint_v3(FILE* f, vec3 v, const char* append)
{
	fprintf(f, "(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

inline void print_v3(vec3 v, const char* append)
{
	printf("(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

inline int fread_v3(FILE* f, vec3* v)
{
	int tmp = fscanf(f, " (%f, %f, %f)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}

inline float len_v3(vec3 a)
{
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

inline vec3 norm_v3(vec3 a)
{
	float l = len_v3(a);
	vec3 c = { a.x/l, a.y/l, a.z/l };
	return c;
}

inline void normalize_v3(vec3* a)
{
	float l = len_v3(*a);
	a->x /= l;
	a->y /= l;
	a->z /= l;
}

inline vec3 add_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x + b.x, a.y + b.y, a.z + b.z };
	return c;
}

inline vec3 sub_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x - b.x, a.y - b.y, a.z - b.z };
	return c;
}

inline vec3 mult_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x * b.x, a.y * b.y, a.z * b.z };
	return c;
}

inline vec3 div_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x / b.x, a.y / b.y, a.z / b.z };
	return c;
}

inline float dot_v3s(vec3 a, vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 scale_v3(vec3 a, float s)
{
	vec3 b = { a.x * s, a.y * s, a.z * s };
	return b;
}

inline int equal_v3s(vec3 a, vec3 b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}

inline int equal_epsilon_v3s(vec3 a, vec3 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
			fabs(a.z - b.z) < epsilon);
}

inline vec3 cross_v3s(const vec3 u, const vec3 v)
{
	vec3 result;
	result.x = u.y*v.z - v.y*u.z;
	result.y = -u.x*v.z + v.x*u.z;
	result.z = u.x*v.y - v.x*u.y;
	return result;
}

inline float angle_v3s(const vec3 u, const vec3 v)
{
	return acos(dot_v3s(u, v));
}

