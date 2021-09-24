
typedef struct primitives_uniforms
{
	mat4 mvp_mat;
} primitives_uniforms;


void primitives_smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4* v_attribs = (vec4*)vertex_attribs;
	((vec4*)vs_output)[0] = v_attribs[4]; //color

	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), v_attribs[0]);
}

void primitives_smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((vec4*)fs_input)[0];
}

void primitives_test(int argc, char** argv, void* data)
{
	GLenum smooth[4] = { SMOOTH, SMOOTH, SMOOTH, SMOOTH };

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




	primitives_uniforms the_uniforms;

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


	//glClearColor(0, 0, 0, 1);
	//glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
	//
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);

	GLuint myshader = pglCreateProgram(primitives_smooth_vs, primitives_smooth_fs, 4, smooth, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	SET_IDENTITY_MAT4(the_uniforms.mvp_mat);

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 5);

	glDrawArrays(GL_LINES, 11, 4);

	glDrawArrays(GL_LINE_LOOP, 15, 4);

	glDrawArrays(GL_LINE_STRIP, 19, 3);
}









