

void base_instancing_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void base_instancing_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

void test_baseinstance(int argc, char** argv, void* data)
{
	float points[] = { -0.05, -0.05, 0,
	                    0.05, -0.05, 0,
	                    0,    0.05, 0 };

	GLuint indices[] = { 0, 1, 2 };

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

	vec3 colors[10] =
	{
		{0.783099, 0.394383, 0.840188 },
		{0.197551, 0.911647, 0.798440 },
		{0.277775, 0.768230, 0.335223 },
		{0.628871, 0.477397, 0.553970 },
		{0.952230, 0.513401, 0.364784 },
		{0.717297, 0.635712, 0.916195 },
		{0.016301, 0.606969, 0.141603 },
		{0.804177, 0.137232, 0.242887 },
		{0.129790, 0.400944, 0.156679 },
		{0.218257, 0.998924, 0.108809 }
	};

	// This is necessary because the formula for grabbing a vertex attribute
	// is
	//
	// floor(gl_InstanceID/divisor) + baseInstance
	//
	// rather than
	//
	// floor((gl_InstanceID+baseInstance)/divisor)
	//
	// so really all your instanced attributes should have the
	// same divisor if you're going to use any *BaseInstance calls
	// or you might go out of bounds or at least  not get
	// what you have expected
	int base_instance = 20;
	vec3 inst_colors[100];
	for (int i=0; i<100; i++) {
		inst_colors[i] = colors[(i+(base_instance/10)) % 10];
	}

	if (argc < 3) {
		GLuint instance_pos;
		glGenBuffers(1, &instance_pos);
		glBindBuffer(GL_ARRAY_BUFFER, instance_pos);
		glBufferData(GL_ARRAY_BUFFER, sizeof(positions), &positions[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribDivisor(1, 1);

		GLuint instance_colors;
		glGenBuffers(1, &instance_colors);
		glBindBuffer(GL_ARRAY_BUFFER, instance_colors);
		glBufferData(GL_ARRAY_BUFFER, sizeof(inst_colors), &inst_colors[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribDivisor(2, 10);

		GLuint triangle;
		glGenBuffers(1, &triangle);
		glBindBuffer(GL_ARRAY_BUFFER, triangle);
		glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	} else {
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, &positions[0]);
		glVertexAttribDivisor(1, 1);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, &inst_colors[0]);
		glVertexAttribDivisor(2, 10);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, points);
	}

	if (argc == 1) {
		GLuint elems;
		glGenBuffers(1, &elems);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elems);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	}

	GLenum flat[3] = { FLAT, FLAT, FLAT };
	GLuint myshader = pglCreateProgram(base_instancing_vs, base_instancing_fs, 3, flat, GL_FALSE);
	glUseProgram(myshader);

	// The shader doesn't actually use any uniforms but might as well make
	// that obvious/explicit by setting it to NULL
	pglSetUniform(NULL);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	if (!argc) {
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 60, base_instance);
	} else if (argc == 1) {
		glDrawElementsInstancedBaseInstance(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0, 60, base_instance);
	} else {
		glDrawElementsInstancedBaseInstance(GL_TRIANGLES, 3, GL_UNSIGNED_INT, indices, 60, base_instance);
	}
}


void base_instancing_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vec4 vert = vertex_attribs[0];
	vec4 offset = vertex_attribs[1];
	vert.x += offset.x;
	vert.y += offset.y;

	vec4 color = ((vec4*)vertex_attribs)[2];
	*(vec3*)vs_output = make_vec3(color.x, color.y, color.z);

	// 0 and 1 are default for z and w (actually we set z to 0 in our points array anyway)
	builtins->gl_Position = vert;
}

void base_instancing_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = make_vec4(fs_input[0], fs_input[1], fs_input[2], 1);
}









