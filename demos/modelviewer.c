#include "chalfedge.h"
#include "cprimitives.h"

#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define CVECTOR_ivec3_IMPLEMENTATION
#include "cvector_ivec3.h"
#define CVECTOR_vec2_IMPLEMENTATION
#include "cvector_vec2.h"
#define CVECTOR_vec3_IMPLEMENTATION
#include "cvector_vec3.h"

#define CVECTOR_IMPLEMENTATION
#define CVECTOR_ONLY_INT
#include "cvector.h"


#include <stdio.h>


#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480

typedef struct vert_attribs
{
	vec3 pos;
	vec3 normal;
} vert_attribs;

vert_attribs make_vert_attribs(vec3 p, vec3 n)
{
	vert_attribs v = { p, n };
	return v;
}

#define RESIZE(a) (((a)+1)*2)

CVEC_NEW_DECLS(vert_attribs)

CVEC_NEW_DEFS(vert_attribs, RESIZE)


vec4 Red = { 1.0f, 0.0f, 0.0f, 0.0f };
vec4 Green = { 0.0f, 1.0f, 0.0f, 0.0f };
vec4 Blue = { 0.0f, 0.0f, 1.0f, 0.0f };

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* texture;

u32* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	mat4 mv_mat;
	mat3 normal_mat;
	
	vec4 v_color;
	vec3 Ka;           // Ambient reflectivity
	vec3 Kd;           // Diffuse reflectivity
	vec3 Ks;           // Specular reflectivity
	float Shininess;   // Specular shininess factor

} My_Uniforms;


// TODO move to gltools?
int load_model(const char* filename, cvector_vec3* verts, cvector_ivec3* tris)
{
	FILE* file = NULL;
	unsigned int num = 0;

	if (!(file = fopen(filename, "r")))
		return 0;

	fscanf(file, "%u", &num);
	if (!num)
		return 0;

	printf("%u vertices\n", num);
	
	if (!cvec_vec3(verts, num, num))
		return 0;

	for (int i=0; i<num; ++i)
		fread_vec3(file, &verts->a[i]);

	fscanf(file, "%u", &num);
	if (!num)
		return 0;

	printf("%u triangles\n", num);
	
	if (!cvec_ivec3(tris, num, num))
		return 0;

	for (int i=0; i<num; ++i)
		fread_ivec3(file, &tris->a[i]);

	fclose(file);

	return 1;
}

void cleanup();
void setup_context();
int handle_events();


void normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void gouraud_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void gouraud_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void phong_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void phong_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);


int polygon_mode;
int cur_prog;
#define NUM_PROGRAMS 2
GLuint programs[NUM_PROGRAMS];
const char* shaders[NUM_PROGRAMS] = { "gouraud", "phong" };

int main(int argc, char** argv)
{
	setup_context();

	GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };

	polygon_mode = 2;


	mat4 proj_mat, rot_mat, vp_mat, view_mat;

	My_Uniforms the_uniforms;

	make_perspective_matrix(proj_mat, DEG_TO_RAD(45), WIDTH/(float)HEIGHT, 1, 100);
	lookAt(view_mat, make_vec3(0, 5, 20), make_vec3(0, 5, -1), make_vec3(0, 1, 0));
	mult_mat4_mat4(vp_mat, proj_mat, view_mat);

	cvector_vec3 verts = { 0 };
	cvector_ivec3 tris = { 0 };
	cvector_vec2 texcoords = { 0 };
	
	vec3 offset = { 0, 5, -1 };

	if (argc == 1) {
		printf("usage: %s [model_file]\n", argv[0]);
		printf("No model given, so generating a sphere...\n");
		generate_sphere(&verts, &tris, &texcoords, 5.0f, 14, 7);

		// translate so it's in the same position as the models
		// couuld also change the camera but meh
		for (int i=0; i<verts.size; ++i)
			verts.a[i] = add_vec3s(verts.a[i], offset);
	} else {
		if (!load_model(argv[1], &verts, &tris)) {
			printf("Failed to load %s!\nGenerating a sphere instead.\n", argv[1]);
			verts.size = 0;
			tris.size = 0;
			texcoords.size = 0;
			generate_sphere(&verts, &tris, &texcoords, 5.0f, 14, 7);
			for (int i=0; i<verts.size; ++i)
				verts.a[i] = add_vec3s(verts.a[i], offset);
		}
	}

	cvector_vec3 face_normals;
	cvec_vec3(&face_normals, 0, tris.size);
	//compute_face_normals(&verts, &tris, &face_normals);
	
	cvector_vec3 normals = { 0 };
	compute_normals(&verts, &tris, NULL, RM_PI/2, &normals);

	cvector_vec3 expanded_verts;
	//cvector_vec3 expanded_normals;

	cvec_vec3(&expanded_verts, 0, tris.size*3);

	for (int i=0; i<tris.size; ++i) {
		cvec_push_vec3(&expanded_verts, verts.a[tris.a[i].x]);
		cvec_push_vec3(&expanded_verts, verts.a[tris.a[i].y]);
		cvec_push_vec3(&expanded_verts, verts.a[tris.a[i].z]);
	}

	int v;
	cvector_vec3 normal_lines = { 0 };
	cvector_vert_attribs vert_data = { 0 };
	cvec_vert_attribs(&vert_data, 0, tris.size*3);
	for (int i=0, j=0; i<tris.size; ++i, j=i*3) {
		v = tris.a[i].x;
		cvec_push_vert_attribs(&vert_data, make_vert_attribs(verts.a[v], normals.a[j]));
		cvec_push_vec3(&normal_lines, verts.a[v]);
		cvec_push_vec3(&normal_lines, add_vec3s(verts.a[v], scale_vec3(normals.a[j], 0.5f)));

		v = tris.a[i].y;
		cvec_push_vert_attribs(&vert_data, make_vert_attribs(verts.a[v], normals.a[j+1]));
		cvec_push_vec3(&normal_lines, verts.a[v]);
		cvec_push_vec3(&normal_lines, add_vec3s(verts.a[v], scale_vec3(normals.a[j+1], 0.5f)));

		v = tris.a[i].z;
		cvec_push_vert_attribs(&vert_data, make_vert_attribs(verts.a[v], normals.a[j+2]));
		cvec_push_vec3(&normal_lines, verts.a[v]);
		cvec_push_vec3(&normal_lines, add_vec3s(verts.a[v], scale_vec3(normals.a[j+2], 0.5f)));
	}


	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, vert_data.size*sizeof(vert_attribs), &vert_data.a[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vert_attribs), 0);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(vert_attribs), sizeof(vec3));



	// TODO add optional normal lines if/when I add them back to opengl_reference version
	//GLuint myshader = pglCreateProgram(normal_vs, normal_fs, 0, NULL, GL_FALSE);
	programs[0] = pglCreateProgram(gouraud_vs, gouraud_fs, 3, smooth, GL_FALSE);
	glUseProgram(programs[0]);
	set_uniform(&the_uniforms);
	programs[1] = pglCreateProgram(phong_vs, phong_fs, 3, smooth, GL_FALSE);
	glUseProgram(programs[1]);
	set_uniform(&the_uniforms);

	// start with gouraud (global inited to 0)
	glUseProgram(programs[cur_prog]);

	the_uniforms.v_color = Red;


	SET_VEC3(the_uniforms.Ka, 0.3, 0.3, 0.3);
	SET_VEC3(the_uniforms.Kd, 0.3, 0.3, 0.3);
	SET_VEC3(the_uniforms.Ks, 1.0, 1.0, 1.0);
	the_uniforms.Shininess = 128.0f;


	glClearColor(0, 0, 0, 1);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);


	unsigned int old_time = 0, new_time=0, counter = 0;

	while (1) {
		if (handle_events()) {
			break;
		}

		
		++counter;
		new_time = SDL_GetTicks();
		if (new_time - old_time >= 3000) {
			printf("%f FPS\n", counter*1000.0f/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		vec3 y_axis = { 0, 1, 0 };
		load_rotation_mat4(rot_mat, y_axis, DEG_TO_RAD(30)*new_time/1000.0f);
		
		extract_rotation_mat4(the_uniforms.normal_mat, rot_mat, 0);

		mult_mat4_mat4(the_uniforms.mvp_mat, vp_mat, rot_mat);

		glDrawArrays(GL_TRIANGLES, 0, expanded_verts.size);

		SDL_UpdateTexture(texture, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, texture, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), ((vec4*)vertex_attribs)[0]);
}

void normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((My_Uniforms*)uniforms)->v_color;
}

inline vec3 reflect(vec3 i, vec3 n)
{
	//return i - 2 * dot_vec3s(i, n) * n;
	return sub_vec3s(i, scale_vec3(n, 2*dot_vec3s(i, n)));
}
extern inline vec3 reflect(vec3 i, vec3 n);

void gouraud_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	//convenience
	vec4* v_attribs = vertex_attribs;
	vec3* vs_out = (vec3*)vs_output;;
	My_Uniforms* u = uniforms;

	vec3 color = u->Ka;

	//directional light and non-local viewer for now
	vec3 light_dir = { -1, 1, 0.5 };
	light_dir = norm_vec3(light_dir);
	vec3 eye_dir = { 0, 0, 1 };


	vec3 eye_normal = mult_mat3_vec3(u->normal_mat, *(vec3*)&v_attribs[4]);
	//vec3 eye_normal = *(vec3*)&v_attribs[4];

	//prevent double dot calc using macro
	float tmp = dot_vec3s(light_dir, eye_normal);
	float diff_intensity = MAX(tmp, 0.0);

	color = add_vec3s(color, scale_vec3(u->Kd, diff_intensity));


	vec3 r = reflect(negate_vec3(light_dir), eye_normal);
	tmp = dot_vec3s(eye_dir, r);
	float spec = MAX(0.0, tmp);
	float fspec;
	if (diff_intensity > 0) {
		fspec = pow(spec, u->Shininess);
		color = add_vec3s(color, scale_vec3(u->Ks, fspec));
	}

	vs_out[0] = color;

	builtins->gl_Position = mult_mat4_vec4(u->mvp_mat, v_attribs[0]);
}

void gouraud_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 color = ((vec3*)fs_input)[0];

	builtins->gl_FragColor.x = color.x;
	builtins->gl_FragColor.y = color.y;
	builtins->gl_FragColor.z = color.z;
	builtins->gl_FragColor.w = 1;

}

void phong_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	//convenience
	vec4* v_attribs = vertex_attribs;
	vec3* vs_out = (vec3*)vs_output;;
	My_Uniforms* u = uniforms;

	vec3 eye_normal = mult_mat3_vec3(u->normal_mat, *(vec3*)&v_attribs[4]);

	vs_out[0] = eye_normal;

	builtins->gl_Position = mult_mat4_vec4(u->mvp_mat, v_attribs[0]);
}

// TODO change to grayscale to match opengl_reference modelviewer?  scalar Ka/d/s?
void phong_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 eye_normal = ((vec3*)fs_input)[0];
	My_Uniforms* u = uniforms;

	//directional light and non-local viewer for now
	vec3 light_dir = { -1, 1, 0.5 };
	vec3 eye_dir = { 0, 0, 1 };
	
	vec3 s = norm_vec3(light_dir);
	vec3 n = norm_vec3(eye_normal);
	vec3 v = eye_dir;

	vec3 color = u->Ka;

	//prevent double dot calc using macro
	float tmp = dot_vec3s(s, n);
	float diff_intensity = MAX(tmp, 0.0);

	color = add_vec3s(color, scale_vec3(u->Kd, diff_intensity));

	vec3 r = reflect(negate_vec3(s), n);
	tmp = dot_vec3s(v, r);
	float spec = MAX(0.0, tmp);
	float fspec;
	if (diff_intensity > 0) {
		fspec = pow(spec, u->Shininess);
		color = add_vec3s(color, scale_vec3(u->Ks, fspec));
	}

	builtins->gl_FragColor.x = color.x;
	builtins->gl_FragColor.y = color.y;
	builtins->gl_FragColor.z = color.z;
	builtins->gl_FragColor.w = 1;
}


void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		printf("SDL_init error: %s\n", SDL_GetError());
		exit(0);
	}

	ren = NULL;
	texture = NULL;
	
	SDL_Window* window = SDL_CreateWindow("Modelviewer", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Failed to create window\n");
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
	set_glContext(&the_Context);
}

void cleanup()
{

	free_glContext(&the_Context);

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}


int handle_events()
{
	SDL_Event e;
	int sc;

	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT)
			return 1;
		if (e.type == SDL_KEYDOWN) {
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

				printf("switching to the %s shader\n", shaders[cur_prog]);
				glUseProgram(programs[cur_prog]);
			}
		} else if (e.type == SDL_MOUSEBUTTONDOWN) {
		}
	}
	return 0;
}


