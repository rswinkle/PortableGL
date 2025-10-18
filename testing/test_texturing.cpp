
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

	if (argc <= 5) {
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// only mag filter is actually used, no matter the size of the image on screen
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (argc != 1) ? GL_NEAREST : GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, test_texture);
	} else {
		// texture rectangle
		glBindTexture(GL_TEXTURE_RECTANGLE, texture);

		GLenum magfilter = (argc == 7) ? GL_LINEAR : GL_NEAREST;
		GLenum wrapping = GL_REPEAT;
		if (argc == 9)
			wrapping = GL_CLAMP_TO_EDGE;
		if (argc == 10)
			wrapping = GL_MIRRORED_REPEAT;
		if (argc == 11)
			wrapping = GL_CLAMP_TO_BORDER;

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
	if (argc < 2 || argc == 6 || argc == 7) {
		glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);
	} else {
		glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords2), tex_coords2, GL_STATIC_DRAW);

		if (argc == 3) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else if (argc == 4) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		} else if (argc == 5) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			GLfloat green[4] = { 0.0, 1.0, 0.0, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&green);
		}
	}
	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);

	GLuint texture_shader;
	if (argc <= 5) {
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

	if (argc <= 5) {
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	} else {
		glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	}

}

void test_tex2D_filtering(int argc, char** argv, void* data)
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

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// only mag filter is actually used, no matter the size of the image on screen
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (!argc) ? GL_NEAREST : GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, test_texture);

	GLuint square;
	glGenBuffers(1, &square);
	glBindBuffer(GL_ARRAY_BUFFER, square);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);

	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);

	GLuint texture_shader = std_shaders[PGL_SHADER_TEX_REPLACE];
	glUseProgram(texture_shader);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);
	the_uniforms.tex0 = texture;

	glClearColor(0.25, 0.25, 0.25, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void test_texrect_filtering(int argc, char** argv, void* data)
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
		0.0, 511.0,
		511.0, 0.0,
		511.0, 511.0
	};

	GLuint texture;
	glGenTextures(1, &texture);

	// texture rectangle
	glBindTexture(GL_TEXTURE_RECTANGLE, texture);

	GLenum magfilter = (argc) ? GL_LINEAR : GL_NEAREST;
	GLenum wrapping = GL_REPEAT; // doesn't really matter, not tested in this test
	if (!load_texture_rect("../media/textures/tex04.jpg", GL_NEAREST, magfilter, wrapping, GL_FALSE)) {
		puts("failed to load texture");
		return;
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);

	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);

	GLuint texture_shader = std_shaders[PGL_SHADER_TEX_RECT_REPLACE];
	glUseProgram(texture_shader);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);
	the_uniforms.tex0 = texture;

	glClearColor(0.25, 0.25, 0.25, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void test_tex2D_wrap_modes(int argc, char** argv, void* data)
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

	// TODO test wrapping on all sides not just positive 
	float tex_coords[] =
	{
		0.0, 0.0,
		0.0, 2.0,
		2.0, 0.0,
		2.0, 2.0,
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

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// only mag filter is actually used, no matter the size of the image on screen
	// TODO LINEAR is actually the deafult mag_filter, nearest for min_filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (!argc) {
		// REPEAT is the default so this isn't actually necessary
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	} else if (argc == 1) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else if (argc == 2) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	} else if (argc == 3) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		GLfloat green[4] = { 0.0, 1.0, 0.0, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&green);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, test_texture);

	GLuint square;
	glGenBuffers(1, &square);
	glBindBuffer(GL_ARRAY_BUFFER, square);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);

	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);

	GLuint texture_shader = std_shaders[PGL_SHADER_TEX_REPLACE];
	glUseProgram(texture_shader);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);
	the_uniforms.tex0 = texture;

	glClearColor(0.25, 0.25, 0.25, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void test_texrect_wrap_modes(int argc, char** argv, void* data)
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
		0.0, 1023.0,
		1023.0, 0.0,
		1023.0, 1023.0
	};

	GLuint texture;
	glGenTextures(1, &texture);

	// texture rectangle
	glBindTexture(GL_TEXTURE_RECTANGLE, texture);

	GLenum wrapping;
	switch (argc) {
	case 0: wrapping = GL_REPEAT; break;
	case 1: wrapping = GL_CLAMP_TO_EDGE; break;
	case 2: wrapping = GL_MIRRORED_REPEAT; break;
	case 3: wrapping = GL_CLAMP_TO_BORDER; break;
	}

	// LINEAR is default for both with tex rects
	if (!load_texture_rect("../media/textures/tex04.jpg", GL_NEAREST, GL_NEAREST, wrapping, GL_FALSE)) {
		puts("failed to load texture");
		return;
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);

	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);

	GLuint texture_shader = std_shaders[PGL_SHADER_TEX_RECT_REPLACE];
	glUseProgram(texture_shader);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);
	the_uniforms.tex0 = texture;

	glClearColor(0.25, 0.25, 0.25, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}






