// Mipmap / LOD suite tests for run_tests (640×640 default FB PNG compare).
// See notes/mipmap_tests.md.
//
// Signature: void name(int num, char** argv, void* data) — num is suite selector.

#include <math.h>
#include <stdio.h>
#include <string.h>

// --- helpers ------------------------------------------------------------------

static int mip_fails;
static int mip_near_color(vec4 c, float r, float g, float b, float eps)
{
	return fabsf(c.x - r) < eps && fabsf(c.y - g) < eps && fabsf(c.z - b) < eps;
}

#define MIP_EXPECT(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "mipmap FAIL: %s\n", msg); \
		mip_fails++; \
	} \
} while (0)

static void mip_fill_solid(Color* px, int n, u8 r, u8 g, u8 b)
{
	for (int i = 0; i < n; i++) {
		px[i].r = r; px[i].g = g; px[i].b = b; px[i].a = 255;
	}
}

static GLuint mip_make_rgb8_chain(void)
{
	Color l0[64], l1[16], l2[4], l3[1];
	mip_fill_solid(l0, 64, 255, 0, 0);
	mip_fill_solid(l1, 16, 0, 255, 0);
	mip_fill_solid(l2, 4, 0, 0, 255);
	mip_fill_solid(l3, 1, 255, 255, 255);

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, l0);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, l1);
	glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, l2);
	glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, l3);
	return tex;
}

static void mip_draw_tex_quad(float x0, float y0, float x1, float y1, GLuint tex)
{
	float points[] = {
		x0, y1, -0.1f,
		x0, y0, -0.1f,
		x1, y1, -0.1f,
		x1, y0, -0.1f,
	};
	float uvs[] = {
		0.f, 1.f,
		0.f, 0.f,
		1.f, 1.f,
		1.f, 0.f,
	};

	GLuint vbo, tbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &tbo);
	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(shaders);
	glUseProgram(shaders[PGL_SHADER_TEX_REPLACE]);

	pgl_uniforms u;
	pglSetUniform(&u);
	SET_IDENTITY_M4(u.mvp_mat);
	u.tex0 = tex;

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &tbo);
}

// FS: sample texture2DLod; lod from uniform
typedef struct {
	GLuint tex;
	float lod;
} mip_lod_uniforms;

static void mip_ident_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(uniforms);
	((vec2*)vs_output)[0] = make_v2(vertex_attribs[2].x, vertex_attribs[2].y);
	builtins->gl_Position = vertex_attribs[0];
}

static void mip_lod_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	mip_lod_uniforms* u = (mip_lod_uniforms*)uniforms;
	float s = fs_input[0], t = fs_input[1];
	builtins->gl_FragColor = texture2DLod(u->tex, s, t, u->lod);
}

static void mip_grad_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	// Fixed high minify ρ → L2 blue for 8×8 base (dUdx=0.5 → ρ=4 → λ=2)
	mip_lod_uniforms* u = (mip_lod_uniforms*)uniforms;
	float s = fs_input[0], t = fs_input[1];
	builtins->gl_FragColor = texture2DGrad(u->tex, s, t, 0.5f, 0.f, 0.f, 0.f);
}

static void mip_draw_fullscreen_lod(GLuint tex, float lod, frag_func fs)
{
	float points[] = {
		-1.f,  1.f, 0.f,
		-1.f, -1.f, 0.f,
		 1.f,  1.f, 0.f,
		 1.f, -1.f, 0.f,
	};
	float uvs[] = {
		0.f, 1.f,
		0.f, 0.f,
		1.f, 1.f,
		1.f, 0.f,
	};

	GLuint vbo, tbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &tbo);
	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLenum smooth[2] = { PGL_SMOOTH2 };
	GLuint prog = pglCreateProgram(mip_ident_vs, fs, 2, smooth, GL_FALSE);
	glUseProgram(prog);

	mip_lod_uniforms u;
	u.tex = tex;
	u.lod = lod;
	pglSetUniform(&u);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &tbo);
	// programs leak is fine for one-shot tests
}

// --- suite entries ------------------------------------------------------------

// num 0: pure logic (Grad, helpers, mag/min, cube, size); green fill if OK else red
void test_mipmap_unit(int num, char** argv, void* data)
{
	PGL_UNUSED(num);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	mip_fails = 0;
	vec4 c;
	float lam;

	GLuint tex = mip_make_rgb8_chain();

	// pgl_lod_*
	lam = pgl_lod_screen_wh(tex, 8.f, 8.f);
	MIP_EXPECT(fabsf(lam) < 0.05f, "pgl_lod_screen_wh same size λ≈0");
	lam = pgl_lod_screen_wh(tex, 2.f, 2.f);
	MIP_EXPECT(fabsf(lam - 2.f) < 0.05f, "pgl_lod_screen_wh 8/2 λ≈2");
	lam = pgl_lod_uv_scale_wh(tex, 2.f, 8.f, 8.f);
	MIP_EXPECT(fabsf(lam - 1.f) < 0.05f, "pgl_lod_uv_scale ×2 → λ≈1");
	// back buffer is WIDTH×HEIGHT (640); 8/640 → λ ≈ log2(0.0125) ≈ -6.3
	lam = pgl_lod_screen(tex);
	MIP_EXPECT(lam < -4.f && lam > -8.f, "pgl_lod_screen uses back_buffer");

	lam = pgl_lod_grad(tex, 0.5f, 0.f, 0.f, 0.f);
	MIP_EXPECT(fabsf(lam - 2.f) < 0.05f, "pgl_lod_grad ρ=4 → λ≈2");
	c = texture2DGrad(tex, 0.5f, 0.5f, 0.5f, 0.f, 0.f, 0.f);
	MIP_EXPECT(mip_near_color(c, 0.f, 0.f, 1.f, 0.05f), "texture2DGrad → L2 blue");
	c = texture2DGrad(tex, 0.5f, 0.5f, 1e-4f, 0.f, 0.f, 1e-4f);
	MIP_EXPECT(mip_near_color(c, 1.f, 0.f, 0.f, 0.05f), "texture2DGrad tiny → L0 red mag");

	// Lod level pick + λ≤0 mag
	c = texture2DLod(tex, 0.5f, 0.5f, 0.f);
	MIP_EXPECT(mip_near_color(c, 1.f, 0.f, 0.f, 0.05f), "Lod 0 red mag");
	c = texture2DLod(tex, 0.5f, 0.5f, 1.f);
	MIP_EXPECT(mip_near_color(c, 0.f, 1.f, 0.f, 0.05f), "Lod 1 green");
	c = texture2DLod(tex, 0.5f, 0.5f, 0.6f);
	MIP_EXPECT(mip_near_color(c, 0.f, 1.f, 0.f, 0.05f), "Lod 0.6 rounds L1");
	c = texture2DLod(tex, 0.5f, 0.5f, -2.f);
	MIP_EXPECT(mip_near_color(c, 1.f, 0.f, 0.f, 0.05f), "negative Lod mag L0");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	c = texture2DLod(tex, 0.5f, 0.5f, 0.9f);
	MIP_EXPECT(mip_near_color(c, 0.1f, 0.9f, 0.f, 0.08f), "trilinear 0.9 blend");
	c = texture2DLod(tex, 0.5f, 0.5f, 0.f);
	MIP_EXPECT(mip_near_color(c, 1.f, 0.f, 0.f, 0.05f), "trilinear lod 0 pure mag");

	// Mag vs min on 2×2 checker
	Color checker[4] = {
		{255, 0, 0, 255}, {0, 255, 0, 255},
		{0, 0, 255, 255}, {0, 0, 0, 255}
	};
	Color l1g[1] = {{128, 128, 128, 255}};
	GLuint chk;
	glGenTextures(1, &chk);
	glBindTexture(GL_TEXTURE_2D, chk);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, checker);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, l1g);

	c = texture2DLod(chk, 0.5f, 0.5f, 0.f);
	int pure = (c.x > 0.9f && c.y < 0.1f && c.z < 0.1f) ||
	           (c.y > 0.9f && c.x < 0.1f && c.z < 0.1f) ||
	           (c.z > 0.9f && c.x < 0.1f && c.y < 0.1f) ||
	           (c.x < 0.1f && c.y < 0.1f && c.z < 0.1f);
	MIP_EXPECT(pure, "lod 0 MAG NEAREST pure");
	c = texture2DLod(chk, 0.5f, 0.5f, 0.4f);
	int blended = (c.x > 0.05f && c.y > 0.05f) || (c.x > 0.05f && c.z > 0.05f) ||
	              (c.y > 0.05f && c.z > 0.05f);
	MIP_EXPECT(blended, "lod 0.4 MIN LINEAR blend");

	// 1D Grad
	Color row0[4], row1[2], row2[1];
	mip_fill_solid(row0, 4, 255, 0, 0);
	mip_fill_solid(row1, 2, 0, 255, 0);
	mip_fill_solid(row2, 1, 0, 0, 255);
	GLuint t1d;
	glGenTextures(1, &t1d);
	glBindTexture(GL_TEXTURE_1D, t1d);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, row0);
	glTexImage1D(GL_TEXTURE_1D, 1, GL_RGBA, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, row1);
	glTexImage1D(GL_TEXTURE_1D, 2, GL_RGBA, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, row2);
	c = texture1DGrad(t1d, 0.25f, 0.5f, 0.f);
	MIP_EXPECT(mip_near_color(c, 0.f, 1.f, 0.f, 0.05f), "1DGrad L1 green");

	// Cubemap generate + Lod
	Color face[16];
	mip_fill_solid(face, 16, 255, 0, 0);
	static const GLenum faces[6] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};
	GLuint cube;
	glGenTextures(1, &cube);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	for (int i = 0; i < 6; i++)
		glTexImage2D(faces[i], 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, face);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	MIP_EXPECT(glGetError() == GL_NO_ERROR, "cube GenerateMipmap");
	const glTexture* ct = pglGetTexture(cube);
	MIP_EXPECT(ct && ct->num_levels == 3, "cube 3 levels");
	c = texture_cubemapLod(cube, 1.f, 0.f, 0.f, 0.f);
	MIP_EXPECT(mip_near_color(c, 1.f, 0.f, 0.f, 0.05f), "cubemapLod 0 red");
	c = texture_cubemapGrad(cube, 1.f, 0.f, 0.f, 1e-5f, 0, 0, 0, 1e-5f, 0);
	MIP_EXPECT(mip_near_color(c, 1.f, 0.f, 0.f, 0.05f), "cubemapGrad red");

	// textureSize non-zero; name 0 invalid (debug)
	ivec3 sz = textureSize(tex, 1);
	MIP_EXPECT(sz.x == 4 && sz.y == 4, "textureSize lod1 4×4");
#ifndef PGL_UNSAFE
	glGetError();
	textureSize(0, 0);
	MIP_EXPECT(glGetError() == GL_INVALID_VALUE, "textureSize(0) INVALID_VALUE");
	glGetError();
	pgl_lod_screen(0);
	MIP_EXPECT(glGetError() == GL_INVALID_VALUE, "pgl_lod_screen(0) INVALID_VALUE");
#endif

	// Phase1-ish: chain layout after generate on a fresh 4×4
	Color red16[16];
	mip_fill_solid(red16, 16, 255, 0, 0);
	GLuint t2;
	glGenTextures(1, &t2);
	glBindTexture(GL_TEXTURE_2D, t2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, red16);
	glGenerateMipmap(GL_TEXTURE_2D);
	ct = pglGetTexture(t2);
	MIP_EXPECT(ct && ct->num_levels == 3, "generate 4×4 → 3 levels");
	MIP_EXPECT(ct->levels[1].w == 2 && ct->levels[1].h == 2, "L1 2×2");
	MIP_EXPECT(ct->data_alloc == (4 * 4 + 2 * 2 + 1) * 4, "chain alloc size");

	glDeleteTextures(1, &tex);
	glDeleteTextures(1, &chk);
	glDeleteTextures(1, &t1d);
	glDeleteTextures(1, &cube);
	glDeleteTextures(1, &t2);

	// Encode pass/fail on the default FB (suite PNG compare)
	if (mip_fails == 0)
		glClearColor(0.f, 0.55f, 0.1f, 1.f); // green = all unit checks passed
	else
		glClearColor(0.8f, 0.05f, 0.05f, 1.f); // red = see stderr
	glClear(GL_COLOR_BUFFER_BIT);
}

// num 1: visual auto LOD — full-screen L0 red, then tiny center L2 blue on dark gray
void test_mipmap_auto_vis(int num, char** argv, void* data)
{
	PGL_UNUSED(num);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	GLuint tex = mip_make_rgb8_chain();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

	// Left half: large UV scale (half of FB width) → L0 red
	// Right half: tiny quads stacked → high λ → blue (L2)
	glClearColor(0.12f, 0.12f, 0.12f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Left large quad: NDC x -1..0, full height
	mip_draw_tex_quad(-1.f, -1.f, 0.f, 1.f, tex);

	// Tiny quad ~2 px near center-right: UV 0–1 over ~2 px → ρ≈8*(1/2)=4 → λ≈2 → L2 blue
	// (NDC half-extent: n_pixels / WIDTH maps to ≈ n pixels full width = 2*n/WIDTH NDC)
	float px = 2.f / (float)WIDTH;
	float py = 2.f / (float)HEIGHT;
	float cx = 0.5f, cy = 0.f;
	mip_draw_tex_quad(cx - px, cy - py, cx + px, cy + py, tex);

	glDeleteTextures(1, &tex);
}

// num 2: vertical bands Lod 0 / 1 / 2 → red | green | blue
void test_mipmap_lod_bands(int num, char** argv, void* data)
{
	PGL_UNUSED(num);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	GLuint tex = mip_make_rgb8_chain();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Three vertical strips via scissor + fullscreen texture2DLod
	int w3 = WIDTH / 3;
	for (int band = 0; band < 3; band++) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(band * w3, 0, (band == 2) ? (WIDTH - 2 * w3) : w3, HEIGHT);
		mip_draw_fullscreen_lod(tex, (float)band, mip_lod_fs);
		glDisable(GL_SCISSOR_TEST);
	}

	glDeleteTextures(1, &tex);
}

// num 3: full-screen texture2DGrad with fixed high ρ → solid blue (L2)
void test_mipmap_grad_vis(int num, char** argv, void* data)
{
	PGL_UNUSED(num);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	GLuint tex = mip_make_rgb8_chain();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	mip_draw_fullscreen_lod(tex, 0.f, mip_grad_fs);
	glDeleteTextures(1, &tex);
}
