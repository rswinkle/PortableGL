

void test_edges(int argc, char** argv, void* data)
{
	float points[] = { -1, 1, 0,
	                    1, 1, 0,
	                    1, -1, 0,
	                    -1, -1, 0 };

	GLuint lines;
	glGenBuffers(1, &lines);
	glBindBuffer(GL_ARRAY_BUFFER, lines);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Using default shader

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

}


