#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"

#include <SDL.h>

#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
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
	vec3 pnt_light_diff;
	vec3 pnt_light_spec;
	float shininess;

	GLuint color_map;
	GLuint spec_map;

	int use_lighting;
	int use_color_map;
	int use_specular_map;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();
int setup_buffers();


void lesson15_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void lesson15_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

GLuint vao;

int main(int argc, char** argv)
{
	setup_context();

	My_Uniforms the_uniforms;

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	int num_sphere_indices = setup_buffers();
	glBindVertexArray(0);

	GLuint earth_tex, specular_tex;
	glGenTextures(1, &earth_tex);
	glBindTexture(GL_TEXTURE_2D, earth_tex);
	if (!load_texture2D("../../media/textures/earth.jpg", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, GL_REPEAT, GL_TRUE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &specular_tex);
	glBindTexture(GL_TEXTURE_2D, specular_tex);
	if (!load_texture2D("../../media/textures/earth-specular.gif", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, GL_REPEAT, GL_TRUE, GL_FALSE, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}


	GLenum vs_outs[] = { PGL_SMOOTH2, PGL_SMOOTH3, PGL_SMOOTH4 };
	GLuint program = pglCreateProgram(lesson15_vs, lesson15_fs, 9, vs_outs, GL_FALSE);
	glUseProgram(program);

	pglSetUniform(&the_uniforms);

	// start with everything on
	the_uniforms.use_lighting = GL_TRUE;
	the_uniforms.use_color_map = GL_TRUE;
	the_uniforms.use_specular_map = GL_TRUE;

	the_uniforms.pnt_light = vec3(-10, 4, -20);
	
	// these never change
	the_uniforms.color_map = earth_tex;
	the_uniforms.spec_map = specular_tex;
	the_uniforms.proj_mat = glm::perspective(glm::radians(45.0f), WIDTH/(float)HEIGHT, 0.1f, 100.0f);

	float elapsed;
	float earthAngle = 180;

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

		earthAngle += 0.05 * elapsed;

		the_uniforms.mv_mat = glm::translate(mat4(1), vec3(0,0,-40));
		the_uniforms.mv_mat = glm::rotate(the_uniforms.mv_mat, glm::radians(23.4f), vec3(1, 0, -1));
		the_uniforms.mv_mat = glm::rotate(the_uniforms.mv_mat, glm::radians(earthAngle), vec3(0, 1, 0));


		//the_uniforms.norm_mat = mat3(the_uniforms.mv_mat);
		the_uniforms.norm_mat = glm::transpose(glm::inverse(mat3(the_uniforms.mv_mat)));

		// The other uniforms are updated directly in GUI code
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, num_sphere_indices, GL_UNSIGNED_INT, 0);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);

		if (nk_begin(ctx, "Controls", nk_rect(WIDTH-GUI_W, 0, GUI_W, HEIGHT),
		    NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
		{
			// NOTE Original had 0.2 for ambient and 0.8 for diffuse
			// and specular, but I think it looks better with
			// .1, .5, and 1 respectively; it makes the specular
			// stand out better
			static struct nk_colorf spec_color = { 1, 1, 1, 1 };
			static struct nk_colorf diff_color = { 0.5, 0.5, 0.5, 1 };
			static struct nk_colorf ambient_color = { 0.1, 0.1, 0.1, 1 };
			//nk_layout_row_static(ctx, 30, GUI_W, 1);
			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_checkbox_label(ctx, "Use color map", &the_uniforms.use_color_map)) {
				printf("Color Map %s\n", (the_uniforms.use_color_map
? "On" : "Off"));
			}
			if (nk_checkbox_label(ctx, "Use specular map", &the_uniforms.use_specular_map)) {
				printf("Specular Map %s\n", (the_uniforms.use_specular_map ? "On" : "Off"));
			}
			if (nk_checkbox_label(ctx, "Use Lighting", &the_uniforms.use_lighting)) {
				printf("Lighting %s\n", (the_uniforms.use_lighting ? "On" : "Off"));
			}

			nk_label(ctx, "Point Light", NK_TEXT_CENTERED);
			nk_label(ctx, "Location", NK_TEXT_CENTERED);

			//nk_layout_row_dynamic(ctx, 0, 2);

			nk_layout_row_template_begin(ctx, 0);
			nk_layout_row_template_push_static(ctx, 20);
			nk_layout_row_template_push_dynamic(ctx);
			nk_layout_row_template_end(ctx);

			nk_label(ctx, "X", NK_TEXT_LEFT);
			nk_property_float(ctx, "#", -20.0, &the_uniforms.pnt_light.x, 20.0, 0.25, 0.08333);
			nk_label(ctx, "Y", NK_TEXT_LEFT);
			nk_property_float(ctx, "#", -20.0, &the_uniforms.pnt_light.y, 20.0, 0.25, 0.08333);
			nk_label(ctx, "Z", NK_TEXT_LEFT);
			nk_property_float(ctx, "#", -20.0, &the_uniforms.pnt_light.z, 20.0, 0.25, 0.08333);

			//nk_layout_row_static(ctx, 30, GUI_W, 1);
			nk_layout_row_dynamic(ctx, 0, 1);

			nk_label(ctx, "Specular Color", NK_TEXT_CENTERED);

			nk_layout_row_dynamic(ctx, 100, 1);
			spec_color = nk_color_picker(ctx, spec_color, NK_RGB);
			nk_layout_row_dynamic(ctx, 0, 1);
			spec_color.r = nk_propertyf(ctx, "#R:", 0, spec_color.r, 1.0f, 0.01f,0.005f);
			spec_color.g = nk_propertyf(ctx, "#G:", 0, spec_color.g, 1.0f, 0.01f,0.005f);
			spec_color.b = nk_propertyf(ctx, "#B:", 0, spec_color.b, 1.0f, 0.01f,0.005f);
			the_uniforms.pnt_light_spec = vec3(spec_color.r, spec_color.g, spec_color.b);

			nk_label(ctx, "Diffuse Color", NK_TEXT_CENTERED);

			nk_layout_row_dynamic(ctx, 100, 1);
			diff_color = nk_color_picker(ctx, diff_color, NK_RGB);
			nk_layout_row_dynamic(ctx, 0, 1);
			diff_color.r = nk_propertyf(ctx, "#R:", 0, diff_color.r, 1.0f, 0.01f,0.005f);
			diff_color.g = nk_propertyf(ctx, "#G:", 0, diff_color.g, 1.0f, 0.01f,0.005f);
			diff_color.b = nk_propertyf(ctx, "#B:", 0, diff_color.b, 1.0f, 0.01f,0.005f);

			the_uniforms.pnt_light_diff = vec3(diff_color.r, diff_color.g, diff_color.b);

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


void lesson15_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
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

void lesson15_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
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

		float specular_weighting = 0.0f;
		float shininess = 32.0f;
		if (u->use_specular_map) {
			shininess = texture2D(u->spec_map, tex_coords.x, tex_coords.y).x * 255.0f;
		}
		if (shininess < 255.0f) {
			vec3 eye_dir = glm::normalize(-vec3(pos));
			vec3 reflection_dir = glm::reflect(-light_dir, normal);

			specular_weighting = pow(glm::max(glm::dot(reflection_dir, eye_dir), 0.0f), shininess);
		}


		float diffuse_weighting = glm::max(glm::dot(normal, light_dir), 0.0f);
		light_weighting = u->ambient + u->pnt_light_diff * diffuse_weighting + u->pnt_light_spec * specular_weighting;
	}

	pgl_vec4 frag_color;
	if (u->use_color_map) {
		frag_color = texture2D(u->color_map, tex_coords.x, tex_coords.y);
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

	window = SDL_CreateWindow("Lesson 15", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
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
			}
		}
		nk_sdl_handle_event(&e);
	}
	nk_input_end(ctx);

	//SDL_PumpEvents() is called above in SDL_PollEvent()
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	if (state[SDL_SCANCODE_PAGEUP]) {
	} else if (state[SDL_SCANCODE_PAGEDOWN]) {
	}

	return 1;
}


int setup_buffers()
{
	vector<vec3> verts;
	vector<vec3> normals;
	vector<vec2> texcoords;
	vector<ivec3> tris;

	float lat_bands = 30;
	float long_bands = 30;
	float radius = 13;

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

	GLuint vert_buf;
	glGenBuffers(1, &vert_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
	glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(vec3), &verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint normal_buf;
	glGenBuffers(1, &normal_buf);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buf);
	glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(vec3), &normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, texcoords.size()*sizeof(vec2), &texcoords[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint elem_buf;
	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tris.size()*sizeof(ivec3), &tris[0], GL_STATIC_DRAW);

	return tris.size() * 3;
}











