

typedef struct stencil_uniforms
{
	mat4 mvp_mat;
} stencil_uniforms;

void st_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), ((vec4*)vertex_attribs)[0]);
}

void st_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = make_vec4(1.0f, 0.0f, 0.0f, 1.0f);
}

void stencil_smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((vec4*)vs_output)[0] = ((vec4*)vertex_attribs)[4]; //color

	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), ((vec4*)vertex_attribs)[0]);
}

void stencil_smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_FragColor = ((vec4*)fs_input)[0];
}

void stencil_test(int argc, char** argv, void* data)
{
	GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };

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

	stencil_uniforms the_uniforms;

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint colors;
	glGenBuffers(1, &colors);
	glBindBuffer(GL_ARRAY_BUFFER, colors);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color_array), color_array, GL_STATIC_DRAW);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(stencil_smooth_vs, stencil_smooth_fs, 4, smooth, GL_FALSE);
	glUseProgram(myshader);
	pglSetUniform(&the_uniforms);

	GLuint basic = pglCreateProgram(st_normal_vs, st_normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(basic);
	pglSetUniform(&the_uniforms);

	glClearColor(0, 0, 0, 1);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);


	// TODO Apparently this is necessary, what's the spec say?
	// should the color buffer and stencil buffer be initialized to 0 on
	// startup automatically or does the user have to do an initial clear?
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glUseProgram(myshader);

	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUseProgram(basic);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);

	scale_mat4(the_uniforms.mvp_mat, 1.2, 1.2, 1.2);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}









