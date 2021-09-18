
typedef struct te_uniforms
{
	vec4 v_color;
} te_uniforms;


void te_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void te_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void test_edges(int argc, char** argv, void* data)
{
	vec4 Red = { 1.0f, 0.0f, 0.0f, 0.0f };

	float points[] = { -1, 1, 0,
	                    1, 1, 0,
	                    1, -1, 0,
	                    -1, -1, 0 };


	te_uniforms the_uniforms;

	GLuint lines;
	glGenBuffers(1, &lines);
	glBindBuffer(GL_ARRAY_BUFFER, lines);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(te_normal_vs, te_normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.v_color = Red;

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

}


void te_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = ((vec4*)vertex_attribs)[0];
}

void te_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((te_uniforms*)uniforms)->v_color;
}







