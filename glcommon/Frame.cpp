
#include "Frame.h"


// Default position and orientation. At the origin, looking
// down the positive Z axis (right handed coordinate system.
Frame::Frame(bool camera, vec3 origin_)
{
	// At origin
// 		origin.x= 0.0f; origin.y = 0.0f; origin.z = 0.0f;
	origin = origin_;


	// Up is up (+Y)
	up.x = 0.0f; up.y = 1.0f; up.z = 0.0f;

	// Forward is Z unless this is a camera frame
	if (!camera) {
		forward.x = 0.0f; forward.y = 0.0f; forward.z = 1.0f;
	} else {
		forward.x = 0.0f; forward.y = 0.0f; forward.z = -1.0f;
	}
}



mat4 Frame::get_matrix(bool rotation_only)
{
	mat4 matrix;

	vec3 vXAxis = cross(up, forward);

	matrix.setc1(vXAxis);
	matrix.setc2(up);
	matrix.setc3(forward);

	// Translation (already done)
	if(rotation_only == true)
		matrix.setc4(vec3(0,0,0));
	else
		matrix.setc4(origin);

	return matrix;
}



mat4 Frame::get_camera_matrix(bool rotation_only)
{
	vec3 x, z;
	mat4 mat;

	// Make rotation matrix
	// Z vector is reversed
	//not sure this is right/necessary now that I changed forward to +Z
	//guess I'll find out eventually
	z = -forward;

	// X vector = Y cross Z
	x = cross(up, z);

	// Matrix has no translation information and is
	// transposed.... (rows instead of columns) TODO what does this mean?
	mat.setx(x);
	mat.sety(up);
	mat.setz(z);
	mat.setw(vec3(0,0,0));

	if(rotation_only)
		return mat;

	// Apply translation too
	mat4 trans;
	trans.setc4(-origin);	//default constructor loads identity so this is all we need to form translation

	//could instead of having the previous 2 lines just mat*(-origin) and drop the result
	//in column 4 of mat.  I think that actually saves mult ops

	return mat*trans;
}


void Frame::rotate_local_y(float fAngle)
{
	mat3 rotMat;

	// Just Rotate around the up vector
	// Create a rotation matrix around my Up (Y) vector
	load_rotation_mat3(rotMat, up, fAngle);

	// Rotate forward pointing vector
	forward = rotMat*forward;

}


void Frame::rotate_local_z(float fAngle)
{
	mat3 rotMat;

	// Only the up vector needs to be rotated
	load_rotation_mat3(rotMat, forward, fAngle);

	up = rotMat*up;
}


void Frame::rotate_local_x(float fAngle)
{
	mat3 rotMat;
	vec3 localX;
	//get local x axis
	localX = cross(up, forward);

	load_rotation_mat3(rotMat, localX, fAngle);
// 		std::cout<<rotMat<<"\n";
// 		int x;
// 		std::cin>>x;
	//have to rotate both up and forward vectors
	up = rotMat*up;
	forward = rotMat*forward;
}


// Reset axes to make sure they are orthonormal. This should be called on occasion
// if the matrix is long-lived and frequently transformed.
void Frame::normalize(bool keep_forward)
{
	vec3 vCross;

	//calculate cross product of up and forward vectors (local x axis
	//use the result to recalculate forward vector
	if (!keep_forward) {
		vCross = cross(up, forward);
		forward = cross(vCross, up);
	} else {
		vCross = cross(forward, up);
		up = cross(vCross, forward);
	}

	//normalize
	up.norm();
	forward.norm();
}


// Rotate in world coordinates...
//Rotates in place around a vector in world coordinates
//Does NOT rotate the frame itself around the world origin (ie the pos of the frame doesn't change)
void Frame::rotate_world(float fAngle, float x, float y, float z)
{
	mat3 rotMat;

	// Create the Rotation matrix
	load_rotation_mat3(rotMat, vec3(x,y,z), fAngle);

	//transform up and forward axis
	up = rotMat*up;
	forward = rotMat*forward;
}


// Rotate around a local axis
void Frame::rotate_local(float fAngle, float x, float y, float z)
{
	vec3 vWorldVect;
	vec3 vLocalVect(x, y, z);

	vWorldVect = local_to_world(vLocalVect, true);

	rotate_world(fAngle, vWorldVect.x, vWorldVect.y, vWorldVect.z);
}


// Convert Coordinate Systems
// This is pretty much, do the transformation represented by the rotation
// and position on the point
// Is it better to stick to the convention that the destination always comes
// first, or use the conventions that "sounds" like the function...
vec3 Frame::local_to_world(const vec3 vLocal, bool bRotOnly)
{
	vec3 vWorld;

		// Create the rotation matrix based on the vectors
	mat4 rotMat = get_matrix(true);

	// Do the rotation (inline it, and remove 4th column...)
	vWorld.x = rotMat[0] * vLocal.x + rotMat[4] * vLocal.y + rotMat[8] *  vLocal.z;
	vWorld.y = rotMat[1] * vLocal.x + rotMat[5] * vLocal.y + rotMat[9] *  vLocal.z;
	vWorld.z = rotMat[2] * vLocal.x + rotMat[6] * vLocal.y + rotMat[10] * vLocal.z;

	// Translate the point
	if(!bRotOnly)
		vWorld += origin;

	return vWorld;
}


	//THIS IS WHERE I AM!
	// Change world coordinates into "local" coordinates
//         vec3 WorldToLocal(const vec3 vWorld)
//         {
//
// 			////////////////////////////////////////////////
//             // Translate the origin
//         	vec3 local = vWorld - origin;
//
//         	matrix44 rotMat = get_matrix(true);
//
//
// 			M3DVector3f vNewWorld;
//             vNewWorld[0] = vWorld[0] - origin[0];
//             vNewWorld[1] = vWorld[1] - origin[1];
//             vNewWorld[2] = vWorld[2] - origin[2];
//
//             // Create the rotation matrix based on the vectors
// 			M3DMatrix44f rotMat;
//             M3DMatrix44f invMat;
// 			get_matrix(rotMat, true);
//
// 			// Do the rotation based on inverted matrix
//             m3dInvertMatrix44(invMat, rotMat);
//
// 			vLocal[0] = invMat[0] * vNewWorld[0] + invMat[4] * vNewWorld[1] + invMat[8] *  vNewWorld[2];
// 			vLocal[1] = invMat[1] * vNewWorld[0] + invMat[5] * vNewWorld[1] + invMat[9] *  vNewWorld[2];
// 			vLocal[2] = invMat[2] * vNewWorld[0] + invMat[6] * vNewWorld[1] + invMat[10] * vNewWorld[2];
//         }
//
//         /////////////////////////////////////////////////////////////////////////////
//         // Transform a point by frame matrix
//         void TransformPoint(M3DVector3f vPointSrc, M3DVector3f vPointDst)
//             {
//             M3DMatrix44f m;
//             get_matrix(m, false);    // Rotate and translate
//             vPointDst[0] = m[0] * vPointSrc[0] + m[4] * vPointSrc[1] + m[8] *  vPointSrc[2] + m[12];// * v[3];
//             vPointDst[1] = m[1] * vPointSrc[0] + m[5] * vPointSrc[1] + m[9] *  vPointSrc[2] + m[13];// * v[3];
//             vPointDst[2] = m[2] * vPointSrc[0] + m[6] * vPointSrc[1] + m[10] * vPointSrc[2] + m[14];// * v[3];
//             }
//
//         ////////////////////////////////////////////////////////////////////////////
//         // Rotate a vector by frame matrix
//         void RotateVector(M3DVector3f vVectorSrc, M3DVector3f vVectorDst)
//             {
//             M3DMatrix44f m;
//             get_matrix(m, true);    // Rotate only
//
//             vVectorDst[0] = m[0] * vVectorSrc[0] + m[4] * vVectorSrc[1] + m[8] *  vVectorSrc[2];
//             vVectorDst[1] = m[1] * vVectorSrc[0] + m[5] * vVectorSrc[1] + m[9] *  vVectorSrc[2];
//             vVectorDst[2] = m[2] * vVectorSrc[0] + m[6] * vVectorSrc[1] + m[10] * vVectorSrc[2];
//             }


