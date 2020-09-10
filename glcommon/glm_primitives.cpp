#include "glm_primitives.h"

//#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define RM_PI 3.14159265358979323846264338327950288
#define RM_2PI 6.28318530717958647692528676655900576


void generate_box(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float dimX, float dimY, float dimZ, bool plane, ivec3 seg, vec3 origin)
{
	vec3 x_vec(dimX, 0, 0);
	vec3 y_vec(0, dimY, 0);
	vec3 z_vec(0, 0, dimZ);

	if (!plane) {

		verts.push_back(origin);
		verts.push_back(origin+x_vec);
		verts.push_back(origin+y_vec);
		verts.push_back(origin+x_vec+y_vec);

		verts.push_back(origin+z_vec);
		verts.push_back(origin+x_vec+z_vec);
		verts.push_back(origin+y_vec+z_vec);
		verts.push_back(origin+x_vec+y_vec+z_vec);

		//back face
		tris.push_back(ivec3(0, 2, 3));
		tex.push_back(vec2(1, 1));
		tex.push_back(vec2(1, 0));
		tex.push_back(vec2(0, 0));

		tris.push_back(ivec3(0, 3, 1));
		tex.push_back(vec2(1, 1));
		tex.push_back(vec2(1, 0));
		tex.push_back(vec2(0, 1));

		//left face
		tris.push_back(ivec3(0, 6, 2));
		tex.push_back(vec2(0, 1));
		tex.push_back(vec2(1, 0));
		tex.push_back(vec2(0, 0));

		tris.push_back(ivec3(0, 4, 6));
		tex.push_back(vec2(0, 1));
		tex.push_back(vec2(1, 1));
		tex.push_back(vec2(1, 0));


		//bottom face
		tris.push_back(ivec3(0, 1, 5));
		tex.push_back(vec2(0, 1));
		tex.push_back(vec2(1, 1));
		tex.push_back(vec2(1, 0));

		tris.push_back(ivec3(0, 5, 4));
		tex.push_back(vec2(0, 1));
		tex.push_back(vec2(1, 0));
		tex.push_back(vec2(0, 0));


		//top face
		tris.push_back(ivec3(7, 3, 2));
		tex.push_back(vec2(1, 1));
		tex.push_back(vec2(1, 0));
		tex.push_back(vec2(0, 0));
	//
		tris.push_back(ivec3(7, 2, 6));
		tex.push_back(vec2(1, 1));
		tex.push_back(vec2(0, 0));
		tex.push_back(vec2(0, 1));


		//right face
		tris.push_back(ivec3(7, 1, 3));
		tex.push_back(vec2(0, 0));
		tex.push_back(vec2(1, 1));
		tex.push_back(vec2(1, 0));
	//
		tris.push_back(ivec3(7, 5, 1));
		tex.push_back(vec2(0, 0));
		tex.push_back(vec2(0, 1));
		tex.push_back(vec2(1, 1));


		//front face
		tris.push_back(ivec3(7, 6, 4));
		tex.push_back(vec2(1, 0));
		tex.push_back(vec2(0, 0));
		tex.push_back(vec2(0, 1));

		tris.push_back(ivec3(7, 4, 5));
		tex.push_back(vec2(1, 0));
		tex.push_back(vec2(0, 1));
		tex.push_back(vec2(1, 1));
	} else {
		float segx = seg.x, segy = seg.y, segz = seg.z;
		//front and back
		generate_plane(verts, tris, tex, origin+x_vec, y_vec/segy, (-x_vec)/segx, segy, segx, true);
		generate_plane(verts, tris, tex, origin+z_vec, y_vec/segy, x_vec/segx, segy, segx, true);
		//left and right
		generate_plane(verts, tris, tex, origin, y_vec/segy, z_vec/segz, segy, segz, true);
		generate_plane(verts, tris, tex, origin+x_vec+z_vec, y_vec/segy, (-z_vec)/segz, segy, segz, true);
		//top and bottom
		generate_plane(verts, tris, tex, origin, z_vec/segz, x_vec/segx, segz, segx, true);
		generate_plane(verts, tris, tex, origin+y_vec+z_vec, (-z_vec)/segz, x_vec/segx, segz, segx, true);

	}
}




void generate_cylinder(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float radius, float height, size_t slices, size_t stacks, float top_radius)
{
	int i = 0, j = 0;

	float stack_height = height/stacks;
	float cur_radius;
	double theta = RM_2PI/slices;


	verts.push_back(vec3(0, 0, 0));

	for (i = 0; i <= stacks; i++) {
		cur_radius = i/stacks * top_radius + (1.0 - i/stacks) * radius;
		for (j = 0; j < slices; j++) {
			verts.push_back(vec3(cur_radius*cos(j*theta), cur_radius*sin(j*theta), i*stack_height));
		}
	}

	verts.push_back(vec3(0, 0, height));

//TODO do this in order
//reversed this cause I accidentally figured it out the other way then noticed it'd be clockwise so just reversed it.
//I'll figure it out for increasing later.
	for (i = slices; i > 0; i--) {
		tris.push_back(ivec3(0, (((i+1)%(slices+1)==0) ? 1 : i+1), i));
		tex.push_back(vec2(0.5, 0.5));
//
		if ((i+1) % (slices+1) == 0)
			tex.push_back(vec2(0.5 + 0.5, 0.5));
		else
			tex.push_back(vec2(0.5 + 0.5*cos(i*theta), 0.5+0.5*sin(i*theta)));

		tex.push_back(vec2(0.5 + 0.5*cos((i-1)*theta), 0.5+0.5*sin((i-1)*theta)));
	}


	for (j = 0; j < stacks; j++) {
		for (i = 1; i <= slices; i++) {
			if (i != slices) {
				tris.push_back(ivec3(i+j*slices, (i+1)+j*slices, i+(j+1)*slices));
				tex.push_back(vec2(float(i-1)/float(slices), float(j)/float(stacks)));
				tex.push_back(vec2(float(i)/float(slices), float(j)/float(stacks)));
				tex.push_back(vec2(float(i-1)/float(slices), float(j+1)/float(stacks)));


				tris.push_back(ivec3((i+1)+j*slices, (i+1)+(j+1)*slices, i+(j+1)*slices));
				tex.push_back(vec2(float(i)/float(slices), float(j)/float(stacks)));
				tex.push_back(vec2(float(i)/float(slices), float(j+1)/float(stacks)));
				tex.push_back(vec2(float(i-1)/float(slices), float(j+1)/float(stacks)));

			} else {
				tris.push_back(ivec3(i+j*slices, (i-slices+1)+j*slices, i+(j+1)*slices));
				tex.push_back(vec2(float(i-1)/float(slices), float(j)/float(stacks)));
				tex.push_back(vec2(float(i)/float(slices), float(j)/float(stacks)));
				tex.push_back(vec2(float(i-1)/float(slices), float(j+1)/float(stacks)));


				tris.push_back(ivec3((i-slices+1)+j*slices, (i+1)+j*slices, i+(j+1)*slices));
				tex.push_back(vec2(float(i)/float(slices), float(j)/float(stacks)));
				tex.push_back(vec2(float(i)/float(slices), float(j+1)/float(stacks)));
				tex.push_back(vec2(float(i-1)/float(slices), float(j+1)/float(stacks)));

			}
		}
	}


	int top_center = verts.size() - 1;

	j = 0;
	for (i = top_center - slices; i < top_center; i++, j++) {
		tris.push_back(ivec3(top_center, i, ( ((i+1)==top_center)? top_center-slices : i+1 ) ));

		tex.push_back(vec2(0.5, 0.5));
		tex.push_back(vec2(0.5 + 0.5*cos(j*theta), 0.5 + 0.5*sin(j*theta) ));

		if ((i+1) != top_center)
			tex.push_back(vec2(0.5 + 0.5*cos((j+1)*theta), 0.5 + 0.5*sin((j+1)*theta) ));
		else
			tex.push_back(vec2(0.5 + 0.5, 0.5));
	}
}




void generate_plane(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, vec3 corner, vec3 v1, vec3 v2, size_t dimV1, size_t dimV2, bool tile)
{
	//TODO should check here if v1 and v2 are too close to the same direction
	int i = 0;

	int orig_size = verts.size();

	for (i = 0; i <= dimV2; i++) {
		for (int j=0; j <= dimV1; j++) {
			verts.push_back(vec3(corner + v2*float(i) + v1*float(j)));
		}
	}

	int j = -1;

	for (i = orig_size; i < orig_size+dimV1*dimV2; i++) {
		if ((i-orig_size) % dimV1 == 0)
			j++;

		tris.push_back(ivec3(i+j, i+j+dimV1+1, i+j+dimV1+2));
		tris.push_back(ivec3(i+j, i+j+dimV1+2, i+j+1));

		if (!tile) {
			tex.push_back(vec2(float(i%dimV1)/dimV1, float(j)/dimV2));
			tex.push_back(vec2(float(i%dimV1)/dimV1, float(j+1)/dimV2));
			tex.push_back(vec2(float(i%dimV1 + 1)/dimV1, float(j+1)/dimV2));

			tex.push_back(vec2(float(i%dimV1)/dimV1, float(j)/dimV2));
			tex.push_back(vec2(float(i%dimV1 + 1)/dimV1, float(j+1)/dimV2));
			tex.push_back(vec2(float(i%dimV1 + 1)/dimV1, float(j)/dimV2));

		} else {
			//just increment per box, tile by setting texture to wrap
			tex.push_back(vec2(i%dimV1, j));
			tex.push_back(vec2(i%dimV1, j+1));
			tex.push_back(vec2(i%dimV1+1, j+1));

			tex.push_back(vec2(i%dimV1, j));
			tex.push_back(vec2(i%dimV2 + 1, j+1));
			tex.push_back(vec2(i%dimV2 + 1, j));
		}
	}
}






void generate_sphere(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float radius, size_t slices, size_t stacks)
{
	float down = RM_PI/float(stacks);
	float around = RM_2PI/float(slices);

	mat4 rotdown4(1), rotaround4(1);
	mat3 rotdown = mat3(glm::rotate(rotdown4, down, vec3(0.0f, 1.0f, 0.0f)));
	mat3 rotaround = mat3(glm::rotate(rotaround4, around, vec3(0.0f, 0.0f, 1.0f)));

	vec3 point(0, 0, radius);
	vec3 tmp;

	verts.push_back(vec3(0, 0, radius));

	for (int i=1; i<stacks; ++i) {

		//rotate down to next stack and add first point (on x axis)
		point = rotdown*point;
		verts.push_back(vec3(point));
		tmp = point;

		for (int j=1; j<slices; ++j) {
			point = rotaround*point;
			verts.push_back(vec3(point));
		}
		point = tmp; //set back to y (copying probably quicker than rotating again)
	}

	verts.push_back(vec3(0, 0, -radius));


	//add top cap triangles
	for (int i=1; i<slices+1; ++i) {
		if (i != slices) {
			tris.push_back(ivec3(0, i, i+1));
			tex.push_back(vec2(float(i-1)/float(slices), 1));
			tex.push_back(vec2(float(i-1)/float(slices), float(stacks-1)/float(stacks)));
			tex.push_back(vec2(float(i)/float(slices), float(stacks-1)/float(stacks)));
		} else {
			tris.push_back(ivec3(0, i, 1));
			tex.push_back(vec2(float(i-1)/float(slices), 1));
			tex.push_back(vec2(float(i-1)/float(slices), float(stacks-1)/float(stacks)));
			tex.push_back(vec2(1, float(stacks-1)/float(stacks)));
		}
	}

	int corner;
	for (int i=1 ; i<stacks-1 ; ++i) {
		for (int j=0; j<slices; ++j) {

			corner = i*slices+1 + j;
			if (j != slices-1) {
				tris.push_back(ivec3(corner, corner+1, corner-slices));
				tris.push_back(ivec3(corner+1, (corner+1)-slices, corner-slices));

				tex.push_back(vec2(float(j)/float(slices), float(stacks-i-1)/float(stacks)));
				tex.push_back(vec2(float(j+1)/float(slices), float(stacks-i-1)/float(stacks)));
				tex.push_back(vec2(float(j)/float(slices), float(stacks-i)/float(stacks)));

				tex.push_back(vec2(float(j+1)/float(slices), float(stacks-i-1)/float(stacks)));
				tex.push_back(vec2(float(j+1)/float(slices), float(stacks-i)/float(stacks)));
				tex.push_back(vec2(float(j)/float(slices), float(stacks-i)/float(stacks)));

			} else {
				tris.push_back(ivec3(corner, i*slices+1, corner-slices));
				tris.push_back(ivec3(i*slices+1, (i-1)*slices+1, corner-slices));

				tex.push_back(vec2(float(j)/float(slices), float(stacks-i-1)/float(stacks)));
				tex.push_back(vec2(1, float(stacks-i-1)/float(stacks)));
				tex.push_back(vec2(float(j)/float(slices), float(stacks-i)/float(stacks)));

				tex.push_back(vec2(1, float(stacks-i-1)/float(stacks)));
				tex.push_back(vec2(1, float(stacks-i)/float(stacks)));
				tex.push_back(vec2(float(j)/float(slices), float(stacks-i)/float(stacks)));
			}
		}
	}

	//add bottom cap
	int bottom = verts.size()-1;
	for (int i=0; i<slices; ++i) {
		if (i != 0) {
			tris.push_back(ivec3(bottom, bottom-i, bottom-i-1));

			tex.push_back(vec2(float(slices-i)/float(slices), 0));
			tex.push_back(vec2(float(slices-i)/float(slices), float(1)/float(stacks)));
			tex.push_back(vec2(float(slices-i-1)/float(slices), float(1)/float(stacks)));

		} else {
			tris.push_back(ivec3(bottom, bottom-slices, bottom-1));

			tex.push_back(vec2(1, 0));
			tex.push_back(vec2(1, float(1)/float(stacks)));
			tex.push_back(vec2(float(slices-1)/float(slices), float(1)/float(stacks)));
		}
	}
}


// Draw a torus (doughnut) in xy plane
void generate_torus(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float major_r, float minor_r, size_t major_slices, size_t minor_slices)
{
    double major_step = RM_2PI / major_slices;
    double minor_step = RM_2PI / minor_slices;
    int i, j;
	
	double a0, a1, b;
	float x0, y0, x1, y1, c, r, z;


	for (i=0; i<major_slices; ++i) {
		a0 = i * major_step;
		a1 = a0 + major_step;
		x0 = (float) cos(a0);
		y0 = (float) sin(a0);
		x1 = (float) cos(a1);
		y1 = (float) sin(a1);

		for (j=0; j<minor_slices; ++j) {
			b = j * minor_step;
			c = (float) cos(b);
			r = minor_r * c + major_r;
			z = minor_r * (float) sin(b);

			verts.push_back(vec3(x0 * r, y0 * r, z));
		}
	}
		
	int s;
	for (i=0; i<major_slices; ++i) {
		s = i*minor_slices;
		if (i != major_slices-1) {
			for (j=0; j<minor_slices; ++j) {
				if (j != minor_slices-1) {
					tris.push_back(ivec3(s+j, s+j+1+minor_slices, s+j+1));
					tris.push_back(ivec3(s+j, s+j+minor_slices, s+j+1+minor_slices));
				} else {
					tris.push_back(ivec3(s+j, s+minor_slices, s));
					tris.push_back(ivec3(s+j, s+j+minor_slices, s+minor_slices));
				}
				tex.push_back(vec2((float)i/(float)major_slices, (float)j/(float)minor_slices));
				tex.push_back(vec2((float)(i+1)/(float)major_slices, (float)(j+1)/(float)minor_slices));
				tex.push_back(vec2((float)i/(float)major_slices, (float)(j+1)/(float)minor_slices));

				tex.push_back(vec2((float)i/(float)major_slices, (float)j/(float)minor_slices));
				tex.push_back(vec2((float)(i+1)/(float)major_slices, (float)j/(float)minor_slices));
				tex.push_back(vec2((float)(i+1)/(float)major_slices, (float)(j+1)/(float)minor_slices));
			}
		} else {
			for (j=0; j<minor_slices; ++j) {
				if (j != minor_slices-1) {
					tris.push_back(ivec3(s+j, j+1, s+j+1));
					tris.push_back(ivec3(s+j, j, j+1));
				} else {
					tris.push_back(ivec3(s+j, 0, s));
					tris.push_back(ivec3(s+j, j, 0));
				}
				tex.push_back(vec2((float)i/(float)major_slices, (float)j/(float)minor_slices));
				tex.push_back(vec2((float)(i+1)/(float)major_slices, (float)(j+1)/(float)minor_slices));
				tex.push_back(vec2((float)i/(float)major_slices, (float)(j+1)/(float)minor_slices));

				tex.push_back(vec2((float)i/(float)major_slices, (float)j/(float)minor_slices));
				tex.push_back(vec2((float)(i+1)/(float)major_slices, (float)j/(float)minor_slices));
				tex.push_back(vec2((float)(i+1)/(float)major_slices, (float)(j+1)/(float)minor_slices));
			}

		}
	}
}



void generate_cone(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float radius, float height, size_t slices, float top_radius)
{
	size_t i;
	vec3 tmp(radius, 0, 0);
	float deg = RM_PI/float(slices);
	mat4 rot_mat4;
	mat3 rot_mat = mat3(glm::rotate(rot_mat4, deg, vec3(0,0,1)));

	verts.push_back(vec3(0, 0, height));
	for (i=0; i<slices; i++) {
		verts.push_back(tmp);
		tmp = rot_mat * tmp;
	}

	verts.push_back(vec3(0, 0, 0));
}


void expand_verts(vector<vec3>& draw_verts, vector<vec3>& verts, vector<ivec3>& triangles)
{
	for (int i=0; i<triangles.size(); ++i) {
		draw_verts.push_back(verts[triangles[i].x]);
		draw_verts.push_back(verts[triangles[i].y]);
		draw_verts.push_back(verts[triangles[i].z]);
	}
}


void expand_tex(vector<vec2>& draw_tex, vector<vec2>& tex, vector<ivec3>& triangles)
{
	for (int i=0; i<triangles.size(); ++i) {
		draw_tex.push_back(tex[triangles[i].x]);
		draw_tex.push_back(tex[triangles[i].y]);
		draw_tex.push_back(tex[triangles[i].z]);
	}
}




