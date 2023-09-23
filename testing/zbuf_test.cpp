

void zbuf_test(int argc, char** argv, void* data)
{
	vec4 Red = { 1.0f, 0.0f, 0.0f, 0.0f };
	vec4 Green = { 0.0f, 1.0f, 0.0f, 0.0f };
	vec4 Blue = { 0.0f, 0.0f, 1.0f, 0.0f };

	float points[] = {
		-1, 1, 0.9,
		-1, -1, 0.9,
		 1, -1, -0.9,

		 1, 1, 0.9,
		-1, -1, -0.9,
		 1, -1, 0.9,

		-0.5, -0.5, 0,
		 0.5, -0.5, 0,
		 0, 0.5, 0,
	};


	pgl_uniforms the_uniforms;

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_IDENTITY]);

	pglSetUniform(&the_uniforms);

	if (argc) {
		glEnable(GL_DEPTH_TEST);
	}
	if (argc == 2) {
		glClearDepth(0);
		glDepthFunc(GL_GREATER);
	} else if (argc == 3) {
		glDepthRange(1, 0);
	}


	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	the_uniforms.v_color = Red;
	glDrawArrays(GL_TRIANGLES, 0, 3);

	if (argc == 4) {
		glDepthMask(GL_FALSE);
	}
	the_uniforms.v_color = Green;
	glDrawArrays(GL_TRIANGLES, 6, 3);
	if (argc == 4) {
		glDepthMask(GL_TRUE);
	}

	the_uniforms.v_color = Blue;
	glDrawArrays(GL_TRIANGLES, 3, 3);

}









