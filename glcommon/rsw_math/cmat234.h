
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
inline vec2 x_m2(mat2 m) {  return make_v2(m[0], m[2]); }
inline vec2 y_m2(mat2 m) {  return make_v2(m[1], m[3]); }
inline vec2 c1_m2(mat2 m) { return make_v2(m[0], m[1]); }
inline vec2 c2_m2(mat2 m) { return make_v2(m[2], m[3]); }

inline void setc1_m2(mat2 m, vec2 v) { m[0]=v.x, m[1]=v.y; }
inline void setc2_m2(mat2 m, vec2 v) { m[2]=v.x, m[3]=v.y; }

inline void setx_m2(mat2 m, vec2 v) { m[0]=v.x, m[2]=v.y; }
inline void sety_m2(mat2 m, vec2 v) { m[1]=v.x, m[3]=v.y; }
#else
inline vec2 x_m2(mat2 m) {  return make_v2(m[0], m[1]); }
inline vec2 y_m2(mat2 m) {  return make_v2(m[2], m[3]); }
inline vec2 c1_m2(mat2 m) { return make_v2(m[0], m[2]); }
inline vec2 c2_m2(mat2 m) { return make_v2(m[1], m[3]); }

inline void setc1_m2(mat2 m, vec2 v) { m[0]=v.x, m[2]=v.y; }
inline void setc2_m2(mat2 m, vec2 v) { m[1]=v.x, m[3]=v.y; }

inline void setx_m2(mat2 m, vec2 v) { m[0]=v.x, m[1]=v.y; }
inline void sety_m2(mat2 m, vec2 v) { m[2]=v.x, m[3]=v.y; }
#endif


#ifndef ROW_MAJOR
inline vec3 x_m3(mat3 m) {  return make_v3(m[0], m[3], m[6]); }
inline vec3 y_m3(mat3 m) {  return make_v3(m[1], m[4], m[7]); }
inline vec3 z_m3(mat3 m) {  return make_v3(m[2], m[5], m[8]); }
inline vec3 c1_m3(mat3 m) { return make_v3(m[0], m[1], m[2]); }
inline vec3 c2_m3(mat3 m) { return make_v3(m[3], m[4], m[5]); }
inline vec3 c3_m3(mat3 m) { return make_v3(m[6], m[7], m[8]); }

inline void setc1_m3(mat3 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z; }
inline void setc2_m3(mat3 m, vec3 v) { m[3]=v.x, m[4]=v.y, m[5]=v.z; }
inline void setc3_m3(mat3 m, vec3 v) { m[6]=v.x, m[7]=v.y, m[8]=v.z; }

inline void setx_m3(mat3 m, vec3 v) { m[0]=v.x, m[3]=v.y, m[6]=v.z; }
inline void sety_m3(mat3 m, vec3 v) { m[1]=v.x, m[4]=v.y, m[7]=v.z; }
inline void setz_m3(mat3 m, vec3 v) { m[2]=v.x, m[5]=v.y, m[8]=v.z; }
#else
inline vec3 x_m3(mat3 m) {  return make_v3(m[0], m[1], m[2]); }
inline vec3 y_m3(mat3 m) {  return make_v3(m[3], m[4], m[5]); }
inline vec3 z_m3(mat3 m) {  return make_v3(m[6], m[7], m[8]); }
inline vec3 c1_m3(mat3 m) { return make_v3(m[0], m[3], m[6]); }
inline vec3 c2_m3(mat3 m) { return make_v3(m[1], m[4], m[7]); }
inline vec3 c3_m3(mat3 m) { return make_v3(m[2], m[5], m[8]); }

inline void setc1_m3(mat3 m, vec3 v) { m[0]=v.x, m[3]=v.y, m[6]=v.z; }
inline void setc2_m3(mat3 m, vec3 v) { m[1]=v.x, m[4]=v.y, m[7]=v.z; }
inline void setc3_m3(mat3 m, vec3 v) { m[2]=v.x, m[5]=v.y, m[8]=v.z; }

inline void setx_m3(mat3 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z; }
inline void sety_m3(mat3 m, vec3 v) { m[3]=v.x, m[4]=v.y, m[5]=v.z; }
inline void setz_m3(mat3 m, vec3 v) { m[6]=v.x, m[7]=v.y, m[8]=v.z; }
#endif


#ifndef ROW_MAJOR
inline vec4 c1_m4(mat4 m) { return make_v4(m[ 0], m[ 1], m[ 2], m[ 3]); }
inline vec4 c2_m4(mat4 m) { return make_v4(m[ 4], m[ 5], m[ 6], m[ 7]); }
inline vec4 c3_m4(mat4 m) { return make_v4(m[ 8], m[ 9], m[10], m[11]); }
inline vec4 c4_m4(mat4 m) { return make_v4(m[12], m[13], m[14], m[15]); }

inline vec4 x_m4(mat4 m) { return make_v4(m[0], m[4], m[8], m[12]); }
inline vec4 y_m4(mat4 m) { return make_v4(m[1], m[5], m[9], m[13]); }
inline vec4 z_m4(mat4 m) { return make_v4(m[2], m[6], m[10], m[14]); }
inline vec4 w_m4(mat4 m) { return make_v4(m[3], m[7], m[11], m[15]); }

//sets 4th row to 0 0 0 1
inline void setc1_m4v3(mat4 m, vec3 v) { m[ 0]=v.x, m[ 1]=v.y, m[ 2]=v.z, m[ 3]=0; }
inline void setc2_m4v3(mat4 m, vec3 v) { m[ 4]=v.x, m[ 5]=v.y, m[ 6]=v.z, m[ 7]=0; }
inline void setc3_m4v3(mat4 m, vec3 v) { m[ 8]=v.x, m[ 9]=v.y, m[10]=v.z, m[11]=0; }
inline void setc4_m4v3(mat4 m, vec3 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=1; }

inline void setc1_m4v4(mat4 m, vec4 v) { m[ 0]=v.x, m[ 1]=v.y, m[ 2]=v.z, m[ 3]=v.w; }
inline void setc2_m4v4(mat4 m, vec4 v) { m[ 4]=v.x, m[ 5]=v.y, m[ 6]=v.z, m[ 7]=v.w; }
inline void setc3_m4v4(mat4 m, vec4 v) { m[ 8]=v.x, m[ 9]=v.y, m[10]=v.z, m[11]=v.w; }
inline void setc4_m4v4(mat4 m, vec4 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=v.w; }

//sets 4th column to 0 0 0 1
inline void setx_m4v3(mat4 m, vec3 v) { m[0]=v.x, m[4]=v.y, m[ 8]=v.z, m[12]=0; }
inline void sety_m4v3(mat4 m, vec3 v) { m[1]=v.x, m[5]=v.y, m[ 9]=v.z, m[13]=0; }
inline void setz_m4v3(mat4 m, vec3 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=0; }
inline void setw_m4v3(mat4 m, vec3 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=1; }

inline void setx_m4v4(mat4 m, vec4 v) { m[0]=v.x, m[4]=v.y, m[ 8]=v.z, m[12]=v.w; }
inline void sety_m4v4(mat4 m, vec4 v) { m[1]=v.x, m[5]=v.y, m[ 9]=v.z, m[13]=v.w; }
inline void setz_m4v4(mat4 m, vec4 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=v.w; }
inline void setw_m4v4(mat4 m, vec4 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=v.w; }
#else
inline vec4 c1_m4(mat4 m) { return make_v4(m[0], m[4], m[8], m[12]); }
inline vec4 c2_m4(mat4 m) { return make_v4(m[1], m[5], m[9], m[13]); }
inline vec4 c3_m4(mat4 m) { return make_v4(m[2], m[6], m[10], m[14]); }
inline vec4 c4_m4(mat4 m) { return make_v4(m[3], m[7], m[11], m[15]); }

inline vec4 x_m4(mat4 m) { return make_v4(m[0], m[1], m[2], m[3]); }
inline vec4 y_m4(mat4 m) { return make_v4(m[4], m[5], m[6], m[7]); }
inline vec4 z_m4(mat4 m) { return make_v4(m[8], m[9], m[10], m[11]); }
inline vec4 w_m4(mat4 m) { return make_v4(m[12], m[13], m[14], m[15]); }

//sets 4th row to 0 0 0 1
inline void setc1_m4v3(mat4 m, vec3 v) { m[0]=v.x, m[4]=v.y, m[8]=v.z, m[12]=0; }
inline void setc2_m4v3(mat4 m, vec3 v) { m[1]=v.x, m[5]=v.y, m[9]=v.z, m[13]=0; }
inline void setc3_m4v3(mat4 m, vec3 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=0; }
inline void setc4_m4v3(mat4 m, vec3 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=1; }

inline void setc1_m4v4(mat4 m, vec4 v) { m[0]=v.x, m[4]=v.y, m[8]=v.z, m[12]=v.w; }
inline void setc2_m4v4(mat4 m, vec4 v) { m[1]=v.x, m[5]=v.y, m[9]=v.z, m[13]=v.w; }
inline void setc3_m4v4(mat4 m, vec4 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=v.w; }
inline void setc4_m4v4(mat4 m, vec4 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=v.w; }

//sets 4th column to 0 0 0 1
inline void setx_m4v3(mat4 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z, m[3]=0; }
inline void sety_m4v3(mat4 m, vec3 v) { m[4]=v.x, m[5]=v.y, m[6]=v.z, m[7]=0; }
inline void setz_m4v3(mat4 m, vec3 v) { m[8]=v.x, m[9]=v.y, m[10]=v.z, m[11]=0; }
inline void setw_m4v3(mat4 m, vec3 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=1; }

inline void setx_m4v4(mat4 m, vec4 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z, m[3]=v.w; }
inline void sety_m4v4(mat4 m, vec4 v) { m[4]=v.x, m[5]=v.y, m[6]=v.z, m[7]=v.w; }
inline void setz_m4v4(mat4 m, vec4 v) { m[8]=v.x, m[9]=v.y, m[10]=v.z, m[11]=v.w; }
inline void setw_m4v4(mat4 m, vec4 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=v.w; }
#endif


inline void fprint_m2(FILE* f, mat2 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f)\n (%f, %f)]%s",
	        m[0], m[2], m[1], m[3], append);
#else
	fprintf(f, "[(%f, %f)\n (%f, %f)]%s",
	        m[0], m[1], m[2], m[3], append);
#endif
}


inline void fprint_m3(FILE* f, mat3 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f, %f)\n (%f, %f, %f)\n (%f, %f, %f)]%s",
	        m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8], append);
#else
	fprintf(f, "[(%f, %f, %f)\n (%f, %f, %f)\n (%f, %f, %f)]%s",
	        m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], append);
#endif
}

inline void fprint_m4(FILE* f, mat4 m, const char* append)
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
inline void print_m2(mat2 m, const char* append)
{
	fprint_m2(stdout, m, append);
}

inline void print_m3(mat3 m, const char* append)
{
	fprint_m3(stdout, m, append);
}

inline void print_m4(mat4 m, const char* append)
{
	fprint_m4(stdout, m, append);
}

//TODO define macros for doing array version
inline vec2 mult_m2_v2(mat2 m, vec2 v)
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


inline vec3 mult_m3_v3(mat3 m, vec3 v)
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

inline vec4 mult_m4_v4(mat4 m, vec4 v)
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

inline void load_rotation_m2(mat2 mat, float angle)
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
inline void scale_m3(mat3 m, float x, float y, float z)
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

inline void scale_m4(mat4 m, float x, float y, float z)
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
inline void translation_m4(mat4 m, float x, float y, float z)
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
inline void extract_rotation_m4(mat3 dst, mat4 src, int normalize)
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

