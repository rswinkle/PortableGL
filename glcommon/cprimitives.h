#pragma once
#ifndef PRIMITIVES_H
#define PRIMITIVES_H

// TODO hmm, could include in all the vector headers but if I used the macros
// it would have to be here...
#include "crsw_math.h"

#include "cvector_vec3.h"
#include "cvector_ivec3.h"
#include "cvector_vec2.h"


#ifdef __cplusplus
extern "C" {
#endif




void make_cylinder(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float radius, float height, size_t slices, size_t stacks, float top_radius);


void make_box(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float dimX, float dimY, float dimZ);

void make_box2(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float dimX, float dimY, float dimZ, int plane, ivec3 seg, vec3 origin);

void make_plane(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, vec3 corner, vec3 v1, vec3 v2, size_t dimV1, size_t dimV2);

//plane faces in direction of v2 x v1 (cross product of v2 and v1) ie v1 = -z and v2 = x, plane would face up/y
//textured with v1 = x, v2 = y so put corner in upper left to get it upright.
//This is because OpenGL treats 0,0 as first pixel of image and for images/framebuffers in memory, that's the top left corner.
void make_plane2(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, vec3 corner, vec3 v1, vec3 v2, size_t dimV1, size_t dimV2, int tile);

void make_sphere(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float radius, size_t slices, size_t stacks);


void make_cone(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float radius, float height, size_t slices);

void make_conical_frustum(cvector_vec3* verts, cvector_ivec3* tris, cvector_vec2* tex, float radius, float height, size_t slices, float top_radius);


void expand_verts(cvector_vec3* draw_verts, cvector_vec3* verts, cvector_ivec3* triangles);
void expand_tex(cvector_vec2* draw_tex, cvector_vec2* tex, cvector_ivec3* triangles);

void make_tetrahedron(cvector_vec3* verts, cvector_ivec3* tris);
void make_cube(cvector_vec3* verts, cvector_ivec3* tris);
void make_octahedron(cvector_vec3* verts, cvector_ivec3* tris);
void make_dodecahedron(cvector_vec3* verts, cvector_ivec3* tris);
void make_icosahedron(cvector_vec3* verts, cvector_ivec3* tris);



#ifdef __cplusplus
}
#endif

#endif
