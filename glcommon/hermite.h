#ifndef HERMITE_H
#define HERMITE_H

#include "rmath.h"
#include <vector>
#include <GL/glew.h>

namespace rm = robert3dmath;
using std::vector;
using rm::vec3;
using rm::ivec3;
using rm::vec4;
using rm::vec2;
using rm::mat4;
using rm::mat3;


class coef
{
	public:
	vec3 a;
	vec3 b;
	vec3 c;
	vec3 d;


	coef() { };
	coef(vec3 A, vec3 B, vec3 C, vec3 D)
	{
		a = A;
		b = B;
		c = C;
		d = D;
	}
};

class hermite
{
	public:
	mat4 hermite_matrix;
	GLuint vertex_array;
	GLuint line;
	GLuint tangent_array;
	GLuint buffer_objects[3];

	vector<vec3> controls;
	vector<vec3> tangents;
	vector<vec3> tangent_lines;
	vector<vec3> glpoints;
	vector<vec3> coefficients;	//4 vec3s per coeficient matrix


	hermite();
	void reset(int num_controls, int segments, vec3 min, vec3 max, float tan_weight);
	void reset(int segments);
	void P_u(float u, int k, vec3 &pu);
	void get_tangent(float u, int k, vec3 &tan);
	void end();
	void draw(mat4 mvp, int shader);
};




#endif