#pragma once
#ifndef RSW_MATH_H
#define RSW_MATH_H

#include <iostream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <stdint.h>


#define RM_PI (3.14159265358979323846)
#define RM_2PI (2.0 * RM_PI)
#define PI_DIV_180 (0.017453292519943296)
#define INV_PI_DIV_180 (57.2957795130823229)

#define DEG_TO_RAD(x)   ((x)*PI_DIV_180)
#define RAD_TO_DEG(x)   ((x)*INV_PI_DIV_180)

// Hour angles
#define HR_TO_DEG(x)    ((x) * (1.0 / 15.0))
#define HR_TO_RAD(x)    DEG_TO_RAD(HR_TO_DEG(x))

#define DEG_TO_HR(x)    ((x) * 15.0)
#define RAD_TO_HR(x)    DEG_TO_HR(RAD_TO_DEG(x))


#define MAX(a, b)  ((a) > (b)) ? (a) : (b)
#define MIN(a, b)  ((a) < (b)) ? (a) : (b)

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;


namespace rsw
{

// returns [0,1)
inline float randf()
{
	return rand() / (RAND_MAX + 1.0f);
}


inline float rand_float(float min, float max)
{
	return min + (max-min) * randf();
}

/*
 * vectors
 *
 *  Matches GLSL behavior for operators
 *
 *
 *
 *
 */
struct mat2;
struct mat3;
struct mat4;

struct vec2
{
	float x;
	float y;


	vec2() : x(), y() {}
	vec2(float a) : x(a), y(a) {}
	vec2(float x, float y) : x(x), y(y) {}
	//vec2(vec3 v) : x(v.x), y(v.y) {}


	float len() { return sqrt(x*x + y*y); }
	vec2 norm() { float l = this->len(); return vec2(x/l, y/l); }
	void normalize() { (*this)/=this->len(); }

	vec2& operator -=(vec2 a) { x -= a.x; y -= a.y; return *this; }
	vec2& operator +=(vec2 a) { x += a.x; y += a.y; return *this; }
	vec2& operator *=(vec2 a) { x *= a.x; y *= a.y; return *this; }
	vec2& operator /=(vec2 a) { x /= a.x; y /= a.y; return *this; }

	vec2& operator *=(float a) { x *= a; y *= a; return *this; }
	vec2& operator /=(float a) { x /= a; y /= a; return *this; }
	vec2& operator +=(float a) { x += a; y += a; return *this; }
	vec2& operator -=(float a) { x -= a; y -= a; return *this; }

	vec2& operator *=(mat2 a);

	vec2 xx() { return vec2(x,x); }
	vec2 xy() { return vec2(x,y); } //no reason for this ...
	vec2 yx() { return vec2(y,x); }
	vec2 yy() { return vec2(y,y); }
};


inline vec2 operator+(vec2 a, vec2 b) { return vec2(a.x+b.x, a.y+b.y); }
inline vec2 operator-(vec2 a, vec2 b) { return vec2(a.x-b.x, a.y-b.y); }
inline vec2 operator*(vec2 a, vec2 b) { return vec2(a.x*b.x, a.y*b.y); }
inline vec2 operator/(vec2 a, vec2 b) { return vec2(a.x/b.x, a.y/b.y); }

inline vec2 operator-(vec2 a) {         return vec2(-a.x, -a.y); }

inline vec2 operator+(vec2 a, float b) { return vec2(a.x+b, a.y+b); }
inline vec2 operator+(float b, vec2 a) { return vec2(a.x+b, a.y+b); }
inline vec2 operator-(vec2 a, float b) { return vec2(a.x-b, a.y-b); }
//inline vec2 operator-(float b, vec2 a) { return vec2(b) - a; }
inline vec2 operator*(vec2 a, float b) { return vec2(a.x*b, a.y*b); }
inline vec2 operator*(float b, vec2 a) { return vec2(a.x*b, a.y*b); }
inline vec2 operator/(vec2 a, float b) { return vec2(a.x/b, a.y/b); }

inline float dot(vec2 a, vec2 b) { return a.x*b.x + a.y*b.y; }
inline float length(vec2 a) { return a.len(); }
inline vec2 normalize(vec2 a) { return a.norm(); }


inline std::ostream& operator<<(std::ostream& stream, const vec2& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<")";
}

inline bool operator==(const vec2& a, const vec2& b)
{
	return ((a.x==b.x) && (a.y==b.y));
}

inline bool eql_epsilon(vec2 a, vec2 b, float epsilon)
{
	return ((fabs(a.x-b.x)<epsilon) && (fabs(a.y-b.y)<epsilon));
}




/**********************************************************/

struct vec3
{
	union {
		struct {
			float x,y,z;
		};
		float pts[3];
	};

	vec3() : x(), y(), z() {}
	vec3(float a) : x(a), y(a), z(a) {}
	vec3(float x, float y, float z) : x(x), y(y), z(z) {}
	vec3(vec2 a, float z) : x(a.x), y(a.y), z(z) {}
	vec3(float x, vec2 a) : x(x), y(a.x), z(a.y) {}
	//vec3(vec4 v) : x(v.x), y(v.y), z(v.z) {}

	float len() const { return sqrt(x*x + y*y + z*z); }
	float len_squared() const { return x*x + y*y + z*z; }
	vec3 norm() const { float l = this->len(); return vec3(x/l, y/l, z/l); }
	void normalize() { (*this)/=this->len(); }

	vec3& operator -=(vec3 a) { x -= a.x; y -= a.y; z -= a.z; return *this; }
	vec3& operator +=(vec3 a) { x += a.x; y += a.y; z += a.z; return *this; }
	vec3& operator *=(vec3 a) { x *= a.x; y *= a.y; z *= a.z; return *this; }
	vec3& operator /=(vec3 a) { x /= a.x; y /= a.y; z /= a.z; return *this; }

	vec3& operator *=(float a) { x *= a; y *= a; z *= a; return *this; }
	vec3& operator /=(float a) { x /= a; y /= a; z /= a; return *this; }
	vec3& operator +=(float a) { x += a; y += a; z += a; return *this; }
	vec3& operator -=(float a) { x -= a; y -= a; z -= a; return *this; }

	//swizzles
	vec2 xx() { return vec2(x,x); }
	vec2 xy() { return vec2(x,y); }
	vec2 xz() { return vec2(x,z); }
	vec2 yx() { return vec2(y,x); }
	vec2 yy() { return vec2(y,y); }
	vec2 yz() { return vec2(y,z); }
	vec2 zx() { return vec2(z,x); }
	vec2 zy() { return vec2(z,y); }
	vec2 zz() { return vec2(z,z); }

	vec3 xxx() { return vec3(x,x,x); }
	vec3 xxy() { return vec3(x,x,y); }
	vec3 xxz() { return vec3(x,x,z); }
	vec3 xyx() { return vec3(x,y,x); }
	vec3 xyy() { return vec3(x,y,y); }
	vec3 xyz() { return *this; }
	vec3 xzx() { return vec3(x,z,x); }
	vec3 xzy() { return vec3(x,z,y); }
	vec3 xzz() { return vec3(x,z,z); }

	vec3 yxx() { return vec3(y,x,x); }
	vec3 yxy() { return vec3(y,x,y); }
	vec3 yxz() { return vec3(y,x,z); }
	vec3 yyx() { return vec3(y,y,x); }
	vec3 yyy() { return vec3(y,y,y); }
	vec3 yyz() { return vec3(y,y,z); }
	vec3 yzx() { return vec3(y,z,x); }
	vec3 yzy() { return vec3(y,z,y); }
	vec3 yzz() { return vec3(y,z,z); }

	vec3 zxx() { return vec3(z,x,x); }
	vec3 zxy() { return vec3(z,x,y); }
	vec3 zxz() { return vec3(z,x,z); }
	vec3 zyx() { return vec3(z,y,x); }
	vec3 zyy() { return vec3(z,y,y); }
	vec3 zyz() { return vec3(z,y,z); }
	vec3 zzx() { return vec3(z,z,x); }
	vec3 zzy() { return vec3(z,z,y); }
	vec3 zzz() { return vec3(z,z,z); }
};



inline vec3 operator+(vec3 a, vec3 b) { return vec3(a.x+b.x, a.y+b.y, a.z+b.z); }
inline vec3 operator-(vec3 a, vec3 b) { return vec3(a.x-b.x, a.y-b.y, a.z-b.z); }
inline vec3 operator*(vec3 a, vec3 b) { return vec3(a.x*b.x, a.y*b.y, a.z*b.z); }
inline vec3 operator/(vec3 a, vec3 b) { return vec3(a.x/b.x, a.y/b.y, a.z/b.z); }

inline vec3 operator-(vec3 a) { return vec3(-a.x, -a.y, -a.z); }

inline vec3 operator+(vec3 a, float b) { return a+=b; }
inline vec3 operator+(float b, vec3 a) { return a+=b; }
inline vec3 operator-(vec3 a, float b) { return a-=b; }
//inline vec3 operator-(float b, vec3 a) { return vec3(b)-a; }  //not sure this is in GLSL
inline vec3 operator*(vec3 a, float b) { return a*=b; }
inline vec3 operator*(float b, vec3 a) { return a*=b; }
inline vec3 operator/(vec3 a, float b) { return a/=b; }


inline float dot(vec3 a, vec3 b)  { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline float length(vec3 a) { return a.len(); }
inline vec3 normalize(vec3 a) { return a.norm(); }

inline std::ostream& operator<<(std::ostream& stream, const vec3& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<")";
}

inline std::istream& operator>>(std::istream& stream, vec3& a)
{
	//no error checking for now
	//ignore junk ie whitespace till open paren, 1000 is arbitrary
	//but probably adequate
	stream.ignore(1000, '(');
	stream >> a.x;
	stream.get();
	stream >> a.y;
	stream.get();
	stream >> a.z;
	stream.get();
	return stream;
}

inline bool operator==(const vec3& a, const vec3& b)
{
	return ((a.x==b.x) && (a.y==b.y) && (a.z==b.z));
}

inline bool operator!=(const vec3& a, const vec3& b)
{
	return ((a.x!=b.x) || (a.y!=b.y) || (a.z!=b.z));
}


inline bool eql_epsilon(vec3 a, vec3 b, float epsilon)
{
	return ((fabs(a.x-b.x)<epsilon) && (fabs(a.y-b.y)<epsilon) && (fabs(a.z-b.z)<epsilon));
}

inline vec3 cross(const vec3 u, const vec3 v)
{
	vec3 result;
	result.x = u.y*v.z - v.y*u.z;
	result.y = -u.x*v.z + v.x*u.z;
	result.z = u.x*v.y - v.x*u.y;
	return result;
}

inline float angle_between_vec3(const vec3 u, const vec3 v)
{
	return acos(dot(u, v));
}



/**********************************************************/
struct vec4
{
	union {
		struct {
			float x,y,z,w;
		};
		float pts[4];
	};

	vec4() : x(), y(), z(), w() {}
	vec4(float a) : x(a), y(a), z(a), w(a) {}
	vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	vec4(vec3 a, float w) : x(a.x), y(a.y), z(a.z), w(w) {}
	vec4(vec2 a, float z, float w) : x(a.x), y(a.y), z(z), w(w) {}
	vec4(vec2 a, vec2 b) : x(a.x), y(a.y), z(b.x), w(b.y) {}
	vec4(float x, float y, vec2 b) : x(x), y(y), z(b.x), w(b.y) {}
	//TODO add other varieties in GLSL

	float len() { return sqrt(x*x + y*y + z*z + w*w); }
	vec4 norm() { float l = this->len(); return vec4(x/l, y/l, z/l, w/l); }
	void normalize() { (*this)/=this->len(); }

	vec3 tovec3() { return vec3(x, y, z); }
	vec2 tovec2() { return vec2(x, y); }
	vec3 vec3h() { return vec3(x/w, y/w, z/w); }
	vec2 vec2h() { return vec2(x/w, y/w); }

	vec4& operator -=(vec4 a) { x -= a.x; y -= a.y; z -= a.z; w -= a.w; return *this; }
	vec4& operator +=(vec4 a) { x += a.x; y += a.y; z += a.z; w += a.w; return *this; }
	vec4& operator *=(vec4 a) { x *= a.x; y *= a.y; z *= a.z; w *= a.w; return *this; }
	vec4& operator /=(vec4 a) { x /= a.x; y /= a.y; z /= a.z; w /= a.w; return *this; }

	vec4& operator *=(float a) { x *= a; y *= a; z *= a; w *= a; return *this; }
	vec4& operator /=(float a) { x /= a; y /= a; z /= a; w /= a; return *this; }
	vec4& operator +=(float a) { x += a; y += a; z += a; w += a; return *this; }
	vec4& operator -=(float a) { x -= a; y -= a; z -= a; w -= a; return *this; }

	//swizzles
	vec2 xx() { return vec2(x,x); }
	vec2 xy() { return vec2(x,y); }
	vec2 xz() { return vec2(x,z); }
	vec2 xw() { return vec2(x,w); }
	vec2 yx() { return vec2(y,x); }
	vec2 yy() { return vec2(y,y); }
	vec2 yz() { return vec2(y,z); }
	vec2 yw() { return vec2(y,w); }
	vec2 zx() { return vec2(z,x); }
	vec2 zy() { return vec2(z,y); }
	vec2 zz() { return vec2(z,z); }
	vec2 zw() { return vec2(z,w); }
	vec2 wx() { return vec2(w,x); }
	vec2 wy() { return vec2(w,y); }
	vec2 wz() { return vec2(w,z); }
	vec2 ww() { return vec2(w,w); }

	//add set swizzle funcs
	//probably supposed to return vec2 ... TODO
	void xy(float a, float b) { x = a; y = b; }
	void xz(float a, float b) { x = a; z = b; }
	void xw(float a, float b) { x = a; w = b; }
	void yx(float a, float b) { y = a; x = b; }
	void yz(float a, float b) { y = a; z = b; }
	void yw(float a, float b) { y = a; w = b; }
	void zx(float a, float b) { z = a; x = b; }
	void zy(float a, float b) { z = a; y = b; }
	void zw(float a, float b) { z = a; w = b; }
	void wx(float a, float b) { w = a; x = b; }
	void wy(float a, float b) { w = a; y = b; }
	void wz(float a, float b) { w = a; z = b; }

	void xy(vec2 v) { x = v.x; y = v.y; }
	void xz(vec2 v) { x = v.x; z = v.y; }
	void xw(vec2 v) { x = v.x; w = v.y; }
	void yx(vec2 v) { y = v.x; x = v.y; }
	void yz(vec2 v) { y = v.x; z = v.y; }
	void yw(vec2 v) { y = v.x; w = v.y; }
	void zx(vec2 v) { z = v.x; x = v.y; }
	void zy(vec2 v) { z = v.x; y = v.y; }
	void zw(vec2 v) { z = v.x; w = v.y; }
	void wx(vec2 v) { w = v.x; x = v.y; }
	void wy(vec2 v) { w = v.x; y = v.y; }
	void wz(vec2 v) { w = v.x; z = v.y; }

	vec3 xxx() { return vec3(x,x,x); }
	vec3 xxy() { return vec3(x,x,y); }
	vec3 xxz() { return vec3(x,x,z); }
	vec3 xyx() { return vec3(x,y,x); }
	vec3 xyy() { return vec3(x,y,y); }
	vec3 xyz() { return vec3(x,y,z); }
	vec3 xzx() { return vec3(x,z,x); }
	vec3 xzy() { return vec3(x,z,y); }
	vec3 xzz() { return vec3(x,z,z); }

	vec3 yxx() { return vec3(y,x,x); }
	vec3 yxy() { return vec3(y,x,y); }
	vec3 yxz() { return vec3(y,x,z); }
	vec3 yyx() { return vec3(y,y,x); }
	vec3 yyy() { return vec3(y,y,y); }
	vec3 yyz() { return vec3(y,y,z); }
	vec3 yzx() { return vec3(y,z,x); }
	vec3 yzy() { return vec3(y,z,y); }
	vec3 yzz() { return vec3(y,z,z); }

	vec3 zxx() { return vec3(z,x,x); }
	vec3 zxy() { return vec3(z,x,y); }
	vec3 zxz() { return vec3(z,x,z); }
	vec3 zyx() { return vec3(z,y,x); }
	vec3 zyy() { return vec3(z,y,y); }
	vec3 zyz() { return vec3(z,y,z); }
	vec3 zzx() { return vec3(z,z,x); }
	vec3 zzy() { return vec3(z,z,y); }
	vec3 zzz() { return vec3(z,z,z); }

};


inline vec4 operator+(vec4 a, vec4 b) { return vec4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }
inline vec4 operator-(vec4 a, vec4 b) { return vec4(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w); }
inline vec4 operator*(vec4 a, vec4 b) { return vec4(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w); }
inline vec4 operator/(vec4 a, vec4 b) { return vec4(a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w); }

//TODO Are these in GLSL
inline vec4 operator+(vec4 a, vec3 b) { return vec4(a.x+b.x, a.y+b.y, a.z+b.z, 1.0f); }
inline vec4 operator-(vec4 a, vec3 b) { return vec4(a.x-b.x, a.y-b.y, a.z-b.z, 1.0f); }
inline vec4 operator-(vec4 a) { return vec4(-a.x, -a.y, -a.z, -a.w); }


inline vec4 operator+(vec4 a, float b) { return a+=b; }
inline vec4 operator+(float b, vec4 a) { return a+=b; }
inline vec4 operator-(vec4 a, float b) { return a-=b; }
//inline vec4 operator-(float b, vec4 a) { return vec4(b) - a; }
inline vec4 operator*(vec4 a, float b) { return a*=b; }
inline vec4 operator*(float b, vec4 a) { return a*=b; }
inline vec4 operator/(vec4 a, float b) { return a/=b; }

inline float dot(vec4 a, vec4 b)  { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
inline float length(vec4 a) { return a.len(); }
inline vec4 normalize(vec4 a) { return a.norm(); }

inline std::ostream& operator<<(std::ostream& stream, const vec4& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<", "<<a.w<<")";
}


inline bool operator==(const vec4& a, const vec4& b)
{
	return ((a.x==b.x) && (a.y==b.y) && (a.z==b.z) && (a.w==b.w));
}

inline bool operator!=(const vec4& a, const vec4& b)
{
	return ((a.x!=b.x) || (a.y!=b.y) || (a.z!=b.z) || (a.w!=b.w));
}


inline bool eql_epsilon(vec4 a, vec4 b, float epsilon)
{
	return ((fabs(a.x-b.x)<epsilon) && (fabs(a.y-b.y)<epsilon) && (fabs(a.z-b.z)<epsilon) && (fabs(a.w-b.w)<epsilon));
}



/*
 * TODO
 * cross product functions
 * get angle between vectors
 * normalize
 */

/*******************************************/

struct dvec2
{
	double x;
	double y;


	dvec2(double x=0, double y=0) : x(x), y(y) {}
	double len() { return sqrt((*this)*(*this)); }
	dvec2 norm() { double l = this->len(); return dvec2(x/l, y/l); }
	void normalize() { (*this)/=this->len(); }

	double operator*(dvec2 b) { return x*b.x + y*b.y; }

	dvec2& operator -=(dvec2 a) { x -= a.x; y -= a.y; return *this; }
	dvec2& operator +=(dvec2 a) { x += a.x; y += a.y; return *this; }
	dvec2& operator *=(double a) { x *= a; y *= a; return *this; }
	dvec2& operator /=(double a) { x /= a; y /= a; return *this; }
};


inline dvec2 operator+(dvec2 a, dvec2 b) { return dvec2(a.x+b.x, a.y+b.y); }
inline dvec2 operator-(dvec2 a, dvec2 b) { return dvec2(a.x-b.x, a.y-b.y); }
inline dvec2 operator-(dvec2 a) {          return dvec2(-a.x, -a.y); }

inline dvec2 operator*(dvec2 a, double b) { return a*=b; }
inline dvec2 operator*(double b, dvec2 a) { return a*=b; }
inline dvec2 operator/(dvec2 a, double b) { return a/=b; }


inline std::ostream& operator<<(std::ostream& stream, const dvec2& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<")";
}

inline bool operator==(const dvec2& a, const dvec2& b)
{
	return ((a.x==b.x) && (a.y==b.y));
}

inline bool eql_epsilon(dvec2 a, dvec2 b, double epsilon)
{
	return ((fabs(a.x-b.x)<epsilon) && (fabs(a.y-b.y)<epsilon));
}




/**********************************************************/
struct dvec4;

struct dvec3
{
	union {
		struct {
			double x,y,z;
		};
		double pts[3];
	};

	dvec3(double x=0, double y=0, double z=0) : x(x), y(y), z(z) {}
	double len() { return sqrt((*this)*(*this)); }
	dvec3 norm() { double l = this->len(); return dvec3(x/l, y/l, z/l); }
	void normalize() { (*this)/=this->len(); }

	double operator*(const dvec3 b) const { return x*b.x + y*b.y + z*b.z; }

	dvec3& operator -=(dvec3 a) { x -= a.x; y -= a.y; z -= a.z; return *this; }
	dvec3& operator +=(dvec3 a) { x += a.x; y += a.y; z += a.z; return *this; }
	dvec3& operator *=(double a) { x *= a; y *= a; z *= a; return *this; }
	dvec3& operator /=(double a) { x /= a; y /= a; z /= a; return *this; }

	//swizzles
	dvec2 xy() { return dvec2(x,y); }
	dvec2 xz() { return dvec2(x,z); }
	dvec2 yx() { return dvec2(y,x); }
	dvec2 yy() { return dvec2(y,y); }
	dvec2 yz() { return dvec2(y,z); }
	dvec2 zx() { return dvec2(z,x); }
	dvec2 zy() { return dvec2(z,y); }
	dvec2 zz() { return dvec2(z,z); }

	dvec3 xxx() { return dvec3(x,x,x); }
	dvec3 xxy() { return dvec3(x,x,y); }
	dvec3 xxz() { return dvec3(x,x,z); }
	dvec3 xyx() { return dvec3(x,y,x); }
	dvec3 xyy() { return dvec3(x,y,y); }
	dvec3 xyz() { return *this; }
	dvec3 xzx() { return dvec3(x,z,x); }
	dvec3 xzy() { return dvec3(x,z,y); }
	dvec3 xzz() { return dvec3(x,z,z); }

	dvec3 yxx() { return dvec3(y,x,x); }
	dvec3 yxy() { return dvec3(y,x,y); }
	dvec3 yxz() { return dvec3(y,x,z); }
	dvec3 yyx() { return dvec3(y,y,x); }
	dvec3 yyy() { return dvec3(y,y,y); }
	dvec3 yyz() { return dvec3(y,y,z); }
	dvec3 yzx() { return dvec3(y,z,x); }
	dvec3 yzy() { return dvec3(y,z,y); }
	dvec3 yzz() { return dvec3(y,z,z); }

	dvec3 zxx() { return dvec3(z,x,x); }
	dvec3 zxy() { return dvec3(z,x,y); }
	dvec3 zxz() { return dvec3(z,x,z); }
	dvec3 zyx() { return dvec3(z,y,x); }
	dvec3 zyy() { return dvec3(z,y,y); }
	dvec3 zyz() { return dvec3(z,y,z); }
	dvec3 zzx() { return dvec3(z,z,x); }
	dvec3 zzy() { return dvec3(z,z,y); }
	dvec3 zzz() { return dvec3(z,z,z); }
};



inline dvec3 operator+(dvec3 a, dvec3 b) {  return dvec3(a.x+b.x, a.y+b.y, a.z+b.z); }
inline dvec3 operator-(dvec3 a, dvec3 b) {  return dvec3(a.x-b.x, a.y-b.y, a.z-b.z); }
//inline double operator*(dvec3 a, dvec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline dvec3 operator-(dvec3 a) { return dvec3(-a.x, -a.y, -a.z); }

inline dvec3 operator*(dvec3 a, double b) { return a*=b; }
inline dvec3 operator*(double b, dvec3 a) { return a*=b; }
inline dvec3 operator/(dvec3 a, double b) { return a/=b; }


inline std::ostream& operator<<(std::ostream& stream, const dvec3& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<")";
}

inline std::istream& operator>>(std::istream& stream, dvec3& a)
{
	//no error checking for now
	//ignore junk ie whitespace till open paren, 1000 is arbitrary
	//but probably adequate
	stream.ignore(1000, '(');
	stream >> a.x;
	stream.get();
	stream >> a.y;
	stream.get();
	stream >> a.z;
	stream.get();
	return stream;
}

inline bool operator==(const dvec3& a, const dvec3& b)
{
	return ((a.x==b.x) && (a.y==b.y) && (a.z==b.z));
}

inline bool operator!=(const dvec3& a, const dvec3& b)
{
	return ((a.x!=b.x) || (a.y!=b.y) || (a.z!=b.z));
}


inline bool eql_epsilon(dvec3 a, dvec3 b, double epsilon)
{
	return ((fabs(a.x-b.x)<epsilon) && (fabs(a.y-b.y)<epsilon) && (fabs(a.z-b.z)<epsilon));
}

inline dvec3 cross(const dvec3 u, const dvec3 v)
{
	dvec3 result;
	result.x = u.y*v.z - v.y*u.z;
	result.y = -u.x*v.z + v.x*u.z;
	result.z = u.x*v.y - v.x*u.y;
	return result;
}

inline double angle_between_dvec3(const dvec3 u, const dvec3 v)
{
	return acos(u * v);
}



/**********************************************************/
struct dvec4
{
	union {
		struct {
			double x,y,z,w;
		};
		double pts[4];
	};

	dvec4(double x=0, double y=0, double z=0, double w=1) : x(x), y(y), z(z), w(w) {}
	dvec4(dvec3 a) : x(a.x), y(a.y), z(a.z), w(1) {}
	dvec3 todvec3() { return dvec3(x, y, z); }
	dvec2 todvec2() { return dvec2(x, y); }
	dvec3 dvec3h() { return dvec3(x/w, y/w, z/w); }
	dvec2 dvec2h() { return dvec2(x/w, y/w); }
//
//	dvec3& norm();
//	double len();
//
//	dvec3& operator-=(dvec3);
//	dvec3& operator+=(dvec3);
//	dvec3& operator*=(double);
//	dvec3& operator/=(double);\

	//swizzles
	dvec2 xx() { return dvec2(x,x); }
	dvec2 xy() { return dvec2(x,y); }
	dvec2 xz() { return dvec2(x,z); }
	dvec2 yx() { return dvec2(y,x); }
	dvec2 yy() { return dvec2(y,y); }
	dvec2 yz() { return dvec2(y,z); }
	dvec2 zx() { return dvec2(z,x); }
	dvec2 zy() { return dvec2(z,y); }
	dvec2 zz() { return dvec2(z,z); }

	dvec3 xxx() { return dvec3(x,x,x); }
	dvec3 xxy() { return dvec3(x,x,y); }
	dvec3 xxz() { return dvec3(x,x,z); }
	dvec3 xyx() { return dvec3(x,y,x); }
	dvec3 xyy() { return dvec3(x,y,y); }
	dvec3 xyz() { return dvec3(x,y,z); }
	dvec3 xzx() { return dvec3(x,z,x); }
	dvec3 xzy() { return dvec3(x,z,y); }
	dvec3 xzz() { return dvec3(x,z,z); }

	dvec3 yxx() { return dvec3(y,x,x); }
	dvec3 yxy() { return dvec3(y,x,y); }
	dvec3 yxz() { return dvec3(y,x,z); }
	dvec3 yyx() { return dvec3(y,y,x); }
	dvec3 yyy() { return dvec3(y,y,y); }
	dvec3 yyz() { return dvec3(y,y,z); }
	dvec3 yzx() { return dvec3(y,z,x); }
	dvec3 yzy() { return dvec3(y,z,y); }
	dvec3 yzz() { return dvec3(y,z,z); }

	dvec3 zxx() { return dvec3(z,x,x); }
	dvec3 zxy() { return dvec3(z,x,y); }
	dvec3 zxz() { return dvec3(z,x,z); }
	dvec3 zyx() { return dvec3(z,y,x); }
	dvec3 zyy() { return dvec3(z,y,y); }
	dvec3 zyz() { return dvec3(z,y,z); }
	dvec3 zzx() { return dvec3(z,z,x); }
	dvec3 zzy() { return dvec3(z,z,y); }
	dvec3 zzz() { return dvec3(z,z,z); }

};

//I can define these functions later but I can effectively do the same thing
//by just calling vec3(a.x, a.y, a.z) when a is dvec4

//float operator*(dvec4 a, vec3 b);

inline std::ostream& operator<<(std::ostream& stream, const dvec4& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<", "<<a.w<<")";
}

inline float operator*(dvec4 a, dvec4 b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

inline bool operator==(const dvec4& a, const dvec4& b)
{
	return ((a.x==b.x) && (a.y==b.y) && (a.z==b.z) && (a.w==b.w));
}

inline bool operator!=(const dvec4& a, const dvec4& b)
{
	return ((a.x!=b.x) || (a.y!=b.y) || (a.z!=b.z) || (a.w!=b.w));
}


inline bool eql_epsilon(dvec4 a, dvec4 b, float epsilon)
{
	return ((fabs(a.x-b.x)<epsilon) && (fabs(a.y-b.y)<epsilon) && (fabs(a.z-b.z)<epsilon) && (fabs(a.w-b.w)<epsilon));
}












struct ivec3;


/**********************************************************/
struct ivec2
{
	public:
	union {
		struct {
			int x,y;
		};
		int pts[2];
	};

	ivec2(int x=0, int y=0) : x(x), y(y) {}


	ivec2& operator+=(ivec2 a) { x += a.x; y += a.y; return *this; }
	ivec2& operator*=(int a) { x *= a; y *= a; return *this; }
	int operator[](int i) { return pts[i]; }

};

inline ivec2 operator+(ivec2 a, ivec2 b) { return ivec2(a.x+b.x, a.y+b.y); }
inline ivec2 operator-(ivec2 a) { return ivec2(-a.x, -a.y); }

inline std::ostream& operator<<(std::ostream& stream, const ivec2 a)
{
	return stream <<"("<<a.x<<", "<<a.y<<")";
}

inline bool operator==(const ivec2& a, const ivec2& b) { return ((a.x==b.x) && (a.y==b.y)); }



/**********************************************************/
struct ivec3
{
	public:
	union {
		struct {
			int x,y,z;
		};
		int pts[3];
	};

	ivec3(int x=0, int y=0, int z=0) : x(x), y(y), z(z) {}


	ivec3& operator+=(ivec3 a) { x += a.x; y += a.y; z += a.z; return *this; }
	ivec3& operator*=(int a) { x *= a; y *= a; z *= a; return *this; }
	int operator[](int i) { return pts[i]; }
};

inline ivec3 operator+(ivec3 a, ivec3 b) { return ivec3(a.x+b.x, a.y+b.y, a.z+b.z); }
inline ivec3 operator-(ivec3 a) { return ivec3(-a.x, -a.y, -a.z); }

inline std::ostream& operator<<(std::ostream& stream, const ivec3& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<")";
}

inline bool operator==(const ivec3& a, const ivec3& b) { return ((a.x==b.x) && (a.y==b.y) && (a.z==b.z)); }

/***********************************************************/
struct ivec4
{
	public:
	union {
		struct {
			int x,y,z,w;
		};
		int pts[4];
	};

	ivec4(int x=0, int y=0, int z=0, int w=1) : x(x), y(y), z(z), w(w) {}


	ivec4& operator+=(ivec4 a) { x += a.x; y += a.y; z += a.z; w += a.w; return *this; }
	ivec4& operator*=(int a) { x *= a; y *= a; z *= a; w *= a; return *this; }
	int operator[](int i) { return pts[i]; }
};

inline ivec4 operator+(ivec4 a, ivec4 b) { return ivec4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }
inline ivec4 operator-(ivec4 a) { return ivec4(-a.x, -a.y, -a.z, -a.w); }

inline std::ostream& operator<<(std::ostream& stream, const ivec4& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<", "<<a.w<<")";
}

inline bool operator==(const ivec4& a, const ivec4& b) { return ((a.x==b.x) && (a.y==b.y) && (a.z==b.z) && (a.w==b.w)); }



/**********************************************************/
struct uvec2
{
	public:
	union {
		struct {
			unsigned int x,y;
		};
		unsigned int pts[2];
	};

	uvec2(unsigned int x=0, unsigned int y=0) : x(x), y(y) {}

	uvec2& operator+=(uvec2 a) { x += a.x; y += a.y; return *this; }
	uvec2& operator*=(unsigned int a) { x *= a; y *= a; return *this; }
	unsigned int operator[](int i) { return pts[i]; }
};

inline uvec2 operator+(uvec2 a, uvec2 b) { return uvec2(a.x+b.x, a.y+b.y); }
inline uvec2 operator-(uvec2 a) { return uvec2(-a.x, -a.y); }

inline std::ostream& operator<<(std::ostream& stream, const uvec2& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<")";
}

inline bool operator==(const uvec2& a, const uvec2& b) { return ((a.x==b.x) && (a.y==b.y)); }

/**********************************************************/
struct uvec3
{
	public:
	union {
		struct {
			int x,y,z;
		};
		unsigned int pts[3];
	};

	uvec3(unsigned int x=0, unsigned int y=0, unsigned int z=0) : x(x), y(y), z(z) {}

	uvec3& operator+=(uvec3 a) { x += a.x; y += a.y; z += a.z; return *this; }
	uvec3& operator*=(unsigned int a) { x *= a; y *= a; z *= a; return *this; }
	unsigned int operator[](int i) { return pts[i]; }

};

inline uvec3 operator+(uvec3 a, uvec3 b) { return uvec3(a.x+b.x, a.y+b.y, a.z+b.z); }
inline uvec3 operator-(uvec3 a) { return uvec3(-a.x, -a.y, -a.z); }

inline std::ostream& operator<<(std::ostream& stream, const uvec3& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<")";
}

inline bool operator==(const uvec3& a, const uvec3& b) { return ((a.x==b.x) && (a.y==b.y) && (a.z==b.z)); }



/**********************************************************/
struct uvec4
{
	public:
	union {
		struct {
			unsigned int x,y,z,w;
		};
		unsigned int pts[4];
	};

	uvec4(unsigned int x=0, unsigned int y=0, unsigned int z=0, unsigned int w=1) : x(x), y(y), z(z), w(w) {}

	uvec4& operator+=(uvec4 a) { x += a.x; y += a.y; z += a.z; w += a.w; return *this; }
	uvec4& operator*=(unsigned int a) { x *= a; y *= a; z *= a; w *= a; return *this; }
	unsigned int operator[](int i) { return pts[i]; }

};

inline uvec4 operator+(uvec4 a, uvec4 b) { return uvec4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }
inline uvec4 operator-(uvec4 a) { return uvec4(-a.x, -a.y, -a.z, -a.w); }

inline std::ostream& operator<<(std::ostream& stream, const uvec4& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<", "<<a.w<<")";
}

inline bool operator==(const uvec4& a, const uvec4& b) { return ((a.x==b.x) && (a.y==b.y) && (a.z==b.z) && (a.w==b.w)); }










/*
 *
 **********************************************************
 * Matrices
 */

struct mat2
{
	float matrix[4];
	mat2() { matrix[0] = matrix[2] = 1; }
	mat2(float i)
	{
		matrix[1] = matrix[3] = 0;
		matrix[0] = matrix[2] = i;
	}
	mat2(float a, float b, float c, float d)
	{
#ifndef ROW_MAJOR
		matrix[0] = a;
		matrix[1] = b;
		matrix[2] = c;
		matrix[3] = d;
#else
		matrix[0] = a;
		matrix[2] = b;
		matrix[1] = c;
		matrix[3] = d;
#endif
	}

	mat2(mat3 m);
	mat2(mat4 m);

	mat2(float a[]) { memcpy(matrix, a, sizeof(float)*4); }

	mat2(vec2 c1, vec2 c2)
	{
		this->setc1(c1);
		this->setc2(c2);
	}

	void set(float a[]) { memcpy(matrix, a, sizeof(float)*4); }

	float& operator [](size_t i) { return matrix[i]; }

#ifndef ROW_MAJOR
	vec2 x() const { return vec2(matrix[0], matrix[2]); }
	vec2 y() const { return vec2(matrix[1], matrix[3]); }
	vec2 c1() const { return vec2(matrix[0], matrix[1]); }
	vec2 c2() const { return vec2(matrix[2], matrix[3]); }

	void setx(vec2 v) { matrix[0]=v.x, matrix[2]=v.y; }
	void sety(vec2 v) { matrix[1]=v.x, matrix[3]=v.y; }

	void setc1(vec2 v) { matrix[0]=v.x, matrix[1]=v.y; }
	void setc2(vec2 v) { matrix[2]=v.x, matrix[3]=v.y; }
#else
	vec2 x() const { return vec2(matrix[0], matrix[1]); }
	vec2 y() const { return vec2(matrix[2], matrix[3]); }
	vec2 c1() const { return vec2(matrix[0], matrix[2]); }
	vec2 c2() const { return vec2(matrix[1], matrix[3]); }

	void setx(vec2 v) { matrix[0]=v.x, matrix[1]=v.y; }
	void sety(vec2 v) { matrix[2]=v.x, matrix[3]=v.y; }

	void setc1(vec2 v) { matrix[0]=v.x, matrix[2]=v.y; }
	void setc2(vec2 v) { matrix[1]=v.x, matrix[3]=v.y; }
#endif

	mat2& operator *=(float a)
	{
		matrix[0] *= a;
		matrix[1] *= a;
		matrix[2] *= a;
		matrix[3] *= a;
		return *this;
	}
	mat2& operator /=(float a)
	{
		matrix[0] /= a;
		matrix[1] /= a;
		matrix[2] /= a;
		matrix[3] /= a;
		return *this;
	}
	mat2& operator +=(float a)
	{
		matrix[0] += a;
		matrix[1] += a;
		matrix[2] += a;
		matrix[3] += a;
		return *this;
	}
	mat2& operator -=(float a)
	{
		matrix[0] -= a;
		matrix[1] -= a;
		matrix[2] -= a;
		matrix[3] -= a;
		return *this;
	}
};

// TODO wrong if ROW_MAJOR since that constructor is defined as col major
inline mat2 operator+(mat2 a, float b) { return mat2(a[0]+b, a[1]+b, a[2]+b, a[3]+b); }
inline mat2 operator-(mat2 a, float b) { return mat2(a[0]-b, a[1]-b, a[2]-b, a[3]-b); }
inline mat2 operator*(mat2 a, float b) { return mat2(a[0]*b, a[1]*b, a[2]*b, a[3]*b); }
inline mat2 operator/(mat2 a, float b) { return mat2(a[0]/b, a[1]/b, a[2]/b, a[3]/b); }

inline mat2 operator-(mat2 a) { return mat2(-a[0], -a[1], -a[2], -a[3]); }

inline vec2 operator*(const mat2& mat, const vec2& vec)
{
	vec2 tmp;
	tmp.x = dot(mat.x(), vec);
	tmp.y = dot(mat.y(), vec);
	return tmp;
}

inline vec2 operator*(const vec2& vec, const mat2& mat)
{
	vec2 tmp;
	tmp.x = dot(vec, mat.c1());
	tmp.y = dot(vec, mat.c2());
	return tmp;
}

inline vec2& vec2::operator *=(mat2 a)
{
	vec2 tmp = *this;
	x = dot(tmp, a.c1());
	y = dot(tmp, a.c2());
	return *this;
}

inline std::ostream& operator<<(std::ostream& stream, const mat2& mat)
{
	return stream <<"["<<mat.x()<<"\n "<<mat.y()<<"]";
}

//implemented inin rmath.cpp
mat2 operator*(const mat2& a, const mat2& b);

// TODO support creating 2D rotation matrices?
//void load_rotation_mat2(mat2& mat, vec2 v, float angle);



struct mat3
{
	float matrix[9];

	mat3()
	{
		memset(matrix, 0, sizeof(float)*9);
		matrix[0] = 1;
		matrix[4] = 1;
		matrix[8] = 1;
	}
	mat3(float i)
	{
		memset(matrix, 0, sizeof(float)*9);
		matrix[0] = i;
		matrix[4] = i;
		matrix[8] = i;
	}
	
	mat3(mat4 m);

	mat3(float array[]) { memcpy(matrix, array, sizeof(float)*9); }

	//match GLSL, stupid col major crap leads to stupid vec * mat instead of proper mat * vec
	mat3(vec3 c1, vec3 c2, vec3 c3) { this->setc1(c1); this->setc2(c2); this->setc3(c3); }

	void set(float array[]) { memcpy(matrix, array, sizeof(float)*9); }

	float& operator [](size_t i) { return matrix[i]; }

	//Getters/setters for rows and columns
#ifndef ROW_MAJOR
	vec3 x() const { return vec3(matrix[0], matrix[3], matrix[6]); }
	vec3 y() const { return vec3(matrix[1], matrix[4], matrix[7]); }
	vec3 z() const { return vec3(matrix[2], matrix[5], matrix[8]); }
	vec3 c1() const { return vec3(matrix[0], matrix[1], matrix[2]); }
	vec3 c2() const { return vec3(matrix[3], matrix[4], matrix[5]); }
	vec3 c3() const { return vec3(matrix[6], matrix[7], matrix[8]); }

	void setc1(vec3 v) { matrix[0]=v.x, matrix[1]=v.y, matrix[2]=v.z; }
	void setc2(vec3 v) { matrix[3]=v.x, matrix[4]=v.y, matrix[5]=v.z; }
	void setc3(vec3 v) { matrix[6]=v.x, matrix[7]=v.y, matrix[8]=v.z; }

	void setx(vec3 v) { matrix[0]=v.x, matrix[3]=v.y, matrix[6]=v.z; }
	void sety(vec3 v) { matrix[1]=v.x, matrix[4]=v.y, matrix[7]=v.z; }
	void setz(vec3 v) { matrix[2]=v.x, matrix[5]=v.y, matrix[8]=v.z; }
#else
	vec3 x() const { return vec3(matrix[0], matrix[1], matrix[2]); }
	vec3 y() const { return vec3(matrix[3], matrix[4], matrix[5]); }
	vec3 z() const { return vec3(matrix[6], matrix[7], matrix[8]); }
	vec3 c1() const { return vec3(matrix[0], matrix[3], matrix[6]); }
	vec3 c2() const { return vec3(matrix[1], matrix[4], matrix[7]); }
	vec3 c3() const { return vec3(matrix[2], matrix[5], matrix[8]); }

	void setc1(vec3 v) { matrix[0]=v.x, matrix[3]=v.y, matrix[6]=v.z; }
	void setc2(vec3 v) { matrix[1]=v.x, matrix[4]=v.y, matrix[7]=v.z; }
	void setc3(vec3 v) { matrix[2]=v.x, matrix[5]=v.y, matrix[8]=v.z; }

	void setx(vec3 v) { matrix[0]=v.x, matrix[1]=v.y, matrix[2]=v.z; }
	void sety(vec3 v) { matrix[3]=v.x, matrix[4]=v.y, matrix[5]=v.z; }
	void setz(vec3 v) { matrix[6]=v.x, matrix[7]=v.y, matrix[8]=v.z; }
#endif

};


inline vec3 operator*(const mat3& mat, const vec3& vec)
{
	vec3 tmp;
	tmp.x = dot(mat.x(), vec);
	tmp.y = dot(mat.y(), vec);
	tmp.z = dot(mat.z(), vec);
	return tmp;
}

inline vec3 operator*(const vec3& vec, const mat3& mat)
{
	vec3 tmp;
	tmp.x = dot(vec, mat.c1());
	tmp.y = dot(vec, mat.c2());
	tmp.z = dot(vec, mat.c3());
	return tmp;
}

inline std::ostream& operator<<(std::ostream& stream, const mat3& mat)
{
	return stream <<"["<<mat.x()<<"\n "<<mat.y()<<"\n "<<mat.z()<<"]";
}

//implemented in rmath.cpp
mat3 operator*(const mat3& a, const mat3& b);
void load_rotation_mat3(mat3& mat, vec3 v, float angle);

// Note no change between row or column major
inline mat2::mat2(mat3 m)
{
	matrix[0] = m[0];
	matrix[1] = m[1];
	matrix[2] = m[3];
	matrix[3] = m[4];
}





/**********************************************************/

struct mat4
{
	float matrix[16];

	mat4()
	{
		//memset 0 correctly sets to 0.0 for IEEE floats
		memset(matrix, 0, sizeof(float)*16);
		matrix[0] = 1;
		matrix[5] = 1;
		matrix[10] = 1;
		matrix[15] = 1;
	}

	mat4(float i)
	{
		//memset 0 correctly sets to 0.0 for IEEE floats
		memset(matrix, 0, sizeof(float)*16);
		matrix[0] = i;
		matrix[5] = i;
		matrix[10] = i;
		matrix[15] = i;
	}

	mat4(float array[]) { memcpy(matrix, array, sizeof(float)*16); }

	//match GLSL, stupid col major crap leads to stupid vec * mat instead of proper mat * vec
	//https://plus.google.com/114825651948330685771/posts/AmnfvYssvSe
	mat4(vec4 c1, vec4 c2, vec4 c3, vec4 c4) { this->setc1(c1); this->setc2(c2); this->setc3(c3); this->setc4(c4); }

	void set(float array[]) { memcpy(matrix, array, sizeof(float)*16); }
//	void set(vec3 x, vec3 y, vec3 z);

	float& operator[](size_t i) { return matrix[i]; }

#ifndef ROW_MAJOR
	vec4 c1() const { return vec4(matrix[ 0], matrix[ 1], matrix[ 2], matrix[ 3]); }
	vec4 c2() const { return vec4(matrix[ 4], matrix[ 5], matrix[ 6], matrix[ 7]); }
	vec4 c3() const { return vec4(matrix[ 8], matrix[ 9], matrix[10], matrix[11]); }
	vec4 c4() const { return vec4(matrix[12], matrix[13], matrix[14], matrix[15]); }

	vec4 x() const { return vec4(matrix[0], matrix[4], matrix[8], matrix[12]); }
	vec4 y() const { return vec4(matrix[1], matrix[5], matrix[9], matrix[13]); }
	vec4 z() const { return vec4(matrix[2], matrix[6], matrix[10], matrix[14]); }
	vec4 w() const { return vec4(matrix[3], matrix[7], matrix[11], matrix[15]); }

	//sets 4th row to 0 0 0 1
	void setc1(vec3 v) { matrix[ 0]=v.x, matrix[ 1]=v.y, matrix[ 2]=v.z, matrix[ 3]=0; }
	void setc2(vec3 v) { matrix[ 4]=v.x, matrix[ 5]=v.y, matrix[ 6]=v.z, matrix[ 7]=0; }
	void setc3(vec3 v) { matrix[ 8]=v.x, matrix[ 9]=v.y, matrix[10]=v.z, matrix[11]=0; }
	void setc4(vec3 v) { matrix[12]=v.x, matrix[13]=v.y, matrix[14]=v.z, matrix[15]=1; }

	void setc1(vec4 v) { matrix[ 0]=v.x, matrix[ 1]=v.y, matrix[ 2]=v.z, matrix[ 3]=v.w; }
	void setc2(vec4 v) { matrix[ 4]=v.x, matrix[ 5]=v.y, matrix[ 6]=v.z, matrix[ 7]=v.w; }
	void setc3(vec4 v) { matrix[ 8]=v.x, matrix[ 9]=v.y, matrix[10]=v.z, matrix[11]=v.w; }
	void setc4(vec4 v) { matrix[12]=v.x, matrix[13]=v.y, matrix[14]=v.z, matrix[15]=v.w; }

	//sets 4th column to 0 0 0 1
	void setx(vec3 v) { matrix[0]=v.x, matrix[4]=v.y, matrix[ 8]=v.z, matrix[12]=0; }
	void sety(vec3 v) { matrix[1]=v.x, matrix[5]=v.y, matrix[ 9]=v.z, matrix[13]=0; }
	void setz(vec3 v) { matrix[2]=v.x, matrix[6]=v.y, matrix[10]=v.z, matrix[14]=0; }
	void setw(vec3 v) { matrix[3]=v.x, matrix[7]=v.y, matrix[11]=v.z, matrix[15]=1; }

	void setx(vec4 v) { matrix[0]=v.x, matrix[4]=v.y, matrix[ 8]=v.z, matrix[12]=v.w; }
	void sety(vec4 v) { matrix[1]=v.x, matrix[5]=v.y, matrix[ 9]=v.z, matrix[13]=v.w; }
	void setz(vec4 v) { matrix[2]=v.x, matrix[6]=v.y, matrix[10]=v.z, matrix[14]=v.w; }
	void setw(vec4 v) { matrix[3]=v.x, matrix[7]=v.y, matrix[11]=v.z, matrix[15]=v.w; }
#else
	vec4 c1() const { return vec4(matrix[0], matrix[4], matrix[8], matrix[12]); }
	vec4 c2() const { return vec4(matrix[1], matrix[5], matrix[9], matrix[13]); }
	vec4 c3() const { return vec4(matrix[2], matrix[6], matrix[10], matrix[14]); }
	vec4 c4() const { return vec4(matrix[3], matrix[7], matrix[11], matrix[15]); }

	vec4 x() const { return vec4(matrix[0], matrix[1], matrix[2], matrix[3]); }
	vec4 y() const { return vec4(matrix[4], matrix[5], matrix[6], matrix[7]); }
	vec4 z() const { return vec4(matrix[8], matrix[9], matrix[10], matrix[11]); }
	vec4 w() const { return vec4(matrix[12], matrix[13], matrix[14], matrix[15]); }
//
//	vec4 x4() const;
//	vec4 y4() const;
//	vec4 z4() const;
//	vec4 w4() const;

	//sets 4th row to 0 0 0 1
	void setc1(vec3 v) { matrix[0]=v.x, matrix[4]=v.y, matrix[8]=v.z, matrix[12]=0; }
	void setc2(vec3 v) { matrix[1]=v.x, matrix[5]=v.y, matrix[9]=v.z, matrix[13]=0; }
	void setc3(vec3 v) { matrix[2]=v.x, matrix[6]=v.y, matrix[10]=v.z, matrix[14]=0; }
	void setc4(vec3 v) { matrix[3]=v.x, matrix[7]=v.y, matrix[11]=v.z, matrix[15]=1; }

	void setc1(vec4 v) { matrix[0]=v.x, matrix[4]=v.y, matrix[8]=v.z, matrix[12]=v.w; }
	void setc2(vec4 v) { matrix[1]=v.x, matrix[5]=v.y, matrix[9]=v.z, matrix[13]=v.w; }
	void setc3(vec4 v) { matrix[2]=v.x, matrix[6]=v.y, matrix[10]=v.z, matrix[14]=v.w; }
	void setc4(vec4 v) { matrix[3]=v.x, matrix[7]=v.y, matrix[11]=v.z, matrix[15]=v.w; }

	//sets 4th column to 0 0 0 1
	void setx(vec3 v) { matrix[0]=v.x, matrix[1]=v.y, matrix[2]=v.z, matrix[3]=0; }
	void sety(vec3 v) { matrix[4]=v.x, matrix[5]=v.y, matrix[6]=v.z, matrix[7]=0; }
	void setz(vec3 v) { matrix[8]=v.x, matrix[9]=v.y, matrix[10]=v.z, matrix[11]=0; }
	void setw(vec3 v) { matrix[12]=v.x, matrix[13]=v.y, matrix[14]=v.z, matrix[15]=1; }

	void setx(vec4 v) { matrix[0]=v.x, matrix[1]=v.y, matrix[2]=v.z, matrix[3]=v.w; }
	void sety(vec4 v) { matrix[4]=v.x, matrix[5]=v.y, matrix[6]=v.z, matrix[7]=v.w; }
	void setz(vec4 v) { matrix[8]=v.x, matrix[9]=v.y, matrix[10]=v.z, matrix[11]=v.w; }
	void setw(vec4 v) { matrix[12]=v.x, matrix[13]=v.y, matrix[14]=v.z, matrix[15]=v.w; }
#endif

};

// is a function in a header file automatically inlined?
inline vec3 operator*(const mat4& mat, const vec3& vec)
{
	vec4 tmp4(vec, 1.0f);
	return vec3(dot(mat.x(),tmp4), dot(mat.y(),tmp4), dot(mat.z(),tmp4));
}

inline vec4 operator*(const mat4& mat, const vec4& vec)
{
	return vec4(dot(mat.x(),vec), dot(mat.y(),vec), dot(mat.z(),vec), dot(mat.w(),vec));
}

inline std::ostream& operator<<(std::ostream& stream, const mat4& mat)
{
	return stream <<"["<<mat.x()<<"\n "<<mat.y()<<"\n "<<mat.z()<<"\n "<<mat.w()<<"]";
}

// Note these are the same whether row or column major
inline mat2::mat2(mat4 m)
{
	matrix[0] = m[0];
	matrix[1] = m[1];
	matrix[2] = m[4];
	matrix[3] = m[5];
}

inline mat3::mat3(mat4 m)
{
	matrix[0] = m[0];
	matrix[1] = m[1];
	matrix[2] = m[2];
	matrix[3] = m[4];
	matrix[4] = m[5];
	matrix[5] = m[6];
	matrix[6] = m[8];
	matrix[7] = m[9];
	matrix[8] = m[10];
}



//implemented in rmath.cpp
mat4 operator*(const mat4& a, const mat4& b);
void load_rotation_mat4(mat4& mat, vec3 vec, float angle);


mat4 invert_mat4(const mat4& mat);





/**********************************************************/
void make_perspective_matrix(mat4 &mat, float fov, float aspect, float near, float far);
void make_pers_matrix(mat4& mat, float z_near, float z_far);

void make_perspective_proj_matrix(mat4 &mat, float left, float right, float bottom, float top, float near, float far);

void make_orthographic_matrix(mat4 &mat, float left, float right, float bottom, float top, float near, float far);

void make_viewport_matrix(mat4 &mat, int x, int y, unsigned int width, unsigned int height, int opengl=1);

void lookAt(mat4 &mat, vec3 eye, vec3 center, vec3 up);







///////////Matrix transformation functions
inline mat3 scale_mat3(float x_scale, float y_scale, float z_scale)
{
	mat3 m;
	m.matrix[0] = x_scale;
	m.matrix[4] = y_scale;
	m.matrix[8] = z_scale;
	return m;
}

inline mat3 scale_mat3(const vec3 scale)
{
	mat3 m;
	m.matrix[0] = scale.x;
	m.matrix[4] = scale.y;
	m.matrix[8] = scale.z;
	return m;
}

inline mat4 scale_mat4(float x_scale, float y_scale, float z_scale)
{
	mat4 m;
	m.matrix[ 0] = x_scale;
	m.matrix[ 5] = y_scale;
	m.matrix[10] = z_scale;
	return m;
}

inline mat4 scale_mat4(const vec3 scale)
{
	mat4 m;
	m.matrix[ 0] = scale.x;
	m.matrix[ 5] = scale.y;
	m.matrix[10] = scale.z;
	return m;
}

// Create a Translation matrix. Only 4x4 matrices have translation components
inline mat4 translation_mat4(float x, float y, float z)
{
	mat4 m;
#ifndef ROW_MAJOR
	m.matrix[12] = x;
	m.matrix[13] = y;
	m.matrix[14] = z;
#else
	m.matrix[ 3] = x;
	m.matrix[ 7] = y;
	m.matrix[11] = z;
#endif
	return m;
}

inline mat4 translation_mat4(const vec3 scale)
{
	mat4 m;
#ifndef ROW_MAJOR
	m.matrix[12] = scale.x;
	m.matrix[13] = scale.y;
	m.matrix[14] = scale.z;
#else
	m.matrix[ 3] = scale.x;
	m.matrix[ 7] = scale.y;
	m.matrix[11] = scale.z;
#endif
	return m;
}







// Extracts the rotation matrix (3x3) from a 4x4 matrix
inline mat3 extract_rotation_mat4(mat4 src, bool normalize=false)
{
	mat3 dst;

	if (normalize) {
		dst.setc1(src.c1().tovec3().norm());
		dst.setc2(src.c2().tovec3().norm());
		dst.setc3(src.c3().tovec3().norm());
	} else {
		dst.setc1(src.c1().tovec3());
		dst.setc2(src.c2().tovec3());
		dst.setc3(src.c3().tovec3());
	}
	return dst;
}

///////////////////////////
//GLSL Functions

inline float clamp_01(float f)
{
	if (f < 0.0f) return 0.0f;
	if (f > 1.0f) return 1.0f;
	return f;
}
inline float clamp(float x, float minVal, float maxVal)
{
	if (x < minVal) return minVal;
	if (x > maxVal) return maxVal;
	return x;
}

#define VECTORIZE2_VEC_STD(func) \
inline vec2 func(vec2 v) \
{ \
	return vec2(std::func(v.x), std::func(v.y)); \
}
#define VECTORIZE3_VEC_STD(func) \
inline vec3 func(vec3 v) \
{ \
	return vec3(std::func(v.x), std::func(v.y), std::func(v.z)); \
}
#define VECTORIZE4_VEC_STD(func) \
inline vec4 func(vec4 v) \
{ \
	return vec4(std::func(v.x), std::func(v.y), std::func(v.z), std::func(v.w)); \
}

#define VECTORIZE_VEC_STD(func) \
	VECTORIZE2_VEC_STD(func) \
	VECTORIZE3_VEC_STD(func) \
	VECTORIZE4_VEC_STD(func)


#define VECTORIZE2_VEC(func) \
inline vec2 func(vec2 v) \
{ \
	return vec2(func(v.x), func(v.y)); \
}
#define VECTORIZE3_VEC(func) \
inline vec3 func(vec3 v) \
{ \
	return vec3(func(v.x), func(v.y), func(v.z)); \
}
#define VECTORIZE4_VEC(func) \
inline vec4 func(vec4 v) \
{ \
	return vec4(func(v.x), func(v.y), func(v.z), func(v.w)); \
}

#define VECTORIZE_VEC(func) \
	VECTORIZE2_VEC(func) \
	VECTORIZE3_VEC(func) \
	VECTORIZE4_VEC(func)

inline vec2 clamp(vec2 x, float minVal, float maxVal)
{
	return vec2(clamp(x.x, minVal, maxVal), clamp(x.y, minVal, maxVal));
}
inline vec3 clamp(vec3 x, float minVal, float maxVal)
{
	return vec3(clamp(x.x, minVal, maxVal), clamp(x.y, minVal, maxVal), clamp(x.z, minVal, maxVal));
}
inline vec4 clamp(vec4 x, float minVal, float maxVal)
{
	return vec4(clamp(x.x, minVal, maxVal), clamp(x.y, minVal, maxVal), clamp(x.z, minVal, maxVal), clamp(x.w, minVal, maxVal));
}


inline vec3 reflect(vec3 i, vec3 n)
{
	return i - 2 * dot(i, n) * n;
}

inline float smoothstep(float edge0, float edge1, float x)
{
	float t = clamp_01((x-edge0)/(edge1-edge0));
	return t*t*(3 - 2*t);
}


inline float mix(float x, float y, float a)
{
	return x*(1-a) + y*a;
}

inline vec2 mix(vec2 x, vec2 y, float a)
{
	return x*(1-a) + y*a;
}

inline vec3 mix(vec3 x, vec3 y, float a)
{
	return x*(1-a) + y*a;
}

inline vec4 mix(vec4 x, vec4 y, float a)
{
	return x*(1-a) + y*a;
}


inline vec2 max(vec2 a, vec2 b)
{
	return vec2(std::max(a.x, b.x), std::max(a.y, b.y));
}

inline vec3 max(vec3 a, vec3 b)
{
	return vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

inline vec4 max(vec4 a, vec4 b)
{
	return vec4(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w));
}

inline vec2 min(vec2 a, vec2 b)
{
	return vec2(std::min(a.x, b.x), std::min(a.y, b.y));
}

inline vec3 min(vec3 a, vec3 b)
{
	return vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

inline vec4 min(vec4 a, vec4 b)
{
	return vec4(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w));
}

VECTORIZE_VEC_STD(abs)
VECTORIZE_VEC_STD(floor)
VECTORIZE_VEC_STD(ceil)

VECTORIZE_VEC_STD(sin)
VECTORIZE_VEC_STD(cos)
VECTORIZE_VEC_STD(tan)
VECTORIZE_VEC_STD(asin)
VECTORIZE_VEC_STD(acos)
VECTORIZE_VEC_STD(atan)
VECTORIZE_VEC_STD(sinh)
VECTORIZE_VEC_STD(cosh)
VECTORIZE_VEC_STD(tanh)

inline float radians(float degrees) { return DEG_TO_RAD(degrees); }
inline float degrees(float radians) { return RAD_TO_DEG(radians); }
inline float fract(float x) { return x - std::floor(x); }

VECTORIZE_VEC(radians)
VECTORIZE_VEC(degrees)
VECTORIZE_VEC(fract)











/*
 *
 * MISC functions
 */
// Returns the same number if it is a power of
// two. Returns a larger integer if it is not a
// power of two. The larger integer is the next
// highest power of two.
inline unsigned int is_pow2(unsigned int val)
{
	unsigned int pow2 = 1;

	while (val > pow2)
		pow2 = (pow2 << 1);

	return pow2;
}



struct Color
{
	Color()
	{
		r = g = b = 0;
		a = 255;
	}
	Color(u8 red, u8 green, u8 blue, u8 alpha=255) : r(red), g(green), b(blue), a(alpha) {}

	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

inline Color vec4_to_Color(vec4 v)
{
	Color c;
	//assume all in the range of [0, 1]
	c.r = v.x * 255;
	c.g = v.y * 255;
	c.b = v.z * 255;
	c.a = v.w * 255;
	return c;
}

inline vec4 Color_to_vec4(Color c)
{
	return vec4((float)c.r/255, (float)c.g/255, (float)c.b/255, (float)c.a/255);
}

struct Line
{
	Line(float x1, float y1, float x2, float y2)
	{
		A = y1 - y2;
		B = x2 - x1;
		C = x1*y2 - x2*y1;
	}

	float func(float x, float y)
	{
		return A*x + B*y + C;
	}

	float findy(float x)
	{
		return -(A*x + C)/B;
	}

	float findx(float y)
	{
		return -(B*y + C)/A;
	}

	float A, B, C;

};


struct Plane
{
	vec3 n;	//normal  points x on plane satisfy n dot x = d
	float d; //d = n dot p

	Plane() {}
	Plane(vec3 a, vec3 b, vec3 c)	//ccw winding
	{
		n = cross(b-a, c-a).norm();
		d = dot(n, a);
	}
};


int intersect_segment_plane(vec3 a, vec3 b, Plane p, float& t, vec3& q);


}

/* RSW_MATH_H */
#endif
