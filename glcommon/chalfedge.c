#include "chalfedge.h"


#include <stdio.h>

void compute_face_normals(cvector_vec3* verts, cvector_ivec3* t, cvector_vec3* normals)
{
	vec3 tmp, v1, v2;
	for (int i=0; i<t->size; ++i) {
		v1 = sub_vec3s(verts->a[t->a[i].y], verts->a[t->a[i].x]);
		v2 = sub_vec3s(verts->a[t->a[i].z], verts->a[t->a[i].x]);
		tmp = cross_vec3s(v1, v2);

		tmp = norm_vec3(tmp);
		cvec_push_vec3(normals, tmp);

		// should I "expand" here? like rsw_halfedge.cpp or have it actually be "face" normals,
		// one per face?  Theoretically I might want that sometimes without expansion
		//cvec_push_vec3(normals, tmp);
		//cvec_push_vec3(normals, tmp);
	}
}


void compute_half_edge(cvector_vec3* verts, cvector_ivec3* tri, half_edge_data* he_data)
{
	if (!he_data)
		return;

	int n_verts = verts->size;
	int n_tris = tri->size;
	
	int cnt = 0;
	
	unsigned int max_edges = 3*n_tris;
	edge_entry **edge_lists = (edge_entry**) malloc(max_edges * sizeof(edge_entry*));
	edge_entry *edge_entries = (edge_entry*) malloc(max_edges * sizeof(edge_entry));
	int tmp;
	unsigned int hash_key;

	int* pts;
	
	
	for (int i=0; i<max_edges; ++i)
		edge_lists[i] = NULL;
	
	for (int i=0; i<n_tris; ++i) {
		pts = &tri->a[i].x;
		for (int j=2, k=0; k<3; j=k, k++) {
			int vj = pts[j];
			int vk = pts[k];
			
			if (vj > vk) {
				tmp = vj;
				vj = vk;
				vk = tmp;
			}
			
			hash_key = (vj*n_verts + vk) % max_edges;
			
			for (edge_entry *edge = edge_lists[hash_key]; ; edge = edge->next) {
				if (edge == NULL) {
					edge_entries[cnt].v_index[0] = vj;
					edge_entries[cnt].v_index[1] = vk;
					edge_entries[cnt].tri_index[0] = i;
					edge_entries[cnt].edge_number[0] = j;
					
					//insert new edge at front of linked list
					edge_entries[cnt].next = edge_lists[hash_key];
					edge_lists[hash_key] = &edge_entries[cnt++];
					
					break;
				}
				
				if (edge->v_index[0] == vj && edge->v_index[1] == vk) {
					edge->tri_index[1] = i;
					edge->edge_number[1] = j;
					break;
				}
			}
		}
	}
	
	
	cnt = 0;    //number of edges
	he_data->he_array = (half_edge*) malloc(max_edges * sizeof(half_edge));
	he_data->v_array = (int*) malloc(n_verts * sizeof(int));
	he_data->face_array = (int*) malloc(n_tris * sizeof(int));

	half_edge* he_array = he_data->he_array;
	int* v_array = he_data->v_array;
	int* face_array = he_data->face_array;

	for (int i=0; i<max_edges; ++i)
		he_array[i].pair = -1;
	
	for(int i=0; i<n_verts; ++i)
		v_array[i] = -1;
	
	for (int i=0; i<n_tris; ++i) {
		face_array[i] = cnt;        //fill in face array with first half edge (it doesn't matter which one)
		pts = &tri->a[i].x;
		for (int j=0; j<3; ++j, cnt++) {
			int vj = pts[j];
			int vk = pts[(j+1)%3];
			
			if (v_array[vk] < 0)
				v_array[vk] = cnt;     //if vertex isn't already filled set it to this half edge (any half edge that points to it works)
			
			
			he_array[cnt].next = cnt+((j!=2) ? 1 : -2);
			he_array[cnt].face = i;
			he_array[cnt].v = vk;
			
			
			if (vj > vk) {
				tmp = vj;
				vj = vk;
				vk = tmp;
			}
			hash_key = (vj*n_verts + vk) % max_edges;
			
			for (edge_entry *edge = edge_lists[hash_key]; ; edge = edge->next) {
				if (edge->v_index[0] == vj && edge->v_index[1] == vk) {
					if (edge->tri_index[0] == i)
						he_array[cnt].pair = edge->tri_index[1]*3 + edge->edge_number[1];
					else
						he_array[cnt].pair = edge->tri_index[0]*3 + edge->edge_number[0];
						
					break;
				}
			}	
		}
	}
	
	
	free(edge_lists);
	free(edge_entries);

	he_data->he_array = he_array;
	he_data->v_array = v_array;
	he_data->face_array = face_array;
}


void compute_normals(cvector_vec3* verts, cvector_ivec3* t, half_edge_data* he_data_in, float sharp_angle, cvector_vec3* normals)
{
	vec3 tmp, v1, v2, ave_normal;

	half_edge_data he_data;
	if (!he_data_in)
		compute_half_edge(verts, t, &he_data);
	else
		he_data = *he_data_in;


	half_edge* he_array = he_data.he_array;
	int* v_array = he_data.v_array;
	int* face_array = he_data.face_array;


	cvector_vec3 face_normals = { 0 };
	for (int i=0; i<t->size; ++i) {
		v1 = sub_vec3s(verts->a[t->a[i].y],  verts->a[t->a[i].x]);
		v2 = sub_vec3s(verts->a[t->a[i].z],  verts->a[t->a[i].x]);
		tmp = cross_vec3s(v1, v2);
	
		cvec_push_vec3(&face_normals, norm_vec3(tmp));
	}
	
	int edge = -1, edge2 = -1, face2=-1;
	float angle = 0;
	vec3 zero = { 0.0f, 0.0f, 0.0f }; // could just do { 0 } but better to be explicit
	cvector_vec3 tmp_normals = { 0 };
	cvector_i tmp_verts = { 0 };
	int* pts, *pts2;
	
	if (sharp_angle > 0) {
		for (int i=0; i<t->size*3; ++i)
			cvec_push_vec3(normals, zero);       //initialize to (0, 0, 0)
		
		//there's gotta be a better way but I can't think of one now.  I know this duplicates computations
		//If I loop vertices how do I split them up to the correct faces when I expand?
		for (int i=0; i<t->size; ++i) {
			pts = &t->a[i].x;

			for (int j=0; j<3; ++j) {
				if (!equal_vec3s(normals->a[i*3+j], zero))
					continue;
				
				tmp_normals.size = 0;
				tmp_verts.size = 0;
				
				cvec_push_i(&tmp_verts, i*3+j);
				edge = v_array[pts[j]];
				
				v1 = face_normals.a[i];
				ave_normal = v1;

				edge2 = he_array[he_array[edge].next].pair;
				while (edge2 != edge) {
					face2 = he_array[edge2].face;
					v2 = face_normals.a[face2];

					pts2 = &t->a[face2].x;
					
					int k;
					//figure out which vertex of the triangle it is for the new face and add it to list
					//of verts that will share this normal
					for (k=0; k<3; ++k)
						if (pts2[k] == pts[j])
							break;
						
					//fprintf(stderr, "k = %d\t", k);
					
					//that normal is already accounted for
					if (!equal_vec3s(normals->a[face2*3+k], zero)) {
						edge2 = he_array[he_array[edge2].next].pair;
						continue;
					}
					
					//same face normals
					if (equal_epsilon_vec3s(v1, v2, 0.00001f)) {
						cvec_push_i(&tmp_verts, face2*3+k);
						edge2 = he_array[he_array[edge2].next].pair;
						
						//printf("prevented face equal %d\n\n", i);
						continue;
					}
					
					
					//v1 and v2 are from face_normals which was already unit length
					angle = angle_vec3s(v1, v2);
					
					
					if (angle < sharp_angle) {
						
						if (equal_vec3s(normals->a[face2*3+k], zero)) {
							cvec_push_i(&tmp_verts, face2*3+k);
						}
						
						//check if that normal has already been added (ie 2 triangles in same plane)
						for (k=0; k<tmp_normals.size; ++k) {
							if (equal_epsilon_vec3s(tmp_normals.a[k], v2, 0.00001f)) {
								//fprintf(stderr, "preventing adding same normal\n");
								break;
								
							}
						}
						if (k == tmp_normals.size) {
							cvec_push_vec3(&tmp_normals, v2);
							ave_normal = add_vec3s(ave_normal, v2);
							//fprintf(stderr, "adding another normal to %d.%d\n", i, j);
						}
					} else {
						//fprintf(stderr, "angle too sharp %d %d\n", face2, i);
					}
					
					edge2 = he_array[he_array[edge2].next].pair;
				} //while (edge2 != edge1)
				
				
				//loop through list of "verts" setting them to the averaged normal
				normalize_vec3(&ave_normal);
				//fprintf(stderr, "tmp_verts size = %lu\t", tmp_verts.size);
				for (int k=0; k<tmp_verts.size; ++k)
					normals->a[tmp_verts.a[k]] = ave_normal;
				
				//normals.push_back(ave_normal.norm());		//should I normalize?  I normalize in shader so it's kind of waste . . .'
			}
		}



	} else {  //do face normals
		for (int i=0; i<t->size; ++i) {
			cvec_push_vec3(normals, face_normals.a[i]);
			cvec_push_vec3(normals, face_normals.a[i]);
			cvec_push_vec3(normals, face_normals.a[i]);
		}
	}
}	

