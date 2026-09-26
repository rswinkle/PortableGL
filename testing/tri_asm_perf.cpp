// 16384 GL_TRIANGLES, every one clipped, almost no pixels.
// Two vertices sit inside the guard and outside the viewport. The third is
// past the guard, so the clipper runs and the fill finds an empty viewport
// intersection.

float tri_asm_perf(int frames, int argc, char** argv, void* data)
{
	const int num_tris = 16384;
	vector<vec3> tris;
	int i;

	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	tris.reserve(num_tris * 3);
	for (i = 0; i < num_tris; ++i) {
		float y = -0.9f + (float)(i & 255) * (1.8f / 255.0f);
		tris.push_back(vec3(2.5f, y, 0.0f));
		tris.push_back(vec3(3.5f, y + 0.01f, 0.0f));
		tris.push_back(vec3(8.0f, y + 0.005f, 0.0f));
	}

	GLuint triangles;
	glGenBuffers(1, &triangles);
	glBindBuffer(GL_ARRAY_BUFFER, triangles);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * tris.size(), &tris[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glClearColor(0, 0, 0, 1);

	int start, end, j;
	start = SDL_GetTicks();
	for (j = 0; j < frames; ++j) {
		if (handle_events())
			break;

		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, tris.size());

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();

	return j / ((end - start) / 1000.0f);
}
