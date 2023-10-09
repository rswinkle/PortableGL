
void hello_indexing(int argc, char** argv, void* data)
{
	float points[] =
	{
		-0.5f,  0.5f, 0.0f,  // top left
		-0.5f, -0.5f, 0.0f,  // bottom left
		 0.5f,  0.5f, 0.0f,  // top right
		 0.5f, -0.5f, 0.0f,  // bottom right
	};

	// not that it matters here, but using CCW
	GLuint indices[] = {
		0, 1, 2,
		2, 1, 3
	};

	// using default VAO 0, already active (like compatibility profile)

	GLuint square, elements;
	glGenBuffers(1, &square);
	glBindBuffer(GL_ARRAY_BUFFER, square);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

	if (argc < 2) {
		glGenBuffers(1, &elements);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	}

	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// using default shader 0, already active

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	switch (argc) {
	case 0:
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		break;
	case 1:
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (GLvoid*)(3*sizeof(GLuint)));
		break;
	case 2:
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
		break;
	case 3:
		glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &indices[3]);
		break;
	}
}




