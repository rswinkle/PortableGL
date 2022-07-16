
typedef struct tp_uniforms
{
	vec4 v_color;
} tp_uniforms;

void tp_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void tp_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);


float tris_perf(int frames, int argc, char** argv, void* data)
{
	srand(10);

	vector<vec3> tris;

#define NUM_TRIS 50

	for (int i=0; i <NUM_TRIS*3; ++i) {
		tris.push_back(vec3(rsw::randf_range(-1, 1), rsw::randf_range(-1, 1), -1));
	}

	tp_uniforms the_uniforms;

	Buffer triangles(1);
	triangles.bind(GL_ARRAY_BUFFER);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*tris.size(), &tris[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	GLuint myshader = pglCreateProgram(tp_normal_vs, tp_normal_fs, 0, NULL, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

	the_uniforms.v_color = Red;

	glClearColor(0, 0, 0, 1);

	int start, end, j;
	start = SDL_GetTicks();
	for (j=0; j<frames; ++j) {
		if (handle_events())
			break;

		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, tris.size());

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();

	// return FPS
	return j / ((end-start)/1000.0f);
}



void tp_normal_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_Position = ((vec4*)vertex_attribs)[0];
}

void tp_normal_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	*(vec4*)&builtins->gl_FragColor = ((tp_uniforms*)uniforms)->v_color;
}

