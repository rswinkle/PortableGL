#include "crsw_math.h"


extern inline vec2 make_v2(float x, float y);
extern inline vec2 neg_v2(vec2 v);
extern inline void fprint_v2(FILE* f, vec2 v, const char* append);
extern inline void print_v2(vec2 v, const char* append);
extern inline int fread_v2(FILE* f, vec2* v);
extern inline float len_v2(vec2 a);
extern inline vec2 norm_v2(vec2 a);
extern inline void normalize_v2(vec2* a);
extern inline vec2 add_v2s(vec2 a, vec2 b);
extern inline vec2 sub_v2s(vec2 a, vec2 b);
extern inline vec2 mult_v2s(vec2 a, vec2 b);
extern inline vec2 div_v2s(vec2 a, vec2 b);
extern inline float dot_v2s(vec2 a, vec2 b);
extern inline vec2 scale_v2(vec2 a, float s);
extern inline int equal_v2s(vec2 a, vec2 b);
extern inline int equal_epsilon_v2s(vec2 a, vec2 b, float epsilon);
extern inline float cross_v2s(vec2 a, vec2 b);
extern inline float angle_v2s(vec2 a, vec2 b);


extern inline vec3 make_v3(float x, float y, float z);
extern inline vec3 neg_v3(vec3 v);
extern inline void fprint_v3(FILE* f, vec3 v, const char* append);
extern inline void print_v3(vec3 v, const char* append);
extern inline int fread_v3(FILE* f, vec3* v);
extern inline float len_v3(vec3 a);
extern inline vec3 norm_v3(vec3 a);
extern inline void normalize_v3(vec3* a);
extern inline vec3 add_v3s(vec3 a, vec3 b);
extern inline vec3 sub_v3s(vec3 a, vec3 b);
extern inline vec3 mult_v3s(vec3 a, vec3 b);
extern inline vec3 div_v3s(vec3 a, vec3 b);
extern inline float dot_v3s(vec3 a, vec3 b);
extern inline vec3 scale_v3(vec3 a, float s);
extern inline int equal_v3s(vec3 a, vec3 b);
extern inline int equal_epsilon_v3s(vec3 a, vec3 b, float epsilon);
extern inline vec3 cross_v3s(const vec3 u, const vec3 v);
extern inline float angle_v3s(const vec3 u, const vec3 v);


extern inline vec4 make_v4(float x, float y, float z, float w);
extern inline vec4 neg_v4(vec4 v);
extern inline void fprint_v4(FILE* f, vec4 v, const char* append);
extern inline void print_v4(vec4 v, const char* append);
extern inline int fread_v4(FILE* f, vec4* v);
extern inline float len_v4(vec4 a);
extern inline vec4 norm_v4(vec4 a);
extern inline void normalize_v4(vec4* a);
extern inline vec4 add_v4s(vec4 a, vec4 b);
extern inline vec4 sub_v4s(vec4 a, vec4 b);
extern inline vec4 mult_v4s(vec4 a, vec4 b);
extern inline vec4 div_v4s(vec4 a, vec4 b);
extern inline float dot_v4s(vec4 a, vec4 b);
extern inline vec4 scale_v4(vec4 a, float s);
extern inline int equal_v4s(vec4 a, vec4 b);
extern inline int equal_epsilon_v4s(vec4 a, vec4 b, float epsilon);


extern inline ivec2 make_iv2(int x, int y);
extern inline void fprint_iv2(FILE* f, ivec2 v, const char* append);
extern inline int fread_iv2(FILE* f, ivec2* v);

extern inline ivec3 make_iv3(int x, int y, int z);
extern inline void fprint_iv3(FILE* f, ivec3 v, const char* append);
extern inline int fread_iv3(FILE* f, ivec3* v);

extern inline ivec4 make_iv4(int x, int y, int z, int w);
extern inline void fprint_iv4(FILE* f, ivec4 v, const char* append);
extern inline int fread_iv4(FILE* f, ivec4* v);

extern inline uvec2 make_uv2(unsigned int x, unsigned int y);
extern inline void fprint_uv2(FILE* f, uvec2 v, const char* append);
extern inline int fread_uv2(FILE* f, uvec2* v);

extern inline uvec3 make_uv3(unsigned int x, unsigned int y, unsigned int z);
extern inline void fprint_uv3(FILE* f, uvec3 v, const char* append);
extern inline int fread_uv3(FILE* f, uvec3* v);

extern inline uvec4 make_uv4(unsigned int x, unsigned int y, unsigned int z, unsigned int w);
extern inline void fprint_uv4(FILE* f, uvec4 v, const char* append);
extern inline int fread_uv4(FILE* f, uvec4* v);

extern inline bvec2 make_bv2(int x, int y);
extern inline void fprint_bv2(FILE* f, bvec2 v, const char* append);
extern inline int fread_bv2(FILE* f, bvec2* v);

extern inline bvec3 make_bv3(int x, int y, int z);
extern inline void fprint_bv3(FILE* f, bvec3 v, const char* append);
extern inline int fread_bv3(FILE* f, bvec3* v);

extern inline bvec4 make_bv4(int x, int y, int z, int w);
extern inline void fprint_bv4(FILE* f, bvec4 v, const char* append);
extern inline int fread_bv4(FILE* f, bvec4* v);

extern inline void fprint_dv2(FILE* f, dvec2 v, const char* append);
extern inline int fread_dv2(FILE* f, dvec2* v);

extern inline void fprint_dv3(FILE* f, dvec3 v, const char* append);
extern inline int fread_dv3(FILE* f, dvec3* v);

extern inline void fprint_dv4(FILE* f, dvec4 v, const char* append);
extern inline int fread_dv4(FILE* f, dvec4* v);

extern inline vec2 v4_to_v2(vec4 a);
extern inline vec3 v4_to_v3(vec4 a);
extern inline vec2 v4_to_v2h(vec4 a);
extern inline vec3 v4_to_v3h(vec4 a);

extern inline void fprint_m2(FILE* f, mat2 m, const char* append);
extern inline void fprint_m3(FILE* f, mat3 m, const char* append);
extern inline void fprint_m4(FILE* f, mat4 m, const char* append);
extern inline void print_m2(mat2 m, const char* append);
extern inline void print_m3(mat3 m, const char* append);
extern inline void print_m4(mat4 m, const char* append);
extern inline vec2 mult_m2_v2(mat2 m, vec2 v);
extern inline vec3 mult_m3_v3(mat3 m, vec3 v);
extern inline vec4 mult_m4_v4(mat4 m, vec4 v);
extern inline void scale_m3(mat3 m, float x, float y, float z);
extern inline void scale_m4(mat4 m, float x, float y, float z);
extern inline void translation_m4(mat4 m, float x, float y, float z);
extern inline void extract_rotation_m4(mat3 dst, mat4 src, int normalize);

extern inline vec2 x_m2(mat2 m);
extern inline vec2 y_m2(mat2 m);
extern inline vec2 c1_m2(mat2 m);
extern inline vec2 c2_m2(mat2 m);

extern inline void setc1_m2(mat2 m, vec2 v);
extern inline void setc2_m2(mat2 m, vec2 v);
extern inline void setx_m2(mat2 m, vec2 v);
extern inline void sety_m2(mat2 m, vec2 v);

extern inline vec3 x_m3(mat3 m);
extern inline vec3 y_m3(mat3 m);
extern inline vec3 z_m3(mat3 m);
extern inline vec3 c1_m3(mat3 m);
extern inline vec3 c2_m3(mat3 m);
extern inline vec3 c3_m3(mat3 m);

extern inline void setc1_m3(mat3 m, vec3 v);
extern inline void setc2_m3(mat3 m, vec3 v);
extern inline void setc3_m3(mat3 m, vec3 v);

extern inline void setx_m3(mat3 m, vec3 v);
extern inline void sety_m3(mat3 m, vec3 v);
extern inline void setz_m3(mat3 m, vec3 v);

extern inline vec4 c1_m4(mat4 m);
extern inline vec4 c2_m4(mat4 m);
extern inline vec4 c3_m4(mat4 m);
extern inline vec4 c4_m4(mat4 m);

extern inline vec4 x_m4(mat4 m);
extern inline vec4 y_m4(mat4 m);
extern inline vec4 z_m4(mat4 m);
extern inline vec4 w_m4(mat4 m);

extern inline void setc1_m4v3(mat4 m, vec3 v);
extern inline void setc2_m4v3(mat4 m, vec3 v);
extern inline void setc3_m4v3(mat4 m, vec3 v);
extern inline void setc4_m4v3(mat4 m, vec3 v);

extern inline void setc1_m4v4(mat4 m, vec4 v);
extern inline void setc2_m4v4(mat4 m, vec4 v);
extern inline void setc3_m4v4(mat4 m, vec4 v);
extern inline void setc4_m4v4(mat4 m, vec4 v);

extern inline void setx_m4v3(mat4 m, vec3 v);
extern inline void sety_m4v3(mat4 m, vec3 v);
extern inline void setz_m4v3(mat4 m, vec3 v);
extern inline void setw_m4v3(mat4 m, vec3 v);

extern inline void setx_m4v4(mat4 m, vec4 v);
extern inline void sety_m4v4(mat4 m, vec4 v);
extern inline void setz_m4v4(mat4 m, vec4 v);
extern inline void setw_m4v4(mat4 m, vec4 v);


void mult_m2_m2(mat2 c, mat2 a, mat2 b)
{
#ifndef ROW_MAJOR
	c[0] = a[0]*b[0] + a[2]*b[1];
	c[2] = a[0]*b[2] + a[2]*b[3];

	c[1] = a[1]*b[0] + a[3]*b[1];
	c[3] = a[1]*b[2] + a[3]*b[3];
#else
	c[0] = a[0]*b[0] + a[1]*b[2];
	c[1] = a[0]*b[1] + a[1]*b[3];

	c[2] = a[2]*b[0] + a[3]*b[2];
	c[3] = a[2]*b[1] + a[3]*b[3];
#endif
}

extern inline void load_rotation_m2(mat2 mat, float angle);

void mult_m3_m3(mat3 c, mat3 a, mat3 b)
{
#ifndef ROW_MAJOR
	c[0] = a[0]*b[0] + a[3]*b[1] + a[6]*b[2];
	c[3] = a[0]*b[3] + a[3]*b[4] + a[6]*b[5];
	c[6] = a[0]*b[6] + a[3]*b[7] + a[6]*b[8];

	c[1] = a[1]*b[0] + a[4]*b[1] + a[7]*b[2];
	c[4] = a[1]*b[3] + a[4]*b[4] + a[7]*b[5];
	c[7] = a[1]*b[6] + a[4]*b[7] + a[7]*b[8];

	c[2] = a[2]*b[0] + a[5]*b[1] + a[8]*b[2];
	c[5] = a[2]*b[3] + a[5]*b[4] + a[8]*b[5];
	c[8] = a[2]*b[6] + a[5]*b[7] + a[8]*b[8];
#else
	c[0] = a[0]*b[0] + a[1]*b[3] + a[2]*b[6];
	c[1] = a[0]*b[1] + a[1]*b[4] + a[2]*b[7];
	c[2] = a[0]*b[2] + a[1]*b[5] + a[2]*b[8];

	c[3] = a[3]*b[0] + a[4]*b[3] + a[5]*b[6];
	c[4] = a[3]*b[1] + a[4]*b[4] + a[5]*b[7];
	c[5] = a[3]*b[2] + a[4]*b[5] + a[5]*b[8];

	c[6] = a[6]*b[0] + a[7]*b[3] + a[8]*b[6];
	c[7] = a[6]*b[1] + a[7]*b[4] + a[8]*b[7];
	c[8] = a[6]*b[2] + a[7]*b[5] + a[8]*b[8];
#endif
}

void load_rotation_m3(mat3 mat, vec3 v, float angle)
{
	float s, c;
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	s = sin(angle);
	c = cos(angle);

	// Rotation matrix is normalized
	normalize_v3(&v);

	xx = v.x * v.x;
	yy = v.y * v.y;
	zz = v.z * v.z;
	xy = v.x * v.y;
	yz = v.y * v.z;
	zx = v.z * v.x;
	xs = v.x * s;
	ys = v.y * s;
	zs = v.z * s;
	one_c = 1.0f - c;

#ifndef ROW_MAJOR
	mat[0] = (one_c * xx) + c;
	mat[3] = (one_c * xy) - zs;
	mat[6] = (one_c * zx) + ys;

	mat[1] = (one_c * xy) + zs;
	mat[4] = (one_c * yy) + c;
	mat[7] = (one_c * yz) - xs;

	mat[2] = (one_c * zx) - ys;
	mat[5] = (one_c * yz) + xs;
	mat[8] = (one_c * zz) + c;
#else
	mat[0] = (one_c * xx) + c;
	mat[1] = (one_c * xy) - zs;
	mat[2] = (one_c * zx) + ys;

	mat[3] = (one_c * xy) + zs;
	mat[4] = (one_c * yy) + c;
	mat[5] = (one_c * yz) - xs;

	mat[6] = (one_c * zx) - ys;
	mat[7] = (one_c * yz) + xs;
	mat[8] = (one_c * zz) + c;
#endif
}



/*
 * mat4
 */

//TODO use restrict?
void mult_m4_m4(mat4 c, mat4 a, mat4 b)
{
#ifndef ROW_MAJOR
	c[ 0] = a[0]*b[ 0] + a[4]*b[ 1] + a[8]*b[ 2] + a[12]*b[ 3];
	c[ 4] = a[0]*b[ 4] + a[4]*b[ 5] + a[8]*b[ 6] + a[12]*b[ 7];
	c[ 8] = a[0]*b[ 8] + a[4]*b[ 9] + a[8]*b[10] + a[12]*b[11];
	c[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14] + a[12]*b[15];

	c[ 1] = a[1]*b[ 0] + a[5]*b[ 1] + a[9]*b[ 2] + a[13]*b[ 3];
	c[ 5] = a[1]*b[ 4] + a[5]*b[ 5] + a[9]*b[ 6] + a[13]*b[ 7];
	c[ 9] = a[1]*b[ 8] + a[5]*b[ 9] + a[9]*b[10] + a[13]*b[11];
	c[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14] + a[13]*b[15];

	c[ 2] = a[2]*b[ 0] + a[6]*b[ 1] + a[10]*b[ 2] + a[14]*b[ 3];
	c[ 6] = a[2]*b[ 4] + a[6]*b[ 5] + a[10]*b[ 6] + a[14]*b[ 7];
	c[10] = a[2]*b[ 8] + a[6]*b[ 9] + a[10]*b[10] + a[14]*b[11];
	c[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];

	c[ 3] = a[3]*b[ 0] + a[7]*b[ 1] + a[11]*b[ 2] + a[15]*b[ 3];
	c[ 7] = a[3]*b[ 4] + a[7]*b[ 5] + a[11]*b[ 6] + a[15]*b[ 7];
	c[11] = a[3]*b[ 8] + a[7]*b[ 9] + a[11]*b[10] + a[15]*b[11];
	c[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];

#else
	c[0] = a[0]*b[0] + a[1]*b[4] + a[2]*b[8] + a[3]*b[12];
	c[1] = a[0]*b[1] + a[1]*b[5] + a[2]*b[9] + a[3]*b[13];
	c[2] = a[0]*b[2] + a[1]*b[6] + a[2]*b[10] + a[3]*b[14];
	c[3] = a[0]*b[3] + a[1]*b[7] + a[2]*b[11] + a[3]*b[15];

	c[4] = a[4]*b[0] + a[5]*b[4] + a[6]*b[8] + a[7]*b[12];
	c[5] = a[4]*b[1] + a[5]*b[5] + a[6]*b[9] + a[7]*b[13];
	c[6] = a[4]*b[2] + a[5]*b[6] + a[6]*b[10] + a[7]*b[14];
	c[7] = a[4]*b[3] + a[5]*b[7] + a[6]*b[11] + a[7]*b[15];

	c[ 8] = a[8]*b[0] + a[9]*b[4] + a[10]*b[8] + a[11]*b[12];
	c[ 9] = a[8]*b[1] + a[9]*b[5] + a[10]*b[9] + a[11]*b[13];
	c[10] = a[8]*b[2] + a[9]*b[6] + a[10]*b[10] + a[11]*b[14];
	c[11] = a[8]*b[3] + a[9]*b[7] + a[10]*b[11] + a[11]*b[15];

	c[12] = a[12]*b[0] + a[13]*b[4] + a[14]*b[8] + a[15]*b[12];
	c[13] = a[12]*b[1] + a[13]*b[5] + a[14]*b[9] + a[15]*b[13];
	c[14] = a[12]*b[2] + a[13]*b[6] + a[14]*b[10] + a[15]*b[14];
	c[15] = a[12]*b[3] + a[13]*b[7] + a[14]*b[11] + a[15]*b[15];
#endif
}

void load_rotation_m4(mat4 mat, vec3 v, float angle)
{
	float s, c;
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	s = sin(angle);
	c = cos(angle);

	// Rotation matrix is normalized
	normalize_v3(&v);

	xx = v.x * v.x;
	yy = v.y * v.y;
	zz = v.z * v.z;
	xy = v.x * v.y;
	yz = v.y * v.z;
	zx = v.z * v.x;
	xs = v.x * s;
	ys = v.y * s;
	zs = v.z * s;
	one_c = 1.0f - c;

#ifndef ROW_MAJOR
	mat[ 0] = (one_c * xx) + c;
	mat[ 4] = (one_c * xy) - zs;
	mat[ 8] = (one_c * zx) + ys;
	mat[12] = 0.0f;

	mat[ 1] = (one_c * xy) + zs;
	mat[ 5] = (one_c * yy) + c;
	mat[ 9] = (one_c * yz) - xs;
	mat[13] = 0.0f;

	mat[ 2] = (one_c * zx) - ys;
	mat[ 6] = (one_c * yz) + xs;
	mat[10] = (one_c * zz) + c;
	mat[14] = 0.0f;

	mat[ 3] = 0.0f;
	mat[ 7] = 0.0f;
	mat[11] = 0.0f;
	mat[15] = 1.0f;
#else
	mat[0] = (one_c * xx) + c;
	mat[1] = (one_c * xy) - zs;
	mat[2] = (one_c * zx) + ys;
	mat[3] = 0.0f;

	mat[4] = (one_c * xy) + zs;
	mat[5] = (one_c * yy) + c;
	mat[6] = (one_c * yz) - xs;
	mat[7] = 0.0f;

	mat[8] = (one_c * zx) - ys;
	mat[9] = (one_c * yz) + xs;
	mat[10] = (one_c * zz) + c;
	mat[11] = 0.0f;

	mat[12] = 0.0f;
	mat[13] = 0.0f;
	mat[14] = 0.0f;
	mat[15] = 1.0f;
#endif
}


/* TODO
static float det_ij(const mat4 m, const int i, const int j)
{
	float ret, mat[3][3];
	int x = 0, y = 0;

	for (int ii=0; ii<4; ++ii) {
		y = 0;
		if (ii == i) continue;
		for (int jj=0; jj<4; ++jj) {
			if (jj == j) continue;
			mat[x][y] = m[ii*4+jj];
			y++;
		}
		x++;
	}



	ret =  mat[0][0]*(mat[1][1]*mat[2][2]-mat[2][1]*mat[1][2]);
	ret -= mat[0][1]*(mat[1][0]*mat[2][2]-mat[2][0]*mat[1][2]);
	ret += mat[0][2]*(mat[1][0]*mat[2][1]-mat[2][0]*mat[1][1]);

	return ret;
}


void invert_m4(mat4 mInverse, const mat4& m)
{
	int i, j;
	float det, detij;
	mat4 inverse_mat;

	// calculate 4x4 determinant
	det = 0.0f;
	for (i = 0; i < 4; i++) {
		det += (i & 0x1) ? (-m.matrix[i] * det_ij(m, 0, i)) : (m.matrix[i] * det_ij(m, 0, i));
	}
	det = 1.0f / det;

	// calculate inverse
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			detij = det_ij(m, j, i);
			inverse_mat[(i*4)+j] = ((i+j) & 0x1) ? (-detij * det) : (detij *det);
		}
	}

}


*/


////////////////////////////////////////////////////////////////////////////////////////////

//assumes converting from canonical view volume [-1,1]^3
//works just like glViewport, x and y are lower left corner.  opengl should be 1.
void make_viewport_m4(mat4 mat, int x, int y, unsigned int width, unsigned int height, int opengl)
{
	float w, h, l, t, b, r;

	if (opengl) {
		//See glspec page 104, integer grid is lower left pixel corners
		w = width, h = height;
		l = x, b = y;
		//range is [l, l+w) x [b , b+h)
		//TODO pick best epsilon?
		r = l + w - 0.01; //epsilon larger than float precision
		t = b + h - 0.01;

#ifndef ROW_MAJOR
		mat[ 0] = (r - l) / 2;
		mat[ 4] = 0;
		mat[ 8] = 0;
		mat[12] = (l + r) / 2;

		mat[ 1] = 0;
		//see below
		mat[ 5] = (t - b) / 2;
		mat[ 9] = 0;
		mat[13] = (b + t) / 2;

		mat[ 2] = 0;
		mat[ 6] = 0;
		mat[10] = 1;
		mat[14] = 0;

		mat[ 3] = 0;
		mat[ 7] = 0;
		mat[11] = 0;
		mat[15] = 1;
#else
		mat[0] = (r - l) / 2;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = (l + r) / 2;

		mat[4] = 0;
		//this used to be negative to flip y till I changed glFramebuffer and draw_pixel to accomplish the same thing
		mat[5] = (t - b) / 2;
		mat[6] = 0;
		mat[7] = (b + t) / 2;

		mat[8] = 0;
		mat[9] = 0;
		mat[10] = 1;
		mat[11] = 0;

		mat[12] = 0;
		mat[13] = 0;
		mat[14] = 0;
		mat[15] = 1;
#endif

	} else {
		//old way with pixel centers at integer coordinates
		//see pages 133/4 and 144 of FoCG
		//necessary for fast integer only bresenham line drawing

		w = width, h = height;
		l = x - 0.5f;
		b = y - 0.5f;
		r = l + w;
		t = b + h;

#ifndef ROW_MAJOR
		mat[ 0] = (r - l) / 2;
		mat[ 4] = 0;
		mat[ 8] = 0;
		mat[12] = (l + r) / 2;

		mat[ 1] = 0;
		//see below
		mat[ 5] = (t - b) / 2;
		mat[ 9] = 0;
		mat[13] = (b + t) / 2;

		mat[ 2] = 0;
		mat[ 6] = 0;
		mat[10] = 1;
		mat[14] = 0;

		mat[ 3] = 0;
		mat[ 7] = 0;
		mat[11] = 0;
		mat[15] = 1;
#else
		mat[0] = (r - l) / 2;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = (l + r) / 2;

		mat[4] = 0;
		//make this negative to reflect y otherwise positive y maps to lower half of the screen
		//this is mapping the unit square [-1,1]^2 to the window size. x is fine because it increases left to right
		//but the screen coordinates (ie framebuffer memory) increase top to bottom opposite of the canonical square
		//negating this is the easiest way to fix it without any side effects.
		mat[5] = (t - b) / 2;
		mat[6] = 0;
		mat[7] = (b + t) / 2;

		mat[8] = 0;
		mat[9] = 0;
		mat[10] = 1;
		mat[11] = 0;

		mat[12] = 0;
		mat[13] = 0;
		mat[14] = 0;
		mat[15] = 1;
#endif
	}
}

//I can't really think of any reason to ever use this matrix alone.
//You'd always do ortho * pers and really if you're doing perspective projection
//just use make_perspective_matrix (or less likely make perspective_proj_matrix)
//
//This function is really just for completeness sake based off of FoCG 3rd edition pg 152
//changed slightly.  z_near and z_far are always positive and z_near < z_far
//
//Inconsistently, to generate an ortho matrix to multiply with that will get the equivalent
//of the other 2 functions you'd use -z_near and -z_far and near > far.
void make_pers_m4(mat4 mat, float z_near, float z_far)
{
#ifndef ROW_MAJOR
	mat[ 0] = z_near;
	mat[ 4] = 0;
	mat[ 8] = 0;
	mat[12] = 0;

	mat[ 1] = 0;
	mat[ 5] = z_near;
	mat[ 9] = 0;
	mat[13] = 0;

	mat[ 2] = 0;
	mat[ 6] = 0;
	mat[10] = z_near + z_far;
	mat[14] = (z_far * z_near);

	mat[ 3] = 0;
	mat[ 7] = 0;
	mat[11] = -1;
	mat[15] = 0;
#else
	mat[0] = z_near;
	mat[1] = 0;
	mat[2] = 0;
	mat[3] = 0;

	mat[4] = 0;
	mat[5] = z_near;
	mat[6] = 0;
	mat[7] = 0;

	mat[ 8] = 0;
	mat[ 9] = 0;
	mat[10] = z_near + z_far;
	mat[11] = (z_far * z_near);

	mat[12] = 0;
	mat[13] = 0;
	mat[14] = -1;
	mat[15] = 0;
#endif
}

// Create a projection matrix
// Similiar to the old gluPerspective... fov is in radians btw...
void make_perspective_m4(mat4 mat, float fov, float aspect, float n, float f)
{
	float t = n * tanf(fov * 0.5f);
	float b = -t;
	float l = b * aspect;
	float r = -l;

	make_perspective_proj_m4(mat, l, r, b, t, n, f);

}

void make_perspective_proj_m4(mat4 mat, float l, float r, float b, float t, float n, float f)
{
#ifndef ROW_MAJOR
	mat[ 0] = (2.0f * n) / (r - l);
	mat[ 4] = 0.0f;
	mat[ 8] = (r + l) / (r - l);
	mat[12] = 0.0f;

	mat[ 1] = 0.0f;
	mat[ 5] = (2.0f * n) / (t - b);
	mat[ 9] = (t + b) / (t - b);
	mat[13] = 0.0f;

	mat[ 2] = 0.0f;
	mat[ 6] = 0.0f;
	mat[10] = -((f + n) / (f - n));
	mat[14] = -((2.0f * (f*n))/(f - n));

	mat[ 3] = 0.0f;
	mat[ 7] = 0.0f;
	mat[11] = -1.0f;
	mat[15] = 0.0f;
#else
	mat[0] = (2.0f * n) / (r - l);
	mat[1] = 0.0f;
	mat[2] = (r + l) / (r - l);
	mat[3] = 0.0f;

	mat[4] = 0.0f;
	mat[5] = (2.0f * n) / (t - b);
	mat[6] = (t + b) / (t - b);
	mat[7] = 0.0f;

	mat[8] = 0.0f;
	mat[9] = 0.0f;
	mat[10] = -((f + n) / (f - n));
	mat[11] = -((2.0f * (f*n))/(f - n));

	mat[12] = 0.0f;
	mat[13] = 0.0f;
	mat[14] = -1.0f;
	mat[15] = 0.0f;
#endif
}

//n and f really are near and far not min and max so if you want the standard looking down the -z axis
// then n > f otherwise n < f
void make_orthographic_m4(mat4 mat, float l, float r, float b, float t, float n, float f)
{
#ifndef ROW_MAJOR
	mat[ 0] = 2.0f / (r - l);
	mat[ 4] = 0;
	mat[ 8] = 0;
	mat[12] = -((r + l)/(r - l));

	mat[ 1] = 0;
	mat[ 5] = 2.0f / (t - b);
	mat[ 9] = 0;
	mat[13] = -((t + b)/(t - b));

	mat[ 2] = 0;
	mat[ 6] = 0;
	mat[10] = 2.0f / (f - n);  //removed - in front of 2 . . . book doesn't have it but superbible did
	mat[14] = -((n + f)/(f - n));

	mat[ 3] = 0;
	mat[ 7] = 0;
	mat[11] = 0;
	mat[15] = 1;
#else
	mat[0] = 2.0f / (r - l);
	mat[1] = 0;
	mat[2] = 0;
	mat[3] = -((r + l)/(r - l));
	mat[4] = 0;
	mat[5] = 2.0f / (t - b);
	mat[6] = 0;
	mat[7] = -((t + b)/(t - b));
	mat[8] = 0;
	mat[9] = 0;
	mat[10] = 2.0f / (f - n);  //removed - in front of 2 . . . book doesn't have it but superbible did
	mat[11] = -((n + f)/(f - n));
	mat[12] = 0;
	mat[13] = 0;
	mat[14] = 0;
	mat[15] = 1;
#endif


	//now I know why the superbible had the -
	//OpenGL uses a left handed canonical view volume [-1,1]^3 when passed the identity matrix
	//ie in Normalized Device Coordinates.  The math/matrix presented in Fundamentals of Computer
	//Graphics assumes a right handed version of the same volume.  The negative isn't necessary
	//if you set n and f correctly as near and far not low and high
}

//per https://www.opengl.org/sdk/docs/man2/xhtml/gluLookAt.xml
//and glm.g-truc.net (glm/gtc/matrix_transform.inl)
void lookAt(mat4 mat, vec3 eye, vec3 center, vec3 up)
{
	SET_IDENTITY_M4(mat);

	vec3 f = norm_v3(sub_v3s(center, eye));
	vec3 s = norm_v3(cross_v3s(f, up));
	vec3 u = cross_v3s(s, f);

	setx_m4v3(mat, s);
	sety_m4v3(mat, u);
	setz_m4v3(mat, neg_v3(f));
	setc4_m4v3(mat, make_v3(-dot_v3s(s, eye), -dot_v3s(u, eye), dot_v3s(f, eye)));
}

extern inline float rsw_randf(void);
extern inline float rsw_randf_range(float min, float max);
extern inline double rsw_map(double x, double a, double b, double c, double d);
extern inline float rsw_mapf(float x, float a, float b, float c, float d);

extern inline Color make_Color(u8 red, u8 green, u8 blue, u8 alpha);
extern inline Color v4_to_Color(vec4 v);
extern inline void print_Color(Color c, const char* append);
extern inline vec4 Color_to_v4(Color c);
extern inline Line make_Line(float x1, float y1, float x2, float y2);
extern inline void normalize_line(Line* line);
extern inline float line_func(Line* line, float x, float y);
extern inline float line_findy(Line* line, float x);
extern inline float line_findx(Line* line, float y);
extern inline float sq_dist_pt_segment2d(vec2 a, vec2 b, vec2 c);
extern inline void closest_pt_pt_segment(vec2 c, vec2 a, vec2 b, float* t, vec2* d);
extern inline float closest_pt_pt_segment_t(vec2 c, vec2 a, vec2 b);

