#define PGL_MANGLE_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <rsw_primitives.h>
#include <rsw_halfedge.h>
#include <rsw_math.h>


#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <stdio.h>
#include <vector>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

using rsw::vec3;
using rsw::vec4;
using rsw::mat3;
using rsw::mat4;


SDL_Window* win;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	mat3 normal_mat;

	vec4 color;

	// not actually used, hardcoded light position
	//vec3 light_dir;

	// as if all lights/materials are white so only have to store intensity
	vec4 material; // (ambient, diffuse, specular, specular_intensity)

} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();

void basic_transform_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void gouraud_ads_grayscale_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void gouraud_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void phong_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void phong_ads_grayscale_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

struct vert_attribs
{
	vec3 pos;
	vec3 normal;

	vert_attribs(vec3 p, vec3 n) : pos(p), normal(n) {}
};

// lots of data duplication here but it's just a quick demo
struct mesh
{
	vector<vec3> verts;
	vector<ivec3> tris;
	vector<vec2> tex;
	vector<vec3> face_normals;
	vector<vec3> normals;

	vector<vec3> draw_verts;

	vector<vert_attribs> vert_data;
	GLuint vao;
	GLuint buffer;

	vector<vec3> face_normal_lines;
	vector<vec3> normal_lines;
	GLuint normal_vao;
	GLuint norm_buf;

};

enum {
	BOX,
	CYLINDER,
	PLANE,
	SPHERE,
	TORUS,
	CONE,
	TETRA,
	CUBE,
	OCTA,
	DODECA,
	ICOSA,
	NUM_SHAPES
};

mesh shapes[NUM_SHAPES];

int polygon_mode;
int cur_prog;
int cur_shape;
bool show_normals;
bool face_normals;

typedef struct shader_pair
{
	vert_func vs;
	frag_func fs;
	int n_interp;
	int interp[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	int use_fragdepth_or_discard;
} shader_pair;

#define NUM_PROGRAMS 2
shader_pair shader_pairs[NUM_PROGRAMS] =
{
	{ gouraud_ads_grayscale_vs, gouraud_fs, 3, { PGL_SMOOTH3 }, GL_FALSE },
	{ phong_vs, phong_ads_grayscale_fs, 3, { PGL_SMOOTH3 }, GL_FALSE }
};

GLuint programs[NUM_PROGRAMS];
const char* shader_names[NUM_PROGRAMS] = { "gouraud", "phong" };

int main(int argc, char** argv)
{
	setup_context();

	polygon_mode = 2;
	cur_prog = 0;
	face_normals = true;

	vec3 tmp;

	make_box(shapes[BOX].verts, shapes[BOX].tris, shapes[BOX].tex, 2.0f, 2.0f, 2.0f, true, ivec3(4, 4, 4), vec3(0,0,0));
	make_cylinder(shapes[CYLINDER].verts, shapes[CYLINDER].tris, shapes[CYLINDER].tex, 1, 2, 30);
	make_plane(shapes[PLANE].verts, shapes[PLANE].tris, shapes[PLANE].tex, vec3(-2, -2, 0), vec3(0,1,0), vec3(1,0,0), 4, 4, false);
	make_sphere(shapes[SPHERE].verts, shapes[SPHERE].tris, shapes[SPHERE].tex, 2.0f, 30, 15);
	make_torus(shapes[TORUS].verts, shapes[TORUS].tris, shapes[TORUS].tex, 2.0f, 0.6f, 40, 20);
	make_cone(shapes[CONE].verts, shapes[CONE].tris, shapes[CONE].tex, 1, 2, 30, 1);

	make_tetrahedron(shapes[TETRA].verts, shapes[TETRA].tris);
	make_cube(shapes[CUBE].verts, shapes[CUBE].tris);
	make_octahedron(shapes[OCTA].verts, shapes[OCTA].tris);
	make_dodecahedron(shapes[DODECA].verts, shapes[DODECA].tris);
	make_icosahedron(shapes[ICOSA].verts, shapes[ICOSA].tris);

	vector<vec3> draw_verts;
	for (int j=0; j<NUM_SHAPES; j++) {
		mesh& s = shapes[j];

		expand_verts(s.draw_verts, s.verts, s.tris);

		compute_face_normals(s.verts, s.tris, s.face_normals);
		compute_normals(s.verts, s.tris, NULL, RM_PI/6, s.normals);

		for (int i=0; i<s.draw_verts.size(); i+=3) {
			s.vert_data.push_back(vert_attribs(s.draw_verts[i],   s.face_normals[i]));
			s.vert_data.push_back(vert_attribs(s.draw_verts[i+1], s.face_normals[i+1]));
			s.vert_data.push_back(vert_attribs(s.draw_verts[i+2], s.face_normals[i+2]));

			tmp = s.draw_verts[i] + s.draw_verts[i+1] + s.draw_verts[i+2];
			tmp /= 3;

			s.face_normal_lines.push_back(tmp);
			s.face_normal_lines.push_back(tmp + s.face_normals[i]*0.5f);

			s.normal_lines.push_back(s.draw_verts[i]);
			s.normal_lines.push_back(s.draw_verts[i] + s.normals[i]*0.5f);
			s.normal_lines.push_back(s.draw_verts[i+1]);
			s.normal_lines.push_back(s.draw_verts[i+1] + s.normals[i+1]*0.5f);
			s.normal_lines.push_back(s.draw_verts[i+2]);
			s.normal_lines.push_back(s.draw_verts[i+2] + s.normals[i+2]*0.5f);
		}

		glGenVertexArrays(1, &s.vao);
		glBindVertexArray(s.vao);

		glGenBuffers(1, &s.buffer);
		glBindBuffer(GL_ARRAY_BUFFER, s.buffer);
		glBufferData(GL_ARRAY_BUFFER, s.vert_data.size()*sizeof(vert_attribs), &s.vert_data[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vert_attribs), 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vert_attribs), (void*)sizeof(vec3));

		glGenVertexArrays(1, &s.normal_vao);
		glBindVertexArray(s.normal_vao);

		glGenBuffers(1, &s.norm_buf);
		glBindBuffer(GL_ARRAY_BUFFER, s.norm_buf);
		glBufferData(GL_ARRAY_BUFFER, s.face_normal_lines.size()*sizeof(vec3), &s.face_normal_lines[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


		glBindVertexArray(0);
	}

	My_Uniforms the_uniforms;

	GLuint normal_prog = pglCreateProgram(basic_transform_vs, uniform_color_fs, 0, NULL, GL_FALSE);
	glUseProgram(normal_prog);
	pglSetUniform(&the_uniforms);

	vec4 Red(1.0, 0.0, 0.0, 1.0);
	the_uniforms.color = Red;

	vec4 material(0.2, 0.6, 1.0, 128.0);
	the_uniforms.material = material;

	shader_pair* s;
	for (int i=0; i<NUM_PROGRAMS; ++i) {
		s = &shader_pairs[i];
		programs[i] = pglCreateProgram(s->vs, s->fs, s->n_interp, s->interp, s->use_fragdepth_or_discard);

		glUseProgram(programs[i]);
		pglSetUniform(&the_uniforms);
	}

	glUseProgram(programs[cur_prog]);

	mat4 proj_mat;
	mat4 view_mat;

	rsw::make_perspective_matrix(proj_mat, DEG_TO_RAD(45), WIDTH/(float)HEIGHT, 1.0f, 100.0f);
	rsw::lookAt(view_mat, vec3(0, 3, 10), vec3(0, 0, 0), vec3(0, 1, 0));

	mat4 mvp_mat;
	mat4 vp_mat = proj_mat * view_mat;
	mat4 rot_mat(1);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	unsigned int old_time = 0, new_time=0, counter = 0;
	while (1) {
		if (handle_events())
			break;

		counter++;
		new_time = SDL_GetTicks();
		if (new_time - old_time > 3000) {
			printf("%f FPS\n", counter*1000.f/(new_time-old_time));
			old_time = new_time;
			counter = 0;
		}

		glUseProgram(programs[cur_prog]);

		rsw::load_rotation_mat4(rot_mat, vec3(0, 1, 0), 0.5f*new_time/1000.0f);

		the_uniforms.normal_mat = mat3(view_mat*rot_mat);
		the_uniforms.mvp_mat = vp_mat * rot_mat;
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(shapes[cur_shape].vao);
		glDrawArrays(GL_TRIANGLES, 0, shapes[cur_shape].vert_data.size());

		if (show_normals) {
			glUseProgram(normal_prog);
			//mvp_mat already set

			glBindVertexArray(shapes[cur_shape].normal_vao);
			if (face_normals) {
				glDrawArrays(GL_LINES, 0, shapes[cur_shape].face_normal_lines.size());
			} else {
				glDrawArrays(GL_LINES, 0, shapes[cur_shape].normal_lines.size());
			}
		}

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	for (int i=0; i<NUM_SHAPES; i++) {
		glDeleteVertexArrays(1, &shapes[i].vao);
		glDeleteBuffers(1, &shapes[i].buffer);

		glDeleteVertexArrays(1, &shapes[i].normal_vao);
		glDeleteBuffers(1, &shapes[i].norm_buf);
	}
	glDeleteProgram(programs[cur_prog]);

	cleanup();

	return 0;
}

void basic_transform_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = (My_Uniforms*)uniforms;
	*(vec4*)&builtins->gl_Position = u->mvp_mat * ((vec4*)vertex_attribs)[0];
}

void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_FragColor = ((My_Uniforms*)uniforms)->color;
}

void gouraud_ads_grayscale_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	//convenience
	vec4* v_attribs = (vec4*)vertex_attribs;
	vec3* vs_out = (vec3*)vs_output;;
	My_Uniforms* u = (My_Uniforms*)uniforms;

	float Ka = u->material.x;
	float Kd = u->material.y;
	float Ks = u->material.z;
	float shininess = u->material.w;

	// constant directional light and non-local viewer for now
	vec3 light_dir(-1, 1, 0.5);
	light_dir.normalize();
	vec3 eye_dir(0, 0, 1);

	vec3 eye_normal = u->normal_mat * *(vec3*)&v_attribs[1];

	//prevent double dot calc using macro
	float tmp = dot(light_dir, eye_normal);
	float diff_intensity = MAX(tmp, 0.0);

	vec3 out_color = Ka + Kd * diff_intensity;


	vec3 r = rsw::reflect(-light_dir, eye_normal);
	tmp = dot(eye_dir, r);
	float spec = MAX(0.0, tmp);
	float fspec;
	if (diff_intensity > 0) {
		fspec = pow(spec, shininess);
		out_color += Ks * fspec;
	}

	vs_out[0] = out_color;
	*(vec4*)&builtins->gl_Position = u->mvp_mat * ((vec4*)v_attribs)[0];
}

void gouraud_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_vec4 color = { fs_input[0], fs_input[1], fs_input[2], 1.0f };
	builtins->gl_FragColor = color;
}

void phong_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	//convenience
	vec4* v_attribs = (vec4*)vertex_attribs;
	vec3* vs_out = (vec3*)vs_output;;
	My_Uniforms* u = (My_Uniforms*)uniforms;

	vec3 eye_normal = u->normal_mat * *(vec3*)&v_attribs[1];
	vs_out[0] = eye_normal;

	*(vec4*)&builtins->gl_Position = u->mvp_mat * v_attribs[0];
}

void phong_ads_grayscale_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 eye_normal = ((vec3*)fs_input)[0];
	My_Uniforms* u = (My_Uniforms*)uniforms;

	//directional light and non-local viewer for now
	vec3 light_dir(-1, 1, 0.5);
	vec3 eye_dir(0, 0, 1);
	
	vec3 s = normalize(light_dir);
	vec3 n = normalize(eye_normal);
	vec3 v = eye_dir;

	float Ka = u->material.x;
	float Kd = u->material.y;
	float Ks = u->material.z;
	float shininess = u->material.w;

	vec3 out_color(Ka);

	//prevent double dot calc using macro
	float tmp = dot(s, n);
	float diff_intensity = MAX(tmp, 0.0);

	out_color += Kd * diff_intensity;

	vec3 r = rsw::reflect(-s, n);
	tmp = dot(v, r);
	float spec = MAX(0.0, tmp);
	float fspec;
	if (diff_intensity > 0) {
		fspec = pow(spec, shininess);
		out_color += Ks * fspec;
	}

	*(vec4*)&builtins->gl_FragColor = vec4(out_color, 1.0f);
}


void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	win = SDL_CreateWindow("Test Primitives", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!win) {
		cleanup();
		exit(0);
	}

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
}

void cleanup()
{
	free_glContext(&the_Context);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);

	SDL_Quit();
}

int handle_events()
{
	SDL_Event e;
	int sc;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return 1;
		} else if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;

			if (sc == SDL_SCANCODE_ESCAPE) {
				return 1;
			} else if (sc == SDL_SCANCODE_P) {
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			} else if (sc == SDL_SCANCODE_S) {
				cur_prog = (cur_prog + 1) % NUM_PROGRAMS;
				puts(shader_names[cur_prog]);

			} else if (sc == SDL_SCANCODE_RIGHT) {
				cur_shape = (cur_shape + 1) % NUM_SHAPES;
			} else if (sc == SDL_SCANCODE_LEFT) {
				cur_shape--;
				if (cur_shape < 0) {
					cur_shape = NUM_SHAPES-1;
				}
			} else if (sc == SDL_SCANCODE_N) {
				show_normals = !show_normals;
			} else if (sc == SDL_SCANCODE_F) {
				face_normals = !face_normals;

				// switch between face normals and blended/vert normals so phong shading actually makes a difference
				for (int j=0; j<NUM_SHAPES; j++) {
					mesh& s = shapes[j];
					vector<vec3>& n = (face_normals) ? s.face_normals : s.normals;
					for (int i=0; i<s.draw_verts.size(); i++) {
						s.vert_data[i].normal = n[i];
					}
					glBindVertexArray(s.vao);

					glBindBuffer(GL_ARRAY_BUFFER, s.buffer);
					glBufferData(GL_ARRAY_BUFFER, s.vert_data.size()*sizeof(vert_attribs), &s.vert_data[0], GL_STATIC_DRAW);

					glBindVertexArray(s.normal_vao);
					glBindBuffer(GL_ARRAY_BUFFER, s.norm_buf);
					if (face_normals) {
						glBufferData(GL_ARRAY_BUFFER, s.face_normal_lines.size()*sizeof(vec3), &s.face_normal_lines[0], GL_STATIC_DRAW);
					} else {
						glBufferData(GL_ARRAY_BUFFER, s.normal_lines.size()*sizeof(vec3), &s.normal_lines[0], GL_STATIC_DRAW);
					}
				}
			}
		}
	}
	return 0;
}





