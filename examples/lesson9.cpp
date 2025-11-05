#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"

#include <SDL.h>

#include <glm_matstack.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>
#include <vector>

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
#define NUM_STARS 50

#ifndef FPS_EVERY_N_SECS
#define FPS_EVERY_N_SECS 1
#endif

#define FPS_DELAY (FPS_EVERY_N_SECS*1000)

using namespace std;

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

	vec3 color;
	GLuint tex;
} My_Uniforms;

void cleanup();
void setup_context();
int handle_events();


void star_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void star_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int filter = 1;

float z = -12;
float tilt = 90;
float spin;

int do_twinkle = GL_TRUE;

GLuint texture;

matrix_stack mvMatrixStack;
My_Uniforms the_uniforms;

// NOTE(rswinkle) I do not like this program structure but I am mostly
// going for a straight port. Maybe later I'll refactor it the way I
// would prefer it
struct Star
{
	float angle;
	float dist;
	float rotationSpeed;
	vec3 color;
	vec3 twinkle;

	Star(float startingDistance, float rotationSpeed)
	{
		angle = 0;
		dist = startingDistance;
		this->rotationSpeed = rotationSpeed;
		printf("dist = %f\nrot_speed = %f\n", dist, rotationSpeed);

		randomize_colors();
	}

	void randomize_colors()
	{
		// TODO try different methods for random colors
		//
		// not sure why sphericalRand isn't working like I expect
		//color = glm::sphericalRand(1);
		//twinkle = glm::sphericalRand(1);
		color = vec3(rand()/(float)RAND_MAX,rand()/(float)RAND_MAX,rand()/(float)RAND_MAX);
		twinkle = vec3(rand()/(float)RAND_MAX,rand()/(float)RAND_MAX,rand()/(float)RAND_MAX);
	}

	// angle, angle, bool, angles in degrees
	void draw(float tilt, float spin, int do_twinkle)
	{
		mvMatrixStack.push();

		// move to star's position
		//
		// these two make it work if you remove tilt
		//mvMatrixStack.rotate(glm::radians(angle), vec3(0, 0, 1));
		//mvMatrixStack.translate(dist, 0, 0);
		
		// with tilt
		mvMatrixStack.rotate(glm::radians(angle), vec3(0, 1, 0));
		mvMatrixStack.translate(dist, 0, 0);

		// rotate back so star is facing viewer
		mvMatrixStack.rotate(glm::radians(-angle), vec3(0, 1, 0));
		mvMatrixStack.rotate(glm::radians(-tilt), vec3(1, 0, 0));

		if (do_twinkle) {
			// Draw a non-rotating star in the alternate "twinkling" color
			the_uniforms.color = twinkle;
			the_uniforms.mv_mat = mvMatrixStack.get_matrix();
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // quad via tri strip
		}

		// all stars spin around z at same rate
		mvMatrixStack.rotate(glm::radians(spin), vec3(0, 0, 1));

		// draw star in main color
		the_uniforms.color = color;
		the_uniforms.mv_mat = mvMatrixStack.get_matrix();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // quad via tri strip

		mvMatrixStack.pop();
	}

	void animate(float elapsed_time)
	{
		float effectiveFPMS = 60/1000.0;
		angle += rotationSpeed * effectiveFPMS * elapsed_time;

		// decrease distance, resetting star to outside of spiral if it's
		// at the center
		dist -= 0.01 * effectiveFPMS * elapsed_time;
		if (dist < 0) {
			dist += 5;
			randomize_colors();
		}
		//printf("angle %f\ndist = %f\n", angle, dist);
		//printf("color = (%f, %f, %f)\n", color.x, color.y, color.z);
		//printf("twinkle = (%f, %f, %f)\n", twinkle.x, twinkle.y, twinkle.z);
	}
};

int main(int argc, char** argv)
{
	setup_context();

	float vertices[] = {
		-1.0, -1.0,  0.0,
		 1.0, -1.0,  0.0,
		-1.0,  1.0,  0.0,
		 1.0,  1.0,  0.0
	};

	float texcoords[] = {
		0.0, 0.0,
		1.0, 0.0,
		0.0, 1.0,
		1.0, 1.0
	};

	vector<Star> stars;
	for (int i=0; i<NUM_STARS; i++) {
		stars.push_back(Star((i/(float)NUM_STARS) * 5.0, i /(float)NUM_STARS));
	}
	//stars.push_back(Star(1.0f, 0.25));

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	if (!load_texture2D("../media/textures/star.gif", GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_TRUE, NULL, NULL, NULL)) {
		printf("failed to load texture\n");
		return 0;
	}

	mvMatrixStack.load_identity();

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vert_buf;
	glGenBuffers(1, &vert_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);


	GLenum vs_outs[] = { PGL_SMOOTH2 };
	GLuint program = pglCreateProgram(star_vs, star_fs, 2, vs_outs, GL_FALSE);
	glUseProgram(program);

	pglSetUniform(&the_uniforms);

	// these never change
	the_uniforms.tex = texture;
	the_uniforms.proj_mat = glm::perspective(glm::radians(45.0f), WIDTH/(float)HEIGHT, 0.1f, 100.0f);

	// TODO why even use a stack when we only have one object?
	matrix_stack mat_stack;
	mat_stack.load_identity();

	float elapsed;

    glEnable(GL_DEPTH_TEST);
	glClearColor(0, 0, 0, 1);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);

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

		for (int i=0; i<stars.size(); i++) {
			stars[i].animate(elapsed);
		}

		//uMVMatrix = glm::translate(mat(1), vec3(0, 0, z));
		//uMVMatrix = glm::rotate(uMVMatrix, glm::radians(tilt), vec3(1,0,0));
		mvMatrixStack.load_identity();
		mvMatrixStack.translate(0, 0, z);
		mvMatrixStack.rotate(glm::radians(tilt), vec3(1, 0, 0));

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(vao);

		for (int i=0; i<stars.size(); i++) {
			stars[i].draw(tilt, spin, do_twinkle);
			spin += 0.1;
		}

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);

		if (nk_begin(ctx, "Controls", nk_rect(WIDTH-GUI_W, 0, GUI_W, HEIGHT),
		    NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
		{
			static const char* on_off[] = { "Off", "On" };
			//nk_layout_row_static(ctx, 30, GUI_W, 1);
			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_checkbox_label(ctx, "Twinkle", &do_twinkle)) {
				printf("Twinkle %s\n", on_off[do_twinkle]);
			}
		}
		nk_end(ctx);

		//nk_sdl_render(NK_ANTI_ALIASING_ON);
		nk_sdl_render(NK_ANTI_ALIASING_OFF);

		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void star_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	*(vec2*)vs_output = *(vec2*)&vertex_attribs[2]; // smooth out texcoord = in_texcoord

	My_Uniforms* u = (My_Uniforms*)uniforms;

	*(vec4*)&builtins->gl_Position = u->proj_mat * u->mv_mat * ((vec4*)vertex_attribs)[0];
}

void star_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];

	My_Uniforms* u = (My_Uniforms*)uniforms;

	pgl_vec4 tex_color = texture2D(u->tex, tex_coords.x, tex_coords.y);
	tex_color.x *= u->color.x;
	tex_color.y *= u->color.y;
	tex_color.z *= u->color.z;

	builtins->gl_FragColor = tex_color;
}

void setup_context()
{
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("Lesson 8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
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
				// Using DSA functions because I can and it's slightly more efficient
				if (filter == 0) {
					puts("GL_NEAREST\n");
					glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				} else {
					puts("GL_LINEAR\n");
					glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}

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
		z += 0.05;
	} else if (state[SDL_SCANCODE_PAGEDOWN]) {
		z -= 0.05;
	}

	return 1;
}









