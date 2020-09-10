#ifndef CHALFEDGE_H
#define CHALFEDGE_H

#include "crsw_math.h"

#include "cvector_vec3.h"
#include "cvector_ivec3.h"

#define CVECTOR_ONLY_INT
#include "cvector.h"


typedef struct half_edge
{
	int next;
	int pair;
	int face;
	int v;
} half_edge;



typedef struct Edge_Entry
{
	int v_index[2];
	int tri_index[2];
	int edge_number[2];
	struct Edge_Entry *next;
} edge_entry;

typedef struct half_edge_data
{
	half_edge* he_array;
	int* v_array;
	int* face_array;
} half_edge_data;

void compute_face_normals(cvector_vec3* verts, cvector_ivec3* t, cvector_vec3* normals);

void compute_half_edge(cvector_vec3* verts, cvector_ivec3* triangles, half_edge_data* he_data);


void compute_normals(cvector_vec3* verts, cvector_ivec3* triangles, half_edge_data* he_data, float sharp_angle, cvector_vec3* normals);

#endif

