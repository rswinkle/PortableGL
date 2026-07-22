
// TODO maybe I shouldn't use external libraries in tests...
// it just complicates things
#include "gltools.h"


void test_tex2D_filtering(int argc, char** argv, void* data)
{
	float points[] =
	{
		-0.8,  0.8, -0.1,
		-0.8, -0.8, -0.1,
		 0.8,  0.8, -0.1,
		 0.8, -0.8, -0.1,
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

	SET_IDENTITY_M4(the_uniforms.mvp_mat);
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

	SET_IDENTITY_M4(the_uniforms.mvp_mat);
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
	};

	// TODO test wrapping on all sides not just positive 
	float tex_coords[] =
	{
		-1.0, -1.0,
		-1.0, 2.0,
		2.0, -1.0,
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

	SET_IDENTITY_M4(the_uniforms.mvp_mat);
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
	};

	float tex_coords[] =
	{
		-512.0, -512.0,
		-512.0, 1024.0,
		1024.0, -512.0,
		1024.0, 1024.0,
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

	SET_IDENTITY_M4(the_uniforms.mvp_mat);
	the_uniforms.tex0 = texture;

	glClearColor(0.25, 0.25, 0.25, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Solid-color mip chain for visual LOD checks (L0 red, L1 green, L2 blue, ...)
static void tex2d_upload_solid_mip_chain(int base_dim)
{
	static const Color level_colors[] = {
		{ 255,   0,   0, 255 }, // L0 red
		{   0, 255,   0, 255 }, // L1 green
		{   0,   0, 255, 255 }, // L2 blue
		{ 255, 255,   0, 255 }, // L3 yellow
		{ 255,   0, 255, 255 }, // L4 magenta
		{   0, 255, 255, 255 }, // L5 cyan
		{ 255, 255, 255, 255 }, // L6 white
		{ 128, 128, 128, 255 }, // L7 gray
	};
	int ncolors = (int)(sizeof(level_colors) / sizeof(level_colors[0]));

	int w = base_dim;
	int h = base_dim;
	int level = 0;
	while (1) {
		int n = w * h;
		Color* buf = (Color*)malloc((size_t)n * sizeof(Color));
		Color c = level_colors[level < ncolors ? level : ncolors - 1];
		for (int i = 0; i < n; ++i)
			buf[i] = c;

		glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
		free(buf);

		if (w == 1 && h == 1)
			break;
		if (w > 1) w /= 2;
		if (h > 1) h /= 2;
		++level;
	}
}

static void tex2d_draw_uv_quad(float x0, float y0, float x1, float y1)
{
	float points[] = {
		x0, y1, -0.1f,
		x0, y0, -0.1f,
		x1, y1, -0.1f,
		x1, y0, -0.1f,
	};
	float tex_coords[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};

	// Client arrays (same idea as other tests that pass argc for client paths)
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, points);
	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, tex_coords);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// argc:
//   0 NEAREST_MIPMAP_NEAREST  — solid color levels, multi-scale quads (auto LOD)
//   1 LINEAR_MIPMAP_NEAREST
//   2 NEAREST_MIPMAP_LINEAR
//   3 LINEAR_MIPMAP_LINEAR
//   4 GenerateMipmap from 8x8 checkerboard, same multi-scale quads
//
// Three quads on a 640x640 framebuffer:
//   large  (~256px)  → low λ  → L0 (red or checker)
//   medium (~8px)    → mid λ  → higher solid level / blurred checker
//   tiny   (~2.5px)  → high λ → highest solid level / gray average
void test_tex2D_mipmaps(int argc, char** argv, void* data)
{
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	GLenum min_filter;
	switch (argc) {
	case 0: min_filter = GL_NEAREST_MIPMAP_NEAREST; break;
	case 1: min_filter = GL_LINEAR_MIPMAP_NEAREST;  break;
	case 2: min_filter = GL_NEAREST_MIPMAP_LINEAR;  break;
	case 3: min_filter = GL_LINEAR_MIPMAP_LINEAR;   break;
	default: min_filter = GL_NEAREST_MIPMAP_NEAREST; break;
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	// NEAREST mag keeps solid-color levels clean for argc 0-3
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (argc == 4) {
		// Checkerboard L0 + glGenerateMipmap
		Color checker[64];
		Color white = { 255, 255, 255, 255 };
		Color black = { 0, 0, 0, 255 };
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				checker[y * 8 + x] = ((x ^ y) & 1) ? white : black;
			}
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, checker);
		glGenerateMipmap(GL_TEXTURE_2D);
	} else {
		tex2d_upload_solid_mip_chain(16);
	}

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	GLuint texture_shader = std_shaders[PGL_SHADER_TEX_REPLACE];
	glUseProgram(texture_shader);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);
	SET_IDENTITY_M4(the_uniforms.mvp_mat);
	the_uniforms.tex0 = texture;

	glClearColor(0.25, 0.25, 0.25, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	// large ~256 px wide (λ < 0 → L0)
	tex2d_draw_uv_quad(-0.90f, -0.85f, -0.10f, 0.85f);
	// medium ~8 px (λ ~ 1 for 16-base solid chain)
	tex2d_draw_uv_quad(0.20f, -0.0125f, 0.245f, 0.0125f);
	// tiny ~2.5 px (λ ~ 2–3)
	tex2d_draw_uv_quad(0.55f, -0.004f, 0.558f, 0.004f);

	glDeleteTextures(1, &texture);
}



