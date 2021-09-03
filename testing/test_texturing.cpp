
typedef struct tt_uniforms
{
	mat4 mvp_mat;
	GLuint tex;
	vec4 v_color;
	float time;
} tt_uniforms;


void texture_replace_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	tt_uniforms* u = (tt_uniforms*)uniforms;
	((vec2*)vs_output)[0] = vec4_to_vec2(((vec4*)vertex_attribs)[2]); //tex_coords

	*(vec4*)&builtins->gl_Position = ((vec4*)vertex_attribs)[0];

}

void texture_replace_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 tex_coords = ((vec3*)fs_input)[0];
	GLuint tex = ((tt_uniforms*)uniforms)->tex;


	builtins->gl_FragColor = texture2D(tex, tex_coords.x, tex_coords.y);
	//print_vec4(stdout, builtins->gl_FragColor, "\n");
}

void tex_rect_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 tex_coords = ((vec3*)fs_input)[0];
	GLuint tex = ((tt_uniforms*)uniforms)->tex;


	builtins->gl_FragColor = texture_rect(tex, tex_coords.x, tex_coords.y);
	//print_vec4(stdout, builtins->gl_FragColor, "\n");
}

void test_texturing(int argc, char** argv, void* data)
{

	tt_uniforms the_uniforms;
	GLenum smooth[2] = { SMOOTH, SMOOTH };

	float points[] =
	{
		-0.8,  0.8, -0.1,
		-0.8, -0.8, -0.1,
		 0.8,  0.8, -0.1,
		 0.8, -0.8, -0.1
	};

	float tex_coords[] =
	{
		0.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		1.0, 1.0,
		0.0, 0.0,
		0.0, 511.0,
		511.0, 0.0,
		511.0, 511.0
	};

	Color test_texture[4] =
	{
		{ 255, 255, 255, 255 },
		{ 0, 0, 0, 255 },
		{ 0, 0, 0, 255 },
		{ 255, 255, 255, 255 }
	};

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// only mag filter is actually used, no matter the size of the image on screen
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (argc != 1) ? GL_NEAREST : GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, test_texture);

	GLuint square;
	glGenBuffers(1, &square);
	glBindBuffer(GL_ARRAY_BUFFER, square);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint texture_replace = pglCreateProgram(texture_replace_vs, texture_replace_fs, 2, smooth, GL_FALSE);
	glUseProgram(texture_replace);
	pglSetUniform(&the_uniforms);

	the_uniforms.tex = texture;


	glClearColor(0.25, 0.25, 0.25, 1);

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}










