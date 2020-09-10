#ifndef MESH_INSTANCE_H
#define MESH_INSTANCE_H

#include "GLFrame.h"

class mesh_instance
{
public:
	GLFrame frame;
	vec3 scale;


	
// 	object()
// 	{
// 		scale = 1;
// 	}
	
	mesh_instance(GLFrame f=GLFrame(), vec3 s=vec3(1,1,1)) {
		frame = f;
		scale = s;
	}
	
	mat4 get_matrix()
	{
		mat4 scale_mat = rm::ScaleMatrix44(scale);
		return frame.get_matrix()*scale_mat;
	}
	
	
	
};




#endif
