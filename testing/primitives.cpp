

void primitives_test(int argc, char** argv, void* data)
{
	float points[] = {
		// triangle strip
		-0.8,  0,   0,
		-0.8, -0.8, 0,
		-0.4,  0,   0,
		-0.4, -0.8, 0,
		 0,    0,   0,
		 0,   -0.8, 0,

		// triangle fan
		 0,   0,   0,
		 0.5, 0,   0,
		 0.5, 0.5, 0,
		 0,   0.5, 0,
		-0.5, 0.5, 0,

		// lines
		-0.95, 0.95, 0,
		-0.8,  0.8,  0,
		-0.75, 0.95, 0,
		-0.6,  0.95, 0,

		// line loop
		-0.5, 0.95, 0,
		-0.4, 0.95, 0,
		-0.4, 0.85, 0,
		-0.5, 0.85, 0,

		// line strip
		-0.3, 0.85, 0,
		-0.1, 0.65, 0,
		 0.1, 0.95, 0
	};

	float color_array[] = {
		// triangle strip
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,

		// triangle fan
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,

		// lines
		0.0, 0.0, 1.0, 0.0,
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,

		// line loop
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		1.0, 0.0, 0.0, 0.0,

		// line strip
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		1.0, 0.0, 0.0, 0.0
	};




	pgl_uniforms the_uniforms;

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint colors;
	glGenBuffers(1, &colors);
	glBindBuffer(GL_ARRAY_BUFFER, colors);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color_array), color_array, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_COLOR);
	glVertexAttribPointer(PGL_ATTR_COLOR, 4, GL_FLOAT, GL_FALSE, 0, 0);


	//glClearColor(0, 0, 0, 1);
	//glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
	//
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	
	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_SHADED]);
	
	pglSetUniform(&the_uniforms);

	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 5);

	glDrawArrays(GL_LINES, 11, 4);

	glDrawArrays(GL_LINE_LOOP, 15, 4);

	glDrawArrays(GL_LINE_STRIP, 19, 3);
}









