
void color_masking(int argc, char** argv, void* data)
{
	float points_n_colors[] = {
		-0.5, -0.5, 0.0,
		 1.0,  0.0, 0.0,

		 0.5, -0.5, 0.0,
		 0.0,  1.0, 0.0,

		 0.0,  0.5, 0.0,
		 0.0,  0.0, 1.0 };


	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glEnableVertexAttribArray(PGL_ATTR_COLOR);
	if (!argc) {
		GLuint triangle;
		glGenBuffers(1, &triangle);
		glBindBuffer(GL_ARRAY_BUFFER, triangle);
		glBufferData(GL_ARRAY_BUFFER, sizeof(points_n_colors), points_n_colors, GL_STATIC_DRAW);
		pglVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, 0);
		pglVertexAttribPointer(PGL_ATTR_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, sizeof(float)*3);
	} else {
		glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, points_n_colors);
		glVertexAttribPointer(PGL_ATTR_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, &points_n_colors[3]);
	}

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_SHADED]);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);
	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);

	// First clear to 0's
	glClear(GL_COLOR_BUFFER_BIT);


	glClearColor(1, 1, 1, 1);
	glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// screen is yellow now (red + green)

	glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Triangle draw doesn't write red ... which ironically leaves the red but
	// not the green from the clear so instead of red - green - blue, you get
	// red - yellow - magenta
	

	/*
	//glClearColor(1, 1, 1, 1);
	//glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT);

	glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	*/


}

