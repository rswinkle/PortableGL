// Formal regression tests for FBO / RTT / MRT (Phases A–C).
// See scratch/ai_work/fbo_tests.md for descriptions and expected images.
//
// Signature matches the suite: (int num, char** argv, void* data)
// where num is test_suite[].num (path/option selector).

// Shared geometry helpers (NDC)
static float s_quad_pts[] = {
	-1.f,  1.f, 0.f,
	-1.f, -1.f, 0.f,
	 1.f,  1.f, 0.f,
	 1.f, -1.f, 0.f,
};

// UV: v=0 at bottom of image (GL-style) when using invert_y RTs correctly
static float s_quad_uv[] = {
	0.f, 1.f,
	0.f, 0.f,
	1.f, 1.f,
	1.f, 0.f,
};

// Smaller inset quad (~60% of viewport) so gray clear shows around it
static float s_inset_pts[] = {
	-0.6f,  0.6f, 0.f,
	-0.6f, -0.6f, 0.f,
	 0.6f,  0.6f, 0.f,
	 0.6f, -0.6f, 0.f,
};

static void fbo_identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	PGL_UNUSED(uniforms);
	builtins->gl_Position = vertex_attribs[0];
}

static void fbo_solid_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	builtins->gl_FragColor = *((vec4*)uniforms);
}

// Bottom half red, top half blue (fragCoord y; y=0 is bottom of FB)
static void fbo_y_band_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	PGL_UNUSED(uniforms);
	float y = builtins->gl_FragCoord.y;
	float half = (float)HEIGHT * 0.5f;
	if (y < half)
		builtins->gl_FragColor = make_v4(1.f, 0.f, 0.f, 1.f);
	else
		builtins->gl_FragColor = make_v4(0.f, 0.f, 1.f, 1.f);
}

static void fbo_mrt_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	PGL_UNUSED(uniforms);
	builtins->gl_FragData[0] = make_v4(1.f, 0.f, 0.f, 1.f);
	builtins->gl_FragData[1] = make_v4(0.f, 1.f, 0.f, 1.f);
}

typedef struct {
	GLuint tex0;
	GLuint tex1;
} fbo_mrt_uniforms;

// Left half samples tex0, right half tex1 (center texel; solid fills)
static void fbo_mrt_split_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	fbo_mrt_uniforms* u = (fbo_mrt_uniforms*)uniforms;
	float x = builtins->gl_FragCoord.x;
	// Sample center of each attachment (solid fill)
	if (x < (float)WIDTH * 0.5f)
		builtins->gl_FragColor = texture2D(u->tex0, 0.5f, 0.5f);
	else
		builtins->gl_FragColor = texture2D(u->tex1, 0.5f, 0.5f);
}

static void fbo_tex_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	// fs_input: interpolated texcoord if we use std shader path; custom for pglCreateProgram with UV
	vec2 tc = ((vec2*)fs_input)[0];
	GLuint tex = *(GLuint*)uniforms;
	builtins->gl_FragColor = texture2D(tex, tc.x, tc.y);
}

static void fbo_tex_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(uniforms);
	builtins->gl_Position = vertex_attribs[0];
	((vec2*)vs_output)[0].x = vertex_attribs[1].x;
	((vec2*)vs_output)[0].y = vertex_attribs[1].y;
}

// Allocate tightly packed RGBA8 buffer for a mapped texture
static Color* fbo_alloc_texels(int w, int h)
{
	return (Color*)calloc((size_t)w * (size_t)h, sizeof(Color));
}

static GLuint fbo_make_color_tex(Color* px, int w, int h)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	pglTextureImage2D(tex, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
	return tex;
}

static void fbo_draw_solid_fullscreen(vec4 color)
{
	GLuint prog = pglCreateProgram(fbo_identity_vs, fbo_solid_fs, 0, NULL, GL_FALSE);
	glUseProgram(prog);
	pglSetUniform(&color);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_pts), s_quad_pts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void fbo_present_tex_inset(GLuint tex)
{
	// Textured inset quad on default FB (gray should show around it)
	GLenum smooth[] = { PGL_SMOOTH, PGL_SMOOTH };
	GLuint prog = pglCreateProgram(fbo_tex_vs, fbo_tex_fs, 2, smooth, GL_FALSE);
	glUseProgram(prog);
	pglSetUniform(&tex);

	GLuint vbo[2];
	glGenBuffers(2, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(s_inset_pts), s_inset_pts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_uv), s_quad_uv, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void fbo_present_tex_fullscreen(GLuint tex)
{
	GLenum smooth[] = { PGL_SMOOTH, PGL_SMOOTH };
	GLuint prog = pglCreateProgram(fbo_tex_vs, fbo_tex_fs, 2, smooth, GL_FALSE);
	glUseProgram(prog);
	pglSetUniform(&tex);

	GLuint vbo[2];
	glGenBuffers(2, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_pts), s_quad_pts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_uv), s_quad_uv, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// ---------------------------------------------------------------------------
// test_fbo_default_safe — solid blue; FBO work must not clobber default FB
// ---------------------------------------------------------------------------
void test_fbo_default_safe(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	glClearColor(0.f, 0.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	const int tw = 128, th = 128;
	Color* px = fbo_alloc_texels(tw, th);
	GLuint tex = fbo_make_color_tex(px, tw, th);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

	glViewport(0, 0, tw, th);
	glClearColor(1.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	vec4 green = { 0.f, 1.f, 0.f, 1.f };
	fbo_draw_solid_fullscreen(green);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);
	// Do not draw — default FB must remain solid blue

	free(px);
}

// ---------------------------------------------------------------------------
// test_fbo_color — offscreen red + green triangle, sample onto gray (inset)
// ---------------------------------------------------------------------------
void test_fbo_color(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int tw = WIDTH, th = HEIGHT;
	Color* px = fbo_alloc_texels(tw, th);
	GLuint tex = fbo_make_color_tex(px, tw, th);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

	glViewport(0, 0, tw, th);
	glClearColor(0.5f, 0.f, 0.f, 1.f); // dark red
	glClear(GL_COLOR_BUFFER_BIT);

	// Green triangle centered
	float tri[] = {
		 0.f,  0.5f, 0.f,
		-0.5f, -0.4f, 0.f,
		 0.5f, -0.4f, 0.f,
	};
	GLuint prog = pglCreateProgram(fbo_identity_vs, fbo_solid_fs, 0, NULL, GL_FALSE);
	glUseProgram(prog);
	vec4 green = { 0.f, 1.f, 0.f, 1.f };
	pglSetUniform(&green);
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri), tri, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	fbo_present_tex_inset(tex);

	free(px);
}

// ---------------------------------------------------------------------------
// test_fbo_y_origin — red bottom / blue top; num=1 uses pglSetTexBackBuffer
// ---------------------------------------------------------------------------
void test_fbo_y_origin(int argc, char** argv, void* data)
{
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int tw = WIDTH, th = HEIGHT;
	Color* px = fbo_alloc_texels(tw, th);
	GLuint tex = fbo_make_color_tex(px, tw, th);

	if (argc == 1) {
		// pglSetTexBackBuffer path
		pglSetTexBackBuffer(tex);
		glViewport(0, 0, tw, th);
		GLuint prog = pglCreateProgram(fbo_identity_vs, fbo_y_band_fs, 0, NULL, GL_FALSE);
		glUseProgram(prog);
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_pts), s_quad_pts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Restore window back buffer (PGL still owns the allocation)
		pglSetBackBuffer(bbufpix, WIDTH, HEIGHT, GL_FALSE);
	} else {
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
		glViewport(0, 0, tw, th);

		GLuint prog = pglCreateProgram(fbo_identity_vs, fbo_y_band_fs, 0, NULL, GL_FALSE);
		glUseProgram(prog);
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_pts), s_quad_pts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.15f, 0.15f, 0.15f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	// Full-screen present so the half-and-half is obvious
	fbo_present_tex_fullscreen(tex);

	free(px);
}

// ---------------------------------------------------------------------------
// test_fbo_depth — overlapping tris with depth via FBO depth attach
// ---------------------------------------------------------------------------
void test_fbo_depth(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int tw = WIDTH, th = HEIGHT;
	Color* cpx = fbo_alloc_texels(tw, th);
	Color* dpx = fbo_alloc_texels(tw, th);
	GLuint ctex = fbo_make_color_tex(cpx, tw, th);
	GLuint dtex = fbo_make_color_tex(dpx, tw, th);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dtex, 0);

	glViewport(0, 0, tw, th);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Same layout idea as zbuf_test: far red, near green, another blue-ish far piece
	float points[] = {
		// far-ish red tri
		-1.f,  1.f,  0.9f,
		-1.f, -1.f,  0.9f,
		 1.f, -1.f, -0.9f,
		// far-ish blue tri
		 1.f,  1.f,  0.9f,
		-1.f, -1.f, -0.9f,
		 1.f, -1.f,  0.9f,
		// near green tri
		-0.5f, -0.5f, 0.f,
		 0.5f, -0.5f, 0.f,
		 0.f,   0.5f, 0.f,
	};

	GLuint prog = pglCreateProgram(fbo_identity_vs, fbo_solid_fs, 0, NULL, GL_FALSE);
	glUseProgram(prog);
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	vec4 red = { 1.f, 0.f, 0.f, 1.f };
	vec4 green = { 0.f, 1.f, 0.f, 1.f };
	vec4 blue = { 0.f, 0.f, 1.f, 1.f };

	pglSetUniform(&red);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	pglSetUniform(&green);
	glDrawArrays(GL_TRIANGLES, 6, 3);
	pglSetUniform(&blue);
	glDrawArrays(GL_TRIANGLES, 3, 3);

	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	fbo_present_tex_fullscreen(ctex);

	free(cpx);
	free(dpx);
}

// ---------------------------------------------------------------------------
// test_fbo_mrt — num 0: split composite; num 1: single draw buffer + gl_FragColor
// ---------------------------------------------------------------------------
void test_fbo_mrt(int argc, char** argv, void* data)
{
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int tw = WIDTH, th = HEIGHT;
	Color* px0 = fbo_alloc_texels(tw, th);
	Color* px1 = fbo_alloc_texels(tw, th);
	GLuint tex0 = fbo_make_color_tex(px0, tw, th);
	GLuint tex1 = fbo_make_color_tex(px1, tw, th);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex1, 0);

	glViewport(0, 0, tw, th);

	if (argc == 1) {
		// Single draw buffer — gl_FragColor path
		GLenum one = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &one);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		vec4 cyan = { 0.f, 1.f, 1.f, 1.f };
		fbo_draw_solid_fullscreen(cyan);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, WIDTH, HEIGHT);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		fbo_present_tex_fullscreen(tex0);
	} else {
		GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, bufs);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		GLuint prog = pglCreateProgram(fbo_identity_vs, fbo_mrt_fs, 0, NULL, GL_FALSE);
		glUseProgram(prog);
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		// Use a large triangle that covers the viewport (avoid edge holes at center)
		float cover[] = {
			-1.f, -1.f, 0.f,
			 3.f, -1.f, 0.f,
			-1.f,  3.f, 0.f,
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(cover), cover, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, WIDTH, HEIGHT);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		GLuint cprog = pglCreateProgram(fbo_identity_vs, fbo_mrt_split_fs, 0, NULL, GL_FALSE);
		glUseProgram(cprog);
		fbo_mrt_uniforms u = { tex0, tex1 };
		pglSetUniform(&u);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_pts), s_quad_pts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	free(px0);
	free(px1);
}
