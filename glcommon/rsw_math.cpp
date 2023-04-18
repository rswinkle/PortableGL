#include "rsw_math.h"

namespace rsw
{

mat2 operator*(const mat2& a, const mat2& b)
{
	mat2 tmp;
	//could use setx/y/z functions and no ifdef?
#ifndef ROW_MAJOR
	tmp[0] = dot(a.x(), b.c1());
	tmp[2] = dot(a.x(), b.c2());
	tmp[1] = dot(a.y(), b.c1());
	tmp[3] = dot(a.y(), b.c2());
#else
	tmp[0] = dot(a.x(), b.c1());
	tmp[1] = dot(a.x(), b.c2());
	tmp[2] = dot(a.y(), b.c1());
	tmp[3] = dot(a.y(), b.c2());
#endif
	return tmp;
}


mat3 operator*(const mat3& a, const mat3& b)
{
	mat3 tmp;
	//could use setx/y/z functions and no ifdef?
#ifndef ROW_MAJOR
	tmp[0] = dot(a.x(), b.c1());
	tmp[3] = dot(a.x(), b.c2());
	tmp[6] = dot(a.x(), b.c3());
	tmp[1] = dot(a.y(), b.c1());
	tmp[4] = dot(a.y(), b.c2());
	tmp[7] = dot(a.y(), b.c3());
	tmp[2] = dot(a.z(), b.c1());
	tmp[5] = dot(a.z(), b.c2());
	tmp[8] = dot(a.z(), b.c3());
#else
	tmp[0] = dot(a.x(), b.c1());
	tmp[1] = dot(a.x(), b.c2());
	tmp[2] = dot(a.x(), b.c3());
	tmp[3] = dot(a.y(), b.c1());
	tmp[4] = dot(a.y(), b.c2());
	tmp[5] = dot(a.y(), b.c3());
	tmp[6] = dot(a.z(), b.c1());
	tmp[7] = dot(a.z(), b.c2());
	tmp[8] = dot(a.z(), b.c3());
#endif
	return tmp;
}


void load_rotation_mat3(mat3& mat, vec3 v, float angle)
{
	float s, c;
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	s = float(std::sin(angle));
	c = float(std::cos(angle));

	// Rotation matrix is normalized
	v.normalize();

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

mat4 operator*(const mat4& a, const mat4& b)
{
	mat4 tmp;
#ifndef ROW_MAJOR
	tmp[ 0] = dot(a.x(), b.c1());
	tmp[ 4] = dot(a.x(), b.c2());
	tmp[ 8] = dot(a.x(), b.c3());
	tmp[12] = dot(a.x(), b.c4());

	tmp[ 1] = dot(a.y(), b.c1());
	tmp[ 5] = dot(a.y(), b.c2());
	tmp[ 9] = dot(a.y(), b.c3());
	tmp[13] = dot(a.y(), b.c4());

	tmp[ 2] = dot(a.z(), b.c1());
	tmp[ 6] = dot(a.z(), b.c2());
	tmp[10] = dot(a.z(), b.c3());
	tmp[14] = dot(a.z(), b.c4());

	tmp[ 3] = dot(a.w(), b.c1());
	tmp[ 7] = dot(a.w(), b.c2());
	tmp[11] = dot(a.w(), b.c3());
	tmp[15] = dot(a.w(), b.c4());
#else
	tmp[0] = dot(a.x(), b.c1());
	tmp[1] = dot(a.x(), b.c2());
	tmp[2] = dot(a.x(), b.c3());
	tmp[3] = dot(a.x(), b.c4());

	tmp[4] = dot(a.y(), b.c1());
	tmp[5] = dot(a.y(), b.c2());
	tmp[6] = dot(a.y(), b.c3());
	tmp[7] = dot(a.y(), b.c4());

	tmp[ 8] = dot(a.z(), b.c1());
	tmp[ 9] = dot(a.z(), b.c2());
	tmp[10] = dot(a.z(), b.c3());
	tmp[11] = dot(a.z(), b.c4());

	tmp[12] = dot(a.w(), b.c1());
	tmp[13] = dot(a.w(), b.c2());
	tmp[14] = dot(a.w(), b.c3());
	tmp[15] = dot(a.w(), b.c4());
#endif

	//bottom row is already appropriately set to 0 0 0 1 from constructor
	//but maybe the last multiplications are necessary in some cases

	return tmp;
}

void load_rotation_mat4(mat4& mat, vec3 v, float angle)
{
	float s, c;
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	s = float(std::sin(angle));
	c = float(std::cos(angle));

//	// Rotation matrix is normalized
	v.normalize();

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



//TODO rewrite/re-grok these functions
//det(M) = det(transpose(M)) thus no ifdef
static float det_ij(const mat4& m, const int i, const int j)
{
	float ret, mat[3][3];
	int x = 0, y = 0;

	for (int ii=0; ii<4; ii++) {
		y = 0;
		if (ii == i) continue;
		for (int jj=0; jj<4; jj++) {
			if (jj == j) continue;
			mat[x][y] = m.matrix[ii*4+jj];
			y++;
		}
		x++;
	}

	ret =  mat[0][0]*(mat[1][1]*mat[2][2]-mat[2][1]*mat[1][2]);
	ret -= mat[0][1]*(mat[1][0]*mat[2][2]-mat[2][0]*mat[1][2]);
	ret += mat[0][2]*(mat[1][0]*mat[2][1]-mat[2][0]*mat[1][1]);

	return ret;
}

mat4 invert_mat4(const mat4& mat)
{
	int i, j;
	float det, detij;
	mat4 inverse_mat;

	// calculate 4x4 determinant
	det = 0.0f;
	for (i = 0; i < 4; i++) {
		det += (i & 0x1) ? (-mat.matrix[i] * det_ij(mat, 0, i)) : (mat.matrix[i] * det_ij(mat, 0, i));
	}
	det = 1.0f / det;

	// calculate inverse
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			detij = det_ij(mat, j, i);
			inverse_mat[(i*4)+j] = ((i+j) & 0x1) ? (-detij * det) : (detij * det);
		}
	}

	return inverse_mat;
}






////////////////////////////////////////////////////////////////////////////////////////////

//assumes converting from canonical view volume [-1,1]^3
//works just like glViewport, x and y are lower left corner.  opengl should be 1.
void make_viewport_matrix(mat4& mat, int x, int y, unsigned int width, unsigned int height, int opengl)
{
	float w, h, l, t, b, r;

	if (opengl) {
		//See glspec page 104, integer grid is lower left pixel corners
		w = width, h = height;
		l = x, b = y;
		//range is [0, w) x [0 , h)
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
void make_pers_matrix(mat4& mat, float z_near, float z_far)
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
void make_perspective_matrix(mat4 &mat, float fov, float aspect, float n, float f)
{
	float t = n * tanf(fov * 0.5f);
	float b = -t;
	float l = b * aspect;
	float r = -l;

	make_perspective_proj_matrix(mat, l, r, b, t, n, f);
}

void make_perspective_proj_matrix(mat4 &mat, float l, float r, float b, float t, float n, float f)
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
void make_orthographic_matrix(mat4 &mat, float l, float r, float b, float t, float n, float f)
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
	//OpenGL uses a left handed canonical view volume [-1,1]^3
	//ie in Normalized Device Coordinates.  The math/matrix presented in Fundamentals of Computer
	//Graphics assumes a right handed version of the same volume.  The negative isn't necessary
	//if you set n and f correctly as near and far not low and high
}



//per https://www.opengl.org/sdk/docs/man2/xhtml/gluLookAt.xml
//and glm.g-truc.net (glm/gtc/matrix_transform.inl)
void lookAt(mat4 &mat, vec3 eye, vec3 center, vec3 up)
{
	mat = mat4();
	vec3 f(normalize(center-eye));
	vec3 s(normalize(cross(f, up)));
	vec3 u(cross(s, f));

	mat.setx(s);
	mat.sety(u);
	mat.setz(-f);
	mat.setc4(vec3(-dot(s, eye), -dot(u, eye), dot(f, eye)));
}




int intersect_segment_plane(vec3 a, vec3 b, Plane p, float& t, vec3& q)
{
	vec3 ab = b - a;
	t = (p.d - dot(p.n, a)) / dot(p.n, ab);

	//if t in [0-1] compute point
	if (t >= 0.0f && t <= 1.0f) {
		q = a + t*ab;
		return 1;
	}

	//else no intersection
	return 0;
}









}	//close robert3dmath namespace
