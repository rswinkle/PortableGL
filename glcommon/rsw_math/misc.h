
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
Color make_Color(void)
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

// return t and closest pt on segment ab to c
inline void closest_pt_pt_segment(vec2 c, vec2 a, vec2 b, float* t, vec2* d)
{
	vec2 ab = sub_vec2s(b, a);

	// project c onto ab, compute t
	float t_ = dot_vec2s(sub_vec2s(c, a), ab) / dot_vec2s(ab, ab);

	// clamp if outside segment
	if (t_ < 0.0f) t_ = 0.0f;
	if (t_ > 1.0f) t_ = 1.0f;

	// compute projected position
	*d = add_vec2s(a, scale_vec2(ab, t_));
	*t = t_;
}

inline float closest_pt_pt_segment_t(vec2 c, vec2 a, vec2 b)
{
	vec2 ab = sub_vec2s(b, a);

	// project c onto ab, compute t
	float t = dot_vec2s(sub_vec2s(c, a), ab) / dot_vec2s(ab, ab);
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

