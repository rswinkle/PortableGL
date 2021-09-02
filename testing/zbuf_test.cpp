
typedef struct zb_uniforms
{
	vec4 v_color;
} zb_uniforms;


void zb_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = ((vec4*)vertex_attribs)[0];
}

void zb_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((zb_uniforms*)uniforms)->v_color;
}

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
		 1, -1, 0.9
	};


	zb_uniforms the_uniforms;

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(zb_normal_vs, zb_normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	if (argc == 1) {
		glEnable(GL_DEPTH_TEST);
	} else if (argc == 2) {
		glEnable(GL_DEPTH_TEST);
		glClearDepth(0);
		glDepthFunc(GL_GREATER);
	} else if (argc == 3) {
		glEnable(GL_DEPTH_TEST);
		glDepthRange(1, 0);
	}


	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	the_uniforms.v_color = Red;
	glDrawArrays(GL_TRIANGLES, 0, 3);

	the_uniforms.v_color = Blue;
	glDrawArrays(GL_TRIANGLES, 3, 3);

}









