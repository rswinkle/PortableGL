
typedef struct pp_uniforms
{
	vec4 v_color;
} pp_uniforms;

void pp_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void pp_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);


float points_perf(int argc, char** argv, void* data)
{
	srand(10);

	vector<vec3> points;
#define NUM_POINTS 10000

	for (int i=0; i < NUM_POINTS; ++i) {
		points.push_back(vec3(rsw::rand_float(-1, 1), rsw::rand_float(-1, 1), -1));
	}

	pp_uniforms the_uniforms;

	Buffer triangle(1);
	triangle.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*points.size(), &points[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(pp_normal_vs, pp_normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.v_color = Red;

	glClearColor(0, 0, 0, 1);

	int start, end, j;
	start = SDL_GetTicks();
	for (j=0; j<argc; ++j) {
		if (handle_events())
			break;

		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_POINTS, 0, NUM_POINTS);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();

	// return FPS
	return j / ((end-start)/1000.0f);
}



void pp_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_Position = ((vec4*)vertex_attribs)[0];
}

void pp_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_FragColor = ((pp_uniforms*)uniforms)->v_color;
}

