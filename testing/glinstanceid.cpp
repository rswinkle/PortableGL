
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

	// have to used hard coded random colors in case
	// of different implementations of rand(), also see
	// note in instancing.cpp
	vec3 inst_colors[20] =
	{
		{ 0.783099, 0.394383, 0.840188 },
		{ 0.197551, 0.911647, 0.798440 },
		{ 0.277775, 0.768230, 0.335223 },
		{ 0.628871, 0.477397, 0.553970 },
		{ 0.952230, 0.513401, 0.364784 },
		{ 0.717297, 0.635712, 0.916195 },
		{ 0.016301, 0.606969, 0.141603 },
		{ 0.804177, 0.137232, 0.242887 },
		{ 0.129790, 0.400944, 0.156679 },
		{ 0.218257, 0.998924, 0.108809 },
		{ 0.612640, 0.839112, 0.512932 },
		{ 0.524287, 0.637552, 0.296032 },
		{ 0.292517, 0.972775, 0.493583 },
		{ 0.769914, 0.526745, 0.771358 },
		{ 0.283315, 0.891529, 0.400229 },
		{ 0.919026, 0.807725, 0.352458 },
		{ 0.525995, 0.949327, 0.069755 },
		{ 0.663227, 0.192214, 0.086056 },
		{ 0.064171, 0.348893, 0.890233 },
		{ 0.063096, 0.457702, 0.020023 }
	};

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









