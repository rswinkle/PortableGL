
void scissoring_test1(int argc, char** argv, void* data)
{
	float points[] = {
		-0.5, -0.5, -0.1,
		0.5, -0.5,  -0.1,
		0,    0.5,  -0.1,

		-0.5, -0.5, -0.3,
		0.5, -0.5,  -0.3,
		0,    0.5,  -0.3,

		-0.5, -0.5, -0.5,
		0.5, -0.5,  -0.5,
		0,    0.5,  -0.5,

		-0.5, -0.5, -0.7,
		0.5, -0.5,  -0.7,
		0,    0.5,  -0.7,

		-0.5, -0.5, -0.9,
		0.5, -0.5,  -0.9,
		0,    0.5,  -0.9,
	};

	switch (argc) {
	case 1:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 2:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	case 3:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(8);
		break;
	case 4:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		glPointSize(8);
		break;
	default:
		break;
	}

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_IDENTITY]);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	vec4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	vec4 Purple = { 1.0f, 0.0f, 1.0f, 1.0f };
	vec4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Make sure to clear entire window to prevent flickering
	// NOTE: since we're only rendering a single frame...
	//glScissor(0, 0, WIDTH, HEIGHT);
	//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Cut off all sides
	glScissor(220, 220, 200, 200);
	the_uniforms.color = Red;
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// We're drawing the identical triangles back to front and using scissoring
	// to show pieces of each to show that it takes precedence over the depth test
	// (and that it's working correctly obviously)

	//glDisable(GL_SCISSOR_TEST);
	// allow right side
	glScissor(420, 220, 500, 200);
	the_uniforms.color = Green;
	glDrawArrays(GL_TRIANGLES, 3, 3);

	// Allow bottom
	glScissor(220, 0, 200, 220);
	the_uniforms.color = Blue;
	glDrawArrays(GL_TRIANGLES, 6, 3);

	//allow left
	glScissor(0, 220, 220, 400);
	the_uniforms.color = Purple;
	glDrawArrays(GL_TRIANGLES, 9, 3);

	//allow top
	glScissor(220, 420, 200, 550);
	the_uniforms.color = Cyan;
	glDrawArrays(GL_TRIANGLES, 12, 3);
}

void scissoring_test2(int argc, char** argv, void* data)
{
	float points[] = {
		-0.5, -0.5, 0.9,
		0.5, -0.5,  0.9,
		0,    0.5,  0.9,

		-0.5, -0.5, 0.7,
		0.5, -0.5,  0.7,
		0,    0.5,  0.7,

		-0.5, -0.5, 0.5,
		0.5, -0.5,  0.5,
		0,    0.5,  0.5,

		-0.5, -0.5, 0.3,
		0.5, -0.5,  0.3,
		0,    0.5,  0.3,

		-0.5, -0.5, 0.1,
		0.5, -0.5,  0.1,
		0,    0.5,  0.1,
	};

	switch (argc) {
	case 1:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 2:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	case 3:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(8);
		break;
	case 4:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		glPointSize(8);
		break;
	default:
		break;
	}

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_IDENTITY]);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	vec4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	vec4 Purple = { 1.0f, 0.0f, 1.0f, 1.0f };
	vec4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_DEPTH_TEST);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Make sure to clear entire window to prevent flickering
	// NOTE: since we're only rendering a single frame...
	//glScissor(0, 0, WIDTH, HEIGHT);
	//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// only top
	glScissor(0, 420, 640, 550);
	the_uniforms.color = Red;
	glDrawArrays(GL_TRIANGLES, 0, 3);

	//glDisable(GL_SCISSOR_TEST);
	// only right side
	glScissor(420, 0, 500, 640);
	the_uniforms.color = Green;
	glDrawArrays(GL_TRIANGLES, 3, 3);

	// only bottom
	glScissor(0, 0, 640, 220);
	the_uniforms.color = Blue;
	glDrawArrays(GL_TRIANGLES, 6, 3);

	// only left
	glScissor(0, 0, 220, 640);
	the_uniforms.color = Purple;
	glDrawArrays(GL_TRIANGLES, 9, 3);

	// cut off all sides
	glScissor(220, 220, 200, 200);
	the_uniforms.color = Cyan;
	glDrawArrays(GL_TRIANGLES, 12, 3);
}

void scissoring_test3(int argc, char** argv, void* data)
{
	float points[] = { -0.5, -0.5, 0,
	                    0.5, -0.5, 0,
	                    0,    0.5, 0 };

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_IDENTITY]);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };

	glEnable(GL_SCISSOR_TEST);
	// TODO test for depth/stencil buffers too

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	the_uniforms.color = Red;
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glScissor(WIDTH/2, 0, WIDTH/2, HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_SCISSOR_TEST);
	glScissor(0, HEIGHT/2, WIDTH, HEIGHT/2);
	glEnable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
}

// Test scissor with GL_LINES and GL_POINTS
void scissoring_test4(int argc, char** argv, void* data)
{
	float points_n_lines[] = {
		// test -x and +y
		-1.1, 0.6, 0,
		-0.6, 1.1, 0,

		// +x and -y
		 1.1, -0.6, 0,
		 0.6, -1.1, 0,

		// more clipping
		-1, 0.9, 0,
		1, -0.9, 0,

		// points below
		 -0.9, 0.5, 0,
		  0.9, 0.5, 0,

		 -1.02, -0.5, 0,
		  1.02, -0.5, 0
	};

	switch (argc) {
		case 1:
			glPointSize(8);
			glLineWidth(8);
			break;
		case 2:
			glPointSize(32);
			glLineWidth(32);
			break;
		default:
			break;
	}


	GLuint verts;
	glGenBuffers(1, &verts);
	glBindBuffer(GL_ARRAY_BUFFER, verts);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points_n_lines), points_n_lines, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	// Don't need a shader or uniform, just using the default shader 0
	// which is just a passthrough vs, draw everything red fs
	glEnable(GL_SCISSOR_TEST);
	glScissor(WIDTH/20.0f, HEIGHT/20.0f, 9*WIDTH/10.0f, 9*HEIGHT/10.0f);

	glDrawArrays(GL_LINES, 0, 6);
	glDrawArrays(GL_POINTS, 6, 4);
}

// FBO half the window: clip follows the bound surface, not leftover window
// lx/ux. Viewport stays 640² so a covering NDC triangle would overflow the
// 320² attachment if clip weren't refreshed. After bind 0 the top half is
// drawable again. glScissor without GL_SCISSOR_TEST does not clip.
//
// Image:
//   top half     = green  (window clip restored)
//   bottom-left  = red    (small FBO presented)
//   bottom-right = cyan   (scissor box is 8×8 but the test is off)
void scissoring_fbo_clip(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int FW = WIDTH / 2;
	const int FH = HEIGHT / 2;

	Color* px = (Color*)calloc((size_t)FW * (size_t)FH, sizeof(Color));
	PGL_EXPECT(px != NULL, "FBO texel alloc");

	GLuint ctex;
	glGenTextures(1, &ctex);
	glBindTexture(GL_TEXTURE_2D, ctex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	pglTextureImage2D(ctex, 0, GL_RGBA, FW, FH, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex, 0);
	PGL_EXPECT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
	           "FBO complete");
	PGL_EXPECT(the_Context.ux == FW && the_Context.uy == FH, "FBO clip is attachment size");
	PGL_EXPECT(the_Context.lx == 0 && the_Context.ly == 0, "FBO clip origin");

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_IDENTITY]);

	pgl_uniforms u;
	memset(&u, 0, sizeof(u));
	pglSetUniform(&u);

	float cover[] = { -1.f, -1.f, 0.f,  3.f, -1.f, 0.f,  -1.f, 3.f, 0.f };
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cover), cover, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Viewport is still the window. Covering NDC would generate 640² fragments;
	// clip must keep writes inside the 320² RT (clear + draw).
	glClearColor(1.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	u.color = make_v4(1.f, 0.f, 0.f, 1.f);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glScissor(0, 0, 8, 8);
	PGL_EXPECT(the_Context.ux == FW && the_Context.uy == FH,
	           "glScissor without test does not clip FBO");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	PGL_EXPECT(the_Context.ux == WIDTH && the_Context.uy == HEIGHT,
	           "bind 0 restores window clip");

	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.25f, 0.25f, 0.25f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	float quad[] = {
		-1.f,  1.f, 0.f,
		-1.f, -1.f, 0.f,
		 1.f,  1.f, 0.f,
		 1.f, -1.f, 0.f,
	};
	float uv[] = {
		0.f, 1.f,
		0.f, 0.f,
		1.f, 1.f,
		1.f, 0.f,
	};
	GLuint qbo, tbo;
	glGenBuffers(1, &qbo);
	glBindBuffer(GL_ARRAY_BUFFER, qbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &tbo);
	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glUseProgram(std_shaders[PGL_SHADER_TEX_REPLACE]);
	SET_IDENTITY_M4(u.mvp_mat);
	u.tex0 = ctex;
	pglSetUniform(&u);
	glViewport(0, 0, FW, FH);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Top half green via scissor+clear. If clip stuck at 320, this box is
	// outside lx/ux and the top stays gray.
	glEnable(GL_SCISSOR_TEST);
	glScissor(0, FH, WIDTH, HEIGHT - FH);
	glClearColor(0.f, 1.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Bottom-right cyan. Same: empty if window clip was not restored.
	glScissor(FW, 0, WIDTH - FW, FH);
	glClearColor(0.f, 1.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);

	// glScissor without the test must not shrink the window clip.
	glScissor(0, 0, 8, 8);
	PGL_EXPECT(the_Context.ux == WIDTH && the_Context.uy == HEIGHT,
	           "glScissor without test does not clip window");

	free(px);
}
