#include "glm_primitives.h"

//#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define RM_PI 3.14159265358979323846264338327950288
#define RM_2PI 6.28318530717958647692528676655900576


void make_box(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float dimX, float dimY, float dimZ, bool plane, ivec3 seg, vec3 origin)
{
	vec3 x_vec(dimX, 0, 0);
	vec3 y_vec(0, dimY, 0);
	vec3 z_vec(0, 0, dimZ);

	int vert_start = verts.size();
	int tri_start = tris.size();

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
		tex.push_back(vec2(0, 0));
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

		for (int i=tri_start; i<tris.size(); i++) {
			tris[i] += ivec3(vert_start);
		}
	} else {
		float segx = seg.x, segy = seg.y, segz = seg.z;
		//front and back
		make_plane(verts, tris, tex, origin+x_vec, y_vec/segy, (-x_vec)/segx, segy, segx, true);
		make_plane(verts, tris, tex, origin+z_vec, y_vec/segy, x_vec/segx, segy, segx, true);
		//left and right
		make_plane(verts, tris, tex, origin, y_vec/segy, z_vec/segz, segy, segz, true);
		make_plane(verts, tris, tex, origin+x_vec+z_vec, y_vec/segy, (-z_vec)/segz, segy, segz, true);
		//top and bottom
		make_plane(verts, tris, tex, origin, z_vec/segz, x_vec/segx, segz, segx, true);
		make_plane(verts, tris, tex, origin+y_vec+z_vec, (-z_vec)/segz, x_vec/segx, segz, segx, true);

	}
}


void make_cylinder(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float radius, float height, size_t slices)
{
	make_cylindrical(verts, tris, tex, radius, height, slices, 1, radius);
}

void make_cylindrical(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float radius, float height, size_t slices, size_t stacks, float top_radius)
{
	int i = 0, j = 0;

	float stack_height = height/stacks;
	float cur_radius;
	double theta = RM_2PI/slices;

	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(vec3(0, 0, 0));

	for (i = 0; i <= stacks; i++) {
		cur_radius = i/(float)stacks * top_radius + (1.0 - i/(float)stacks) * radius;
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

	int top_center = verts.size() - vert_start - 1;

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

	for (i=tri_start; i<tris.size(); i++) {
		tris[i] += ivec3(vert_start);
	}
}




void make_plane(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, vec3 corner, vec3 v1, vec3 v2, size_t dimV1, size_t dimV2, bool tile)
{
	//TODO should check here if v1 and v2 are too close to the same direction
	int i = 0;

	int vert_start = verts.size();
	int tri_start = tris.size();

	for (i = 0; i <= dimV2; i++) {
		for (int j=0; j <= dimV1; j++) {
			verts.push_back(vec3(corner + v2*float(i) + v1*float(j)));
		}
	}

	int j = -1;

	for (i = 0; i < dimV1*dimV2; i++) {
		if (i % dimV1 == 0)
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

	for (i=tri_start; i<tris.size(); i++) {
		tris[i] += ivec3(vert_start);
	}
}






void make_sphere(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float radius, size_t slices, size_t stacks)
{
	float down = RM_PI/float(stacks);
	float around = RM_2PI/float(slices);

	mat4 rotdown4(1), rotaround4(1);
	mat3 rotdown = mat3(glm::rotate(rotdown4, down, vec3(0.0f, 1.0f, 0.0f)));
	mat3 rotaround = mat3(glm::rotate(rotaround4, around, vec3(0.0f, 0.0f, 1.0f)));

	int vert_start = verts.size();
	int tri_start = tris.size();

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
	int bottom = verts.size()-vert_start-1;
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

	for (int i=tri_start; i<tris.size(); i++) {
		tris[i] += ivec3(vert_start);
	}
}


// Draw a torus (doughnut) in xy plane
void make_torus(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float major_r, float minor_r, size_t major_slices, size_t minor_slices)
{
    double major_step = RM_2PI / major_slices;
    double minor_step = RM_2PI / minor_slices;
    int i, j;

	double a0, a1, b;
	float x0, y0, x1, y1, c, r, z;

	int vert_start = verts.size();
	int tri_start = tris.size();

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

	for (i=tri_start; i<tris.size(); i++) {
		tris[i] += ivec3(vert_start);
	}
}



void make_cone(vector<vec3>& verts, vector<ivec3>& tris, vector<vec2>& tex, float radius, float height, size_t slices, size_t stacks, bool flip)
{
	if (!flip) {
		make_cylindrical(verts, tris, tex, radius, height, slices, stacks, 0.0f);
	} else {
		make_cylindrical(verts, tris, tex, 0.0f, height, slices, stacks, radius);
	}
}


void expand_verts(vector<vec3>& draw_verts, vector<vec3>& verts, vector<ivec3>& triangles)
{
	if (!triangles.empty()) {
		for (int i=0; i<triangles.size(); ++i) {
			draw_verts.push_back(verts[triangles[i].x]);
			draw_verts.push_back(verts[triangles[i].y]);
			draw_verts.push_back(verts[triangles[i].z]);
		}
	} else {
		draw_verts.assign(verts.begin(), verts.end());
	}
}


void expand_tex(vector<vec2>& draw_tex, vector<vec2>& tex, vector<ivec3>& triangles)
{
	if (!triangles.empty()) {
		for (int i=0; i<triangles.size(); ++i) {
			draw_tex.push_back(tex[triangles[i].x]);
			draw_tex.push_back(tex[triangles[i].y]);
			draw_tex.push_back(tex[triangles[i].z]);
		}
	} else {
		draw_tex.assign(tex.begin(), tex.end());
	}
}


// platonic solids, just verts and triangles

void make_tetrahedron(vector<vec3>& verts, vector<ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(vec3(1, 1, 1));
	verts.push_back(vec3(1, -1, -1));
	verts.push_back(vec3(-1, 1, -1));
	verts.push_back(vec3(-1, -1, 1));

	tris.push_back(ivec3(0, 1, 2));
	tris.push_back(ivec3(0, 2, 3));
	tris.push_back(ivec3(0, 3, 1));
	tris.push_back(ivec3(1, 3, 2));

	/*
	// other orientation
	verts.push_back(vec3(-1, 1, 1));
	verts.push_back(vec3(1, -1, 1));
	verts.push_back(vec3(1, 1, -1));
	verts.push_back(vec3(-1, -1, -1));

	tris.push_back(ivec3(0, 1, 2));
	tris.push_back(ivec3(0, 2, 3));
	tris.push_back(ivec3(0, 3, 1));
	tris.push_back(ivec3(1, 2, 3));

	*/

	for (int i=tri_start; i<tris.size(); i++) {
		tris[i] += ivec3(vert_start);
	}
}





void make_cube(vector<vec3>& verts, vector<ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(vec3(-1, -1, -1));
	verts.push_back(vec3(1, -1, -1));
	verts.push_back(vec3(-1, 1, -1));
	verts.push_back(vec3(1, 1, -1));

	verts.push_back(vec3(-1, -1, 1));
	verts.push_back(vec3(1, -1, 1));
	verts.push_back(vec3(-1, 1, 1));
	verts.push_back(vec3(1, 1, 1));

	//back face
	tris.push_back(ivec3(0, 2, 3));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));

	tris.push_back(ivec3(0, 3, 1));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(0, 1));

	//left face
	tris.push_back(ivec3(0, 6, 2));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));

	tris.push_back(ivec3(0, 4, 6));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));


	//bottom face
	tris.push_back(ivec3(0, 1, 5));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));

	tris.push_back(ivec3(0, 5, 4));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));


	//top face
	tris.push_back(ivec3(7, 3, 2));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));

	tris.push_back(ivec3(7, 2, 6));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(0, 1));


	//right face
	tris.push_back(ivec3(7, 1, 3));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));

	tris.push_back(ivec3(7, 5, 1));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 1));


	//front face
	tris.push_back(ivec3(7, 6, 4));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(0, 1));

	tris.push_back(ivec3(7, 4, 5));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 1));

	for (int i=tri_start; i<tris.size(); i++) {
		tris[i] += ivec3(vert_start);
	}
}

void make_octahedron(vector<vec3>& verts, vector<ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(vec3(1, 0, 0));
	verts.push_back(vec3(0, 0, -1));
	verts.push_back(vec3(-1, 0, 0));
	verts.push_back(vec3(0, 0, 1));
	verts.push_back(vec3(0, 1, 0));
	verts.push_back(vec3(0, -1, 0));

	tris.push_back(ivec3(0, 1, 4));
	tris.push_back(ivec3(1, 2, 4));
	tris.push_back(ivec3(2, 3, 4));
	tris.push_back(ivec3(3, 0, 4));

	tris.push_back(ivec3(1, 0, 5));
	tris.push_back(ivec3(2, 1, 5));
	tris.push_back(ivec3(3, 2, 5));
	tris.push_back(ivec3(0, 3, 5));

	for (int i=tri_start; i<tris.size(); i++) {
		tris[i] += ivec3(vert_start);
	}
}


#define phi 1.618
void make_dodecahedron(vector<vec3>& verts, vector<ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(vec3(1, 1, 1));
	verts.push_back(vec3(1/phi, phi, 0));
	verts.push_back(vec3(-1, 1, 1));
	verts.push_back(vec3(0, 1/phi, phi));
	verts.push_back(vec3(-1/phi, phi, 0));
	verts.push_back(vec3(1, 1, 1));
	verts.push_back(vec3(phi, 0, 1/phi));
	verts.push_back(vec3(1, 1, -1));
	verts.push_back(vec3(1/phi, phi, 0));
	verts.push_back(vec3(phi, 0, -1/phi));

	verts.push_back(vec3(1, 1, 1));
	verts.push_back(vec3(0, 1/phi, phi));
	verts.push_back(vec3(1, -1, 1));
	verts.push_back(vec3(phi, 0, 1/phi));
	verts.push_back(vec3(0, -1/phi, phi));
	verts.push_back(vec3(1/phi, phi, 0));
	verts.push_back(vec3(1, 1, -1));
	verts.push_back(vec3(-1, 1, -1));
	verts.push_back(vec3(-1/phi, phi, 0));
	verts.push_back(vec3(0, 1/phi, -phi));

	verts.push_back(vec3(phi, 0, 1/phi));
	verts.push_back(vec3(1, -1, 1));
	verts.push_back(vec3(1, -1, -1));
	verts.push_back(vec3(phi, 0, -1/phi));
	verts.push_back(vec3(1/phi, -phi, 0));
	verts.push_back(vec3(0, 1/phi, phi));
	verts.push_back(vec3(-1, 1, 1));
	verts.push_back(vec3(-1, -1, 1));
	verts.push_back(vec3(0, -1/phi, phi));
	verts.push_back(vec3(-phi, 0, 1/phi));

	verts.push_back(vec3(-1/phi, phi, 0));
	verts.push_back(vec3(-1, 1, -1));
	verts.push_back(vec3(-phi, 0, 1/phi));
	verts.push_back(vec3(-1, 1, 1));
	verts.push_back(vec3(-phi, 0, -1/phi));
	verts.push_back(vec3(-1, -1, 1));
	verts.push_back(vec3(-1/phi, -phi, 0));
	verts.push_back(vec3(1, -1, 1));
	verts.push_back(vec3(0, -1/phi, phi));
	verts.push_back(vec3(1/phi, -phi, 0));

	verts.push_back(vec3(1, -1, -1));
	verts.push_back(vec3(0, -1/phi, -phi));
	verts.push_back(vec3(1, 1, -1));
	verts.push_back(vec3(phi, 0, -1/phi));
	verts.push_back(vec3(0, 1/phi, -phi));
	verts.push_back(vec3(-1, -1, -1));
	verts.push_back(vec3(-phi, 0, -1/phi));
	verts.push_back(vec3(0, 1/phi, -phi));
	verts.push_back(vec3(0, -1/phi, -phi));
	verts.push_back(vec3(-1, 1, -1));

	verts.push_back(vec3(-1, -1, -1));
	verts.push_back(vec3(-1/phi, -phi, 0));
	verts.push_back(vec3(-phi, 0, 1/phi));
	verts.push_back(vec3(-phi, 0, -1/phi));
	verts.push_back(vec3(-1, -1, 1));
	verts.push_back(vec3(-1, -1, -1));
	verts.push_back(vec3(0, -1/phi, -phi));
	verts.push_back(vec3(1/phi, -phi, 0));
	verts.push_back(vec3(-1/phi, -phi, 0));
	verts.push_back(vec3(1, -1, -1));

	tris.push_back(ivec3(0, 1, 2));
	tris.push_back(ivec3(2, 3, 0));
	tris.push_back(ivec3(1, 4, 2));
	tris.push_back(ivec3(5, 6, 7));
	tris.push_back(ivec3(7, 8, 5));
	tris.push_back(ivec3(6, 9, 7));
	tris.push_back(ivec3(10, 11, 12));
	tris.push_back(ivec3(12, 13, 10));
	tris.push_back(ivec3(11, 14, 12));
	tris.push_back(ivec3(15, 16, 17));
	tris.push_back(ivec3(17, 18, 15));
	tris.push_back(ivec3(16, 19, 17));
	tris.push_back(ivec3(20, 21, 22));
	tris.push_back(ivec3(22, 23, 20));
	tris.push_back(ivec3(21, 24, 22));
	tris.push_back(ivec3(25, 26, 27));
	tris.push_back(ivec3(27, 28, 25));
	tris.push_back(ivec3(26, 29, 27));
	tris.push_back(ivec3(30, 31, 32));
	tris.push_back(ivec3(32, 33, 30));
	tris.push_back(ivec3(31, 34, 32));
	tris.push_back(ivec3(35, 36, 37));
	tris.push_back(ivec3(37, 38, 35));
	tris.push_back(ivec3(36, 39, 37));
	tris.push_back(ivec3(40, 41, 42));
	tris.push_back(ivec3(42, 43, 40));
	tris.push_back(ivec3(41, 44, 42));
	tris.push_back(ivec3(45, 46, 47));
	tris.push_back(ivec3(47, 48, 45));
	tris.push_back(ivec3(46, 49, 47));
	tris.push_back(ivec3(50, 51, 52));
	tris.push_back(ivec3(52, 53, 50));
	tris.push_back(ivec3(51, 54, 52));
	tris.push_back(ivec3(55, 56, 57));
	tris.push_back(ivec3(57, 58, 55));
	tris.push_back(ivec3(56, 59, 57));

	for (int i=tri_start; i<tris.size(); i++) {
		tris[i] += ivec3(vert_start);
	}
}

void make_icosahedron(vector<vec3>& verts, vector<ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(vec3(0, 1, phi));
	verts.push_back(vec3(0, -1, phi));
	verts.push_back(vec3(0, -1, -phi));
	verts.push_back(vec3(0, 1, -phi));

	verts.push_back(vec3(1, phi, 0));
	verts.push_back(vec3(-1, phi, 0));
	verts.push_back(vec3(-1, -phi, 0));
	verts.push_back(vec3(1, -phi, 0));

	verts.push_back(vec3(phi, 0, 1));
	verts.push_back(vec3(phi, 0, -1));
	verts.push_back(vec3(-phi, 0, -1));
	verts.push_back(vec3(-phi, 0, 1));

	// "top" centered at vertex 0
	tris.push_back(ivec3(0, 11, 1));
	tris.push_back(ivec3(0, 1, 8));
	tris.push_back(ivec3(0, 8, 4));
	tris.push_back(ivec3(0, 4, 5));
	tris.push_back(ivec3(0, 5, 11));

	// adjacent triangles to top
	tris.push_back(ivec3(1, 11, 6));
	tris.push_back(ivec3(1, 7, 8));
	tris.push_back(ivec3(8, 9, 4));
	tris.push_back(ivec3(5, 4, 3));
	tris.push_back(ivec3(11, 5, 10));

	//adjacent triangles to bottom
	tris.push_back(ivec3(6, 7, 1));
	tris.push_back(ivec3(7, 9, 8));
	tris.push_back(ivec3(9, 3, 4));
	tris.push_back(ivec3(3, 10, 5));
	tris.push_back(ivec3(10, 6, 11));

	// "bottom" centered at v 2
	tris.push_back(ivec3(2, 7, 6));
	tris.push_back(ivec3(2, 9, 7));
	tris.push_back(ivec3(2, 3, 9));
	tris.push_back(ivec3(2, 10, 3));
	tris.push_back(ivec3(2, 6, 10));

	for (int i=tri_start; i<tris.size(); i++) {
		tris[i] += ivec3(vert_start);
	}
}

