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

#define WIDTH 640
#define HEIGHT 480

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

pix_t* bbufpix;
pix_t* framebuftex_pix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mv_mat;
	mat4 proj_mat;
	mat3 norm_mat;

	vec3 mat_ambient;
	vec3 mat_diff;
	vec3 mat_spec;
	vec3 mat_emissive;
	float mat_shininess;

	vec3 ambient;

	vec3 pnt_light;
	vec3 pnt_light_diff;
	vec3 pnt_light_spec;

	GLuint tex;

	int show_specular;
	int use_textures;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();
void setup_buffers();
void draw_scene_on_screen();

void lesson16_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void lesson16_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

float z;
int polygon_mode;

GLuint cube_vao, moon_vao, macbook_vao;
GLuint macbook_screen_vao;
int cube_tris, moon_tris, macbook_tris;

int fb_w = 512;
int fb_h = 512;

float moonAngle = 180;
float cubeAngle = 0;

My_Uniforms the_uniforms;

matrix_stack mvstack;
GLuint program;
GLuint box_tex, moon_tex;

int main(int argc, char** argv)
{
	setup_context();

	setup_buffers();

	polygon_mode = 2;

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

	GLuint framebuffer_tex;
	glGenTextures(1, &framebuffer_tex);
	glBindTexture(GL_TEXTURE_2D, framebuffer_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Have to be the same size as default framebuffer for hack to work
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_w, fb_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	pglGetTextureData(framebuffer_tex, (GLvoid**)&framebuftex_pix);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Using this array for both
	GLenum vs_outs[] = { PGL_SMOOTH2, PGL_SMOOTH3, PGL_SMOOTH4 };

	program = pglCreateProgram(lesson16_vs, lesson16_fs, 9, vs_outs, GL_FALSE);
	glUseProgram(program);

	pglSetUniform(&the_uniforms);

	the_uniforms.use_textures = GL_TRUE;
	the_uniforms.show_specular = GL_TRUE;

	the_uniforms.pnt_light = vec3(-1, 2, -1);
	the_uniforms.ambient = vec3(0.2);
	the_uniforms.pnt_light_diff = vec3(0.8);
	the_uniforms.pnt_light_spec = vec3(0.8);


	// laptop body
	the_uniforms.mat_ambient = vec3(1);
	the_uniforms.mat_diff = vec3(1);
	the_uniforms.mat_spec = vec3(1.5); // this does not make sense to me
	the_uniforms.mat_shininess = 5;
	the_uniforms.mat_emissive = vec3(0);

	the_uniforms.proj_mat = glm::perspective(glm::radians(45.0f), WIDTH/(float)HEIGHT, 0.1f, 100.0f);

	float elapsed;

	glClearColor(0,0,0,1);

	float laptopAngle = 0;

	//glEnable(GL_CULL_FACE);
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
		laptopAngle -= 0.005 * elapsed;

		draw_scene_on_screen();

		// we use the same viewport for everything because we have to
		//glViewport(0, 0, WIDTH, HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		mvstack.load_identity();
		mvstack.push();

		mvstack.translate(0, -0.4, -1.7);
		mvstack.rotate(glm::radians(laptopAngle), vec3(0, 1, 0));
		mvstack.rotate(glm::radians(-90.0f), vec3(1,0,0));

		the_uniforms.mv_mat = mvstack.get_matrix();
		//the_uniforms.norm_mat = mat3(the_uniforms.mv_mat);
		the_uniforms.norm_mat = glm::transpose(glm::inverse(mat3(the_uniforms.mv_mat)));

		the_uniforms.show_specular = GL_TRUE;

		the_uniforms.pnt_light = vec3(-1, 2, -1);
		the_uniforms.ambient = vec3(0.2);
		the_uniforms.pnt_light_diff = vec3(0.8);
		the_uniforms.pnt_light_spec = vec3(0.8);

		// laptop body is quite shiny and has no texture.  It reflects
		// lots of specular light
		the_uniforms.mat_ambient = vec3(1);
		the_uniforms.mat_diff = vec3(1);
		the_uniforms.mat_spec = vec3(1.5); // this does not make sense to me
		the_uniforms.mat_shininess = 5;
		the_uniforms.mat_emissive = vec3(0);
		the_uniforms.use_textures = GL_FALSE;

		glBindVertexArray(macbook_vao);
		glDrawElements(GL_TRIANGLES, macbook_tris*3, GL_UNSIGNED_INT, 0);

		// macbook screen
		the_uniforms.mat_ambient = vec3(0);
		the_uniforms.mat_diff = vec3(0);
		the_uniforms.mat_spec = vec3(0.5);
		the_uniforms.mat_shininess = 20;
		the_uniforms.mat_emissive = vec3(1.5);
		the_uniforms.use_textures = GL_TRUE;

		the_uniforms.tex = framebuffer_tex;
		//glBindTexture(GL_TEXTURE_2D, framebuffer_tex);

		glBindVertexArray(macbook_screen_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		mvstack.pop();


		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);

		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void lesson16_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
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

void lesson16_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];
	vec3 normal = glm::normalize(*(vec3*)&fs_input[2]);
	vec4 pos = *(vec4*)&fs_input[5];

	My_Uniforms* u = (My_Uniforms*)uniforms;

	vec3 ambient_weighting = u->ambient;
	vec3 light_dir = glm::normalize(u->pnt_light - vec3(pos));

	vec3 specular_weighting = vec3(0.0f);
	if (u->show_specular) {
		vec3 eye_dir = glm::normalize(-vec3(pos));
		vec3 reflection_dir = glm::reflect(-light_dir, normal);

		float specular_brightness = pow(glm::max(glm::dot(reflection_dir, eye_dir), 0.0f), u->mat_shininess);
		specular_weighting = u->pnt_light_spec * specular_brightness;
	}

	float diffuse_brightness = glm::max(glm::dot(normal, light_dir), 0.0f);
	vec3 diffuse_weighting = u->pnt_light_diff * diffuse_brightness;

	vec3 mat_amb = u->mat_ambient;
	vec3 mat_diff = u->mat_diff;
	vec3 mat_spec = u->mat_spec;
	vec3 mat_emissive = u->mat_emissive;
	float alpha = 1.0f;
	pgl_vec4 tex_color;
	if (u->use_textures) {
		tex_color = texture2D(u->tex, tex_coords.x, tex_coords.y);
		vec3 tc = vec3(tex_color.x, tex_color.y, tex_color.z);
		mat_amb *= tc;
		mat_diff *= tc;
		mat_emissive *= tc;
		alpha = tex_color.w;
	}

	vec4 frag_color = vec4(
		mat_amb * ambient_weighting
		+ mat_diff * diffuse_weighting
		+ mat_spec * specular_weighting
		+ mat_emissive,
		alpha
	);

	*(vec4*)&builtins->gl_FragColor = frag_color;
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("Lesson 16", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
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

	my_chdir(SDL_GetBasePath());
}

void cleanup()
{
	free_glContext(&the_Context);

	// Need to do this because of the render to texture hack
	// the framebuffer texture will be freed as a texture
	// but because we were swapping back and forth with pglSetBackBuffer()
	// pgl sets the color buffer to "user managed/alloced" and does not
	// free it for us
	free(bbufpix);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

int handle_events()
{
	SDL_Event e;
	int sc;
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
	}

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
	float cube_vertices[] = {
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

	GLuint cube_triangles[] = {
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
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
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangles), cube_triangles, GL_STATIC_DRAW);

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

	//TODO necessary?
	glBindVertexArray(0);

#include "../media/models/macbook.h"

	glGenVertexArrays(1, &macbook_vao);
	glBindVertexArray(macbook_vao);

	glGenBuffers(1, &vert_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &normal_buf);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexNormals), vertexNormals, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexTextureCoords), vertexTextureCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	macbook_tris = sizeof(indices)/sizeof(indices[0])/3;

	glBindVertexArray(0);

	glGenVertexArrays(1, &macbook_screen_vao);
	glBindVertexArray(macbook_screen_vao);

	float screen_vertices[] =
	{
		 0.580687, 0.659, 0.813106,
		-0.580687, 0.659, 0.813107,
		 0.580687, 0.472, 0.113121,
		-0.580687, 0.472, 0.113121
	};
	float screen_vertexNormals[] =
	{
		 0.000000, -0.965926, 0.258819,
		 0.000000, -0.965926, 0.258819,
		 0.000000, -0.965926, 0.258819,
		 0.000000, -0.965926, 0.258819
	};
	float screen_textureCoords[] =
	{
		1.0, 1.0,
		0.0, 1.0,
		1.0, 0.0,
		0.0, 0.0
	};
	glGenBuffers(1, &vert_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &normal_buf);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertexNormals), screen_vertexNormals, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screen_textureCoords), screen_textureCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

}

void draw_scene_on_screen()
{
	// hack for render to texture
	pglSetBackBuffer(framebuftex_pix, WIDTH, HEIGHT);

	// already the same
	//glViewport(0, 0, fb_w, fb_h);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	the_uniforms.ambient = vec3(0.2);

	the_uniforms.pnt_light = vec3(0, 0, -5);
	the_uniforms.pnt_light_diff = vec3(0.8);
	the_uniforms.show_specular = GL_FALSE;
	// don't need to update this since !show_specular
	//the_uniforms.pnt_light_spec = vec3(0.8);

	// TODO do I even need to update all of these?
	the_uniforms.mat_ambient = vec3(1);
	the_uniforms.mat_diff = vec3(1);
	the_uniforms.mat_spec = vec3(0);
	the_uniforms.mat_shininess = 0;
	the_uniforms.mat_emissive = vec3(0);
	the_uniforms.use_textures = GL_TRUE;

	mvstack.load_identity();
	mvstack.translate(vec3(0,0,-5));
	mvstack.rotate(glm::radians(30.0f), vec3(1, 0, 0));

	mvstack.push();
	mvstack.rotate(glm::radians(moonAngle), vec3(0, 1, 0));
	mvstack.translate(2, 0, 0);

	//glBindTexture(GL_TEXTURE_2D, moon_tex);
	the_uniforms.tex = moon_tex;

	the_uniforms.mv_mat = mvstack.get_matrix();
	//the_uniforms.norm_mat = mat3(the_uniforms.mv_mat);
	the_uniforms.norm_mat = glm::transpose(glm::inverse(mat3(the_uniforms.mv_mat)));

	glBindVertexArray(moon_vao);
	glDrawElements(GL_TRIANGLES, moon_tris*3, GL_UNSIGNED_INT, 0);

	mvstack.pop();

	mvstack.push();

	//glBindTexture(GL_TEXTURE_2D, box_tex);
	the_uniforms.tex = box_tex;

	mvstack.rotate(glm::radians(cubeAngle), vec3(0, 1, 0));
	mvstack.translate(1.25, 0, 0);

	// Only MVmatrix and NMatrix change
	the_uniforms.mv_mat = mvstack.get_matrix();
	//the_uniforms.norm_mat = mat3(the_uniforms.mv_mat);
	the_uniforms.norm_mat = glm::transpose(glm::inverse(mat3(the_uniforms.mv_mat)));

	glBindVertexArray(cube_vao);
	glDrawElements(GL_TRIANGLES, cube_tris*3, GL_UNSIGNED_INT, 0);

	mvstack.pop();

	// other half of hack for render to texture
	pglSetBackBuffer(bbufpix, WIDTH, HEIGHT);
}









