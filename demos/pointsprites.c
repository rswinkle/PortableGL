
#define PORTABLEGL_IMPLEMENTATION
#include <gltools.h>


#include <stdio.h>
#include <stdbool.h>


#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WIDTH 640
#define HEIGHT 480



vec4 Red = { 1.0f, 0.0f, 0.0f, 0.0f };
vec4 Green = { 0.0f, 1.0f, 0.0f, 0.0f };
vec4 Blue = { 0.0f, 0.0f, 1.0f, 0.0f };

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;

u32* bbufpix;

glContext the_Context;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	GLuint tex;
	GLuint dissolve_tex;
	float dissolve_factor;
} My_Uniforms;

void cleanup();
void setup_context();
void setup_gl_data();


void passthrough_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void point_sprites_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void tex_point_sprites_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char** argv)
{
	setup_context();

	float points[] = { -0.5, -0.5, 0,
	                    0.5, -0.5, 0,
	                    0,    0.5, 0 };

	GLuint textures[2];
	glGenTextures(2, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	if (!load_texture2D("../media/textures/test1.jpg", GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_FALSE)) {
		printf("failed to load texture\n");
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	if (!load_texture2D("../media/textures/clouds.tga", GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_FALSE)) {
		printf("failed to load texture\n");
		return 0;
	}


	My_Uniforms the_uniforms;
	the_uniforms.tex = textures[0];
	the_uniforms.dissolve_tex = textures[1];

	mat4 identity = IDENTITY_MAT4();

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*9, points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint simple_prog = pglCreateProgram(passthrough_vs, point_sprites_fs, 0, NULL, GL_FALSE);
	glUseProgram(simple_prog);
	set_uniform(&the_uniforms);

	GLuint texture_prog = pglCreateProgram(passthrough_vs, tex_point_sprites_fs, 0, NULL, GL_FALSE);
	glUseProgram(texture_prog);
	set_uniform(&the_uniforms);

	memcpy(the_uniforms.mvp_mat, identity, sizeof(mat4));


	glPointSize(100.0);


	SDL_Event e;
	bool quit = false;

	unsigned int old_time = 0, new_time=0, counter = 0;
	float diss_factor;
		
	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				quit = true;
			if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
				quit = true;
			if (e.type == SDL_MOUSEBUTTONDOWN)
				quit = true;
		}

		++counter;
		new_time = SDL_GetTicks();
		if (new_time - old_time >= 3000) {
			printf("%f FPS\n", counter*1000.0f/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}

		diss_factor = fmodf(new_time/1000.0f, 10.0f);
		diss_factor /= 10.0f;

		the_uniforms.dissolve_factor = diss_factor;
		
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(simple_prog);
		glDrawArrays(GL_POINTS, 0, 2);

		glUseProgram(texture_prog);
		glDrawArrays(GL_POINTS, 2, 1);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


void passthrough_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = ((vec4*)vertex_attribs)[0];
}


void point_sprites_simple_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 p = builtins->gl_PointCoord;

	builtins->gl_FragColor.x = p.x;
	builtins->gl_FragColor.y = p.y;
	builtins->gl_FragColor.z = 0;
	builtins->gl_FragColor.w = 1;
}

void point_sprites_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 p = builtins->gl_PointCoord;
	p.x = p.x * 2.0 - 1.0;
	p.y = p.y * 2.0 - 1.0;

	float len = dot_vec2s(p, p);
	if (len > 1.0) {
		builtins->discard = GL_TRUE;
		return;
	}

	SET_VEC4(builtins->gl_FragColor, fmodf(len*3, 1), 0, 0, 1);
}

void tex_point_sprites_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	My_Uniforms* u = uniforms;

	vec2 p_coord = builtins->gl_PointCoord;

	vec4 dissolve_color = texture2D(u->dissolve_tex, p_coord.x, p_coord.y);
	if (dissolve_color.x < u->dissolve_factor) {
		builtins->discard = GL_TRUE;
		return;
	}

	vec2 p;
	p.x = p_coord.x * 2.0 - 1.0;
	p.y = p_coord.y * 2.0 - 1.0;

	//minkowski distance
	const float power = 8.0;
	float len = powf(powf(fabsf(p.x), power) + powf(fabsf(p.y), power), 1.0f/power);
	if (len > 1.0) {
		builtins->discard = GL_TRUE;
		return;
	}
	
	builtins->gl_FragColor = texture2D(u->tex, p_coord.x, p_coord.y);
}

void setup_context()
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		printf("SDL_init error: %s\n", SDL_GetError());
		exit(0);
	}

	ren = NULL;
	tex = NULL;
	
	SDL_Window* window = SDL_CreateWindow("Pointsprites", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Failed to create window\n");
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) {
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


