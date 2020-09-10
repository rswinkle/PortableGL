#ifndef CURVES_H
#define CURVES_H


#include "rmath.h"
#include "TriangleMesh.h"
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




class bezier3D
{
public:
	vector<vec3> controls;
	vector<vec3> glpoints;
	GLuint buffer_objects[2];
	GLuint gl_controls, line;
	
	
	bezier3D() {}
	void compute_glpoints(unsigned int segments);
	void end();
	void draw(mat4 mvp, int shader, bool controls = false);

	
};



class bezier_patch
{
public:
	vec3 *controls;
	Mesh bez_mesh;
	
	
	
	bezier_patch() {}
	bezier_patch(unsigned int m, unsigned int n);
};


//could just create a static structure of first 20 levels of pascals triangle or something
unsigned int fact(unsigned int n);

// float fact(unsigned int n)
// {
// 	unsigned int result = 1;
// 	while (n) {
// 		result *= n--;
// 	}
// 	return float(result);
// }




float bernstein(unsigned int n, unsigned int i, float t);


//I meant to copy controls.  don't want a reference . . . should I use NULL, 0, (void*)0, or nullptr?
vec3 de_casteljau(vector<vec3> controls, float t, vec3* tangent = NULL);








#endif