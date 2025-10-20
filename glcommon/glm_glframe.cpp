
#include "glm_glframe.h"

#include <glm/gtc/matrix_transform.hpp>

using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;

// Default position and orientation. At the origin, looking
// down the positive Z axis (right handed coordinate system.
GLFrame::GLFrame(bool camera, vec3 orig)
{
	// At origin
	origin = orig;

	// Up is up (+Y)
	up.x = 0.0f; up.y = 1.0f; up.z = 0.0f;

	// Forward is Z unless this is a camera frame
	// TODO try left handed system, or a perspective proj matrix
	// that doesn't negate Z?
	if (!camera) {
		forward.x = 0.0f; forward.y = 0.0f; forward.z = 1.0f;
	} else {
		forward.x = 0.0f; forward.y = 0.0f; forward.z = -1.0f;
	}
}



mat4 GLFrame::get_matrix(bool rotation_only)
{
	vec3 x_axis = glm::cross(up, forward);

	mat4 matrix(1);
	matrix[0] = vec4(x_axis, 0);
	matrix[1] = vec4(up, 0);
	matrix[2] = vec4(forward, 0);
	/*
	matrix.setc1(x_axis);
	matrix.setc2(up);
	matrix.setc3(forward);
	*/

	// Translation (already done)
	if(!rotation_only)
		matrix[3] = vec4(origin, 0);
		//matrix.setc4(origin);

	return matrix;
}



mat4 GLFrame::get_camera_matrix(bool rotation_only)
{
	vec3 x, z;
	mat4 mat(1);

	// Make rotation matrix
	// Z vector is reversed due to pespective projection matrix
	z = -forward;

	x = glm::cross(up, z);

	// Matrix has no translation information and is
	// transposed.... (rows instead of columns)
	mat[0] = vec4(x.x, up.x, z.x, 0);
	mat[1] = vec4(x.y, up.y, z.y, 0);
	mat[2] = vec4(x.z, up.z, z.z, 0);
	//mat[3] already set
	

	if (rotation_only)
		return mat;

	// Apply translation too
	mat4 trans(1);
	trans[3] = vec4(-origin, 1);

	//TODO
	//could instead of having the previous 2 lines just mat*(-origin) and drop the result
	//in column 4 of mat.  I think that actually saves mult ops

	return mat*trans;
}


void GLFrame::rotate_local_y(float angle)
{
	mat4 rot_mat4(1.0f);

	// Just Rotate around the up vector
	// Create a rotation matrix around my Up (Y) vector
	mat3 rot_mat = mat3(glm::rotate(rot_mat4, angle, up));

	// Rotate forward pointing vector
	forward = rot_mat * forward;

}


void GLFrame::rotate_local_z(float angle)
{
	mat4 rot_mat4(1.0f);

	// Only the up vector needs to be rotated
	mat3 rot_mat = mat3(glm::rotate(rot_mat4, angle, forward));
	up = rot_mat * up;
}


void GLFrame::rotate_local_x(float angle)
{
	//get local x axis
	vec3 local_x = glm::cross(up, forward);

	mat4 rot_mat4(1.0f);
	mat3 rot_mat = mat3(glm::rotate(rot_mat4, angle, local_x));

	//have to rotate both up and forward vectors
	up = rot_mat * up;
	forward = rot_mat * forward;
}


// Reset axes to make sure they are orthonormal. This should be called on occasion
// if the matrix is long-lived and frequently transformed.
void GLFrame::normalize(bool keep_forward)
{
	vec3 cross;

	//calculate cross product of up and forward vectors (local x axis
	//use the result to recalculate forward vector
	if (!keep_forward) {
		cross = glm::cross(up, forward);
		forward = glm::cross(cross, up);
	} else {
		cross = glm::cross(forward, up);
		up = glm::cross(cross, forward);
	}

	//normalize
	up = glm::normalize(up);
	forward = glm::normalize(forward);
}


// Rotate in world coordinates...
//Rotates in place around a vector in world coordinates
//Does NOT rotate the frame itself around the world origin (ie the pos of the frame doesn't change)
void GLFrame::rotate_world(float angle, float x, float y, float z)
{
	mat4 rot_mat4(1);

	// Create the Rotation matrix
	mat3 rot_mat = mat3(glm::rotate(rot_mat4, angle, vec3(x,y,z)));

	//transform up and forward axis
	up = rot_mat * up;
	forward = rot_mat * forward;
}


// Rotate around a local axis
void GLFrame::rotate_local(float angle, float x, float y, float z)
{
	vec3 local_vec(x, y, z);
	vec3 world_vec = local_to_world(local_vec, true);
	rotate_world(angle, world_vec.x, world_vec.y, world_vec.z);
}


// Convert Coordinate Systems
// This is pretty much, do the transformation represented by the rotation
// and position on the point
// Is it better to stick to the convention that the destination always comes
// first, or use the conventions that "sounds" like the function...
vec3 GLFrame::local_to_world(const vec3 local, bool rot_only)
{
	// Create the rotation matrix based on the vectors
	mat3 rot_mat = mat3(get_matrix(true));

	// Do the rotation
	vec3 world = rot_mat * local;

	// Translate the point
	if(!rot_only)
		world += origin;

	return world;
}


