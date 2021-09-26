

typedef struct t1D_uniforms
{
	GLuint tex;
} t1D_uniforms;


void tex1d_replace_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	vs_output[0] = ((vec4*)vertex_attribs)[2].x; //tex_coord

	*(vec4*)&builtins->gl_Position = ((vec4*)vertex_attribs)[0];

}

void tex1d_replace_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 tex_coords = ((vec3*)fs_input)[0];
	GLuint tex = ((t1D_uniforms*)uniforms)->tex;


	builtins->gl_FragColor = texture1D(tex, tex_coords.x);
	//print_vec4(stdout, builtins->gl_FragColor, "\n");
}

void test_texture1D(int argc, char** argv, void* data)
{

	t1D_uniforms the_uniforms;
	GLenum smooth[2] = { SMOOTH, SMOOTH };

	float points[] =
	{
		-0.8,  0.8, -0.1,
		-0.8, -0.8, -0.1,
		 0.8,  0.8, -0.1,
		 0.8, -0.8, -0.1
	};

	float tex_coords1[] =
	{
		0.0, 0.0,
		1.0, 1.0
	};

	float tex_coords2[] =
	{
		0.0, 0.0,
		2.0, 2.0,
	};

	float* tex_coords;

	Color test_texture[2] =
	{
		{ 255, 255, 255, 255 },
		{ 0, 0, 0, 255 }
	};

	GLuint texture;
	glGenTextures(1, &texture);

	if (argc <= 4) {
		glBindTexture(GL_TEXTURE_1D, texture);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// only mag filter is actually used, no matter the size of the image on screen
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, (argc != 1) ? GL_NEAREST : GL_LINEAR);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_COMPRESSED_RGBA, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, test_texture);
	}


	GLuint square;
	glGenBuffers(1, &square);
	glBindBuffer(GL_ARRAY_BUFFER, square);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	tex_coords = tex_coords1;
	if (argc >= 2) {
		tex_coords = tex_coords2;
	}

	GLuint tex_buf;
	glGenBuffers(1, &tex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords1), tex_coords, GL_STATIC_DRAW);

	// default is GL_REPEAT for argc == 2
	if (argc == 3) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else if (argc == 4) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	}
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint texture_shader = pglCreateProgram(tex1d_replace_vs, tex1d_replace_fs, 1, smooth, GL_FALSE);
	glUseProgram(texture_shader);
	pglSetUniform(&the_uniforms);

	the_uniforms.tex = texture;


	glClearColor(0.25, 0.25, 0.25, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}










