

float tris_perf(int frames, int argc, char** argv, void* data)
{
	srand(10);

	vector<vec3> tris;

#define NUM_TRIS 50

	for (int i=0; i <NUM_TRIS*3; ++i) {
		tris.push_back(vec3(rsw::randf_range(-1, 1), rsw::randf_range(-1, 1), -1));
	}

	GLuint triangles;
	glGenBuffers(1, &triangles);
	glBindBuffer(GL_ARRAY_BUFFER, triangles);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*tris.size(), &tris[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// using default shader 0

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


