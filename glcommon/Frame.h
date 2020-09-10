#pragma once
#ifndef GLFRAME_H_
#define GLFRAME_H_

#include "rsw_math.h"

using rsw::vec3;
using rsw::vec4;
using rsw::mat4;
using rsw::mat3;

struct Frame
{
	vec3 origin;	// Where am I?
	vec3 forward;	// Where am I going?
	vec3 up;		// Which way is up?

	Frame(bool camera=false, vec3 origin_ = vec3());

	vec3 get_z() { return forward; }
	vec3 get_y() { return up; }
	vec3 get_x() { return cross(up, forward); }

	void translate_world(float x, float y, float z)
		{ origin.x += x; origin.y += y; origin.z += z; }

	void translate_local(float x, float y, float z)
		{ move_forward(z); move_up(y); move_right(x);	}

	void move_forward(float fDelta) { origin += forward * fDelta; }
	void move_up(float fDelta) { origin += up * fDelta; }

	void move_right(float fDelta)
	{
		vec3 cross = rsw::cross(up, forward);
		origin += cross * fDelta;
	}

	mat4 get_matrix(bool bRotationOnly = false);
	mat4 get_camera_matrix(bool bRotationOnly = false);
	

	void rotate_local_y(float fAngle);
	void rotate_local_z(float fAngle);
	void rotate_local_x(float fAngle);

	void normalize(bool keep_forward);

	void rotate_world(float fAngle, float x, float y, float z);
	void rotate_local(float fAngle, float x, float y, float z);

	vec3 local_to_world(const vec3 vLocal, bool bRotOnly = false);


};

#endif /* GLFRAME_H_ */
