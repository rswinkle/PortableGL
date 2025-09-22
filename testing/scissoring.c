
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
