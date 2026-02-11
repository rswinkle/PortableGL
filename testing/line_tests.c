

// TODO testing for AA lines with blending on, implement widths
// other than 1 for AA lines?
//
// Have a new test build with BETTER_THICK_LINES defined.
//
// lots of line stuff to do.

// This is specifically to tost that I correctly use the clipped
// endpoint interpolated values rather than the original values
// for clipped lines which I wasn't for a long time.
void line_interpolation(int argc, char** argv, void* data)
{
	// TODO have lines that are clipped on every plane
	// at the very least -x to x, -y to y, and -z to z
	// even if they aren't clipped across non-paired planes
	float points_n_colors[] =
	{
		-0.8, 0.9, 0.0,
		 1.0, 0.0, 0.0,
		 0.4, 0.9, 0.0,
		 0.0, 0.0, 1.0,

		-5.0, 0.7, 0.0,
		 1.0, 0.0, 0.0,
		 5.0, 0.7, 0.0,
		 0.0, 0.0, 1.0,

		-0.8, 0.4, 0.0,
		 1.0, 0.0, 0.0,
		 0.4, 0.4, 0.0,
		 0.0, 0.0, 1.0,

		-5.0, 0.2, 0.0,
		 1.0, 0.0, 0.0,
		 5.0, 0.2, 0.0,
		 0.0, 0.0, 1.0,

		-5.0, -0.2, 0.0,
		 1.0, 0.0, 0.0,
		 5.0, -0.2, 0.0,
		 0.0, 0.0, 1.0,

		-0.8, -0.4, 0.0,
		 1.0,  0.0, 0.0,
		 0.4, -0.4, 0.0,
		 0.0,  0.0, 1.0,

		-0.8, -0.9, 0.0,
		 1.0,  0.0, 0.0,
		 0.4, -0.9, 0.0,
		 0.0,  0.0, 1.0,

		-5.0, -0.7, 0.0,
		 1.0,  0.0, 0.0,
		 5.0, -0.7, 0.0,
		 0.0,  0.0, 1.0
	};

	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glEnableVertexAttribArray(PGL_ATTR_COLOR);
	GLuint lines;
	glGenBuffers(1, &lines);
	glBindBuffer(GL_ARRAY_BUFFER, lines);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points_n_colors), points_n_colors, GL_STATIC_DRAW);
	pglVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, 0);
	pglVertexAttribPointer(PGL_ATTR_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, sizeof(float)*3);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_SHADED]);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);
	SET_IDENTITY_M4(the_uniforms.mvp_mat);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_LINES, 0, 4);

	glLineWidth(8);
	glDrawArrays(GL_LINES, 4, 4);


	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1);
	glDrawArrays(GL_LINES, 8, 4);

	glLineWidth(8);
	glDrawArrays(GL_LINES, 12, 4);
}




