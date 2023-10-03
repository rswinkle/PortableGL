
enum { ATTR_VERTEX, ATTR_COLOR };

// Not actually used/needed
typedef struct ti_uniforms
{
	vec4 v_color;
} ti_uniforms;

// TODO compare speed of interleaved vs segregated attributes (both 1 and 2 buffers for the latter)
struct vert_attribs
{
	vec3 pos;
	vec3 color;

	vert_attribs(vec3 p, vec3 c) : pos(p), color(c) {}
};

void ti_smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void ti_smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);


float tris_interp_perf(int frames, int argc, char** argv, void* data)
{
	GLenum smooth[3] = { SMOOTH, SMOOTH, SMOOTH };

	srand(10);

	vector<vert_attribs> tris;

#define NUM_TRIS_INTERP 30

	for (int i=0; i<NUM_TRIS_INTERP; ++i) {
		tris.push_back(vert_attribs(vec3(rsw::randf_range(-1, 1), rsw::randf_range(-1, 1), -1), vec3(1, 0, 0)));
		tris.push_back(vert_attribs(vec3(rsw::randf_range(-1, 1), rsw::randf_range(-1, 1), -1), vec3(0, 1, 0)));
		tris.push_back(vert_attribs(vec3(rsw::randf_range(-1, 1), rsw::randf_range(-1, 1), -1), vec3(0, 0, 1)));
	}

	ti_uniforms the_uniforms;

	GLuint triangles;
	glGenBuffers(1, &triangles);
	glBindBuffer(GL_ARRAY_BUFFER, triangles);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vert_attribs)*tris.size(), &tris[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_VERTEX);
	pglVertexAttribPointer(ATTR_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof(vert_attribs), 0);
	glEnableVertexAttribArray(ATTR_COLOR);
	pglVertexAttribPointer(ATTR_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(vert_attribs), sizeof(vec3));

	GLuint myshader = pglCreateProgram(ti_smooth_vs, ti_smooth_fs, 3, smooth, GL_FALSE);
	glUseProgram(myshader);

	pglSetUniform(&the_uniforms);

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



void ti_smooth_vs(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((vec4*)vs_output)[0] = ((vec4*)vertex_attribs)[ATTR_COLOR];

	*(vec4*)&builtins->gl_Position = ((vec4*)vertex_attribs)[ATTR_VERTEX];
}

void ti_smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	// I don't think this is guaranteed to work correctly, since I only interpolated 3 floats
	// the default vec4(0,0,0,1) thing only applies to vertex attributes as they enter the vertex shader
	// ie you specify an attribute with less than 4 elements but in the shader use a larger vector type
	// the remaining elements will default to above values
	//
	//*(vec4*)&builtins->gl_FragColor = ((vec4*)fs_input)[0];
	
	builtins->gl_FragColor.x = fs_input[0];
	builtins->gl_FragColor.y = fs_input[1];
	builtins->gl_FragColor.z = fs_input[2];
	builtins->gl_FragColor.w = 1.0f;
}

