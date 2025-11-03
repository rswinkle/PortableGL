#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"

#include <SDL.h>

#include <glm_matstack.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>

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

	vec3 light_dir;
	vec3 light_color;

	GLuint tex;

	int use_lighting;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();


void directional_light_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void directional_light_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

float z = -5;
float x_rot, y_rot;
float x_speed, y_speed;
int filter;

vec3 light_dir(-0.25, -0.25, -1.0);

GLuint texture;

int main(int argc, char** argv)
{
	setup_context();

	My_Uniforms the_uniforms;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	if (!load_texture2D("../media/textures/crate.gif", GL_NEAREST, GL_NEAREST, GL_MIRRORED_REPEAT, GL_TRUE, NULL, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}

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

	float normals[] = {
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

	float texcoords[] = {
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


	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vert_buf;
	glGenBuffers(1, &vert_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint normal_buf;
	glGenBuffers(1, &normal_buf);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint elem_buf;
	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);


	GLenum vs_outs[5] = { PGL_SMOOTH2, PGL_SMOOTH3 };
	GLuint program = pglCreateProgram(directional_light_vs, directional_light_fs, 5, vs_outs, GL_FALSE);
	glUseProgram(program);

	pglSetUniform(&the_uniforms);

	// start with lighting on
	the_uniforms.use_lighting = GL_TRUE;
	
	// these never change
	the_uniforms.tex = texture;
	the_uniforms.proj_mat = glm::perspective(glm::radians(45.0f), WIDTH/(float)HEIGHT, 0.1f, 100.0f);

	// TODO why even use a stack when we only have one object?
	matrix_stack mat_stack;
	mat_stack.load_identity();

	float elapsed;

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

		x_rot += x_speed*elapsed / 1000.0f;
		y_rot += y_speed*elapsed / 1000.0f;

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		mat_stack.push();
		mat_stack.translate(0, 0, z);

		mat_stack.rotate(glm::radians(x_rot), 1, 0, 0);
		mat_stack.rotate(glm::radians(y_rot), 0, 1, 0);

		the_uniforms.mv_mat = mat_stack.get_matrix();

		//the_uniforms.norm_mat = mat3(the_uniforms.mv_mat);
		the_uniforms.norm_mat = glm::transpose(glm::inverse(mat3(the_uniforms.mv_mat)));

		the_uniforms.light_dir = glm::normalize(-light_dir);
		// These are updated directly in GUI code
		//the_uniforms.ambient;
		//the_uniforms.light_color = light_color;
		//the_uniforms.use_lighting = use_lighting;

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, sizeof(triangles)/sizeof(triangles[0]), GL_UNSIGNED_INT, 0);

		mat_stack.pop();

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);

		if (nk_begin(ctx, "Controls", nk_rect(WIDTH-GUI_W, 0, GUI_W, HEIGHT),
		    NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
		{
			static struct nk_colorf dir_color = { 0.8, 0.8, 0.8, 1 };
			static struct nk_colorf ambient_color = { 0.2, 0.2, 0.2, 1 };
			//nk_layout_row_static(ctx, 30, GUI_W, 1);
			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_checkbox_label(ctx, "Use Lighting", &the_uniforms.use_lighting)) {
				printf("Lighting %s\n", (the_uniforms.use_lighting ? "On" : "Off"));
			}

			nk_label(ctx, "Directional Light", NK_TEXT_CENTERED);
			nk_label(ctx, "Direction", NK_TEXT_CENTERED);

			//nk_layout_row_dynamic(ctx, 0, 2);

			nk_layout_row_template_begin(ctx, 0);
			nk_layout_row_template_push_static(ctx, 20);
			nk_layout_row_template_push_dynamic(ctx);
			nk_layout_row_template_end(ctx);

			nk_label(ctx, "X", NK_TEXT_CENTERED);
			nk_property_float(ctx, "#", -1.0, &light_dir.x, 1.0, 0.25, 0.08333);
			nk_label(ctx, "Y", NK_TEXT_CENTERED);
			nk_property_float(ctx, "#", -1.0, &light_dir.y, 1.0, 0.25, 0.08333);
			nk_label(ctx, "Z", NK_TEXT_CENTERED);
			nk_property_float(ctx, "#", -1.0, &light_dir.z, 1.0, 0.25, 0.08333);

			//nk_layout_row_static(ctx, 30, GUI_W, 1);
			nk_layout_row_dynamic(ctx, 0, 1);

			nk_label(ctx, "Color", NK_TEXT_CENTERED);

			nk_layout_row_dynamic(ctx, 100, 1);
			dir_color = nk_color_picker(ctx, dir_color, NK_RGB);
			nk_layout_row_dynamic(ctx, 0, 1);
			dir_color.r = nk_propertyf(ctx, "#R:", 0, dir_color.r, 1.0f, 0.01f,0.005f);
			dir_color.g = nk_propertyf(ctx, "#G:", 0, dir_color.g, 1.0f, 0.01f,0.005f);
			dir_color.b = nk_propertyf(ctx, "#B:", 0, dir_color.b, 1.0f, 0.01f,0.005f);

			the_uniforms.light_color = vec3(dir_color.r, dir_color.g, dir_color.b);

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


void directional_light_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec3 normal = *(vec3*)&vertex_attribs[1];

	*(vec2*)vs_output = *(vec2*)&vertex_attribs[2]; // smooth out texcoord = in_texcoord

	My_Uniforms* u = (My_Uniforms*)uniforms;

	*(vec4*)&builtins->gl_Position = u->proj_mat * u->mv_mat * ((vec4*)vertex_attribs)[0];

	if (!u->use_lighting) {
		//smooth out lightweighting = vec3(1.0);
		*(vec3*)&vs_output[2] = vec3(1.0f);
	} else {
		vec3 transformed_normal = u->norm_mat * normal;
		float dir_light_weighting = glm::max(glm::dot(transformed_normal, u->light_dir), 0.0f);

		*(vec3*)&vs_output[2] = u->ambient + u->light_color * dir_light_weighting;

	}
}

void directional_light_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];
	vec3 light_weighting = *(vec3*)&fs_input[2];

	My_Uniforms* u = (My_Uniforms*)uniforms;

	pgl_vec4 tex_color = texture2D(u->tex, tex_coords.x, tex_coords.y);
	tex_color.x *= light_weighting.x;
	tex_color.y *= light_weighting.y;
	tex_color.z *= light_weighting.z;

	builtins->gl_FragColor = tex_color;

	// TODO try glm swizzle
	//*(vec4*)&builtins->gl_FragColor = vec4(vec3(tex_color) * light_weighting, tex_color.a);
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("Lesson 7", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
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
			} else if (sc == SDL_SCANCODE_F) {
				filter = (filter + 1) % 2;
				// NOTE, only magfilter is meaningful, and it applies universally
				// regardless of the texture size relative to display size
				//
				// Using DSA functions since I can
				if (filter == 0) {
					puts("GL_NEAREST\n");
					glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				} else {
					puts("GL_LINEAR\n");
					glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}

			} else if (sc == SDL_SCANCODE_LEFT) {
				y_speed -= 1;
			} else if (sc == SDL_SCANCODE_RIGHT) {
				y_speed += 1;
			} else if (sc == SDL_SCANCODE_UP) {
				x_speed -= 1;
			} else if (sc == SDL_SCANCODE_DOWN) {
				x_speed += 1;
			}
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







