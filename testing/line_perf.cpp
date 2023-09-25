

float lines_perf(int frames, int argc, char** argv, void* data)
{
	srand(10);

	vector<vec3> lines;

	for (int i=0; i < 1000; ++i) {
		lines.push_back(vec3(rsw::randf_range(-1, 1), rsw::randf_range(-1, 1), 0));
	}

	GLuint line_buf;
	glGenBuffers(1, &line_buf);
	glBindBuffer(GL_ARRAY_BUFFER, line_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*lines.size(), &lines[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// using default shader 0

	glClearColor(0, 0, 0, 1);

	int start, end, i;
	start = SDL_GetTicks();
	for (i=0; i<frames; ++i) {
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


