#pragma once
#ifndef GLM_GLFRAME_H
#define GLM_GLFRAME_H

#include <glm/glm.hpp>


struct GLFrame
{
	glm::vec3 origin;
	glm::vec3 forward;
	glm::vec3 up;

	GLFrame(bool camera=false, glm::vec3 orig = glm::vec3(0));

	glm::vec3 get_z() { return forward; }
	glm::vec3 get_y() { return up; }
	glm::vec3 get_x() { return glm::cross(up, forward); }

	void translate_world(float x, float y, float z)
		{ origin.x += x; origin.y += y; origin.z += z; }

	void translate_local(float x, float y, float z)
		{ move_forward(z); move_up(y); move_right(x); }

	void move_forward(float delta) { origin += forward * delta; }
	void move_up(float delta) { origin += up * delta; }

	void move_right(float delta)
	{
		glm::vec3 cross = glm::cross(up, forward);
		origin += cross * delta;
	}

	glm::mat4 get_matrix(bool rotation_only = false);
	glm::mat4 get_camera_matrix(bool rotation_only = false);
	

	void rotate_local_y(float angle);
	void rotate_local_z(float angle);
	void rotate_local_x(float angle);

	void normalize(bool keep_forward);

	void rotate_world(float angle, float x, float y, float z);
	void rotate_local(float angle, float x, float y, float z);

	glm::vec3 local_to_world(const glm::vec3 local, bool rot_only = false);


};

#endif


