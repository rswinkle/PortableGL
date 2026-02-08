#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"

#include <SDL.h>

#include <glm_matstack.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#define my_chdir(x) chdir(x)
#elif defined(_WIN32)
#include <direct.h>
#define my_chdir(x) _chdir(x)
#endif

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024


#define WIDTH 800
#define HEIGHT 480

#define GUI_W 200

#ifndef FPS_EVERY_N_SECS
#define FPS_EVERY_N_SECS 1
#endif

#define FPS_DELAY (FPS_EVERY_N_SECS*1000)

using namespace std;

using glm::ivec3;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;
struct nk_context* ctx;

pix_t* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mv_mat;
	mat4 proj_mat;
	mat3 norm_mat;

	vec3 ambient;

	vec3 pnt_light;
	vec3 pnt_light_color;

	GLuint tex;

	int use_lighting;
	int use_textures;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();
void setup_buffers();

void point_light_per_vertex_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void point_light_per_vertex_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void point_light_per_frag_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void point_light_per_frag_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

float z;
int polygon_mode;
int use_frag_lighting = 1;

GLuint cube_vao, moon_vao;
int cube_tris, moon_tris;

int main(int argc, char** argv)
{
	setup_context();

	setup_buffers();

	polygon_mode = 2;

	GLuint box_tex, moon_tex;
	glGenTextures(1, &box_tex);
	glBindTexture(GL_TEXTURE_2D, box_tex);
	if (!load_texture2D("../../media/textures/crate.gif", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, GL_REPEAT, GL_TRUE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}

	glGenTextures(1, &moon_tex);
	glBindTexture(GL_TEXTURE_2D, moon_tex);
	if (!load_texture2D("../../media/textures/moon.gif", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, GL_REPEAT, GL_TRUE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}

	My_Uniforms the_uniforms;

	// Using this array for both
	GLenum vs_outs[] = { PGL_SMOOTH2, PGL_SMOOTH3, PGL_SMOOTH4 };

	GLuint vert_lighting = pglCreateProgram(point_light_per_vertex_vs, point_light_per_vertex_fs, 5, vs_outs, GL_FALSE);
	glUseProgram(vert_lighting);
	pglSetUniform(&the_uniforms);

	GLuint frag_lighting = pglCreateProgram(point_light_per_frag_vs, point_light_per_frag_fs, 9, vs_outs, GL_FALSE);
	glUseProgram(frag_lighting);

	pglSetUniform(&the_uniforms);

	// start with lighting and textures on
	the_uniforms.use_lighting = GL_TRUE;
	the_uniforms.use_textures = GL_TRUE;
	the_uniforms.pnt_light = vec3(0, 0, -5);
	
	the_uniforms.proj_mat = glm::perspective(glm::radians(45.0f), WIDTH/(float)HEIGHT, 0.1f, 100.0f);

	GLuint cur_program = (use_frag_lighting) ? frag_lighting : vert_lighting;
	glUseProgram(cur_program);

	float elapsed;

	glClearColor(0,0,0,1);

	float moonAngle = 180;
	float cubeAngle = 0;

	matrix_stack mvstack;
	mvstack.load_identity();
	mvstack.translate(vec3(0,0,-5));
	mvstack.rotate(glm::radians(30.0f), vec3(1, 0, 0));

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	int last_time = SDL_GetTicks();
	int old_time = 0, new_time=0, counter = 0;
	int ms;
	while (handle_events()) {
		new_time = SDL_GetTicks();
		counter++;
		ms = new_time - old_time;
		if (ms >= FPS_DELAY) {
			printf("%d  %f FPS\n", ms, counter*1000.0f/ms);
			old_time = new_time;
			counter = 0;
		}
		elapsed = new_time - last_time;
		last_time = new_time;

		moonAngle += 0.05 * elapsed;
		cubeAngle += 0.05 * elapsed;

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glUseProgram(cur_program);

		// NOTE the bind isn't actually necessary since PGL doesn't have the concept
		// of texture units/active textures so we access textures in shaders directly
		// via the names return from glGenTextures/glCreateTextures
		//glBindTexture(GL_TEXTURE_2D, moon_tex);
		the_uniforms.tex = moon_tex;

		mvstack.push();
		mvstack.rotate(glm::radians(moonAngle), vec3(0, 1, 0));
		mvstack.translate(2, 0, 0);

		the_uniforms.mv_mat = mvstack.get_matrix();

		//the_uniforms.norm_mat = mat3(the_uniforms.mv_mat);
		the_uniforms.norm_mat = glm::transpose(glm::inverse(mat3(the_uniforms.mv_mat)));

		// The other uniforms are updated directly in GUI code

		glBindVertexArray(moon_vao);
		glDrawElements(GL_TRIANGLES, moon_tris*3, GL_UNSIGNED_INT, 0);

		mvstack.pop();

		mvstack.push();
		//glBindTexture(GL_TEXTURE_2D, box_tex);
		the_uniforms.tex = box_tex;

		mvstack.rotate(glm::radians(cubeAngle), vec3(0, 1, 0));
		mvstack.translate(1.25, 0, 0);

		the_uniforms.mv_mat = mvstack.get_matrix();

		//the_uniforms.norm_mat = mat3(the_uniforms.mv_mat);
		the_uniforms.norm_mat = glm::transpose(glm::inverse(mat3(the_uniforms.mv_mat)));

		glBindVertexArray(cube_vao);
		glDrawElements(GL_TRIANGLES, cube_tris*3, GL_UNSIGNED_INT, 0);

		mvstack.pop();

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);

		if (nk_begin(ctx, "Controls", nk_rect(WIDTH-GUI_W, 0, GUI_W, HEIGHT),
		    NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
		{
			static struct nk_colorf pt_lt_color = { 0.8, 0.8, 0.8, 1 };
			static struct nk_colorf ambient_color = { 0.2, 0.2, 0.2, 1 };
			//nk_layout_row_static(ctx, 30, GUI_W, 1);
			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_checkbox_label(ctx, "Use Lighting", &the_uniforms.use_lighting)) {
				printf("Lighting %s\n", (the_uniforms.use_lighting ? "On" : "Off"));
			}
			if (nk_checkbox_label(ctx, "Per-fragment lighting", &use_frag_lighting)) {
				printf("Frag Lighting %s\n", (use_frag_lighting ? "On" : "Off"));
				if (use_frag_lighting) {
					cur_program = frag_lighting;
				} else {
					cur_program = vert_lighting;
				}
			}
			if (nk_checkbox_label(ctx, "Use Textures", &the_uniforms.use_textures)) {
				printf("Textures %s\n", (the_uniforms.use_textures ? "On" : "Off"));
			}

			nk_label(ctx, "Point Light", NK_TEXT_CENTERED);
			nk_label(ctx, "Location", NK_TEXT_CENTERED);

			//nk_layout_row_dynamic(ctx, 0, 2);

			nk_layout_row_template_begin(ctx, 0);
			nk_layout_row_template_push_static(ctx, 20);
			nk_layout_row_template_push_dynamic(ctx);
			nk_layout_row_template_end(ctx);

			nk_label(ctx, "X", NK_TEXT_CENTERED);
			nk_property_float(ctx, "#", -40.0, &the_uniforms.pnt_light.x, 20.0, 1.0, 0.2);
			nk_label(ctx, "Y", NK_TEXT_CENTERED);
			nk_property_float(ctx, "#", -40.0, &the_uniforms.pnt_light.y, 20.0, 1.0, 0.2);
			nk_label(ctx, "Z", NK_TEXT_CENTERED);
			nk_property_float(ctx, "#", -40.0, &the_uniforms.pnt_light.z, 20.0, 1.0, 0.2);

			//nk_layout_row_static(ctx, 30, GUI_W, 1);
			nk_layout_row_dynamic(ctx, 0, 1);

			nk_label(ctx, "Color", NK_TEXT_CENTERED);

			nk_layout_row_dynamic(ctx, 100, 1);
			pt_lt_color = nk_color_picker(ctx, pt_lt_color, NK_RGB);
			nk_layout_row_dynamic(ctx, 0, 1);
			pt_lt_color.r = nk_propertyf(ctx, "#R:", 0, pt_lt_color.r, 1.0f, 0.01f,0.005f);
			pt_lt_color.g = nk_propertyf(ctx, "#G:", 0, pt_lt_color.g, 1.0f, 0.01f,0.005f);
			pt_lt_color.b = nk_propertyf(ctx, "#B:", 0, pt_lt_color.b, 1.0f, 0.01f,0.005f);

			the_uniforms.pnt_light_color = vec3(pt_lt_color.r, pt_lt_color.g, pt_lt_color.b);

			nk_label(ctx, "Ambient Light", NK_TEXT_CENTERED);
			nk_label(ctx, "Color", NK_TEXT_CENTERED);

			nk_layout_row_dynamic(ctx, 100, 1);
			ambient_color = nk_color_picker(ctx, ambient_color, NK_RGB);
			nk_layout_row_dynamic(ctx, 0, 1);
			ambient_color.r = nk_propertyf(ctx, "#R:", 0, ambient_color.r, 1.0f, 0.01f,0.005f);
			ambient_color.g = nk_propertyf(ctx, "#G:", 0, ambient_color.g, 1.0f, 0.01f,0.005f);
			ambient_color.b = nk_propertyf(ctx, "#B:", 0, ambient_color.b, 1.0f, 0.01f,0.005f);

			the_uniforms.ambient = vec3(ambient_color.r, ambient_color.g, ambient_color.b);
		}
		nk_end(ctx);

		//nk_sdl_render(NK_ANTI_ALIASING_ON);
		nk_sdl_render(NK_ANTI_ALIASING_OFF);

		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void point_light_per_vertex_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec3 normal = *(vec3*)&vertex_attribs[1];

	*(vec2*)vs_output = *(vec2*)&vertex_attribs[2]; // smooth out texcoord = in_texcoord

	My_Uniforms* u = (My_Uniforms*)uniforms;

	vec4 mv_pos = u->mv_mat * ((vec4*)vertex_attribs)[0];
	*(vec4*)&builtins->gl_Position = u->proj_mat * mv_pos;

	if (!u->use_lighting) {
		//smooth out lightweighting = vec3(1.0);
		*(vec3*)&vs_output[2] = vec3(1.0f);
	} else {
		// TODO try swizzle
		vec3 light_dir = glm::normalize(u->pnt_light - vec3(mv_pos));

		vec3 transformed_normal = u->norm_mat * normal;
		float dir_light_weighting = glm::max(glm::dot(transformed_normal, light_dir), 0.0f);

		*(vec3*)&vs_output[2] = u->ambient + u->pnt_light_color * dir_light_weighting;
	}
}

void point_light_per_vertex_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];
	vec3 light_weighting = *(vec3*)&fs_input[2];

	My_Uniforms* u = (My_Uniforms*)uniforms;

	pgl_vec4 frag_color;
	if (u->use_textures) {
		frag_color = texture2D(u->tex, tex_coords.x, tex_coords.y);
		frag_color.x *= light_weighting.x;
		frag_color.y *= light_weighting.y;
		frag_color.z *= light_weighting.z;
	} else {
		frag_color = make_vec4(light_weighting.x, light_weighting.y, light_weighting.z, 1.0f);
	}

	builtins->gl_FragColor = frag_color;

	// TODO try glm swizzle
	//*(vec4*)&builtins->gl_FragColor = vec4(vec3(tex_color) * light_weighting, tex_color.a);
}

void point_light_per_frag_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec3 normal = *(vec3*)&vertex_attribs[1];

	My_Uniforms* u = (My_Uniforms*)uniforms;

	// TODO rearrange? pos, norm, tex?

	// smooth out texcoord = in_texcoord
	*(vec2*)vs_output = *(vec2*)&vertex_attribs[2];
	// smooth out transformed_normal
	*(vec3*)&vs_output[2] = u->norm_mat * normal;
	// smooth out position
	vec4 pos = u->mv_mat * ((vec4*)vertex_attribs)[0];
	*(vec4*)&vs_output[5] = pos;

	*(vec4*)&builtins->gl_Position = u->proj_mat * pos;
}

void point_light_per_frag_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];
	vec3 normal = glm::normalize(*(vec3*)&fs_input[2]);
	vec4 pos = *(vec4*)&fs_input[5];

	My_Uniforms* u = (My_Uniforms*)uniforms;

	vec3 light_weighting;
	if (!u->use_lighting) {
		light_weighting = vec3(1.0);
	} else {
		// TODO swizzle pos.xyz
		vec3 light_dir = glm::normalize(u->pnt_light - vec3(pos));
		float dir_light_weighting = glm::max(glm::dot(normal, light_dir), 0.0f);
		light_weighting = u->ambient + u->pnt_light_color * dir_light_weighting;
	}

	pgl_vec4 frag_color;
	if (u->use_textures) {
		frag_color = texture2D(u->tex, tex_coords.x, tex_coords.y);
		frag_color.x *= light_weighting.x;
		frag_color.y *= light_weighting.y;
		frag_color.z *= light_weighting.z;
	} else {
		frag_color = make_vec4(light_weighting.x, light_weighting.y, light_weighting.z, 1.0f);
	}

	builtins->gl_FragColor = frag_color;
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("Lesson 13", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("SDL_CreateWindow error: %s\n", SDL_GetError());
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(0);
	}

    ctx = nk_sdl_init(window, ren);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
    nk_sdl_font_stash_end();
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    /*nk_style_set_font(ctx, &roboto->handle);*/}

	my_chdir(SDL_GetBasePath());
}

void cleanup()
{
	free_glContext(&the_Context);

	nk_sdl_shutdown();

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

int handle_events()
{
	SDL_Event e;
	int sc;
	nk_input_begin(ctx);
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return 0;
		} else if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;

			if (sc == SDL_SCANCODE_ESCAPE) {
				return 0;
			} else if (sc == SDL_SCANCODE_LEFT) {
			} else if (sc == SDL_SCANCODE_RIGHT) {
			} else if (sc == SDL_SCANCODE_UP) {
			} else if (sc == SDL_SCANCODE_DOWN) {
			} else if (sc == SDL_SCANCODE_P) {
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		} else if (e.type == SDL_MOUSEMOTION) {
		}
		nk_sdl_handle_event(&e);
	}
	nk_input_end(ctx);

	//SDL_PumpEvents() is called above in SDL_PollEvent()
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	if (state[SDL_SCANCODE_PAGEUP]) {
		z += 0.05;
	} else if (state[SDL_SCANCODE_PAGEDOWN]) {
		z -= 0.05;
	}

	return 1;
}


void setup_buffers()
{
	float vertices[] = {
		// Front face
		-1.0, -1.0,  1.0,
		 1.0, -1.0,  1.0,
		 1.0,  1.0,  1.0,
		-1.0,  1.0,  1.0,
		// Back face
		-1.0, -1.0, -1.0,
		-1.0,  1.0, -1.0,
		 1.0,  1.0, -1.0,
		 1.0, -1.0, -1.0,
		// Top face
		-1.0,  1.0, -1.0,
		-1.0,  1.0,  1.0,
		 1.0,  1.0,  1.0,
		 1.0,  1.0, -1.0,
		// Bottom face
		-1.0, -1.0, -1.0,
		 1.0, -1.0, -1.0,
		 1.0, -1.0,  1.0,
		-1.0, -1.0,  1.0,
		// Right face
		 1.0, -1.0, -1.0,
		 1.0,  1.0, -1.0,
		 1.0,  1.0,  1.0,
		 1.0, -1.0,  1.0,
		// Left face
		-1.0, -1.0, -1.0,
		-1.0, -1.0,  1.0,
		-1.0,  1.0,  1.0,
		-1.0,  1.0, -1.0
	};

	float cube_normals[] = {
		// Front face
		 0.0,  0.0,  1.0,
		 0.0,  0.0,  1.0,
		 0.0,  0.0,  1.0,
		 0.0,  0.0,  1.0,
		// Back face
		 0.0,  0.0, -1.0,
		 0.0,  0.0, -1.0,
		 0.0,  0.0, -1.0,
		 0.0,  0.0, -1.0,
		// Top face
		 0.0,  1.0,  0.0,
		 0.0,  1.0,  0.0,
		 0.0,  1.0,  0.0,
		 0.0,  1.0,  0.0,
		// Bottom face
		 0.0, -1.0,  0.0,
		 0.0, -1.0,  0.0,
		 0.0, -1.0,  0.0,
		 0.0, -1.0,  0.0,
		// Right face
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		// Left face
		-1.0,  0.0,  0.0,
		-1.0,  0.0,  0.0,
		-1.0,  0.0,  0.0,
		-1.0,  0.0,  0.0
	};

	float cube_texcoords[] = {
		// Front face
		0.0, 0.0,
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		// Back face
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		// Top face
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
		1.0, 1.0,
		// Bottom face
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
		// Right face
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		// Left face
		0.0, 0.0,
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0
	};

	GLuint triangles[] = {
		0, 1, 2,      0, 2, 3,    // Front face
		4, 5, 6,      4, 6, 7,    // Back face
		8, 9, 10,     8, 10, 11,  // Top face
		12, 13, 14,   12, 14, 15, // Bottom face
		16, 17, 18,   16, 18, 19, // Right face
		20, 21, 22,   20, 22, 23  // Left face
	};

	cube_tris = 12;

	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);

	GLuint vert_buf;
	glGenBuffers(1, &vert_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint normal_buf;
	glGenBuffers(1, &normal_buf);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_texcoords), cube_texcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint elem_buf;
	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

	glBindVertexArray(0);

	glGenVertexArrays(1, &moon_vao);
	glBindVertexArray(moon_vao);

	vector<vec3> verts;
	vector<vec3> normals;
	vector<vec2> texcoords;
	vector<ivec3> tris;

	float lat_bands = 30;
	float long_bands = 30;
	float radius = 1;

	for (float lat_n = 0; lat_n <= lat_bands; lat_n++) {
		float theta = lat_n * glm::pi<float>() / lat_bands;
		float sin_theta = sin(theta);
		float cos_theta = cos(theta);
		
		for (float long_n=0; long_n <= long_bands; long_n++) {
			float phi = long_n * 2 * glm::pi<float>() / long_bands;
			float sin_phi = sin(phi);
			float cos_phi = cos(phi);

			float x = cos_phi * sin_theta;
			float y = cos_theta;
			float z = sin_phi * sin_theta;
			float u = 1 - (long_n / long_bands);
			float v = 1 - (lat_n / lat_bands);

			normals.push_back(vec3(x, y, z));
			texcoords.push_back(vec2(u, v));
			verts.push_back(vec3(radius*x, radius*y, radius*z));
		}
	}

	for (int i=0; i<lat_bands; i++) {
		for (int j=0; j<long_bands; j++) {
			float first = (i * (long_bands+1)) + j;
			float second = first + long_bands + 1;

			// original CW winding, means CULL_FACE messes it up
			//tris.push_back(ivec3(first, second, first+1));
			//tris.push_back(ivec3(second, second+1, first+1));
			
			tris.push_back(ivec3(first, first+1, second));
			tris.push_back(ivec3(second, first+1, second+1));
		}
	}
	moon_tris = tris.size();

	glGenBuffers(1, &vert_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
	glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(vec3), &verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &normal_buf);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buf);
	glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(vec3), &normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, texcoords.size()*sizeof(vec2), &texcoords[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tris.size()*sizeof(ivec3), &tris[0], GL_STATIC_DRAW);

	glBindVertexArray(0);
}







