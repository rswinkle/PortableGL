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

static void fbo_depth_vis_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tc = ((vec2*)fs_input)[0];
	GLuint tex = *(GLuint*)uniforms;
	vec4 t = texture2D(tex, tc.x, tc.y);
	builtins->gl_FragColor = make_v4(t.x, t.x, t.x, 1.f);
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

// Color-only FBO with GL_DEPTH_TEST left on (SSAO/bloom). Spec: no depth
// buffer ⇒ depth test is implicitly disabled. Scratch Z at 0 used to fail LESS.
void test_fbo_color_only_z(int argc, char** argv, void* data)
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
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "color-only complete");

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	vec4 red = { 1.f, 0.f, 0.f, 1.f };
	fbo_draw_solid_fullscreen(red);
	vec4 s = texelFetch2D(tex, tw / 2, th / 2, 0);
	PGL_EXPECT(s.x > 0.05f && s.y < 0.05f, "color-only FBO draws with depth test on");

	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	fbo_present_tex_fullscreen(tex);

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

#ifndef PGL_NO_DEPTH_NO_STENCIL
// Depth-only FBO: DrawBuffer/ReadBuffer GL_NONE, clip from depth size.
// Image: top green (window clip restored), bottom-left black (near depth
// sampled as gray), bottom-right cyan.
void test_fbo_depth_only(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int FW = WIDTH / 2;
	const int FH = HEIGHT / 2;
	float* dpx = (float*)calloc((size_t)FW * (size_t)FH, sizeof(float));
	PGL_EXPECT(dpx != NULL, "depth texel alloc");

	GLuint dtex;
	glGenTextures(1, &dtex);
	glBindTexture(GL_TEXTURE_2D, dtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	pglTextureImage2D(dtex, 0, GL_DEPTH_COMPONENT, FW, FH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, dpx);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dtex, 0);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER,
	           "depth-only default draw is INCOMPLETE_DRAW_BUFFER");

	glDrawBuffer(GL_NONE);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER,
	           "depth-only default read is INCOMPLETE_READ_BUFFER");

	glReadBuffer(GL_NONE);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "depth-only + NONE/NONE is complete");
	PGL_EXPECT(the_Context.ux == FW && the_Context.uy == FH, "depth-only clip is attachment size");
	PGL_EXPECT(the_Context.lx == 0 && the_Context.ly == 0, "depth-only clip origin");

	// Viewport still 640²; covering NDC at z=-1 (near) must clip to 320².
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT);

	GLuint prog = pglCreateProgram(fbo_identity_vs, fbo_solid_fs, 0, NULL, GL_FALSE);
	glUseProgram(prog);
	vec4 unused = { 1.f, 0.f, 0.f, 1.f };
	pglSetUniform(&unused);
	// In-NDC strip; z=-0.9 → window depth ~0.05 after PGL's [-1,1]→[0,1] map
	float cover[] = {
		-1.f,  1.f, -0.9f,
		-1.f, -1.f, -0.9f,
		 1.f,  1.f, -0.9f,
		 1.f, -1.f, -0.9f,
	};
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cover), cover, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	vec4 d = texelFetch2D(dtex, FW / 2, FH / 2, 0);
	PGL_EXPECT(d.x < 0.1f, "near depth written");

	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	PGL_EXPECT(the_Context.ux == WIDTH && the_Context.uy == HEIGHT,
	           "bind 0 restores window clip");

	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	GLenum smooth[] = { PGL_SMOOTH, PGL_SMOOTH };
	GLuint vis = pglCreateProgram(fbo_tex_vs, fbo_depth_vis_fs, 2, smooth, GL_FALSE);
	glUseProgram(vis);
	pglSetUniform(&dtex);
	GLuint qbo[2];
	glGenBuffers(2, qbo);
	glBindBuffer(GL_ARRAY_BUFFER, qbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_pts), s_quad_pts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, qbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_uv), s_quad_uv, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glViewport(0, 0, FW, FH);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(1);
	glEnable(GL_SCISSOR_TEST);
	glScissor(0, FH, WIDTH, HEIGHT - FH);
	glClearColor(0.f, 1.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glScissor(FW, 0, WIDTH - FW, FH);
	glClearColor(0.f, 1.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	free(dpx);
}
#endif

// Color blit: 320² red FBO → bottom-left of the window. Dual READ/DRAW bind.
void test_fbo_blit_color(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int FW = WIDTH / 2, FH = HEIGHT / 2;
	Color* px = fbo_alloc_texels(FW, FH);
	GLuint tex = fbo_make_color_tex(px, FW, FH);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "blit src complete");
	glClearColor(1.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	GLint draw_b = 0, read_b = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_b);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_b);
	PGL_EXPECT(draw_b == 0 && read_b == (GLint)fbo, "split DRAW 0 / READ fbo");
	PGL_EXPECT(the_Context.ux == WIDTH && the_Context.uy == HEIGHT, "draw clip is window");

	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlitFramebuffer(0, 0, FW, FH, 0, 0, FW, FH, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "color blit ok");

	free(px);
}

#ifndef PGL_NO_DEPTH_NO_STENCIL
// Depth blit: near 320² FBO depth → window BL, then a mid-z green fill.
// BL stays gray (near depth rejects); rest is green.
void test_fbo_blit_depth(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int FW = WIDTH / 2, FH = HEIGHT / 2;
	float* dpx = (float*)calloc((size_t)FW * FH, sizeof(float));
	PGL_EXPECT(dpx != NULL, "depth blit alloc");

	GLuint dtex;
	glGenTextures(1, &dtex);
	glBindTexture(GL_TEXTURE_2D, dtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	pglTextureImage2D(dtex, 0, GL_DEPTH_COMPONENT, FW, FH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, dpx);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dtex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "depth blit src complete");

	glEnable(GL_DEPTH_TEST);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT);
	GLuint prog = pglCreateProgram(fbo_identity_vs, fbo_solid_fs, 0, NULL, GL_FALSE);
	glUseProgram(prog);
	vec4 unused = { 0, 0, 0, 1 };
	pglSetUniform(&unused);
	float cover[] = {
		-1.f,  1.f, -0.9f,
		-1.f, -1.f, -0.9f,
		 1.f,  1.f, -0.9f,
		 1.f, -1.f, -0.9f,
	};
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cover), cover, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlitFramebuffer(0, 0, FW, FH, 0, 0, FW, FH, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "depth blit ok");

	vec4 green = { 0.f, 1.f, 0.f, 1.f };
	pglSetUniform(&green);
	float mid[] = {
		-1.f,  1.f, 0.f,
		-1.f, -1.f, 0.f,
		 1.f,  1.f, 0.f,
		 1.f, -1.f, 0.f,
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(mid), mid, GL_STATIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_DEPTH_TEST);

	free(dpx);
}

typedef struct {
	GLuint tex;
	vec3 dir;
} fbo_cube_vis_u;

static void fbo_cube_depth_vis_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	(void)fs_input;
	fbo_cube_vis_u* u = (fbo_cube_vis_u*)uniforms;
	vec4 t = texture_cubemap(u->tex, u->dir.x, u->dir.y, u->dir.z);
	builtins->gl_FragColor = make_v4(t.x, t.x, t.x, 1.f);
}

// Depth cubemap: +X filled near, -X left far. Sample with texture_cubemap.
// Image: top green, BL black (+X), BR white (-X).
void test_fbo_cube_depth(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int CS = WIDTH / 2;
	GLuint dtex;
	glGenTextures(1, &dtex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, dtex);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
		             CS, CS, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "depth cubemap faces ok");

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	GLuint prog = pglCreateProgram(fbo_identity_vs, fbo_solid_fs, 0, NULL, GL_FALSE);
	glUseProgram(prog);
	vec4 unused = { 0, 0, 0, 1 };
	pglSetUniform(&unused);
	float cover[] = {
		-1.f,  1.f, -0.9f,
		-1.f, -1.f, -0.9f,
		 1.f,  1.f, -0.9f,
		 1.f, -1.f, -0.9f,
	};
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cover), cover, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearDepth(1.0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
	                       GL_TEXTURE_CUBE_MAP_NEGATIVE_X, dtex, 0);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "-X face complete");
	glClear(GL_DEPTH_BUFFER_BIT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
	                       GL_TEXTURE_CUBE_MAP_POSITIVE_X, dtex, 0);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "+X face complete");
	PGL_EXPECT(the_Context.ux == CS && the_Context.uy == CS, "cube face clip");
	glClear(GL_DEPTH_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	vec4 px = texture_cubemap(dtex, 1.f, 0.f, 0.f);
	vec4 nx = texture_cubemap(dtex, -1.f, 0.f, 0.f);
	PGL_EXPECT(px.x < 0.1f, "+X sample near");
	PGL_EXPECT(nx.x > 0.9f, "-X sample far");

	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_SCISSOR_TEST);
	glScissor(CS, 0, CS, CS);
	glClearColor(1.f, 1.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	GLuint vis = pglCreateProgram(fbo_identity_vs, fbo_cube_depth_vis_fs, 0, NULL, GL_FALSE);
	glUseProgram(vis);
	fbo_cube_vis_u u = { dtex, { 1.f, 0.f, 0.f } };
	pglSetUniform(&u);
	glViewport(0, 0, CS, CS);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	u.dir = make_v3(-1.f, 0.f, 0.f);
	glViewport(CS, 0, CS, CS);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glEnable(GL_SCISSOR_TEST);
	glScissor(0, CS, WIDTH, HEIGHT - CS);
	glClearColor(0.f, 1.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
}
#endif

static int fbo_near4(vec4 c, float r, float g, float b, float a, float eps)
{
	return fabsf(c.x - r) <= eps && fabsf(c.y - g) <= eps &&
	       fabsf(c.z - b) <= eps && fabsf(c.w - a) <= eps;
}

typedef struct {
	GLuint tex;
	vec3 dir;
} fbo_cube_color_u;

static void fbo_cube_color_vis_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	(void)fs_input;
	fbo_cube_color_u* u = (fbo_cube_color_u*)uniforms;
	builtins->gl_FragColor = texture_cubemap(u->tex, u->dir.x, u->dir.y, u->dir.z);
}

// Color cubemap FBO: +X red, -X blue. Float upload + float RT checked via PGL_EXPECT.
// Image: top green, BL blit of -X (blue), BR sample of +X (red).
void test_fbo_cube_color(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	GLuint up;
	glGenTextures(1, &up);
	glBindTexture(GL_TEXTURE_CUBE_MAP, up);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	float texel[4] = { 0.25f, 0.5f, 0.75f, 1.f };
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA16F, 1, 1, 0,
	             GL_RGBA, GL_FLOAT, texel);
	for (int i = 1; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, 1, 1, 0,
		             GL_RGBA, GL_FLOAT, NULL);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "float color cube upload");
	PGL_EXPECT(fbo_near4(texture_cubemap(up, 1.f, 0.f, 0.f), 0.25f, 0.5f, 0.75f, 1.f, 0.02f),
	           "uploaded +X float sample");

	const int CS = WIDTH / 2;
	GLuint cube;
	glGenTextures(1, &cube);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, CS, CS, 0,
		             GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "U8 color cube faces ok");

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_CUBE_MAP_POSITIVE_X, cube, 0);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "+X color complete");
	PGL_EXPECT(the_Context.ux == CS && the_Context.uy == CS, "cube color clip");
	glClearColor(1.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_CUBE_MAP_NEGATIVE_X, cube, 0);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "-X color complete");
	glClearColor(0.f, 0.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// RGB565 window pix_t: FBO U8 clear/write uses PGL_RMAX (31), so sampled
	// vec4 red is ~0.12 not 1. Check dominant channel, not exact 8888.
	vec4 px = texture_cubemap(cube, 1.f, 0.f, 0.f);
	vec4 nx = texture_cubemap(cube, -1.f, 0.f, 0.f);
	PGL_EXPECT(px.x > px.y && px.x > px.z && px.x > 0.05f, "+X sample red");
	PGL_EXPECT(nx.z > nx.x && nx.z > nx.y && nx.z > 0.05f, "-X sample blue");
	vec4 py = texture_cubemap(cube, 0.f, 1.f, 0.f);
	PGL_EXPECT(py.x < 0.1f && py.y < 0.1f && py.z < 0.1f, "+Y uncleared");

	const int FSZ = 8;
	GLuint ftex;
	glGenTextures(1, &ftex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ftex);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, FSZ, FSZ, 0,
		             GL_RGBA, GL_FLOAT, NULL);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "float color cube faces ok");

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_CUBE_MAP_POSITIVE_Z, ftex, 0);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "float +Z complete");
	glClearColor(1.f, 0.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	vec4 pz = texture_cubemap(ftex, 0.f, 0.f, 1.f);
	PGL_EXPECT(pz.x > 0.9f && pz.y < 0.1f && pz.z > 0.9f, "+Z sample magenta");
	vec4 nz = texture_cubemap(ftex, 0.f, 0.f, -1.f);
	PGL_EXPECT(nz.x < 0.1f && nz.y < 0.1f && nz.z < 0.1f, "-Z uncleared");

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_CUBE_MAP_NEGATIVE_X, cube, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_SCISSOR_TEST);
	glScissor(0, CS, WIDTH, HEIGHT - CS);
	glClearColor(0.f, 1.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(0, 0, CS, CS, 0, 0, CS, CS, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "cube color blit ok");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GLuint vis = pglCreateProgram(fbo_identity_vs, fbo_cube_color_vis_fs, 0, NULL, GL_FALSE);
	glUseProgram(vis);
	fbo_cube_color_u u = { cube, { 1.f, 0.f, 0.f } };
	pglSetUniform(&u);
	float cover[] = {
		-1.f,  1.f, 0.f,
		-1.f, -1.f, 0.f,
		 1.f,  1.f, 0.f,
		 1.f, -1.f, 0.f,
	};
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cover), cover, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glViewport(CS, 0, CS, CS);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

typedef struct {
	GLuint tex;
	float lod;
} fbo_mip_lod_u;

static void fbo_mip_lod_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	(void)fs_input;
	fbo_mip_lod_u* u = (fbo_mip_lod_u*)uniforms;
	builtins->gl_FragColor = texture2DLod(u->tex, 0.5f, 0.5f, u->lod);
}

// U8 2D: L0 red, FBO-clear L1 blue. Left lod0 red, right lod1 blue.
// Float 2D/cube mip attach checked via PGL_EXPECT.
void test_fbo_mip_color(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int S0 = WIDTH / 4;
	const int S1 = S0 / 2;
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, S0, S0, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, S1, S1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "U8 2D mip TexImage");

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "L0 complete");
	glClearColor(1.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 1);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "L1 complete");
	PGL_EXPECT(the_Context.ux == S1 && the_Context.uy == S1, "L1 clip size");
	glClearColor(0.f, 0.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	vec4 l0 = texture2DLod(tex, 0.5f, 0.5f, 0.f);
	vec4 l1 = texture2DLod(tex, 0.5f, 0.5f, 1.f);
	PGL_EXPECT(l0.x > l0.y && l0.x > l0.z && l0.x > 0.05f, "L0 sample red");
	PGL_EXPECT(l1.z > l1.x && l1.z > l1.y && l1.z > 0.05f, "L1 sample blue");

	const int FSZ = 8;
	GLuint ftex;
	glGenTextures(1, &ftex);
	glBindTexture(GL_TEXTURE_2D, ftex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, FSZ, FSZ, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, FSZ / 2, FSZ / 2, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ftex, 1);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "float L1 complete");
	glClearColor(1.f, 0.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	vec4 fz = texture2DLod(ftex, 0.5f, 0.5f, 1.f);
	PGL_EXPECT(fz.x > 0.9f && fz.y < 0.1f && fz.z > 0.9f, "float L1 magenta");
	vec4 f0 = texture2DLod(ftex, 0.5f, 0.5f, 0.f);
	PGL_EXPECT(f0.x < 0.1f && f0.y < 0.1f && f0.z < 0.1f, "float L0 uncleared");

	GLuint cube;
	glGenTextures(1, &cube);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	for (int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, FSZ, FSZ, 0,
		             GL_RGBA, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 1, GL_RGBA16F, FSZ / 2, FSZ / 2, 0,
		             GL_RGBA, GL_FLOAT, NULL);
	}
	PGL_EXPECT(glGetError() == GL_NO_ERROR, "float cube mip TexImage");
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_CUBE_MAP_POSITIVE_X, cube, 1);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "cube +X L1 complete");
	glClearColor(0.f, 1.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	vec4 cx = texture_cubemapLod(cube, 1.f, 0.f, 0.f, 1.f);
	PGL_EXPECT(cx.y > 0.9f && cx.x < 0.1f && cx.z < 0.1f, "cube +X L1 green");
	vec4 nx = texture_cubemapLod(cube, -1.f, 0.f, 0.f, 1.f);
	PGL_EXPECT(nx.x < 0.1f && nx.y < 0.1f && nx.z < 0.1f, "cube -X L1 uncleared");
	vec4 cx0 = texture_cubemapLod(cube, 1.f, 0.f, 0.f, 0.f);
	PGL_EXPECT(cx0.x < 0.1f && cx0.y < 0.1f && cx0.z < 0.1f, "cube +X L0 uncleared");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	GLuint vis = pglCreateProgram(fbo_identity_vs, fbo_mip_lod_fs, 0, NULL, GL_FALSE);
	glUseProgram(vis);
	float cover[] = {
		-1.f,  1.f, 0.f,
		-1.f, -1.f, 0.f,
		 1.f,  1.f, 0.f,
		 1.f, -1.f, 0.f,
	};
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cover), cover, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	fbo_mip_lod_u u = { tex, 0.f };
	pglSetUniform(&u);
	glViewport(0, 0, WIDTH / 2, HEIGHT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	u.lod = 1.f;
	pglSetUniform(&u);
	glViewport(WIDTH / 2, 0, WIDTH / 2, HEIGHT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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

// Per-attachment ClearBufferfv / ClearNamedFramebufferfv (COLOR 0 vs 1).
void test_fbo_clear_buffer(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
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
	GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, bufs);

	float red[] = { 1.f, 0.f, 0.f, 1.f };
	float green[] = { 0.f, 1.f, 0.f, 1.f };
	glClearBufferfv(GL_COLOR, 0, red);
	glClearBufferfv(GL_COLOR, 1, green);

	vec4 c0 = texelFetch2D(tex0, tw / 2, th / 2, 0);
	vec4 c1 = texelFetch2D(tex1, tw / 2, th / 2, 0);
	PGL_EXPECT(c0.x > c0.y && c0.x > c0.z, "ClearBufferfv COLOR 0 red");
	PGL_EXPECT(c1.y > c1.x && c1.y > c1.z, "ClearBufferfv COLOR 1 green");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	float blue[] = { 0.f, 0.f, 1.f, 1.f };
	glClearNamedFramebufferfv(fbo, GL_COLOR, 0, blue);
	c0 = texelFetch2D(tex0, tw / 2, th / 2, 0);
	c1 = texelFetch2D(tex1, tw / 2, th / 2, 0);
	PGL_EXPECT(c0.z > c0.x && c0.z > c0.y, "ClearNamedFramebufferfv COLOR 0 blue");
	PGL_EXPECT(c1.y > c1.x && c1.y > c1.z, "named clear leaves COLOR 1");

#ifndef PGL_UNSAFE
	glGetError();
	glClearBufferuiv(GL_COLOR, 0, (const GLuint*)red);
	PGL_EXPECT(glGetError() == GL_INVALID_OPERATION, "ClearBufferuiv COLOR");
	GLint si = 0;
	glClearBufferiv(GL_COLOR, 0, &si);
	PGL_EXPECT(glGetError() == GL_INVALID_OPERATION, "ClearBufferiv COLOR");
#endif

	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	GLuint cprog = pglCreateProgram(fbo_identity_vs, fbo_mrt_split_fs, 0, NULL, GL_FALSE);
	glUseProgram(cprog);
	fbo_mrt_uniforms u = { tex0, tex1 };
	pglSetUniform(&u);
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad_pts), s_quad_pts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	free(px0);
	free(px1);
}
