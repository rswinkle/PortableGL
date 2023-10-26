
// TODO maybe I shouldn't use external libraries in tests...
// it just complicates things
#include "gltools.h"


void test_texturing(int argc, char** argv, void* data)
{
	float points[] =
	{
		-0.8,  0.8, -0.1,
		-0.8, -0.8, -0.1,
		 0.8,  0.8, -0.1,
		 0.8, -0.8, -0.1,

		-0.8,  0.8, -0.1,
		-0.8, -0.8, -0.1,
		 0.8,  0.8, -0.1,
		 0.8, -0.8, -0.1
	};

	float tex_coords[] =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		1.0, 1.0,
		0.0, 0.0,
		0.0, 511.0,
		511.0, 0.0,
		511.0, 511.0
	};

	float tex_coords2[] =
	{
		0.0, 0.0,
		0.0, 2.0,
		2.0, 0.0,
		2.0, 2.0,
		0.0, 0.0,
		0.0, 1023.0,
		1023.0, 0.0,
		1023.0, 1023.0
	};

	Color test_texture[4] =
	{
		{ 255, 255, 255, 255 },
		{ 0, 0, 0, 255 },
		{ 0, 0, 0, 255 },
		{ 255, 255, 255, 255 }
	};

	GLuint texture;
	glGenTextures(1, &texture);

	if (argc <= 4) {
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// only mag filter is actually used, no matter the size of the image on screen
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (argc != 1) ? GL_NEAREST : GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, test_texture);
	} else {
		// texture rectangle
		glBindTexture(GL_TEXTURE_RECTANGLE, texture);

		GLenum magfilter = (argc == 6) ? GL_LINEAR : GL_NEAREST;
		GLenum wrapping = GL_REPEAT;
		if (argc == 8)
			wrapping = GL_CLAMP_TO_EDGE;
		if (argc == 9)
			wrapping = GL_MIRRORED_REPEAT;


		if (!load_texture_rect("../media/textures/tex04.jpg", GL_NEAREST, magfilter, wrapping, GL_FALSE)) {
			puts("failed to load texture");
			return;
		}

	}


	GLuint square;
	glGenBuffers(1, &square);
	glBindBuffer(GL_ARRAY_BUFFER, square);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	if (argc < 2 || argc == 5 || argc == 6) {
		glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);
	} else {
		glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords2), tex_coords2, GL_STATIC_DRAW);

		if (argc == 3) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else if (argc == 4) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		}
	}
	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);

	GLuint texture_shader;
	if (argc <= 4) {
		texture_shader = std_shaders[PGL_SHADER_TEX_REPLACE];
	} else {
		texture_shader = std_shaders[PGL_SHADER_TEX_RECT_REPLACE];
	
	}
	glUseProgram(texture_shader);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);
	the_uniforms.tex0 = texture;

	glClearColor(0.25, 0.25, 0.25, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	if (argc <= 4) {
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	} else {
		glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	}

}










