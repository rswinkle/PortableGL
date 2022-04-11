
typedef struct instanceid_uniforms
{
	vec2* inst_offsets;
	vec3* inst_colors;
} instanceid_uniforms;


void glinstanceid_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void glinstanceid_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void test_instanceid(int argc, char** argv, void* data)
{
	srand(0);

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

	vec3 inst_colors[20];
	for (int i=0; i<20; i++) {
		inst_colors[i] = make_vec3(rsw_randf(), rsw_randf(), rsw_randf());
	}

	instanceid_uniforms the_uniforms;
	the_uniforms.inst_offsets = positions;
	the_uniforms.inst_colors = inst_colors;

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(glinstanceid_vs, glinstanceid_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 100);

}


void glinstanceid_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	instanceid_uniforms* u = (instanceid_uniforms*)uniforms;

	vec4 vert = ((vec4*)vertex_attribs)[0];
	vec2 offset = u->inst_offsets[builtins->gl_InstanceID];
	vert.x += offset.x;
	vert.y += offset.y;

	builtins->gl_Position = vert;
}

void glinstanceid_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	instanceid_uniforms* u = (instanceid_uniforms*)uniforms;
	vec3 color = u->inst_colors[builtins->gl_InstanceID/5];
	builtins->gl_FragColor = make_vec4(color.x, color.y, color.z, 1);
}









