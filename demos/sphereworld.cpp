#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#define MANGLE_TYPES
#define EXCLUDE_GLSL

#include "rsw_math.h"
#include "gltools.h"
#include "GLObjects.h"
#include "rsw_glframe.h"
#include "rsw_primitives.h"
#include "rsw_halfedge.h"
#include "controls.h"

#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

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
	mat4 mvp_mat;
	mat4 mv_mat;
	mat3 normal_mat;


	GLuint tex0;
	GLuint tex1;
	GLuint tex2;

	vec3 Ka;           // Ambient reflectivity
	vec3 Kd;           // Diffuse reflectivity
	vec3 Ks;           // Specular reflectivity
	float Shininess;   // Specular shininess factor

	vec3 light_pos;
	vec4 modulate;
} My_Uniforms;

void setup_context();
bool handle_events();
void cleanup();

enum
{
	TEX_REPLACE_INSTANCED,
	TEX_REPLACE_MODULATED,

	TEX_ADS_INSTANCED,
	TEX_ADS_MODULATED,
	NUM_PROGRAMS
};

//#define NUM_PROGRAMS 4
void texture_replace_instanced_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void texture_replace_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void texture_replace_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void texture_replace_modulate_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void texture_ADS_instanced_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void texture_ADS_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void texture_ADS_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void texture_ADS_modulate_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);


GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };
GLenum noperspective[4] = { NOPERSPECTIVE, NOPERSPECTIVE, NOPERSPECTIVE, NOPERSPECTIVE };


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
	{ texture_replace_instanced_vs, texture_replace_fs, 2, { SMOOTH, SMOOTH }, GL_FALSE },
	{ texture_replace_vs, texture_replace_modulate_fs, 2, { SMOOTH, SMOOTH }, GL_FALSE },

	//gouraud in vertex shader
	{ texture_ADS_instanced_vs, texture_ADS_fs, 3, { SMOOTH, SMOOTH, SMOOTH }, GL_FALSE },
	{ texture_ADS_vs, texture_ADS_modulate_fs, 3, { SMOOTH, SMOOTH, SMOOTH }, GL_FALSE }
};

enum
{
	ATTR_VERTEX = 0,
	ATTR_NORMAL = 1,
	ATTR_TEXCOORD = 2
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

GLFrame camera_frame(true, vec3(0, 0.3, 2.5));


int width, height;
float fov, zmin, zmax;
bool show_cursor = false;
bool depth_test = true;
bool cull = true;
int polygon_mode;
mat4 projection;

GLenum provoking_mode = GL_LAST_VERTEX_CONVENTION;

#define NUM_TEXTURES 3
GLuint textures[NUM_TEXTURES];

int tex_index;

#define NUM_SPHERES 25
#define FLOOR_SIZE 20


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
	fov = 35;
	zmin = 0.5;
	zmax = 100;
	
	const char* controls_file = NULL;
	if (argc == 2)
		controls_file = argv[1];
	else 
		controls_file = "./qwerty_controls.config";

	parse_config_file(controls_file, control_keys, (int*)keyvalues, NCONTROLS);

	for (int i=0; i<NCONTROLS; ++i) {
		//printf("Physical %s key acting as %s key\n", SDL_GetScancodeName(SDL_GetScancodeFromKey(keyvalues[i])),
		//SDL_GetKeyName(keyvalues[i]));
		
		keyvalues[i] = SDL_GetScancodeFromKey(keyvalues[i]);
	}

	width = WIDTH; 
	height = HEIGHT;
	//srand(time(NULL));


	glGenTextures(NUM_TEXTURES, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	if (!load_texture2D("../media/textures/marble.tga", GL_LINEAR, GL_LINEAR, GL_REPEAT, false, false)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[1]);

	if (!load_texture2D("../media/textures/mars.tga", GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, false, false)) {
		printf("failed to load texture\n");
		return 0;
	}

	glBindTexture(GL_TEXTURE_2D, textures[2]);
	if (!load_texture2D("../media/textures/moon.tga", GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, false, false)) {
		printf("failed to load texture\n");
		return 0;
	}




	mat4 identity;

	Vertex_Array torus_vao(1);
	torus_vao.bind();

	vector<vec3> torus_verts;
	vector<vec2> torus_tex;
	vector<ivec3> torus_tris;
	vector<vec3> torus_draw_verts;
	vector<vec2> torus_draw_tex;
	float major = .3;
	make_torus(torus_verts, torus_tris, torus_tex, major, 0.4-major, 40, 20);

	for (int i=0; i<torus_verts.size(); ++i)
		torus_verts[i].y += 0.41;

	vector<vec3> torus_normals;
	compute_normals(torus_verts, torus_tris, NULL, DEG_TO_RAD(50), torus_normals);

	expand_verts(torus_draw_verts, torus_verts, torus_tris);

	Buffer torus_buf(1);
	torus_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*torus_draw_verts.size()*3, &torus_draw_verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_VERTEX);
	glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Buffer torus_norm_buf(1);
	torus_norm_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*torus_normals.size()*3, &torus_normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_NORMAL);
	glVertexAttribPointer(ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Buffer torus_tex_buf(1);
	torus_tex_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*torus_tex.size()*2, &torus_tex[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_TEXCOORD);
	glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);


	/*
	Vertex_Array normals(1);
	normals.bind();
	vector<vec3> normal_lines;
	for (int i=0; i<torus_normals.size(); ++i) {
	if (i % 6 == 0)
	cout <<torus_draw_verts[i] << "   " << torus_normals[i] << "\n";

	normal_lines.push_back(torus_draw_verts[i]);
	normal_lines.push_back(torus_draw_verts[i] + 0.02*torus_normals[i]);
	}

	Buffer normal_buf(1);
	normal_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*normal_lines.size()*3, &normal_lines[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_VERTEX);
	glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
*/


	vector<vec3> floor_verts;
	vector<vec3> floor_draw_verts;
	vector<vec2> floor_tex;
	vector<ivec3> floor_tris;
	make_plane(floor_verts, floor_tris, floor_tex, vec3(-FLOOR_SIZE/2.0, 0, FLOOR_SIZE/2.0), vec3(0, 0, -FLOOR_SIZE), vec3(FLOOR_SIZE, 0, 0), 1, 1, false);
	//make_plane(floor_verts, floor_tris, floor_tex, vec3(-FLOOR_SIZE/2.0, 0, FLOOR_SIZE/2.0), vec3(0, 0, -1), vec3(1, 0, 0), FLOOR_SIZE, FLOOR_SIZE, false);

	float tex_repeat = FLOOR_SIZE/4;
	for (int i=0; i<floor_tex.size(); ++i) {
		floor_tex[i] *= tex_repeat;
		//cout << floor_tex[i] << "\n";
	}


	vector<vec3> floor_normals;
	compute_normals(floor_verts, floor_tris, NULL, 0, floor_normals);

	expand_verts(floor_draw_verts, floor_verts, floor_tris);

	Vertex_Array floor_vao(1);
	floor_vao.bind();

	Buffer floor(1);
	floor.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*floor_draw_verts.size()*3, &floor_draw_verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_VERTEX);
	glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Buffer floor_norm_buf(1);
	floor_norm_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*floor_normals.size()*3, &floor_normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_NORMAL);
	glVertexAttribPointer(ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Buffer floor_tex_buf(1);
	floor_tex_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*floor_tex.size()*2, &floor_tex[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_TEXCOORD);
	glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);


	Vertex_Array sphere_vao(1);
	sphere_vao.bind();
	vector<vec3> sphere_verts;
	vector<vec3> sphere_draw_verts;
	vector<vec2> sphere_tex;
	vector<ivec3> sphere_tris;
	make_sphere(sphere_verts, sphere_tris, sphere_tex, 0.1, 26, 13);

	vector<vec3> sphere_normals;
	compute_normals(sphere_verts, sphere_tris, NULL, DEG_TO_RAD(50), sphere_normals);

	expand_verts(sphere_draw_verts, sphere_verts, sphere_tris);

	Buffer sphere(1);
	sphere.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*sphere_draw_verts.size()*3, &sphere_draw_verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_VERTEX);
	glVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Buffer sphere_normal_buf(1);
	sphere_normal_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*sphere_normals.size()*3, &sphere_normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_NORMAL);
	glVertexAttribPointer(ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);

	Buffer sphere_tex_buf(1);
	sphere_tex_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*sphere_tex.size()*2, &sphere_tex[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_TEXCOORD);
	glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);


	vector<vec3> instance_pos;
	for (int i=0; i<NUM_SPHERES+1; ++i) {
		if (i)
			instance_pos.push_back(vec3(rsw::randf_range(-FLOOR_SIZE/2.0, FLOOR_SIZE/2.0), 0.4, rsw::randf_range(-FLOOR_SIZE/2.0, FLOOR_SIZE/2.0)));
		else
			instance_pos.push_back(vec3());
        //GLfloat x = ((GLfloat)((rand() % 400) - 200) * 0.1f);
        //GLfloat z = ((GLfloat)((rand() % 400) - 200) * 0.1f);
	}

	GLuint inst_buf;
	glGenBuffers(1, &inst_buf);
	glBindBuffer(GL_ARRAY_BUFFER, inst_buf);
	pglBufferData(GL_ARRAY_BUFFER, instance_pos.size()*3*sizeof(float), &instance_pos[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribDivisor(3, 1);


	glBindVertexArray(0);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glViewport(0, 0, WIDTH, HEIGHT);


	glShaderPair* s;
	for (int i=0; i<NUM_PROGRAMS; ++i) {
		s = &shader_pairs[i];
		my_programs[i] = pglCreateProgram(s->vertex_shader, s->fragment_shader, s->vs_output_size, s->interpolation, s->use_frag_depth);
		glUseProgram(my_programs[i]);
		pglSetUniform(&the_uniforms);
	}

	rsw::make_perspective_matrix(projection, DEG_TO_RAD(fov), float(width)/float(height), zmin, zmax);

	the_uniforms.Ka = vec3(0.1, 0.1, 0.1);
	the_uniforms.Kd = vec3(0.8, 0.8, 0.8);
	the_uniforms.Ks = vec3(1.0, 1.0, 1.0);
	the_uniforms.Shininess = 128.0f;

	SDL_SetRelativeMouseMode(SDL_TRUE);

	vec4 modulate_color(1, 1, 1, 0.75);

	mat4 scale_mat = rsw::scale_mat4(1.0f, -1.0f, 1.0f);
	mat4 translate_sphere = rsw::translation_mat4(0.8f, 0.4f, 0.0f);

	mat4 VP, torus_rot_mat, sphere_rot_mat;
	mat4 camera_mat;


	//load_matrix(MVP);
	unsigned int old_time = 0, new_time, counter = 0;
	float total_time = 0;
	while (1) {
		if (handle_events())
			break;

		++counter;
		new_time = SDL_GetTicks();
		total_time = new_time/1000.0f;
		if (new_time - old_time >= 3000) {
			printf("%f FPS\n", counter*1000.0f/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}

		if (rand() % 50 == 0) {
			puts("rearranging spheres");
			for (int i=1; i<NUM_SPHERES+1; ++i) {
				instance_pos[i] = vec3(rsw::randf_range(-FLOOR_SIZE/2.0, FLOOR_SIZE/2.0), 0.4, rsw::randf_range(-FLOOR_SIZE/2.0, FLOOR_SIZE/2.0));
			}
		}


		if (!depth_test)
			glClear(GL_COLOR_BUFFER_BIT);
		else
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// First draw the spheres' reflections
		// hence the GL_CW and scale -1 y

		camera_mat = camera_frame.get_camera_matrix();

		VP = projection * camera_mat;

		cur_shader = my_programs[TEX_ADS_INSTANCED];
		glUseProgram(cur_shader);

		the_uniforms.mvp_mat = VP * scale_mat;

		glFrontFace(GL_CW);

		the_uniforms.tex0 = textures[2];
		sphere_vao.bind();

		//stationary spheres first, instanced
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, sphere_draw_verts.size(), NUM_SPHERES, 1);


		//now the moving, rotating sphere
		rsw::load_rotation_mat4(sphere_rot_mat, vec3(0, 1, 0), -2*total_time*DEG_TO_RAD(60.0f));
		the_uniforms.mvp_mat = VP * scale_mat * sphere_rot_mat * translate_sphere;
		glDrawArrays(GL_TRIANGLES, 0, sphere_draw_verts.size());


		//finally the rotating torus reflection
		rsw::load_rotation_mat4(torus_rot_mat, vec3(0, 1, 0), total_time*DEG_TO_RAD(60.0f));
		the_uniforms.mvp_mat = VP * scale_mat * torus_rot_mat;

		the_uniforms.tex0 = textures[1];
		torus_vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, torus_draw_verts.size());


		//Back to normal for the rest
		glFrontFace(GL_CCW);


		// Now the floor with blending enabled to make the fake reflection
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		cur_shader = my_programs[TEX_REPLACE_MODULATED];
		glUseProgram(cur_shader);
		the_uniforms.mvp_mat = VP;
		the_uniforms.modulate = modulate_color;

		the_uniforms.tex0 = textures[0];
		floor_vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, floor_draw_verts.size());

		glDisable(GL_BLEND);


		//Now the real objects in the same order as before
		cur_shader = my_programs[TEX_ADS_INSTANCED];
		glUseProgram(cur_shader);
		the_uniforms.mvp_mat = VP;

		the_uniforms.normal_mat = rsw::extract_rotation_mat4(camera_mat);

		the_uniforms.tex0 = textures[2];
		sphere_vao.bind();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, sphere_draw_verts.size(), NUM_SPHERES, 1);

		the_uniforms.mvp_mat = VP * sphere_rot_mat * translate_sphere;

		the_uniforms.normal_mat = rsw::extract_rotation_mat4(camera_mat * sphere_rot_mat);
		glDrawArrays(GL_TRIANGLES, 0, sphere_draw_verts.size());



		the_uniforms.mvp_mat = VP * torus_rot_mat;
		the_uniforms.normal_mat = rsw::extract_rotation_mat4(camera_mat * torus_rot_mat);

		the_uniforms.tex0 = textures[1];
		torus_vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, torus_draw_verts.size());


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
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "SDL_Init error: " << SDL_GetError() << "\n";
		exit(0);
	}

	window = SDL_CreateWindow("Sphereworld", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, PIX_FORMAT, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, rmask, gmask, bmask, amask)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
}

void cleanup()
{

	free_glContext(&the_Context);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}



void texture_replace_instanced_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* v_attribs = (vec4*)vertex_attribs;

	((vec2*)vs_output)[0] = ((vec4*)v_attribs)[2].xy(); //tex_coords

	*(vec4*)&builtins->gl_Position = *((mat4*)uniforms) * (v_attribs[0] + vec4(v_attribs[3].xyz(), 0));

}
void texture_replace_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];
	GLuint tex = ((My_Uniforms*)uniforms)->tex0;

	builtins->gl_FragColor = texture2D(tex, tex_coords.x, tex_coords.y);
}




void texture_replace_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((vec2*)vs_output)[0] = ((vec4*)vertex_attribs)[2].xy(); //tex_coords

	*(vec4*)&builtins->gl_Position = *((mat4*)uniforms) * ((vec4*)vertex_attribs)[0];

}

void texture_replace_modulate_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];
	GLuint tex = ((My_Uniforms*)uniforms)->tex0;
	vec4 modulate = ((My_Uniforms*)uniforms)->modulate;

	//builtins->gl_FragColor = texture2D(tex, tex_coords.x, tex_coords.y);
	//builtins->gl_FragColor = mult_vec4s(builtins->gl_FragColor, *(glinternal_vec4*)&modulate);

	glinternal_vec4 tmp = texture2D(tex, tex_coords.x, tex_coords.y);
	*(vec4*)&builtins->gl_FragColor = modulate * vec4(tmp.x, tmp.y, tmp.z, tmp.w);
}




void texture_ADS_instanced_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* vert_attribs = (vec4*)vertex_attribs;
	My_Uniforms* u = (My_Uniforms*)uniforms;

	vec3 light_dir = vec3(0, 1, 0);
	vec3 eye_dir = vec3(0, 0, 1);

	//to normalize or not depending on whether the matrix is normal
	vec3 eye_normal = u->normal_mat * vert_attribs[ATTR_NORMAL].xyz();

	float light_intensity = u->Ka.x;

	float diff = max(0.0f, dot(eye_normal, light_dir));
	light_intensity += diff * u->Kd.x;


	vec3 r = reflect(-light_dir, eye_normal);
	float spec = max(0.0f, dot(eye_dir, r));
	float fspec;
	if (diff > 0) {
		fspec = pow(spec, u->Shininess);
		light_intensity += u->Ks.x * fspec;
	}


	vs_output[0] = vert_attribs[ATTR_TEXCOORD].x;
	vs_output[1] = vert_attribs[ATTR_TEXCOORD].y;
	
	vs_output[2] = light_intensity;

	*(vec4*)&builtins->gl_Position = u->mvp_mat * (vert_attribs[ATTR_VERTEX] + vec4(vert_attribs[3].xyz(), 0));
}

void texture_ADS_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = vec2(fs_input[0], fs_input[1]);
	float light_intensity = fs_input[2];

	GLuint tex = ((My_Uniforms*)uniforms)->tex0;

	//many ways to handle the fact that texture2D returns it's own internal type
	//which is glinternal_vec4 if MANGLE_TYPES is defined
	glinternal_vec4 color = texture2D(tex, tex_coords.x, tex_coords.y);
	color = scale_vec4(color, light_intensity);

	builtins->gl_FragColor = color;
}


void texture_ADS_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* vert_attribs = (vec4*)vertex_attribs;
	My_Uniforms* u = (My_Uniforms*)uniforms;

	vec3 light_dir = vec3(0, 1, 0);
	vec3 eye_dir = vec3(0, 0, 1);

	//to normalize or not depending on whether the matrix is normal
	vec3 eye_normal = u->normal_mat * vert_attribs[ATTR_NORMAL].xyz();

	float light_intensity = u->Ka.x;

	float diff = max(0.0f, dot(eye_normal, light_dir));
	light_intensity += diff * u->Kd.x;

	/* no specular for now
	 */

	vec3 r = reflect(-light_dir, eye_normal);
	float spec = max(0.0f, dot(eye_dir, r));
	float fspec;
	if (diff > 0) {
		fspec = pow(spec, u->Shininess);
		light_intensity += u->Ks.x * fspec;
	}


	vs_output[0] = vert_attribs[ATTR_TEXCOORD].x;
	vs_output[1] = vert_attribs[ATTR_TEXCOORD].y;
	
	vs_output[2] = light_intensity;

	*(vec4*)&builtins->gl_Position = u->mvp_mat * vert_attribs[ATTR_VERTEX];

}
void texture_ADS_modulate_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{


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
      	  	 // 	  SDL_GetScancodeName(keysym.scancode),
      	  	  //	  SDL_GetKeyName(keysym.sym));
			
			if (keysym.sym == SDLK_ESCAPE) {
				return true;
			} else if (keysym.scancode == keyvalues[PROVOKING]) {
				provoking_mode = (provoking_mode == GL_LAST_VERTEX_CONVENTION) ? GL_FIRST_VERTEX_CONVENTION : GL_LAST_VERTEX_CONVENTION;
				glProvokingVertex(provoking_mode);
			} else if (keysym.scancode == keyvalues[INTERPOLATION]) {
				if (interp_mode == SMOOTH) {
					printf("noperspective\n");
					pglSetInterp(shader_pairs[cur_shader].vs_output_size, noperspective);
					interp_mode = NOPERSPECTIVE;
				} else {
					pglSetInterp(shader_pairs[cur_shader].vs_output_size, smooth);
					interp_mode = SMOOTH;
					printf("smooth\n");
				}
			} else if (keysym.scancode == keyvalues[SHADER]) {
				// TODO not actually used, maybe add phong so I have something to switch between?
				// cur_shader = (cur_shader + 1) % NUM_PROGRAMS;
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

#define MOVE_SPEED 2
	
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
		camera_frame.move_forward(time * MOVE_SPEED);
	}
	if (state[keyvalues[BACK]]) {
		camera_frame.move_forward(time * -MOVE_SPEED);
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
