
float tri_clipxy_perf(int frames, int argc, char** argv, void* data)
{
	srand(42);

	vector<vec3> tris;

	float s = -1.0f;
	float b = 0.9f;
	float tw = 0.1f;
	for (int i=0; i<10; i++) {
		tris.push_back(vec3(s, b, 0));
		tris.push_back(vec3(s+2*tw, b, 0));
		tris.push_back(vec3(s+tw, 1.2, 0));

		tris.push_back(vec3(s+2*tw, -b, 0));
		tris.push_back(vec3(s, -b, 0));
		tris.push_back(vec3(s+tw, -1.2, 0));

		s += 3*tw;
	}
	s = -1.0f;
	b = 0.9f;
	tw = 0.1f;
	for (int i=0; i<10; i++) {
		tris.push_back(vec3(b, s+2*tw, 0));
		tris.push_back(vec3(b, s, 0));
		tris.push_back(vec3(1.2, s+tw, 0));

		tris.push_back(vec3(-b, s, 0));
		tris.push_back(vec3(-b, s+2*tw, 0));
		tris.push_back(vec3(-1.2, s+tw, 0));

		s += 3*tw;
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

float tri_clipz_perf(int frames, int argc, char** argv, void* data)
{
	srand(42);
	vector<vec3> tris;

	for (int i=0; i<15; i++) {
		tris.push_back(vec3(-0.2, 0.1, 0));
		tris.push_back(vec3(0.2, 0.1, 0));
		tris.push_back(vec3(0, 0.6, 1.2));

		tris.push_back(vec3(0.2, -0.1, -1.2));
		tris.push_back(vec3(-0.2, -0.1, -1.2));
		tris.push_back(vec3(0, -0.6, 0));

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

float tri_clipxyz_perf(int frames, int argc, char** argv, void* data)
{
	srand(10);

	vector<vec3> tris;

#define NUM_TRIS 50

	vec3 p1, p2, p3;
	for (int i=0; i <NUM_TRIS*3; ++i) {
		p1 = vec3(rsw::randf_range(-1.5, 1.5), rsw::randf_range(-1.5, 1.5), rsw::randf_range(-1.5, 1.5));
		p2 = p1 + rsw::random_in_unit_sphere()/2;
		p3 = p1 + rsw::random_in_unit_sphere()/2;
		tris.push_back(p1);
		tris.push_back(p2);
		tris.push_back(p3);
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




