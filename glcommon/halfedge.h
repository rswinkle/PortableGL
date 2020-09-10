#ifndef HALFEDGE_H
#define HALFEDGE_H

#include "rsw_math.h"

#include <vector>

using namespace std;

using rsw::vec3;
using rsw::ivec3;



class half_edge
{
public:
	half_edge() : next(-1), pair(-1), face(-1), v(-1) {}
	int next;
	int pair;
	int face;
	int v;
};



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


void compute_face_normals(vector<vec3>& verts, vector<ivec3>& triangles, vector<vec3>& normals);

void compute_half_edge(vector<vec3>& verts, vector<ivec3>& tri, half_edge_data* he_data);


void compute_normals(vector<vec3>&verts, vector<ivec3>& triangles, half_edge_data* he_data, float sharp_angle, vector<vec3>& normals);

#endif
