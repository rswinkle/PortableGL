

float points_perf(int frames, int argc, char** argv, void* data)
{
	srand(10);

	vector<vec3> points;
#define NUM_POINTS 12000

	for (int i=0; i < NUM_POINTS; ++i) {
		points.push_back(vec3(rsw::randf_range(-1.1, 1.1), rsw::randf_range(-1.1, 1.1), -1));
	}

	GLuint point_buf;
	glGenBuffers(1, &point_buf);
	glBindBuffer(GL_ARRAY_BUFFER, point_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*points.size(), &points[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Using default shader 0

	glClearColor(0, 0, 0, 1);

	int num_points = NUM_POINTS;
	if (argc >= 2) {
		num_points /= (argc*argc-2);
	}
	glPointSize(argc);

	int start, end, j;
	start = SDL_GetTicks();
	for (j=0; j<frames; ++j) {
		if (handle_events())
			break;

		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_POINTS, 0, num_points);

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(u32));
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}
	end = SDL_GetTicks();

	// return FPS
	return j / ((end-start)/1000.0f);
}


