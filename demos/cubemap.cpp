#include "rsw_math.h"

#define MANGLE_TYPES
#include "gltools.h"
#include "rsw_glframe.h"
#include "rsw_primitives.h"

#define PORTABLEGL_IMPLEMENTATION
#include "GLObjects.h"

#include "stb_image.h"

#include <iostream>
#include <vector>
#include <stdio.h>


#define SDL_MAIN_HANDLED
#include <SDL.h>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

using rsw::vec4;
using rsw::vec3;
using rsw::ivec3;
using rsw::vec2;
using rsw::mat4;
using rsw::mat3;


typedef struct My_Uniforms
{
	mat4 mvp_mat;
	mat4 mv_mat;
	mat3 normal_mat;
	mat4 inverse_camera;
	GLuint tex;
	vec4 v_color;
	
} My_Uniforms;

bool handle_events();
void cleanup();
void setup_context();
void setup_gl_data();


void reflection_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void reflection_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void skybox_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void skybox_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);



vec4 Red(1.0f, 0.0f, 0.0f, 0.0f);
vec4 Green(0.0f, 1.0f, 0.0f, 0.0f);
vec4 Blue(0.0f, 0.0f, 1.0f, 0.0f);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* SDL_tex;

u32* bbufpix;

glContext the_Context;

My_Uniforms the_uniforms;
GLFrame camera_frame(true, vec3(0, 0, 4));
int width, height, mousex, mousey;

int polygon_mode;

GLenum cube[6] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

const char* cube_map_textures[] =
{
	//"../media/textures/pos_x.tga",
	//"../media/textures/neg_x.tga",
	//"../media/textures/pos_y.tga",
	//"../media/textures/neg_y.tga",
	//"../media/textures/pos_z.tga",
	//"../media/textures/neg_z.tga"
	
	"../media/textures/skybox/right.jpg",
	"../media/textures/skybox/left.jpg",
	"../media/textures/skybox/top.jpg",
	"../media/textures/skybox/bottom.jpg",
	"../media/textures/skybox/front.jpg",
	"../media/textures/skybox/back.jpg"
};


int main(int argc, char** argv)
{

	setup_context();


	width = WIDTH;
	height = HEIGHT;

	polygon_mode = 2;

	//can't turn off C++ destructors
	{

	GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };

	rsw::Color test_texture[6][9];

	rsw::Color tmp[6] =
	{
		rsw::Color(255, 0, 0, 255),
		rsw::Color(0, 255, 0, 255),
		rsw::Color(0, 0, 255, 255),
		rsw::Color(255, 255, 0, 255),
		rsw::Color(255, 0, 255, 255),
		rsw::Color(0, 255, 255, 255)
	};
	for (int i=0; i<6; ++i) {
		for (int j=0; j<9; ++j) {
			test_texture[i][j] = tmp[i];
		}
	}



	GLuint cube_map_tex;
	glGenTextures(1, &cube_map_tex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_tex);

	load_texture_cubemap(cube_map_textures, GL_NEAREST, GL_NEAREST, GL_FALSE);
	
	/*
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	for (int i=0; i<6; ++i) {
		printf("I'm calling %d\n", cube[i]);
		glTexImage2D(cube[i], 0, GL_COMPRESSED_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, test_texture[i]);
		check_errors(i);
	}

	*/


	vector<vec3> box_verts;
	vector<vec2> box_tex;
	vector<ivec3> box_tris;
	make_box(box_verts, box_tris, box_tex, 40, 40, 40, false, ivec3(1,1,1), vec3(-20, -20, -20));
	//make_box(box_verts, box_tris, box_tex, 40, 40, 40, true, ivec3(2,2,2), vec3(-20, -20, -20));
	vector<vec3> box_draw_verts;


	expand_verts(box_draw_verts, box_verts, box_tris);
	
	vector<vec3> sphere_verts;
	vector<vec2> sphere_tex;
	vector<ivec3> sphere_tris;
	make_sphere(sphere_verts, sphere_tris, sphere_tex, 1.0f, 52, 26);
	vector<vec3> sphere_draw_verts;

	expand_verts(sphere_draw_verts, sphere_verts, sphere_tris);


	mat4 identity;

	
	GLuint box_vao;
	glGenVertexArrays(1, &box_vao);
	glBindVertexArray(box_vao);

	GLuint box;
	glGenBuffers(1, &box);
	glBindBuffer(GL_ARRAY_BUFFER, box);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*box_draw_verts.size()*3, &box_draw_verts[0], GL_STATIC_DRAW);


	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint sphere_vao;
	glGenVertexArrays(1, &sphere_vao);
	glBindVertexArray(sphere_vao);
	
	GLuint sphere[2];
	glGenBuffers(1, sphere);
	glBindBuffer(GL_ARRAY_BUFFER, sphere[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*sphere_draw_verts.size()*3, &sphere_draw_verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);



	GLuint reflection_shader = pglCreateProgram(reflection_vs, reflection_fs, 3, smooth, GL_FALSE);
	glUseProgram(reflection_shader);
	pglSetUniform(&the_uniforms);

	GLuint skybox_shader = pglCreateProgram(skybox_vs, skybox_fs, 3, smooth, GL_FALSE);
	glUseProgram(skybox_shader);

	pglSetUniform(&the_uniforms);

	mat4 camera, VP, MVP, projection, inverse_camera, camera_rot_only;
	mat3 normal_mat;

	rsw::make_perspective_matrix(projection, DEG_TO_RAD(35), float(WIDTH)/float(HEIGHT), 1.0f, 100.0f);


	the_uniforms.tex = cube_map_tex;

	SDL_SetRelativeMouseMode(SDL_TRUE);

	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);


	unsigned int old_time = 0, new_time=0, counter = 0;

	while (1) {
		if (handle_events())
			break;

		++counter;
		new_time = SDL_GetTicks();
		if (new_time - old_time >= 3000) {
			printf("%f FPS\n", counter*1000.0f/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}


		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClear(GL_COLOR_BUFFER_BIT);

		camera = camera_frame.get_camera_matrix();
		camera_rot_only = camera_frame.get_camera_matrix(true);
		normal_mat = extract_rotation_mat4(camera);

		the_uniforms.mvp_mat = projection * camera;
		the_uniforms.mv_mat = camera;
		the_uniforms.normal_mat = normal_mat;
		the_uniforms.inverse_camera = invert_mat4(camera_rot_only);
		
		
		glUseProgram(reflection_shader);

		glBindVertexArray(sphere_vao);
		glDrawArrays(GL_TRIANGLES, 0, sphere_draw_verts.size());

	



		the_uniforms.mvp_mat = projection * camera_rot_only;

		glDisable(GL_CULL_FACE);
		glUseProgram(skybox_shader);
		glBindVertexArray(box_vao);
		glDrawArrays(GL_TRIANGLES, 0, box_draw_verts.size());

		glEnable(GL_CULL_FACE);

		SDL_UpdateTexture(SDL_tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, SDL_tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}


	}

	cleanup();

	return 0;
}

void reflection_vs(float* vs_output, void* v_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* vertex_attribs = (vec4*)v_attribs;
	vec4* gl_Position = (vec4*)&builtins->gl_Position;
	My_Uniforms* the_uniforms = (My_Uniforms*)uniforms;

	//sphere has radius 1 so each vertex is it's own normal
	vec3 eye_normal = the_uniforms->normal_mat * vertex_attribs[0].xyz();
	vec4 eye_vert4 = the_uniforms->mv_mat * vertex_attribs[0];
	vec3 eye_vert = eye_vert4.vec3h().norm();

	vec4 coords = vec4(reflect(eye_vert, eye_normal), 1.0f);

	coords = the_uniforms->inverse_camera * coords;
	((vec3*)vs_output)[0] = coords.xyz().norm();

	*gl_Position = the_uniforms->mvp_mat * vertex_attribs[0];

}
void reflection_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 tex_coords = ((vec3*)fs_input)[0];

	builtins->gl_FragColor = texture_cubemap(((My_Uniforms*)uniforms)->tex, tex_coords.x, tex_coords.y, tex_coords.z);
}

void skybox_vs(float* vs_output, void* v_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* vertex_attribs = (vec4*)v_attribs;
	vec4* gl_Position = (vec4*)&builtins->gl_Position;
	My_Uniforms* the_uniforms = (My_Uniforms*)uniforms;


	((vec3*)vs_output)[0] = vertex_attribs[0].xyz().norm();
	
	*gl_Position = the_uniforms->mvp_mat * vertex_attribs[0];
}

void skybox_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 tex_coords = ((vec3*)fs_input)[0];

	//cout << tex_coords << "\n";

	builtins->gl_FragColor = texture_cubemap(((My_Uniforms*)uniforms)->tex, tex_coords.x, tex_coords.y, tex_coords.z);
}


void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("Cubemap", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	SDL_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
	set_glContext(&the_Context);
}

void cleanup()
{

	free_glContext(&the_Context);

	SDL_DestroyTexture(SDL_tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

bool handle_events()
{
	SDL_Event event;
	int sc;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			sc = event.key.keysym.scancode;
			//printf("%c %c\n", event.key.keysym.scancode, event.key.keysym.sym);
			//printf("Physical %s key acting as %s key",
			//SDL_GetScancodeName(keysym.scancode),
			//SDL_GetKeyName(keysym.sym));
			
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				return true;
			case SDL_SCANCODE_P:
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			default:
				;
			}


			break; //sdl_keydown
		case SDL_MOUSEMOTION:
		{
			float degx = event.motion.xrel/20.0f;
			float degy = event.motion.yrel/20.0f;
			
			camera_frame.rotate_local_y(DEG_TO_RAD(-degx));
			camera_frame.rotate_local_x(DEG_TO_RAD(degy));
		} break;

		case SDL_QUIT:
			return true;

		}
	}

	//SDL_PumpEvents() is called above in SDL_PollEvent()
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	static unsigned int last_time = 0, cur_time;

	cur_time = SDL_GetTicks();
	float time = (cur_time - last_time)/1000.0f;

#define MOVE_SPEED 1

	mat4 tmp;
	
	if (state[SDL_SCANCODE_A]) {
		camera_frame.move_right(time * MOVE_SPEED);
	}
	if (state[SDL_SCANCODE_D]) {
		camera_frame.move_right(time * -MOVE_SPEED);
	}
	if (state[SDL_SCANCODE_W]) {
		camera_frame.move_forward(time * MOVE_SPEED);
	}
	if (state[SDL_SCANCODE_S]) {
		camera_frame.move_forward(time * -MOVE_SPEED);
	}
	if (state[SDL_SCANCODE_Q]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(-60*time));
	}
	if (state[SDL_SCANCODE_E]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(60*time));
	}
	if (state[SDL_SCANCODE_LSHIFT]) {
		camera_frame.move_up(time * MOVE_SPEED);
	}
	if (state[SDL_SCANCODE_SPACE]) {
		camera_frame.move_up(time * -MOVE_SPEED);
	}

	last_time = cur_time;

	return false;
}
