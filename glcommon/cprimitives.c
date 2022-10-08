#include "cprimitives.h"

// TODO put in crsw_math.h?
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


void make_box(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float dimX, float dimY, float dimZ)
{
	int plane = FALSE;
	ivec3 seg = { 1, 1, 1 };
	vec3 origin = { 0, 0, 0 };
	make_box2(verts, tris, tex, dimX, dimY, dimZ, plane, seg, origin);
}


void make_box2(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float dimX, float dimY, float dimZ, int plane, ivec3 seg, vec3 origin)
{
	vec3 x_vec = { dimX, 0, 0 };
	vec3 y_vec = { 0, dimY, 0 };
	vec3 z_vec = { 0, 0, dimZ };

	vec3 orig_x = add_vec3s(origin, x_vec);
	vec3 orig_y = add_vec3s(origin, y_vec);
	vec3 orig_xy = add_vec3s(orig_x, y_vec);
	vec3 orig_xyz = add_vec3s(orig_xy, z_vec);

	vec3 orig_z = add_vec3s(origin, z_vec);
	vec3 orig_zx = add_vec3s(orig_z, x_vec);
	vec3 orig_zy = add_vec3s(orig_z, y_vec);

	int vert_start = verts->size;
	int tri_start = tris->size;

	if (!plane) {
		cvec_push_vec3(verts, origin);
		cvec_push_vec3(verts, orig_x);
		cvec_push_vec3(verts, orig_y);
		cvec_push_vec3(verts, orig_xy);

		cvec_push_vec3(verts, orig_z);
		cvec_push_vec3(verts, orig_zx);
		cvec_push_vec3(verts, orig_zy);
		cvec_push_vec3(verts, orig_xyz);

		//back face
		cvec_push_ivec3(tris, make_ivec3(0, 2, 3));
		cvec_push_vec2(tex, make_vec2(1, 1));
		cvec_push_vec2(tex, make_vec2(1, 0));
		cvec_push_vec2(tex, make_vec2(0, 0));

		cvec_push_ivec3(tris, make_ivec3(0, 3, 1));
		cvec_push_vec2(tex, make_vec2(1, 1));
		cvec_push_vec2(tex, make_vec2(0, 0));
		cvec_push_vec2(tex, make_vec2(0, 1));

		// left face
		cvec_push_ivec3(tris, make_ivec3(0, 6, 2));
		cvec_push_vec2(tex, make_vec2(0, 1));
		cvec_push_vec2(tex, make_vec2(1, 0));
		cvec_push_vec2(tex, make_vec2(0, 0));

		cvec_push_ivec3(tris, make_ivec3(0, 4, 6));
		cvec_push_vec2(tex, make_vec2(0, 1));
		cvec_push_vec2(tex, make_vec2(1, 1));
		cvec_push_vec2(tex, make_vec2(1, 0));

		//bottom face
		cvec_push_ivec3(tris, make_ivec3(0, 1, 5));
		cvec_push_vec2(tex, make_vec2(0, 1));
		cvec_push_vec2(tex, make_vec2(1, 1));
		cvec_push_vec2(tex, make_vec2(1, 0));

		cvec_push_ivec3(tris, make_ivec3(0, 5, 4));
		cvec_push_vec2(tex, make_vec2(0, 1));
		cvec_push_vec2(tex, make_vec2(1, 0));
		cvec_push_vec2(tex, make_vec2(0, 0));

		//top face
		cvec_push_ivec3(tris, make_ivec3(7, 3, 2));
		cvec_push_vec2(tex, make_vec2(1, 1));
		cvec_push_vec2(tex, make_vec2(1, 0));
		cvec_push_vec2(tex, make_vec2(0, 0));

		cvec_push_ivec3(tris, make_ivec3(7, 2, 6));
		cvec_push_vec2(tex, make_vec2(1, 1));
		cvec_push_vec2(tex, make_vec2(0, 0));
		cvec_push_vec2(tex, make_vec2(0, 1));

		//right face
		cvec_push_ivec3(tris, make_ivec3(7, 1, 3));
		cvec_push_vec2(tex, make_vec2(0, 0));
		cvec_push_vec2(tex, make_vec2(1, 1));
		cvec_push_vec2(tex, make_vec2(1, 0));

		cvec_push_ivec3(tris, make_ivec3(7, 5, 1));
		cvec_push_vec2(tex, make_vec2(0, 0));
		cvec_push_vec2(tex, make_vec2(0, 1));
		cvec_push_vec2(tex, make_vec2(1, 1));

		//front face
		cvec_push_ivec3(tris, make_ivec3(7, 6, 4));
		cvec_push_vec2(tex, make_vec2(1, 0));
		cvec_push_vec2(tex, make_vec2(0, 0));
		cvec_push_vec2(tex, make_vec2(0, 1));

		cvec_push_ivec3(tris, make_ivec3(7, 4, 5));
		cvec_push_vec2(tex, make_vec2(1, 0));
		cvec_push_vec2(tex, make_vec2(0, 1));
		cvec_push_vec2(tex, make_vec2(1, 1));

		for (int i=tri_start; i<tris->size; i++) {
			//TODO
			//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
			tris->a[i].x += vert_start;
			tris->a[i].y += vert_start;
			tris->a[i].z += vert_start;
		}
	} else {
		vec3 x_div_segx = { x_vec.x/seg.x, x_vec.y/seg.x, x_vec.z/seg.x };
		vec3 y_div_segy = { y_vec.x/seg.y, y_vec.y/seg.y, y_vec.z/seg.y };
		vec3 z_div_segz = { z_vec.x/seg.z, z_vec.y/seg.z, z_vec.z/seg.z };

		//front and back
		make_plane2(verts, tris, tex, orig_x, y_div_segy, negate_vec3(x_div_segx), seg.y, seg.x, TRUE);
		make_plane2(verts, tris, tex, orig_z, y_div_segy, x_div_segx, seg.y, seg.x, TRUE);
		//left and right
		make_plane2(verts, tris, tex, origin, y_div_segy, z_div_segz, seg.y, seg.z, TRUE);
		make_plane2(verts, tris, tex, orig_zx, y_div_segy, negate_vec3(z_div_segz), seg.y, seg.z, TRUE);
		//top and bottom
		make_plane2(verts, tris, tex, origin, z_div_segz, x_div_segx, seg.z, seg.x, TRUE);
		make_plane2(verts, tris, tex, orig_zy, negate_vec3(z_div_segz), x_div_segx, seg.z, seg.x, TRUE);
	}
}


void make_cylinder(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float radius, float height, size_t slices)
{
	make_cylindrical(verts, tris, tex, radius, height, slices, 1, radius);
}

void make_cylindrical(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float radius, float height, size_t slices, size_t stacks, float top_radius)
{
	int i = 0, j = 0;

	float stack_height = height/stacks;
	float cur_radius;
	double theta = RM_2PI/slices;

	int vert_start = verts->size;
	int tri_start = tris->size;


	cvec_push_vec3(verts, make_vec3(0, 0, 0));

	for (i = 0; i <= stacks; i++) {
		cur_radius = i/stacks * top_radius + (1.0 - i/stacks) * radius;
		for (j = 0; j < slices; j++) {
			cvec_push_vec3(verts, make_vec3(radius*cos(j*theta), radius*sin(j*theta), i*stack_height));
		}
	}

	cvec_push_vec3(verts, make_vec3(0, 0, height));

//reversed this cause I accidentally figured it out the other way then noticed it'd be clockwise so just reversed it.
//I'll figure it out for increasing later.
	for (i = slices; i > 0; i--) {
		cvec_push_ivec3(tris, make_ivec3(0, (((i+1)%(slices+1)==0) ? 1 : i+1), i));
		cvec_push_vec2(tex, make_vec2(0.5, 0.5));

		if ((i+1) % (slices+1) == 0)
			cvec_push_vec2(tex, make_vec2(0.5 + 0.5, 0.5));
		else
			cvec_push_vec2(tex, make_vec2(0.5 + 0.5*cos(i*theta), 0.5+0.5*sin(i*theta)));

		cvec_push_vec2(tex, make_vec2(0.5 + 0.5*cos((i-1)*theta), 0.5+0.5*sin((i-1)*theta)));
	}


	for (j = 0; j < stacks; j++) {
		for (i = 1; i <= slices; i++) {
			if (i != slices) {
				cvec_push_ivec3(tris, make_ivec3(i+j*slices, (i+1)+j*slices, i+(j+1)*slices));
				// TODO this is excessive casting but it was easiest with searh-replace.  Only casting
				// the denominator should be enough.
				cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), (float)(j)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(i)/(float)(slices), (float)(j)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), (float)(j+1)/(float)(stacks)));


				cvec_push_ivec3(tris, make_ivec3((i+1)+j*slices, (i+1)+(j+1)*slices, i+(j+1)*slices));
				cvec_push_vec2(tex, make_vec2((float)(i)/(float)(slices), (float)(j)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(i)/(float)(slices), (float)(j+1)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), (float)(j+1)/(float)(stacks)));

			} else {
				cvec_push_ivec3(tris, make_ivec3(i+j*slices, (i-slices+1)+j*slices, i+(j+1)*slices));
				cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), (float)(j)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(i)/(float)(slices), (float)(j)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), (float)(j+1)/(float)(stacks)));


				cvec_push_ivec3(tris, make_ivec3((i-slices+1)+j*slices, (i+1)+j*slices, i+(j+1)*slices));
				cvec_push_vec2(tex, make_vec2((float)(i)/(float)(slices), (float)(j)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(i)/(float)(slices), (float)(j+1)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), (float)(j+1)/(float)(stacks)));

			}
		}
	}

	int top_center = verts->size - vert_start - 1;

	j = 0;
	for (i = top_center - slices; i < top_center; i++, j++) {
		cvec_push_ivec3(tris, make_ivec3(top_center, i, ( ((i+1)==top_center)? top_center-slices : i+1 ) ));

		cvec_push_vec2(tex, make_vec2(0.5, 0.5));
		cvec_push_vec2(tex, make_vec2(0.5 + 0.5*cos(j*theta), 0.5 + 0.5*sin(j*theta) ));

		if ((i+1) != top_center)
			cvec_push_vec2(tex, make_vec2(0.5 + 0.5*cos((j+1)*theta), 0.5 + 0.5*sin((j+1)*theta) ));
		else
			cvec_push_vec2(tex, make_vec2(0.5 + 0.5, 0.5));
	}

	for (int i=tri_start; i<tris->size; i++) {
		//TODO
		//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
		tris->a[i].x += vert_start;
		tris->a[i].y += vert_start;
		tris->a[i].z += vert_start;
	}
}


void make_plane(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, vec3 corner, vec3 v1, vec3 v2, size_t dimV1, size_t dimV2)
{
	int tile = FALSE;
	make_plane2(verts, tris, tex, corner, v1, v2, dimV1, dimV2, tile);
}


void make_plane2(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, vec3 corner, vec3 v1, vec3 v2, size_t dimV1, size_t dimV2, int tile)
{
	//should check here if v1 and v2 are too close to the same direction
	int i = 0;

	int vert_start = verts->size;
	int tri_start = tris->size;

	vec3 tmp_vec;

	for (i = 0; i <= dimV2; i++) {
		for (int j=0; j <= dimV1; j++) {
			SET_VEC3(tmp_vec, corner.x + v2.x*i + v1.x*j, corner.y + v2.y*i + v1.y*j, corner.z + v2.z*i + v1.z*j);
			//tmp_vec = add_vec3s(add_vec3s(corner, scale_vec3(v2, i)), scale_vec3(v1, j));
			cvec_push_vec3(verts, tmp_vec);
		}
	}

	int j = -1;

	for (i = 0; i < dimV1*dimV2; i++) {
		if (i % dimV1 == 0)
			j++;

		cvec_push_ivec3(tris, make_ivec3(i+j, i+j+dimV1+1, i+j+dimV1+2));
		cvec_push_ivec3(tris, make_ivec3(i+j, i+j+dimV1+2, i+j+1));

//		cvec_push_ivec3(tris, make_ivec3(i+j, i+j+1, i+j+1+dimV2));
//		cvec_push_ivec3(tris, make_ivec3(i+j+1+dimV2, i+j+1, i+j+dimV2+2));

		if (!tile) {
			float dimV1f = dimV1, dimV2f = dimV2;
			cvec_push_vec2(tex, make_vec2((i%dimV1)/dimV1f, (j)/dimV2f));
			cvec_push_vec2(tex, make_vec2((i%dimV1)/dimV1f, (j+1)/dimV2f));
			cvec_push_vec2(tex, make_vec2((i%dimV1 + 1)/dimV1f, (j+1)/dimV2f));

			cvec_push_vec2(tex, make_vec2((i%dimV1)/dimV1f, (j)/dimV2f));
			cvec_push_vec2(tex, make_vec2((i%dimV1 + 1)/dimV1f, (j+1)/dimV2f));
			cvec_push_vec2(tex, make_vec2((i%dimV1 + 1)/dimV1f, (j)/dimV2f));
		} else {
			//just increment per box, tile by setting texture to wrap
			cvec_push_vec2(tex, make_vec2(i%dimV1, j));
			cvec_push_vec2(tex, make_vec2(i%dimV1, j+1));
			cvec_push_vec2(tex, make_vec2(i%dimV1+1, j+1));

			cvec_push_vec2(tex, make_vec2(i%dimV1, j));
			cvec_push_vec2(tex, make_vec2(i%dimV2 + 1, j+1));
			cvec_push_vec2(tex, make_vec2(i%dimV2 + 1, j));
		}
	}
	for (int i=tri_start; i<tris->size; i++) {
		//TODO
		//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
		tris->a[i].x += vert_start;
		tris->a[i].y += vert_start;
		tris->a[i].z += vert_start;
	}
}






void make_sphere(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float radius, size_t slices, size_t stacks)
{
	float down = RM_PI/stacks;
	float around = RM_2PI/slices;

	mat3 rotdown, rotaround;
	load_rotation_mat3(rotdown, make_vec3(0, 1, 0), down);
	load_rotation_mat3(rotaround, make_vec3(0, 0, 1), around);

	int vert_start = verts->size;
	int tri_start = tris->size;

	vec3 point = { 0, 0, radius };
	vec3 tmp;

	cvec_push_vec3(verts, point);

	for (int i=1; i<stacks; ++i) {

		//rotate down to next stack and add first point (on x axis)
		point = mult_mat3_vec3(rotdown, point);
		cvec_push_vec3(verts, point);
		tmp = point;

		for (int j=1; j<slices; ++j) {
			point = mult_mat3_vec3(rotaround, point);
			cvec_push_vec3(verts, point);
		}
		point = tmp; //set back to y (copying probably quicker than rotating again)
	}

	cvec_push_vec3(verts, make_vec3(0, 0, -radius));


	//add top cap triangles
	for (int i=1; i<slices+1; ++i) {
		if (i != slices) {
			cvec_push_ivec3(tris, make_ivec3(0, i, i+1));
			// TODO again, excessive casting
			cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), 1));
			cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), (float)(stacks-1)/(float)(stacks)));
			cvec_push_vec2(tex, make_vec2((float)(i)/(float)(slices), (float)(stacks-1)/(float)(stacks)));
		} else {
			cvec_push_ivec3(tris, make_ivec3(0, i, 1));
			cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), 1));
			cvec_push_vec2(tex, make_vec2((float)(i-1)/(float)(slices), (float)(stacks-1)/(float)(stacks)));
			cvec_push_vec2(tex, make_vec2(1, (float)(stacks-1)/(float)(stacks)));
		}
	}

	int corner;
	for (int i=1 ; i<stacks-1 ; ++i) {
		for (int j=0; j<slices; ++j) {

			corner = i*slices+1 + j;
			if (j != slices-1) {
				cvec_push_ivec3(tris, make_ivec3(corner, corner+1, corner-slices));
				cvec_push_ivec3(tris, make_ivec3(corner+1, (corner+1)-slices, corner-slices));

				// TODO more excessive casting
				cvec_push_vec2(tex, make_vec2((float)(j)/(float)(slices), (float)(stacks-i-1)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(j+1)/(float)(slices), (float)(stacks-i-1)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(j)/(float)(slices), (float)(stacks-i)/(float)(stacks)));

				cvec_push_vec2(tex, make_vec2((float)(j+1)/(float)(slices), (float)(stacks-i-1)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(j+1)/(float)(slices), (float)(stacks-i)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(j)/(float)(slices), (float)(stacks-i)/(float)(stacks)));

			} else {
				cvec_push_ivec3(tris, make_ivec3(corner, i*slices+1, corner-slices));
				cvec_push_ivec3(tris, make_ivec3(i*slices+1, (i-1)*slices+1, corner-slices));

				cvec_push_vec2(tex, make_vec2((float)(j)/(float)(slices), (float)(stacks-i-1)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2(1, (float)(stacks-i-1)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(j)/(float)(slices), (float)(stacks-i)/(float)(stacks)));

				cvec_push_vec2(tex, make_vec2(1, (float)(stacks-i-1)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2(1, (float)(stacks-i)/(float)(stacks)));
				cvec_push_vec2(tex, make_vec2((float)(j)/(float)(slices), (float)(stacks-i)/(float)(stacks)));
			}
		}
	}

	//add bottom cap
	int bottom = verts->size - vert_start - 1;
	for (int i=0; i<slices; ++i) {
		if (i != 0) {
			cvec_push_ivec3(tris, make_ivec3(bottom, bottom-i, bottom-i-1));

			cvec_push_vec2(tex, make_vec2((float)(slices-i)/(float)(slices), 0));
			cvec_push_vec2(tex, make_vec2((float)(slices-i)/(float)(slices), (float)(1)/(float)(stacks)));
			cvec_push_vec2(tex, make_vec2((float)(slices-i-1)/(float)(slices), (float)(1)/(float)(stacks)));

		} else {
			cvec_push_ivec3(tris, make_ivec3(bottom, bottom-slices, bottom-1));

			cvec_push_vec2(tex, make_vec2(1, 0));
			cvec_push_vec2(tex, make_vec2(1, (float)(1)/(float)(stacks)));
			cvec_push_vec2(tex, make_vec2((float)(slices-1)/(float)(slices), (float)(1)/(float)(stacks)));
		}
	}
	for (int i=tri_start; i<tris->size; i++) {
		//TODO
		//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
		tris->a[i].x += vert_start;
		tris->a[i].y += vert_start;
		tris->a[i].z += vert_start;
	}
}



void make_cone(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float radius, float height, size_t slices, int flip)
{
	if (!flip) {
		make_cylindrical(verts, tris, tex, radius, height, slices, 1, 0.0f);
	} else {
		make_cylindrical(verts, tris, tex, 0.0f, height, slices, 1, radius);
	}
}

// Draw a torus (doughnut) in xy plane
void make_torus(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float major_r, float minor_r, size_t major_slices, size_t minor_slices)
{
    double major_step = RM_2PI / major_slices;
    double minor_step = RM_2PI / minor_slices;
    int i, j;

	double a0, a1, b;
	float x0, y0, x1, y1, c, r, z;

	int vert_start = verts->size;
	int tri_start = tris->size;

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

			cvec_push_vec3(verts, make_vec3(x0 * r, y0 * r, z));
		}
	}

	int s;
	for (i=0; i<major_slices; ++i) {
		s = i*minor_slices;
		if (i != major_slices-1) {
			for (j=0; j<minor_slices; ++j) {
				if (j != minor_slices-1) {
					cvec_push_ivec3(tris, make_ivec3(s+j, s+j+1+minor_slices, s+j+1));
					cvec_push_ivec3(tris, make_ivec3(s+j, s+j+minor_slices, s+j+1+minor_slices));
				} else {
					cvec_push_ivec3(tris, make_ivec3(s+j, s+minor_slices, s));
					cvec_push_ivec3(tris, make_ivec3(s+j, s+j+minor_slices, s+minor_slices));
				}
				cvec_push_vec2(tex, make_vec2((float)i/(float)major_slices, (float)j/(float)minor_slices));
				cvec_push_vec2(tex, make_vec2((float)(i+1)/(float)major_slices, (float)(j+1)/(float)minor_slices));
				cvec_push_vec2(tex, make_vec2((float)i/(float)major_slices, (float)(j+1)/(float)minor_slices));

				cvec_push_vec2(tex, make_vec2((float)i/(float)major_slices, (float)j/(float)minor_slices));
				cvec_push_vec2(tex, make_vec2((float)(i+1)/(float)major_slices, (float)j/(float)minor_slices));
				cvec_push_vec2(tex, make_vec2((float)(i+1)/(float)major_slices, (float)(j+1)/(float)minor_slices));
			}
		} else {
			for (j=0; j<minor_slices; ++j) {
				if (j != minor_slices-1) {
					cvec_push_ivec3(tris, make_ivec3(s+j, j+1, s+j+1));
					cvec_push_ivec3(tris, make_ivec3(s+j, j, j+1));
				} else {
					cvec_push_ivec3(tris, make_ivec3(s+j, 0, s));
					cvec_push_ivec3(tris, make_ivec3(s+j, j, 0));
				}
				cvec_push_vec2(tex, make_vec2((float)i/(float)major_slices, (float)j/(float)minor_slices));
				cvec_push_vec2(tex, make_vec2((float)(i+1)/(float)major_slices, (float)(j+1)/(float)minor_slices));
				cvec_push_vec2(tex, make_vec2((float)i/(float)major_slices, (float)(j+1)/(float)minor_slices));

				cvec_push_vec2(tex, make_vec2((float)i/(float)major_slices, (float)j/(float)minor_slices));
				cvec_push_vec2(tex, make_vec2((float)(i+1)/(float)major_slices, (float)j/(float)minor_slices));
				cvec_push_vec2(tex, make_vec2((float)(i+1)/(float)major_slices, (float)(j+1)/(float)minor_slices));
			}
		}
	}

	for (int i=tri_start; i<tris->size; i++) {
		//TODO
		//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
		tris->a[i].x += vert_start;
		tris->a[i].y += vert_start;
		tris->a[i].z += vert_start;
	}
}


void expand_verts(cvector_vec3* draw_verts, cvector_vec3* verts, cvector_ivec3* triangles)
{
	if (triangles->size) {
		for (int i=0; i<triangles->size; ++i) {
			cvec_push_vec3(draw_verts, verts->a[triangles->a[i].x]);
			cvec_push_vec3(draw_verts, verts->a[triangles->a[i].y]);
			cvec_push_vec3(draw_verts, verts->a[triangles->a[i].z]);
		}
	} else {
		cvec_copy_vec3(draw_verts, verts);
	}
}


void expand_tex(cvector_vec2* draw_tex, cvector_vec2* tex, cvector_ivec3* triangles)
{
	if (triangles->size) {
		for (int i=0; i<triangles->size; ++i) {
			cvec_push_vec2(draw_tex, tex->a[triangles->a[i].x]);
			cvec_push_vec2(draw_tex, tex->a[triangles->a[i].y]);
			cvec_push_vec2(draw_tex, tex->a[triangles->a[i].z]);
		}
	} else {
		cvec_copy_vec2(draw_tex, tex);
	}
}


// platonic solids, just verts and triangles

void make_tetrahedron(cvector_vec3* verts, cvector_ivec3* tris)
{
	int vert_start = verts->size;
	int tri_start = tris->size;

	cvec_push_vec3(verts, make_vec3(1, 1, 1));
	cvec_push_vec3(verts, make_vec3(1, -1, -1));
	cvec_push_vec3(verts, make_vec3(-1, 1, -1));
	cvec_push_vec3(verts, make_vec3(-1, -1, 1));

	cvec_push_ivec3(tris, make_ivec3(0, 1, 2));
	cvec_push_ivec3(tris, make_ivec3(0, 2, 3));
	cvec_push_ivec3(tris, make_ivec3(0, 3, 1));
	cvec_push_ivec3(tris, make_ivec3(1, 3, 2));

	for (int i=tri_start; i<tris->size; i++) {
		//TODO
		//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
		tris->a[i].x += vert_start;
		tris->a[i].y += vert_start;
		tris->a[i].z += vert_start;
	}
}





void make_cube(cvector_vec3* verts, cvector_ivec3* tris)
{
	int vert_start = verts->size;
	int tri_start = tris->size;

	cvec_push_vec3(verts, make_vec3(-1, -1, -1));
	cvec_push_vec3(verts, make_vec3(1, -1, -1));
	cvec_push_vec3(verts, make_vec3(-1, 1, -1));
	cvec_push_vec3(verts, make_vec3(1, 1, -1));

	cvec_push_vec3(verts, make_vec3(-1, -1, 1));
	cvec_push_vec3(verts, make_vec3(1, -1, 1));
	cvec_push_vec3(verts, make_vec3(-1, 1, 1));
	cvec_push_vec3(verts, make_vec3(1, 1, 1));

	//back face
	cvec_push_ivec3(tris, make_ivec3(0, 2, 3));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));

	cvec_push_ivec3(tris, make_ivec3(0, 3, 1));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(0, 1));

	//left face
	cvec_push_ivec3(tris, make_ivec3(0, 6, 2));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));

	cvec_push_ivec3(tris, make_ivec3(0, 4, 6));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));


	//bottom face
	cvec_push_ivec3(tris, make_ivec3(0, 1, 5));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));

	cvec_push_ivec3(tris, make_ivec3(0, 5, 4));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));


	//top face
	cvec_push_ivec3(tris, make_ivec3(7, 3, 2));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));

	cvec_push_ivec3(tris, make_ivec3(7, 2, 6));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(0, 1));


	//right face
	cvec_push_ivec3(tris, make_ivec3(7, 1, 3));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(1, 1));
	//tex.push_back(vec2(1, 0));

	cvec_push_ivec3(tris, make_ivec3(7, 5, 1));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 1));


	//front face
	cvec_push_ivec3(tris, make_ivec3(7, 6, 4));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 0));
	//tex.push_back(vec2(0, 1));

	cvec_push_ivec3(tris, make_ivec3(7, 4, 5));
	//tex.push_back(vec2(1, 0));
	//tex.push_back(vec2(0, 1));
	//tex.push_back(vec2(1, 1));

	for (int i=tri_start; i<tris->size; i++) {
		//TODO
		//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
		tris->a[i].x += vert_start;
		tris->a[i].y += vert_start;
		tris->a[i].z += vert_start;
	}
}

void make_octahedron(cvector_vec3* verts, cvector_ivec3* tris)
{
	int vert_start = verts->size;
	int tri_start = tris->size;

	cvec_push_vec3(verts, make_vec3(1, 0, 0));
	cvec_push_vec3(verts, make_vec3(0, 0, -1));
	cvec_push_vec3(verts, make_vec3(-1, 0, 0));
	cvec_push_vec3(verts, make_vec3(0, 0, 1));
	cvec_push_vec3(verts, make_vec3(0, 1, 0));
	cvec_push_vec3(verts, make_vec3(0, -1, 0));

	cvec_push_ivec3(tris, make_ivec3(0, 1, 4));
	cvec_push_ivec3(tris, make_ivec3(1, 2, 4));
	cvec_push_ivec3(tris, make_ivec3(2, 3, 4));
	cvec_push_ivec3(tris, make_ivec3(3, 0, 4));

	cvec_push_ivec3(tris, make_ivec3(1, 0, 5));
	cvec_push_ivec3(tris, make_ivec3(2, 1, 5));
	cvec_push_ivec3(tris, make_ivec3(3, 2, 5));
	cvec_push_ivec3(tris, make_ivec3(0, 3, 5));

	for (int i=tri_start; i<tris->size; i++) {
		//TODO
		//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
		tris->a[i].x += vert_start;
		tris->a[i].y += vert_start;
		tris->a[i].z += vert_start;
	}
}


#define phi 1.618
void make_dodecahedron(cvector_vec3* verts, cvector_ivec3* tris)
{
	int vert_start = verts->size;
	int tri_start = tris->size;

	cvec_push_vec3(verts, make_vec3(1, 1, 1));
	cvec_push_vec3(verts, make_vec3(1/phi, phi, 0));
	cvec_push_vec3(verts, make_vec3(-1, 1, 1));
	cvec_push_vec3(verts, make_vec3(0, 1/phi, phi));
	cvec_push_vec3(verts, make_vec3(-1/phi, phi, 0));
	cvec_push_vec3(verts, make_vec3(1, 1, 1));
	cvec_push_vec3(verts, make_vec3(phi, 0, 1/phi));
	cvec_push_vec3(verts, make_vec3(1, 1, -1));
	cvec_push_vec3(verts, make_vec3(1/phi, phi, 0));
	cvec_push_vec3(verts, make_vec3(phi, 0, -1/phi));

	cvec_push_vec3(verts, make_vec3(1, 1, 1));
	cvec_push_vec3(verts, make_vec3(0, 1/phi, phi));
	cvec_push_vec3(verts, make_vec3(1, -1, 1));
	cvec_push_vec3(verts, make_vec3(phi, 0, 1/phi));
	cvec_push_vec3(verts, make_vec3(0, -1/phi, phi));
	cvec_push_vec3(verts, make_vec3(1/phi, phi, 0));
	cvec_push_vec3(verts, make_vec3(1, 1, -1));
	cvec_push_vec3(verts, make_vec3(-1, 1, -1));
	cvec_push_vec3(verts, make_vec3(-1/phi, phi, 0));
	cvec_push_vec3(verts, make_vec3(0, 1/phi, -phi));

	cvec_push_vec3(verts, make_vec3(phi, 0, 1/phi));
	cvec_push_vec3(verts, make_vec3(1, -1, 1));
	cvec_push_vec3(verts, make_vec3(1, -1, -1));
	cvec_push_vec3(verts, make_vec3(phi, 0, -1/phi));
	cvec_push_vec3(verts, make_vec3(1/phi, -phi, 0));
	cvec_push_vec3(verts, make_vec3(0, 1/phi, phi));
	cvec_push_vec3(verts, make_vec3(-1, 1, 1));
	cvec_push_vec3(verts, make_vec3(-1, -1, 1));
	cvec_push_vec3(verts, make_vec3(0, -1/phi, phi));
	cvec_push_vec3(verts, make_vec3(-phi, 0, 1/phi));

	cvec_push_vec3(verts, make_vec3(-1/phi, phi, 0));
	cvec_push_vec3(verts, make_vec3(-1, 1, -1));
	cvec_push_vec3(verts, make_vec3(-phi, 0, 1/phi));
	cvec_push_vec3(verts, make_vec3(-1, 1, 1));
	cvec_push_vec3(verts, make_vec3(-phi, 0, -1/phi));
	cvec_push_vec3(verts, make_vec3(-1, -1, 1));
	cvec_push_vec3(verts, make_vec3(-1/phi, -phi, 0));
	cvec_push_vec3(verts, make_vec3(1, -1, 1));
	cvec_push_vec3(verts, make_vec3(0, -1/phi, phi));
	cvec_push_vec3(verts, make_vec3(1/phi, -phi, 0));

	cvec_push_vec3(verts, make_vec3(1, -1, -1));
	cvec_push_vec3(verts, make_vec3(0, -1/phi, -phi));
	cvec_push_vec3(verts, make_vec3(1, 1, -1));
	cvec_push_vec3(verts, make_vec3(phi, 0, -1/phi));
	cvec_push_vec3(verts, make_vec3(0, 1/phi, -phi));
	cvec_push_vec3(verts, make_vec3(-1, -1, -1));
	cvec_push_vec3(verts, make_vec3(-phi, 0, -1/phi));
	cvec_push_vec3(verts, make_vec3(0, 1/phi, -phi));
	cvec_push_vec3(verts, make_vec3(0, -1/phi, -phi));
	cvec_push_vec3(verts, make_vec3(-1, 1, -1));

	cvec_push_vec3(verts, make_vec3(-1, -1, -1));
	cvec_push_vec3(verts, make_vec3(-1/phi, -phi, 0));
	cvec_push_vec3(verts, make_vec3(-phi, 0, 1/phi));
	cvec_push_vec3(verts, make_vec3(-phi, 0, -1/phi));
	cvec_push_vec3(verts, make_vec3(-1, -1, 1));
	cvec_push_vec3(verts, make_vec3(-1, -1, -1));
	cvec_push_vec3(verts, make_vec3(0, -1/phi, -phi));
	cvec_push_vec3(verts, make_vec3(1/phi, -phi, 0));
	cvec_push_vec3(verts, make_vec3(-1/phi, -phi, 0));
	cvec_push_vec3(verts, make_vec3(1, -1, -1));

	cvec_push_ivec3(tris, make_ivec3(0, 1, 2));
	cvec_push_ivec3(tris, make_ivec3(2, 3, 0));
	cvec_push_ivec3(tris, make_ivec3(1, 4, 2));
	cvec_push_ivec3(tris, make_ivec3(5, 6, 7));
	cvec_push_ivec3(tris, make_ivec3(7, 8, 5));
	cvec_push_ivec3(tris, make_ivec3(6, 9, 7));
	cvec_push_ivec3(tris, make_ivec3(10, 11, 12));
	cvec_push_ivec3(tris, make_ivec3(12, 13, 10));
	cvec_push_ivec3(tris, make_ivec3(11, 14, 12));
	cvec_push_ivec3(tris, make_ivec3(15, 16, 17));
	cvec_push_ivec3(tris, make_ivec3(17, 18, 15));
	cvec_push_ivec3(tris, make_ivec3(16, 19, 17));
	cvec_push_ivec3(tris, make_ivec3(20, 21, 22));
	cvec_push_ivec3(tris, make_ivec3(22, 23, 20));
	cvec_push_ivec3(tris, make_ivec3(21, 24, 22));
	cvec_push_ivec3(tris, make_ivec3(25, 26, 27));
	cvec_push_ivec3(tris, make_ivec3(27, 28, 25));
	cvec_push_ivec3(tris, make_ivec3(26, 29, 27));
	cvec_push_ivec3(tris, make_ivec3(30, 31, 32));
	cvec_push_ivec3(tris, make_ivec3(32, 33, 30));
	cvec_push_ivec3(tris, make_ivec3(31, 34, 32));
	cvec_push_ivec3(tris, make_ivec3(35, 36, 37));
	cvec_push_ivec3(tris, make_ivec3(37, 38, 35));
	cvec_push_ivec3(tris, make_ivec3(36, 39, 37));
	cvec_push_ivec3(tris, make_ivec3(40, 41, 42));
	cvec_push_ivec3(tris, make_ivec3(42, 43, 40));
	cvec_push_ivec3(tris, make_ivec3(41, 44, 42));
	cvec_push_ivec3(tris, make_ivec3(45, 46, 47));
	cvec_push_ivec3(tris, make_ivec3(47, 48, 45));
	cvec_push_ivec3(tris, make_ivec3(46, 49, 47));
	cvec_push_ivec3(tris, make_ivec3(50, 51, 52));
	cvec_push_ivec3(tris, make_ivec3(52, 53, 50));
	cvec_push_ivec3(tris, make_ivec3(51, 54, 52));
	cvec_push_ivec3(tris, make_ivec3(55, 56, 57));
	cvec_push_ivec3(tris, make_ivec3(57, 58, 55));
	cvec_push_ivec3(tris, make_ivec3(56, 59, 57));

	for (int i=tri_start; i<tris->size; i++) {
		//TODO
		//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
		tris->a[i].x += vert_start;
		tris->a[i].y += vert_start;
		tris->a[i].z += vert_start;
	}
}

void make_icosahedron(cvector_vec3* verts, cvector_ivec3* tris)
{
	int vert_start = verts->size;
	int tri_start = tris->size;

	cvec_push_vec3(verts, make_vec3(0, 1, phi));
	cvec_push_vec3(verts, make_vec3(0, -1, phi));
	cvec_push_vec3(verts, make_vec3(0, -1, -phi));
	cvec_push_vec3(verts, make_vec3(0, 1, -phi));

	cvec_push_vec3(verts, make_vec3(1, phi, 0));
	cvec_push_vec3(verts, make_vec3(-1, phi, 0));
	cvec_push_vec3(verts, make_vec3(-1, -phi, 0));
	cvec_push_vec3(verts, make_vec3(1, -phi, 0));

	cvec_push_vec3(verts, make_vec3(phi, 0, 1));
	cvec_push_vec3(verts, make_vec3(phi, 0, -1));
	cvec_push_vec3(verts, make_vec3(-phi, 0, -1));
	cvec_push_vec3(verts, make_vec3(-phi, 0, 1));

	// "top" centered at vertex 0
	cvec_push_ivec3(tris, make_ivec3(0, 11, 1));
	cvec_push_ivec3(tris, make_ivec3(0, 1, 8));
	cvec_push_ivec3(tris, make_ivec3(0, 8, 4));
	cvec_push_ivec3(tris, make_ivec3(0, 4, 5));
	cvec_push_ivec3(tris, make_ivec3(0, 5, 11));

	// adjacent triangles to top
	cvec_push_ivec3(tris, make_ivec3(1, 11, 6));
	cvec_push_ivec3(tris, make_ivec3(1, 7, 8));
	cvec_push_ivec3(tris, make_ivec3(8, 9, 4));
	cvec_push_ivec3(tris, make_ivec3(5, 4, 3));
	cvec_push_ivec3(tris, make_ivec3(11, 5, 10));

	//adjacent triangles to bottom
	cvec_push_ivec3(tris, make_ivec3(6, 7, 1));
	cvec_push_ivec3(tris, make_ivec3(7, 9, 8));
	cvec_push_ivec3(tris, make_ivec3(9, 3, 4));
	cvec_push_ivec3(tris, make_ivec3(3, 10, 5));
	cvec_push_ivec3(tris, make_ivec3(10, 6, 11));
	
	// "bottom" centered at v 2
	cvec_push_ivec3(tris, make_ivec3(2, 7, 6));
	cvec_push_ivec3(tris, make_ivec3(2, 9, 7));
	cvec_push_ivec3(tris, make_ivec3(2, 3, 9));
	cvec_push_ivec3(tris, make_ivec3(2, 10, 3));
	cvec_push_ivec3(tris, make_ivec3(2, 6, 10));

	for (int i=tri_start; i<tris->size; i++) {
		//TODO
		//tris->a[i] = add_ivec3s(tris->a[i], make_ivec3(vert_start, vert_start, vert_start));
		tris->a[i].x += vert_start;
		tris->a[i].y += vert_start;
		tris->a[i].z += vert_start;
	}
}

