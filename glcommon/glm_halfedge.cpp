#include "glm_halfedge.h"


#include <stdio.h>
#include <math.h>

using glm::vec3;
using glm::ivec3;

using namespace std;




void compute_face_normals(vector<vec3>& verts, vector<ivec3>& t, vector<vec3>& normals)
{
	vec3 tmp, v1, v2;
	for (int i=0; i<t.size(); ++i) {
		v1 = verts[t[i].y] - verts[t[i].x];
		v2 = verts[t[i].z] - verts[t[i].x];
		tmp = glm::cross(v1, v2);

		tmp = normalize(tmp);
		normals.push_back(tmp);
		normals.push_back(tmp);
		normals.push_back(tmp);
	}
}


void compute_half_edge(vector<vec3>& verts, vector<ivec3>& tri, half_edge_data* he_data)
{
	if (!he_data)
		return;

	int n_verts = verts.size();
	int n_tris = tri.size();
	
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
		pts = &tri[i].x;
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

					edge_entries[cnt].tri_index[1] = -1;
					edge_entries[cnt].edge_number[1] = -1;
					
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
	
	
	cnt = 0;   //number of edges
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
		face_array[i] = cnt;      //fill in face array with first half edge (it doesn't matter which one)
		pts = &tri[i].x;
		for (int j=0; j<3; ++j, ++cnt) {
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
					if (edge->tri_index[0] == i) {
						if (edge->tri_index[1] != -1)
							he_array[cnt].pair = edge->tri_index[1]*3 + edge->edge_number[1];
					} else {
						he_array[cnt].pair = edge->tri_index[0]*3 + edge->edge_number[0];
					}
						
					break;
				}
			}
		}
	}

	//make sure a boundary vertex points
	//to the boundary half edge so we can loop over
	//all the faces of that vertex
	for (int i=0; i<cnt; ++i) {
		if (he_array[i].pair == -1)
			v_array[he_array[i].v] = i;
	}
	
	
	free(edge_lists);
	free(edge_entries);

	he_data->he_array = he_array;
	he_data->v_array = v_array;
	he_data->face_array = face_array;
}


//stupid
inline bool eql_epsilon_vec3(vec3 a, vec3 b, float epsilon)
{
	return ((fabs(a.x-b.x)<epsilon) && (fabs(a.y-b.y)<epsilon) && (fabs(a.z-b.z)<epsilon));
}


/*

void compute_normals(vector<vec3>&verts, vector<ivec3>& t, half_edge_data* he_data_in, float sharp_angle, vector<vec3>&normals)
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


	vector<vec3> face_normals;
	for (int i=0; i<t.size(); ++i) {
		v1 = verts[t[i].y] - verts[t[i].x];
		v2 = verts[t[i].z] - verts[t[i].x];
		tmp = normalize(glm::cross(v1, v2));
	
		face_normals.push_back(tmp);
	}

	int edge = -1, edge2 = -1, face2=-1;
	float angle = 0;
	vector<vec3> tmp_normals;
	vector<int> tmp_verts;

	//I guess this way works best but it doesn't handle complex cases well
	vector<vec3> v_normals;
	for (int i=0; i<verts.size(); ++i) {
		tmp_normals.clear();
		edge = v_array[i];
		
		v1 = face_normals[he_array[edge].face];
		ave_normal = v1;

		edge2 = he_array[he_array[edge].next].pair;
		while (edge2 != -1 && edge2 != edge) {
			face2 = he_array[edge2].face;
			v2 = face_normals[face2];
			
			//v1 and v2 are from face_normals which was already unit length
			angle = acos(glm::dot(v1, v2));
			
			//fprintf(stderr, "angle = %f\t%f\n", angle, v1*v2);
			
			if (angle < sharp_angle) {
				//ave_normal += v2;
				int k;
				for (k=0; k<tmp_normals.size(); ++k) {
					if (eql_epsilon_vec3(tmp_normals[k], v2, 0.0001f))
						break;
				}
				if (k == tmp_normals.size()) {
					tmp_normals.push_back(v2);
					ave_normal += v2;
					//fprintf(stderr, "adding another normal to %d.%d\n", i, j);
				}
			}
			
			edge2 = he_array[he_array[edge2].next].pair;
		}
		ave_normal = normalize(ave_normal);
		v_normals.push_back(ave_normal);    //should I normalize?  I normalize in shader so it's kind of waste . . .
	}

	for (int i=0; i<t.size(); ++i) {
		normals.push_back(v_normals[t[i].x]);
		normals.push_back(v_normals[t[i].y]);
		normals.push_back(v_normals[t[i].z]);
	}
}

*/




void compute_normals(vector<vec3>& verts, vector<ivec3>& t, half_edge_data* he_data_in, float sharp_angle, vector<vec3>& normals)
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

	vector<vec3> face_normals;
	for (int i=0; i<t.size(); ++i) {
		v1 = verts[t[i].y] - verts[t[i].x];
		v2 = verts[t[i].z] - verts[t[i].x];
		tmp = normalize(glm::cross(v1, v2));
	
		face_normals.push_back(tmp);
		//printf("(%f, %f, %f)\n", tmp.x, tmp.y, tmp.z);
	}
	//printf("face_normals\n");
	//getchar();
	
	int edge = -1, edge2 = -1, face2=-1;
	float angle = 0;
	vec3 zero(0);
	vector<vec3> tmp_normals;
	vector<int> tmp_verts;
	int* pts, *pts2;
	
	if (sharp_angle > 0) {
		for (int i=0; i<t.size()*3; ++i)
			normals.push_back(vec3(0));  //initialize to (0, 0, 0)
		
		//there's gotta be a better way but I can't think of one now.
		//If I loop vertices how do I split them up correctly when I expand?
		for (int i=0; i<t.size(); ++i) {
			pts = &t[i].x;

			for (int j=0; j<3; ++j) {
				if (normals[i*3+j] != zero)
					continue;
				
				tmp_normals.clear();
				tmp_verts.clear();
				
				tmp_verts.push_back(i*3+j);
				edge = v_array[pts[j]];
				
				v1 = face_normals[i];
				ave_normal = v1;

				edge2 = he_array[he_array[edge].next].pair;
				while (edge2 != -1 && edge2 != edge) {
					face2 = he_array[edge2].face;
					v2 = face_normals[face2];

					pts2 = &t[face2].x;
					
					int k;
					//figure out which vertex of the triangle it is for the new face and add it to list
					//of verts that will share this normal
					for (k=0; k<3; ++k)
						if (pts2[k] == pts[j])
							break;
						
					//fprintf(stderr, "k = %d\t", k);
					
					//that normal is already accounted for
					if (normals[face2*3+k] != zero) {
						edge2 = he_array[he_array[edge2].next].pair;
						continue;
					}
					
					//same face normals
					if (eql_epsilon_vec3(v1, v2, 0.00001f)) {
						tmp_verts.push_back(face2*3+k);
						edge2 = he_array[he_array[edge2].next].pair;
						continue;
					}
					
					//v1 and v2 are from face_normals which was already unit length
					angle = acos(glm::dot(v1, v2));
					
					//fprintf(stderr, "angle = %f\t%f\n", angle, v1*v2);
					
					if (angle < sharp_angle) {
						if (normals[face2*3+k] == zero) {
							tmp_verts.push_back(face2*3+k);
						}
						
						//check if that normal has already been added (ie 2 triangles in same plane)
						for (k=0; k<tmp_normals.size(); ++k) {
							if (eql_epsilon_vec3(tmp_normals[k], v2, 0.00001f)) {
								//fprintf(stderr, "preventing adding same normal\n");
								break;
								
							}
						}
						if (k == tmp_normals.size()) {
							tmp_normals.push_back(v2);
							ave_normal += v2;
							//fprintf(stderr, "adding another normal to %d.%d\n", i, j);
						}
					} else {
						//fprintf(stderr, "angle too sharp %d %d\n", face2, i);
						//std::cout<<v1<<"\t"<<v2<<"\t"<<length(v1)<<"\t"<<length(v2)<<"\t"<<angle<<"\t"<<sharp_angle<<"\n\n";
					}
					
					edge2 = he_array[he_array[edge2].next].pair;
				} //while (edge2 != edge1)
				
				
				//loop through list of "verts" setting them to the averaged normal
				ave_normal = normalize(ave_normal);
				//fprintf(stderr, "tmp_verts size = %lu\t", tmp_verts.size());
				for (int k=0; k<tmp_verts.size(); ++k)
					normals[tmp_verts[k]] = ave_normal;
				
				//normals.push_back(ave_normal.norm());		//should I normalize?  I normalize in shader so it's kind of waste . . .'
			}
		}

	} else {   //do face normals
		for (int i=0; i<t.size(); ++i) {
			normals.push_back(face_normals[i]);
			normals.push_back(face_normals[i]);
			normals.push_back(face_normals[i]);
		}
	}
}

