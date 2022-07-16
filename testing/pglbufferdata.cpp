


void pbd_smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* v_attribs = (vec4*)vertex_attribs;
	((vec4*)vs_output)[0] = v_attribs[4]; //color

	builtins->gl_Position = v_attribs[0];
}

void pbd_smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((vec4*)fs_input)[0];
}

void pglbufdata_test(int argc, char** argv, void* data)
{
	GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };

	float points_n_colors[] = {
		-0.5, -0.5, 0.0,
		 1.0,  0.0, 0.0,

		 0.5, -0.5, 0.0,
		 0.0,  1.0, 0.0,

		 0.0,  0.5, 0.0,
		 0.0,  0.0, 1.0 };

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	pglBufferData(GL_ARRAY_BUFFER, sizeof(points_n_colors), points_n_colors, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	pglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, 0);
	glEnableVertexAttribArray(4);
	pglVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, sizeof(float)*3);


	points_n_colors[13] = 1.0;
	points_n_colors[15] = 1.0;

	GLuint myshader = pglCreateProgram(pbd_smooth_vs, pbd_smooth_fs, 4, smooth, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(NULL);

	glClearColor(0, 0, 0, 1);

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);

}










