
#include "gltools.h"

float texture_perf(int frames, int argc, char** argv, void* data)
{
	float points[] =
	{
		-1.0,  1.0, 0,
		-1.0, -1.0, 0,
		 1.0,  1.0, 0,
		 1.0, -1.0, 0
	};

	float tex_coords[] =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		1.0, 1.0,
	};

	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int filter = (argc) ? GL_LINEAR : GL_NEAREST;

	if (!load_texture2D("../media/textures/tex04.jpg", filter, filter, GL_REPEAT, false, false, NULL, NULL)) {
		puts("failed to load texture");
		return 0;
	}


	// TODO maybe use built in shader
	GLuint square, tex_buf;
	glGenBuffers(1, &square);
	glBindBuffer(GL_ARRAY_BUFFER, square);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_TEX_REPLACE]);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);
	SET_IDENTITY_M4(the_uniforms.mvp_mat);
	the_uniforms.tex0 = texture;

	int start, end, j;
	start = SDL_GetTicks();
	for (j=0; j<frames; ++j) {
		if (handle_events())
			break;

		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();


	// return FPS
	return j / ((end-start)/1000.0f);
}


void texture_replace_frame(float* fs_input, Shader_Builtins* builtins, void* uniforms);

float drawframe_tex_perf(int frames, int argc, char** argv, void* data)
{
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int filter = (argc) ? GL_LINEAR : GL_NEAREST;

	if (!load_texture2D("../media/textures/tex04.jpg", filter, filter, GL_REPEAT, true, false, NULL, NULL)) {
		puts("failed to load texture");
		return 0;
	}

	int start, end, j;
	start = SDL_GetTicks();
	for (j=0; j<frames; ++j) {
		if (handle_events())
			break;

		glClear(GL_COLOR_BUFFER_BIT);

		pglDrawFrame2(texture_replace_frame, &texture);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();


	// return FPS
	return j / ((end-start)/1000.0f);
}

/*void texture_replace_vs(float* vs_output, pgl_vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)*/
/*{*/
/*	((vec2*)vs_output)[0] = ((vec4*)vertex_attribs)[2].xy(); //tex_coords*/
/**/
/*	*(vec4*)&builtins->gl_Position = ((vec4*)vertex_attribs)[0];*/
/**/
/*}*/



float drawgeometry_tex_perf(int frames, int argc, char** argv, void* data)
{
	float points[] =
	{
		-1.0, -1.0,
		-1.0,  1.0,
		 1.0, -1.0,
		 1.0,  1.0
	};

	float points_tr[8];
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

	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int filter = (argc) ? GL_LINEAR : GL_NEAREST;

	if (!load_texture2D("../media/textures/tex04.jpg", filter, filter, GL_REPEAT, false, false, NULL, NULL)) {
		puts("failed to load texture");
		return 0;
	}

	int start, end, j;
	start = SDL_GetTicks();
	for (j=0; j<frames; ++j) {
		if (handle_events())
			break;

		glClear(GL_COLOR_BUFFER_BIT);
		pgl_draw_geometry_raw(texture, points_tr, sizeof(float)*2, colors, sizeof(pgl_Color), tex_coords, sizeof(float)*2, 4, indices, 6, 4);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();


	// return FPS
	return j / ((end-start)/1000.0f);
}

void texture_replace_frame(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	GLuint tex = *(GLuint*)uniforms;

	// TODO maybe I should just do a texrect so I can use straight fragcoord values...
	pgl_vec4 fc = builtins->gl_FragCoord;
	GLfloat u = fc.x/WIDTH;
	GLfloat v = fc.y/HEIGHT;

	builtins->gl_FragColor = texture2D(tex, u, v);
}
