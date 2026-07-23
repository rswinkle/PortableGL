
#include "rsw_math.h"

//#define PGL_HERMITE_SMOOTHING
#define PGL_PREFIX_TYPES
#define PORTABLEGL_IMPLEMENTATION
#include "gltools.h"

#include "stb_image.h"

#include <iostream>
#include <stdio.h>


#define SDL_MAIN_HANDLED
#include <SDL.h>

#define WIDTH 640
#define HEIGHT 480

using namespace std;

using rsw::vec4;
using rsw::vec3;
using rsw::vec2;
using rsw::mat4;

bool handle_events();
void cleanup();
void setup_context();
void setup_gl_data();


vec4 Red(1.0f, 0.0f, 0.0f, 0.0f);
vec4 Green(0.0f, 1.0f, 0.0f, 0.0f);
vec4 Blue(0.0f, 0.0f, 1.0f, 0.0f);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* SDL_tex;

pix_t* bbufpix;

glContext the_Context;

int tex_index;
int tex_filter; // index into filter_modes[]

static const GLenum filter_modes[] = {
	GL_NEAREST,
	GL_LINEAR,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_LINEAR,
};
static const char* filter_mode_names[] = {
	"GL_NEAREST (min+mag)",
	"GL_LINEAR (min+mag)",
	"GL_NEAREST_MIPMAP_NEAREST",
	"GL_LINEAR_MIPMAP_NEAREST",
	"GL_NEAREST_MIPMAP_LINEAR",
	"GL_LINEAR_MIPMAP_LINEAR",
};
#define NUM_FILTER_MODES ((int)(sizeof(filter_modes)/sizeof(filter_modes[0])))

static int is_mip_filter(GLenum f)
{
	return f == GL_NEAREST_MIPMAP_NEAREST || f == GL_NEAREST_MIPMAP_LINEAR
	    || f == GL_LINEAR_MIPMAP_NEAREST  || f == GL_LINEAR_MIPMAP_LINEAR;
}
static int is_nearest_style(GLenum f)
{
	return f == GL_NEAREST || f == GL_NEAREST_MIPMAP_NEAREST || f == GL_NEAREST_MIPMAP_LINEAR;
}

#define NUM_TEXTURES 3
GLuint textures[NUM_TEXTURES];

static void apply_tex_filter(GLuint tex, GLenum mode)
{
	GLenum mag = is_nearest_style(mode) ? GL_NEAREST : GL_LINEAR;
	glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, mode);
	glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, mag);
	if (is_mip_filter(mode))
		glGenerateTextureMipmap(tex);
}

// delta +1 (F) or -1 (D)
static void cycle_tex_filter(int delta)
{
	tex_filter = (tex_filter + delta + NUM_FILTER_MODES) % NUM_FILTER_MODES;
	printf("Filter: %s\n", filter_mode_names[tex_filter]);
	if (is_mip_filter(filter_modes[tex_filter])) {
		puts("  (pgl_draw_geometry_raw has no auto LOD — mip modes still use level 0)");
	}
	for (int i = 0; i < NUM_TEXTURES; ++i)
		apply_tex_filter(textures[i], filter_modes[tex_filter]);
}

float points[] =
{
	-0.5, -0.5,
	-0.5,  0.5,
	 0.5, -0.5,
	 0.5,  0.5
};

float points_tr[8];


int main(int argc, char** argv)
{

	setup_context();

	for (int i=0; i<8; i+=2) {
		points_tr[i] = rsw_mapf(points[i], -1.0f, 1.0f, 0, WIDTH);
		points_tr[i+1] = rsw_mapf(points[i+1], -1.0f, 1.0f, 0, HEIGHT);
	}

	pgl_Color colors[] =
	{
		{ 255, 255, 255, 255 },
		{ 255, 255, 255, 255 },
		{ 255, 255, 255, 255 },
		{ 255, 255, 255, 255 }
	};

	int indices[] =
	{
		0, 1, 2,
		2, 1, 3
	};

	float tex_coords[] =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		1.0, 1.0,
	};

	rsw::Color test_texture[9];
	for (int i=0; i<9; ++i) {
		if (i % 2)
			test_texture[i] = rsw::Color(0, 0, 0, 255);
		else
			test_texture[i] = rsw::Color(255, 255, 255, 255);
	}



	glGenTextures(NUM_TEXTURES, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	if (!load_texture2D("../media/textures/test1.jpg", GL_NEAREST, GL_NEAREST, GL_MIRRORED_REPEAT, false, false, NULL, NULL)) {
		puts("failed to load texture");
		return 0;
	}


	glBindTexture(GL_TEXTURE_2D, textures[1]);

	if (!load_texture2D("../media/textures/test2.jpg", GL_NEAREST, GL_NEAREST, GL_REPEAT, false, false, NULL, NULL)) {
		puts("failed to load texture");
		return 0;
	}

	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, test_texture);

	// Pre-build mip chains so F can cycle into *MIPMAP* modes immediately
	for (int i = 0; i < NUM_TEXTURES; ++i)
		glGenerateTextureMipmap(textures[i]);

	tex_index = 0;
	tex_filter = 0;
	puts("Controls: 1 = next texture, F/D = next/prev filter, arrows = scale/rotate");
	puts("  Note: this demo uses pgl_draw_geometry_raw(), which has no automatic");
	puts("  mip LOD (sidesteps the main pipeline). *MIPMAP* modes still sample");
	puts("  level 0; only NEAREST vs LINEAR within L0 changes.  Real auto-LOD");
	puts("  needs glDraw* + a fragment shader (see demos/texturing).  texture2DLod");
	puts("  can pick a level explicitly in a custom FS, not in geometry_raw.");
	puts("  (*MIPMAP_LINEAR trilinear only on the normal draw path, not geometry_raw.)");
	printf("Filter: %s\n", filter_mode_names[tex_filter]);

	glClearColor(0, 0, 0, 1);

	unsigned int orig_time = SDL_GetTicks();
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
		//SDL_Delay(2);

		glClear(GL_COLOR_BUFFER_BIT);
		pgl_draw_geometry_raw(textures[tex_index], points_tr, sizeof(float)*2, colors, sizeof(pgl_Color), tex_coords, sizeof(float)*2, 4, indices, 6, 4);

		SDL_UpdateTexture(SDL_tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		//Render the scene
		SDL_RenderCopy(ren, SDL_tex, NULL, NULL);
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

	window = SDL_CreateWindow("Texturing", SDL_WINDOWPOS_CENTERED,  SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		cerr << "Failed to create window\n";
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	SDL_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
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
	SDL_Scancode sc;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			sc = event.key.keysym.scancode;
			
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				return true;
			case SDL_SCANCODE_1:
				tex_index = (tex_index + 1) % NUM_TEXTURES;
				break;
			case SDL_SCANCODE_F:
				cycle_tex_filter(+1);
				break;
			case SDL_SCANCODE_D:
				cycle_tex_filter(-1);
				break;
			default:
				;
			}


			break; //sdl_keydown

		case SDL_QUIT:
			return true;
			break;
		}
	}

	//SDL_PumpEvents() is called above in SDL_PollEvent()
	const Uint8 *state = SDL_GetKeyboardState(NULL);

	static unsigned int last_time = 0, cur_time;

	cur_time = SDL_GetTicks();
	float time = (cur_time - last_time)/1000.0f;

#define MOVE_SPEED DEG_TO_RAD(30)

	int do_update = 0;
	
	if (state[SDL_SCANCODE_LEFT]) {
		for (int i=0; i<8; i+=2) {
			points[i] *= 0.99;
		}
		do_update = 1;
	}
	if (state[SDL_SCANCODE_RIGHT]) {
		for (int i=0; i<8; i+=2) {
			points[i] *= 1.01;
		}
		do_update = 1;
	}
	if (state[SDL_SCANCODE_UP]) {
		for (int i=1; i<8; i+=2) {
			points[i] *= 0.99;
		}
		do_update = 1;
	}
	if (state[SDL_SCANCODE_DOWN]) {
		for (int i=1; i<8; i+=2) {
			points[i] *= 1.01;
		}
		do_update = 1;
	}

	if (do_update) {
		for (int i=0; i<8; i+=2) {
			points_tr[i] = rsw_mapf(points[i], -1.0f, 1.0f, 0, WIDTH);
			points_tr[i+1] = rsw_mapf(points[i+1], -1.0f, 1.0f, 0, HEIGHT);
		}
	}

	last_time = cur_time;

	return false;
}

