
typedef struct fbc_uniforms
{
	mat4 mvp_mat;
	vec4 v_color;
} fbc_uniforms;


void fbc_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void fbc_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void front_back_culling(int argc, char** argv, void* data)
{
	vec4 Red = { 1.0f, 0.0f, 0.0f, 0.0f };

	float points[] = {
		// bottom two are CCW
		-0.8, -0.8, 0,
		-0.2, -0.8, 0,
		-0.5, -0.3, 0,

		0.2, -0.8, 0,
		0.8, -0.8, 0,
		0.5, -0.3, 0,

		// top two are CW
		-0.2, 0.3, 0,
		-0.8, 0.3, 0,
		-0.5, 0.8, 0,

		0.8, 0.3, 0,
		0.2, 0.3, 0,
		0.5, 0.8, 0
	};

	switch (argc) {
		case 1:
			glEnable(GL_CULL_FACE);
			break;
		case 2:
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CW);
			break;
		case 3:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case 4:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glFrontFace(GL_CW);
			break;
		case 5:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT_AND_BACK);
			break;
		default:
			break;
	}

	fbc_uniforms the_uniforms;
	mat4 identity = IDENTITY_MAT4();

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(fbc_normal_vs, fbc_normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.v_color = Red;

	memcpy(the_uniforms.mvp_mat, identity, sizeof(mat4));

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 12);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	//glDrawArrays(GL_TRIANGLES, 3, 3);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glDrawArrays(GL_TRIANGLES, 6, 3);
}


void fbc_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), ((vec4*)vertex_attribs)[0]);
}

void fbc_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((ht_uniforms*)uniforms)->v_color;
}








