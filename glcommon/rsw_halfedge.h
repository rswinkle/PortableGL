#ifndef RSW_HALFEDGE_H
#define RSW_HALFEDGE_H

#include <rsw_math.h>

#include <vector>

using std::vector;

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


void compute_face_normals(vector<rsw::vec3>& verts, vector<rsw::ivec3>& triangles, vector<rsw::vec3>& normals);

void compute_half_edge(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tri, half_edge_data* he_data);


void compute_normals(vector<rsw::vec3>&verts, vector<rsw::ivec3>& triangles, half_edge_data* he_data, float sharp_angle, vector<rsw::vec3>& normals);



#endif

