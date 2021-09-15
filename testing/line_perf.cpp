
typedef struct lp_uniforms
{
	vec4 v_color;
} lp_uniforms;



void lp_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void lp_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

float lines_perf(int argc, char** argv, void* data)
{
	srand(10);

	vector<vec3> lines;

	for (int i=0; i < 1000; ++i) {
		lines.push_back(vec3(rsw::rand_float(-1, 1), rsw::rand_float(-1, 1), 0));
	}

	lp_uniforms the_uniforms;

	Buffer line_buf(1);
	line_buf.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*lines.size(), &lines[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(lp_normal_vs, lp_normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.v_color = Red;
	glClearColor(0, 0, 0, 1);

	int start, end, i;
	start = SDL_GetTicks();
	for (i=0; i<argc; ++i) {
		if (handle_events())
			break;

		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_LINES, 0, lines.size());

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();

	// return FPS
	return i / ((end-start)/1000.0f);
}


void lp_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_Position = ((vec4*)vertex_attribs)[0];
}

void lp_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_FragColor = ((lp_uniforms*)uniforms)->v_color;
}

