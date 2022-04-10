
typedef struct instancing_uniforms
{
	mat4 mvp_mat;
	vec4 v_color;
} instancing_uniforms;


void instancing_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void instancing_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void test_instancing(int argc, char** argv, void* data)
{
	vec4 Red = { 1.0f, 0.0f, 0.0f, 0.0f };

	float points[] = { -0.05, -0.05, 0,
	                    0.05, -0.05, 0,
	                    0,    0.05, 0 };

	vec2 positions[100];
	int i = 0;
	float offset = 0.1f;
	for (int y = -10; y < 10; y += 2)
	{
		for (int x = -10; x < 10; x += 2)
		{
			vec2 pos;
			pos.x = (float)x / 10.0f + offset;
			pos.y = (float)y / 10.0f + offset;
			positions[i++] = pos;
		}
	}

	vec3 inst_colors[10];
	for (int i=0; i<10; i++) {
		inst_colors[i] = make_vec3(rsw_randf(), rsw_randf(), rsw_randf());
	}

	GLuint instance_pos;
	glGenBuffers(1, &instance_pos);
	glBindBuffer(GL_ARRAY_BUFFER, instance_pos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 100, &positions[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);
	glVertexAttribDivisor(1, 1);

	GLuint instance_colors;
	glGenBuffers(1, &instance_colors);
	glBindBuffer(GL_ARRAY_BUFFER, instance_colors);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 10, &inst_colors[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
	glVertexAttribDivisor(2, 10);



	instancing_uniforms the_uniforms;
	mat4 identity = IDENTITY_MAT4();

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLenum flat[3] = { FLAT, FLAT, FLAT };
	GLuint myshader = pglCreateProgram(instancing_vs, instancing_fs, 3, flat, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.v_color = Red;

	memcpy(the_uniforms.mvp_mat, identity, sizeof(mat4));

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 100);

}


void instancing_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4 vert = ((vec4*)vertex_attribs)[0];
	vec4 offset = ((vec4*)vertex_attribs)[1];
	vert.x += offset.x;
	vert.y += offset.y;

	vec4 color = ((vec4*)vertex_attribs)[2];
	*(vec3*)vs_output = make_vec3(color.x, color.y, color.z);

	// 0 and 1 are default for z and w
	builtins->gl_Position = vert;
}

void instancing_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = make_vec4(fs_input[0], fs_input[1], fs_input[2], 1);
}








