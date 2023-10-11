
float blend_test(int frames, int argc, char** argv, void* data)
{
	pgl_vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	pgl_vec4 Green = {0.0f, 1.0f, 0.0f, 1.0f };
	pgl_vec4 Blue = {0.0f, 0.0f, 1.0f, 1.0f };
	pgl_vec4 Black = {0.0f, 0.0f, 0.0f, 1.0f };

	float points[] = {
		-0.75, 0.75, 0,
		-0.75, 0.25, 0,
		-0.25, 0.75, 0,
		-0.25, 0.25, 0,

		 0.25, 0.75, 0,
		 0.25, 0.25, 0,
		 0.75, 0.75, 0,
		 0.75, 0.25, 0,

		-0.75, -0.25, 0,
		-0.75, -0.75, 0,
		-0.25, -0.25, 0,
		-0.25, -0.75, 0,

		 0.25, -0.25, 0,
		 0.25, -0.75, 0,
		 0.75, -0.25, 0,
		 0.75, -0.75, 0,

	// TODO expand these verts so I can use 1 call with GL_TRIANGLES
	// ... or look into primitive restart?
//mix with white
		-0.15, 0.15, -0.1,
		-0.15, -0.15, -0.1,
		 0.15, 0.15, -0.1,
		 0.15, -0.15, -0.1,

// mix with red
		-0.40, 0.65, -0.1,
		-0.40, 0.35, -0.1,
		-0.10, 0.65, -0.1,
		-0.10, 0.35, -0.1,

// mix with green
		0.10, 0.65, -0.1,
		0.10, 0.35, -0.1,
		0.40, 0.65, -0.1,
		0.40, 0.35, -0.1,

// mix with blue
		-0.40, -0.35, -0.1,
		-0.40, -0.65, -0.1,
		-0.10, -0.35, -0.1,
		-0.10, -0.65, -0.1,

// mix with black
		0.10, -0.35, -0.1,
		0.10, -0.65, -0.1,
		0.40, -0.35, -0.1,
		0.40, -0.65, -0.1,

	};

	pgl_uniforms the_uniforms;

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_IDENTITY]);

	pglSetUniform(&the_uniforms);

	glClearColor(1, 1, 1, 1);

	int start, end, i;
	start = SDL_GetTicks();
	for (i=0; i<frames; ++i) {
		if (handle_events())
			break;
		glClear(GL_COLOR_BUFFER_BIT);
		
		the_uniforms.color = Red;
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		the_uniforms.color = Green;
		glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
		the_uniforms.color = Blue;
		glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
		the_uniforms.color = Black;
		glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SET_VEC4(the_uniforms.color, 1, 0, 0, 0.5f);
		glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);

		glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

		glDrawArrays(GL_TRIANGLE_STRIP, 24, 4);

		glDrawArrays(GL_TRIANGLE_STRIP, 28, 4);

		glDrawArrays(GL_TRIANGLE_STRIP, 32, 4);

		glDisable(GL_BLEND);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();

	// return FPS
	return i / ((end-start)/1000.0f);
}

