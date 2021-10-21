#include <iostream>
#include <cstdlib>
#include <ctime>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define MANGLE_TYPES

#include "rsw_math.h"
#include "gltools.h"
#include "rsw_glframe.h"
#include "rsw_primitives.h"
#include "controls.h"

#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"
//#include "gl_unsafe.h"

#include "stb_image.h"




#define WIDTH 640
#define HEIGHT 480

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
uint32_t rmask = 0xff000000;
uint32_t gmask = 0x00ff0000;
uint32_t bmask = 0x0000ff00;
uint32_t amask = 0x000000ff;
#define PIX_FORMAT SDL_PIXELFORMAT_RGBA8888
#else
uint32_t rmask = 0x000000ff;
uint32_t gmask = 0x0000ff00;
uint32_t bmask = 0x00ff0000;
uint32_t amask = 0xff000000;
#define PIX_FORMAT SDL_PIXELFORMAT_ABGR8888
#endif


using namespace std;

using rsw::Color;
using rsw::vec3;
using rsw::vec4;
using rsw::mat4;


typedef struct My_Uniforms
{
	mat4 mvp;
	GLuint tex;
	vec3 light_dir;
} My_Uniforms;

void setup_context();
bool handle_events();
void cleanup();

#define NUM_PROGRAMS 2
void interpolate_vs(float* vs_output, void* v_attribs, Shader_Builtins* builtins, void* uniforms);
void interpolate_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void texture_replace_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void texture_replace_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);



GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };
GLenum noperspective[4] = { NOPERSPECTIVE, NOPERSPECTIVE, NOPERSPECTIVE, NOPERSPECTIVE };
//GLenum flat[GL_MAX_VERTEX_OUTPUT_COMPONENTS]; = {  };


typedef struct glShaderPair
{
	vert_func vertex_shader;
	frag_func fragment_shader;
	//void* uniform;
	int vs_output_size;
	GLenum interpolation[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	GLboolean use_frag_depth;
} glShaderPair;

glShaderPair shader_pairs[NUM_PROGRAMS] =
{
	{ interpolate_vs, interpolate_fs, 3, { SMOOTH, SMOOTH, SMOOTH }, GL_FALSE },
	{ texture_replace_vs, texture_replace_fs, 2, { SMOOTH, SMOOTH }, GL_FALSE }
};





glContext the_Context;

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;




vector<vec3> box_verts;
vector<ivec3> box_tris;
vector<vec2> box_tex;

GLenum interp_mode = SMOOTH;


GLuint cur_shader;

GLuint my_programs[NUM_PROGRAMS];

My_Uniforms the_uniforms;

GLFrame camera_frame(true, vec3(0, 0, 20));

int width, height;
float fov, zmin, zmax;
bool show_cursor = false;
bool depth_test = false;
bool cull = true;
int polygon_mode;
mat4 projection;

GLenum provoking_mode = GL_LAST_VERTEX_CONVENTION;

#define NUM_TEXTURES 3
GLuint textures[NUM_TEXTURES];

int tex_index;


const char* control_keys[] =
{
	"left",
	"right",
	"forward",
	"back",
	"tiltleft",
	"tiltright",
	"hidecursor",
	"fovup",
	"fovdown",
	"zminup",
	"zmindown",
	"provoking",
	"interpolation",
	"shader",
	"depthtest",
	"polygonmode"
};


enum Control {
	LEFT=0,
	RIGHT,
	FORWARD,
	BACK,
	TILTLEFT,
	TILTRIGHT,
	HIDECURSOR,
	FOVUP,
	FOVDOWN,
	ZMINUP,
	ZMINDOWN,
	PROVOKING,
	INTERPOLATION,
	SHADER,
	DEPTHTEST,
	POLYGONMODE,

	NCONTROLS
};

SDL_Keycode keyvalues[NCONTROLS];

int main(int argc, char** argv)
{
	setup_context();

	polygon_mode = 2;
	cull = true;
	fov = 40;
	zmin = 1;
	zmax = 50;
	
	parse_config_file("./controls.config", control_keys, (int*)keyvalues, NCONTROLS);

	for (int i=0; i<NCONTROLS; ++i) {
		//printf("Physical %s key acting as %s key\n", SDL_GetScancodeName(SDL_GetScancodeFromKey(keyvalues[i])),
		//SDL_GetKeyName(keyvalues[i]));
		
		keyvalues[i] = SDL_GetScancodeFromKey(keyvalues[i]);
	}

	width = WIDTH;
	height = HEIGHT;

	srand(time(NULL));

	GLuint plane_buf, plane_color_buf, plane_elem_buf, plane_vao, plane_tex_buf;
	vector<vec3> plane_verts;
	vector<ivec3> plane_tris;
	vector<vec2> plane_tex;
	vector<vec3> plane_colors;
	generate_plane(plane_verts, plane_tris, plane_tex, vec3(-12, 0, -12), vec3(24.0f/10.0f, 0, 0), vec3(0, 0, 24.0f/10.0f), 10, 10);
	for (int i=0; i<plane_verts.size(); ++i) {
		//plane_colors.push_back(vec3(1.0f, 0.0f, 0.0f));
		plane_colors.push_back(vec3(rsw::rand_float(0, 1), rsw::rand_float(0, 1), rsw::rand_float(0, 1)));
	}
	
	glGenVertexArrays(1, &plane_vao);
	glBindVertexArray(plane_vao);


	glGenBuffers(1, &plane_buf);
	glBindBuffer(GL_ARRAY_BUFFER, plane_buf);
	glBufferData(GL_ARRAY_BUFFER, plane_verts.size()*3*sizeof(float), &plane_verts[0], GL_STATIC_DRAW);
	//glBufferData(GL_ARRAY_BUFFER, box_expanded.size()*3*sizeof(float), &box_expanded[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &plane_color_buf);
	glBindBuffer(GL_ARRAY_BUFFER, plane_color_buf);
	glBufferData(GL_ARRAY_BUFFER, plane_colors.size()*3*sizeof(float), &plane_colors[0], GL_STATIC_DRAW);
	//glBufferData(GL_ARRAY_BUFFER, colors_expanded.size()*3*sizeof(float), &colors_expanded[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &plane_tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, plane_tex_buf);
	glBufferData(GL_ARRAY_BUFFER, plane_tex.size()*2*sizeof(float), &plane_tex[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);



	GLuint box_buf, color_buf, inst_buf, elem_buf, box_tex_buf;
	//generate_cylinder(cylinder, 2, 8, 100, 50);
	generate_box(box_verts, box_tris, box_tex, 6, 3, 1.5);
	vector<vec3> colors;
	colors.push_back(vec3(1.0f, 0.0f, 0.0f));
	colors.push_back(vec3(0.0f, 1.0f, 0.0f));
	colors.push_back(vec3(1.0f, 0.0f, 0.0f));
	colors.push_back(vec3(0.0f, 0.0f, 1.0f));
	colors.push_back(vec3(1.0f, 0.0f, 0.0f));
	colors.push_back(vec3(0.0f, 1.0f, 0.0f));
	colors.push_back(vec3(1.0f, 0.0f, 0.0f));
	colors.push_back(vec3(0.0f, 0.0f, 1.0f));

	vector<vec3> colors_expanded;
	vector<vec3> box_expanded;
	expand_verts(box_expanded, box_verts, box_tris);
	expand_verts(colors_expanded, colors, box_tris);

	vector<vec3> instance_pos;
	for (int i=0; i<100; ++i) {
		if (i)
			instance_pos.push_back(vec3(rsw::rand_float(-5, 5), rsw::rand_float(-4, 4), rsw::rand_float(-4, 4)));
		else
			instance_pos.push_back(vec3());
	}


	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


	//Setup box or boxes
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);


	glGenBuffers(1, &box_buf);
	glBindBuffer(GL_ARRAY_BUFFER, box_buf);
	//glBufferData(GL_ARRAY_BUFFER, box_verts.size()*3*sizeof(float), &box_verts[0], GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, box_expanded.size()*3*sizeof(float), &box_expanded[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &color_buf);
	glBindBuffer(GL_ARRAY_BUFFER, color_buf);
	//glBufferData(GL_ARRAY_BUFFER, colors.size()*3*sizeof(float), &colors[0], GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, colors_expanded.size()*3*sizeof(float), &colors_expanded[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);

	/*
	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, box_tris.size()*3*sizeof(int), &box_tris[0], GL_STATIC_DRAW);
	*/

	glGenBuffers(1, &inst_buf);
	glBindBuffer(GL_ARRAY_BUFFER, inst_buf);
	glBufferData(GL_ARRAY_BUFFER, instance_pos.size()*3*sizeof(float), &instance_pos[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribDivisor(3, 1);

	glGenBuffers(1, &box_tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, box_tex_buf);
	glBufferData(GL_ARRAY_BUFFER, box_tex.size()*2*sizeof(float), &box_tex[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);

	//Set up lines
	vector<vec3> line_verts;
	for (int i=0, j=-10; i < 11; ++i, j+=2) {
		line_verts.push_back(vec3(j, -1, -10));
		line_verts.push_back(vec3(j, -1, 10));
		line_verts.push_back(vec3(-10, -1, j));
		line_verts.push_back(vec3(10, -1, j));
	}

	GLuint line_vao, line_buf;
	glGenVertexArrays(1, &line_vao);
	glBindVertexArray(line_vao);


	glGenBuffers(1, &line_buf);
	glBindBuffer(GL_ARRAY_BUFFER, line_buf);
	glBufferData(GL_ARRAY_BUFFER, line_verts.size()*3*sizeof(float), &line_verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);



	glShaderPair* s;
	for (int i=0; i<NUM_PROGRAMS; ++i) {
		s = &shader_pairs[i];
		my_programs[i] = pglCreateProgram(s->vertex_shader, s->fragment_shader, s->vs_output_size, s->interpolation, s->use_frag_depth);
		glUseProgram(my_programs[i]);
		pglSetUniform(&the_uniforms);
	}
	glUseProgram(0);
	pglSetUniform(&the_uniforms);


	glGenTextures(NUM_TEXTURES, textures);
	printf("textures = %d %d\n", textures[0], textures[1]);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	if (!load_texture2D("../media/textures/test1.jpg", GL_LINEAR, GL_LINEAR, GL_REPEAT, false, false)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[1]);

	if (!load_texture2D("../media/textures/test2.jpg", GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, false, false)) {
		printf("failed to load texture\n");
		return 0;
	}

	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);	//I think

	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, width, height, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, NULL);


	the_uniforms.tex = textures[tex_index];


	glEnable(GL_CULL_FACE);

	mat4 translate = translation_mat4(vec3(0, 0, -20));


	GLFrame modelframe;
	mat4 camera, VP, MVP;

	//glViewport(-WIDTH/4, HEIGHT/4, WIDTH/2, HEIGHT/2);
	glViewport(0, 0, width, height);
	rsw::make_perspective_matrix(projection, DEG_TO_RAD(fov), float(width)/float(height), zmin, zmax);

	camera = camera_frame.get_matrix();


	//cout << "viewport\n" << *(mat4*)the_Context.vp_mat << "\n\nprojection\n" << projection << "\n\ncamera\n"
	//     << projection * modelframe.get_matrix() << "\n\n" << projection*camera << "\n\n" << projection*camera_frame.get_camera_matrix() << "\n\n";


	//load_matrix(MVP);
	unsigned int old_time = 0, new_time, frame_count = 0, last_frame = 0;
	float frame_time = 0;
	while (1) {
		if (handle_events())
			break;

		++frame_count;
		new_time = SDL_GetTicks();
		frame_time = (new_time - last_frame)/1000.0f;
		last_frame = new_time;

		if (new_time > old_time + 3000) {
			printf("%d %f FPS\n", new_time-old_time, frame_count*1000/((float)(new_time-old_time)));
			old_time = new_time;
			frame_count = 0;
		}

		if (!depth_test)
			glClear(GL_COLOR_BUFFER_BIT);
		else
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(my_programs[cur_shader]);
		the_uniforms.tex = textures[tex_index];

		modelframe.rotate_local(DEG_TO_RAD(30)*frame_time, 0, 1, 0);
		VP = projection * camera_frame.get_camera_matrix();
		MVP = VP * modelframe.get_matrix();
		the_uniforms.mvp = MVP;

		glBindVertexArray(vao);
		//glDrawArrays(GL_TRIANGLES, 0, box_expanded.size());
		//glDrawArrays(GL_LINES, 0, box_verts.size());

		glDrawArraysInstanced(GL_TRIANGLES, 0, box_expanded.size(), 1);
		//glDrawElements(GL_TRIANGLES, box_tris.size()*3, GL_UNSIGNED_INT, 0);
		//glDrawElementsInstanced(GL_TRIANGLES, box_tris.size()*3, GL_UNSIGNED_INT, 0, 10);

		the_uniforms.mvp = VP;
		//glBindVertexArray(plane_vao);
		//glDrawElements(GL_TRIANGLES, plane_tris.size()*3, GL_UNSIGNED_INT, 0);

		
		glBindVertexArray(line_vao);
		glUseProgram(0);
		the_uniforms.mvp = VP;
		glDrawArrays(GL_LINES, 0, line_verts.size());


		glBindTexture(GL_TEXTURE_2D, textures[tex_index]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, the_Context.back_buffer.buf);

		if (!depth_test)
			glClear(GL_COLOR_BUFFER_BIT);
		else
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		//glUseProgram(my_programs[1]);
		//the_uniforms.tex = textures[2];
		glUseProgram(my_programs[cur_shader]);
		the_uniforms.tex = textures[tex_index];

		the_uniforms.mvp = MVP;

		glBindVertexArray(vao);
		//glDrawArrays(GL_TRIANGLES, 0, box_expanded.size());
		//glDrawArrays(GL_LINES, 0, box_verts.size());

		glDrawArraysInstanced(GL_TRIANGLES, 0, box_expanded.size(), 1);
		//glDrawElements(GL_TRIANGLES, box_tris.size()*3, GL_UNSIGNED_INT, 0);
		//glDrawElementsInstanced(GL_TRIANGLES, box_tris.size()*3, GL_UNSIGNED_INT, 0, 10);

		the_uniforms.mvp = VP;
		//glBindVertexArray(plane_vao);
		//glDrawElements(GL_TRIANGLES, plane_tris.size()*3, GL_UNSIGNED_INT, 0);

		
		glBindVertexArray(line_vao);
		glUseProgram(0);
		the_uniforms.mvp = VP;
		glDrawArrays(GL_LINES, 0, line_verts.size());


		glinternal_Color red = { 255, 0, 0, 255 };
		glinternal_Color green = { 0, 255, 0, 255 };
		glinternal_Color blue = { 0, 0, 255, 255 };
		glinternal_vec2 p1, p2, p3;
		SET_VEC2(p1, 10, 10);
		SET_VEC2(p2, 90, 150);
		SET_VEC2(p3, 170, 10);

		/*
		 * Just testing drawing directly to screen, bypassing gl state
		 * could be used to draw text/GUIs/HUD etc more simply
		for (int k=0; k<1; ++k) {
			put_line(green, rand()%width, rand()%height, rand()%width, rand()%height);
			put_triangle(red, green, blue, p1, p2, p3);
		}
		*/


		SDL_UpdateTexture(tex, NULL, bbufpix, width * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);

	}


	cleanup();

	return 0;
}



void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("swrenderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	bbufpix = (u32*) malloc(WIDTH * HEIGHT * sizeof(u32));

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask)) {
		puts("Failed to initialize glContext");
		exit(0);
	}

	set_glContext(&the_Context);
}

void cleanup()
{

	free_glContext(&the_Context);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}


void interpolate_vs(float* vs_output, void* v_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* vertex_attribs = (vec4*)v_attribs;
	vec4* gl_Position = (vec4*)&builtins->gl_Position;

	((vec3*)vs_output)[0] = vertex_attribs[4].tovec3(); //color

	//cout << vertex_attribs[0] << "\n";
	//printf("%f %f %f\n", vs_output[0], vs_output[1], vs_output[2]);

	//printf("instance = %d\n", builtins->gl_InstanceID);
	//printf("%f %f %f\n", vertex_attribs[2].x, vertex_attribs[2].y, vertex_attribs[2].z);
	//*(builtins->gl_Position) = *((mat4*)uniforms) * (vertex_attribs[0] + vertex_attribs[2]);

	*gl_Position = *((mat4*)uniforms) * (vertex_attribs[0] + vec4(vertex_attribs[3].xyz(), 0));

	//printf("%f %f %f\n", builtins->gl_Position->x, builtins->gl_Position->y, builtins->gl_Position->z);
}

void interpolate_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_FragColor = vec4(((vec3*)fs_input)[0], 1.0f);
	//*(vec4*)&builtins->gl_FragColor = vec4(builtins->gl_FragCoord.z, builtins->gl_FragCoord.z, builtins->gl_FragCoord.z);
}


void texture_replace_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((vec2*)vs_output)[0] = ((vec4*)vertex_attribs)[2].xy(); //tex_coords

	*(vec4*)&builtins->gl_Position = *((mat4*)uniforms) * ((vec4*)vertex_attribs)[0];

}

void texture_replace_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];
	GLuint tex = ((My_Uniforms*)uniforms)->tex;


	builtins->gl_FragColor = texture2D(tex, tex_coords.x, tex_coords.y);
}

bool handle_events()
{
	SDL_Event event;
	SDL_Keysym keysym;

	bool remake_projection = false;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			keysym = event.key.keysym;
			//printf("%c %c\n", event.key.keysym.scancode, event.key.keysym.sym);
			//printf("Physical %s key acting as %s key",
			//SDL_GetScancodeName(keysym.scancode),
			//SDL_GetKeyName(keysym.sym);
			
			if (keysym.sym == SDLK_ESCAPE) {
				return true;
			} else if (keysym.scancode == keyvalues[PROVOKING]) {
				provoking_mode = (provoking_mode == GL_LAST_VERTEX_CONVENTION) ? GL_FIRST_VERTEX_CONVENTION : GL_LAST_VERTEX_CONVENTION;
				glProvokingVertex(provoking_mode);
			} else if (keysym.scancode == keyvalues[INTERPOLATION]) {
				glUseProgram(my_programs[cur_shader]);
				if (interp_mode == SMOOTH) {
					printf("noperspective\n");
					//todo change this func to DSA style, ie call it with the program to modify
					//rather than always modifying the current shader
					pglSetInterp(shader_pairs[cur_shader].vs_output_size, noperspective);
					interp_mode = NOPERSPECTIVE;
				} else {
					pglSetInterp(shader_pairs[cur_shader].vs_output_size, smooth);
					interp_mode = SMOOTH;
					printf("smooth\n");
				}
			} else if (keysym.scancode == keyvalues[SHADER]) {
				cur_shader = (cur_shader + 1) % NUM_PROGRAMS;
			} else if (keysym.scancode == keyvalues[HIDECURSOR]) {
				show_cursor = !show_cursor;
				SDL_SetRelativeMouseMode((SDL_bool)show_cursor);
				//SDL_ShowCursor(show_cursor);
			} else if (keysym.scancode == keyvalues[DEPTHTEST]) {
				if (depth_test)
					glDisable(GL_DEPTH_TEST);
				else
					glEnable(GL_DEPTH_TEST);

				depth_test = !depth_test;
			} else if (keysym.scancode == keyvalues[POLYGONMODE]) {
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			} else if (keysym.sym == SDLK_1) {
				tex_index = (tex_index + 1) % NUM_TEXTURES;
				the_uniforms.tex = textures[tex_index];
			}

			break; //sdl_keydown

		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				printf("window size %d x %d\n", event.window.data1, event.window.data2);
				width = event.window.data1;
				height = event.window.data2;

				remake_projection = true;

				bbufpix = (u32*)pglResizeFramebuffer(width, height);
				glViewport(0, 0, width, height);
				SDL_DestroyTexture(tex);
				tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, width, height);

				glBindTexture(GL_TEXTURE_2D, textures[2]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

				break;
			}
			break;

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

#define MOVE_SPEED 5
	
	if (state[keyvalues[LEFT]]) {
		camera_frame.move_right(time * MOVE_SPEED);
	}
	if (state[keyvalues[RIGHT]]) {
		camera_frame.move_right(time * -MOVE_SPEED);
	}
	if (state[SDL_SCANCODE_LSHIFT]) {
		camera_frame.move_up(time * MOVE_SPEED);
	}
	if (state[SDL_SCANCODE_SPACE]) {
		camera_frame.move_up(time * -MOVE_SPEED);
	}
	if (state[keyvalues[FORWARD]]) {
		camera_frame.move_forward(time*20);
	}
	if (state[keyvalues[BACK]]) {
		camera_frame.move_forward(time*-20);
	}
	if (state[keyvalues[TILTLEFT]]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(-60*time));
	}
	if (state[keyvalues[TILTRIGHT]]) {
		camera_frame.rotate_local_z(DEG_TO_RAD(60*time));
	}
	if (state[keyvalues[FOVUP]]) {
		if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]) {
			if (zmax < 2000)
				zmax += 1;
		} else {
			if (fov < 170)
				fov += 0.2;
		}
		printf("zmax = %f\nfov = %f\n", zmax, fov);
		remake_projection = true;
	}
	if (state[keyvalues[FOVDOWN]]) {
		if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]) {
			if (zmax > 20)
				zmax -= 1;
		} else {
			if (fov > 10)
				fov -= 0.2;
		}
		printf("zmax = %f\nfov = %f\n", zmax, fov);
		remake_projection = true;
	}
	if (state[keyvalues[ZMINUP]]) {
		if (zmin < 50) {
			zmin += 0.1;
			remake_projection = true;
			printf("zmin = %f\n", zmin);
		}
	}
	if (state[keyvalues[ZMINDOWN]]) {
		if (zmin > 0.5) {
			zmin -= 0.1;
			remake_projection = true;
			printf("zmin = %f\n", zmin);
		}
	}

	if (remake_projection) {
		rsw::make_perspective_matrix(projection, DEG_TO_RAD(fov), float(width)/float(height), zmin, zmax);
		remake_projection = false;
	}

	last_time = cur_time;

	return false;
}
