

void stencil_test(int argc, char** argv, void* data)
{
	float points[] =
	{
		-0.5, -0.5, 0,
		 0.5, -0.5, 0,
		 0,    0.5, 0
	};

	float color_array[] =
	{
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	};

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

	pgl_uniforms the_uniforms = { .mvp_mat = IDENTITY_M4(), .color = { 1.0f, 0.0f, 0.0f, 1.0f } };
	SET_IDENTITY_M4(the_uniforms.mvp_mat);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	GLuint myshader = std_shaders[PGL_SHADER_SHADED];
	glUseProgram(myshader);
	pglSetUniform(&the_uniforms);

	GLuint basic = std_shaders[PGL_SHADER_FLAT];
	glUseProgram(basic);
	pglSetUniform(&the_uniforms);

	glClearColor(0, 0, 0, 1);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

	// TODO Apparently this is necessary, what's the spec say?
	// should the color/zbuf/stencil buffer be initialized to 0 on
	// startup automatically or does the user have to do an initial clear?
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glUseProgram(myshader);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUseProgram(basic);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);

	scale_m4(the_uniforms.mvp_mat, 1.2, 1.2, 1.2);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

