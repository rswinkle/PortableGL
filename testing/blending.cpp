


typedef struct blend_uniforms
{
	vec4 v_color;
} blend_uniforms;


void blend_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void blend_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void blend_test(int argc, char** argv, void* data)
{
	vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	vec4 Green = {0.0f, 1.0f, 0.0f, 1.0f };
	vec4 Blue = {0.0f, 0.0f, 1.0f, 1.0f };
	vec4 Black = {0.0f, 0.0f, 0.0f, 1.0f };

	float points[] = {
		-0.75, 0.75, 0,
		-0.75, 0.25, 0,
		-0.25, 0.75, 0,
		-0.25, 0.25, 0,

		 0.25, 0.75, 0,
		 0.25, 0.25, 0,
		 0.75, 0.75, 0,
		 0.75, 0.25, 0,
						
		-0.75, -0.25, 0,
		-0.75, -0.75, 0,
		-0.25, -0.25, 0,
		-0.25, -0.75, 0,

		 0.25, -0.25, 0,
		 0.25, -0.75, 0,
		 0.75, -0.25, 0,
		 0.75, -0.75, 0,
	
//mix with white
		-0.15, 0.15, -0.1,
		-0.15, -0.15, -0.1,
		 0.15, 0.15, -0.1,
		 0.15, -0.15, -0.1,

// mix with red
		-0.40, 0.65, -0.1,
		-0.40, 0.35, -0.1,
		-0.10, 0.65, -0.1,
		-0.10, 0.35, -0.1,

// mix with green
		0.10, 0.65, -0.1,
		0.10, 0.35, -0.1,
		0.40, 0.65, -0.1,
		0.40, 0.35, -0.1,

// mix with blue
		-0.40, -0.35, -0.1,
		-0.40, -0.65, -0.1,
		-0.10, -0.35, -0.1,
		-0.10, -0.65, -0.1,

// mix with black
		0.10, -0.35, -0.1,
		0.10, -0.65, -0.1,
		0.40, -0.35, -0.1,
		0.40, -0.65, -0.1,

	};


	blend_uniforms the_uniforms;

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(blend_normal_vs, blend_normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.v_color = Red;


	glClearColor(1, 1, 1, 1);

	glClear(GL_COLOR_BUFFER_BIT);
	
	the_uniforms.v_color = Red;
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	the_uniforms.v_color = Green;
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	the_uniforms.v_color = Blue;
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	the_uniforms.v_color = Black;
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	SET_VEC4(the_uniforms.v_color, 1, 0, 0, 0.5f);
	glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);

	glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

	glDrawArrays(GL_TRIANGLE_STRIP, 24, 4);

	glDrawArrays(GL_TRIANGLE_STRIP, 28, 4);

	glDrawArrays(GL_TRIANGLE_STRIP, 32, 4);

	glDisable(GL_BLEND);

}


void blend_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = ((vec4*)vertex_attribs)[0];
}

void blend_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((blend_uniforms*)uniforms)->v_color;
}







