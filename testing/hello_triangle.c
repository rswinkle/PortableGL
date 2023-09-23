
void hello_triangle(int argc, char** argv, void* data)
{

	float points[] = { -0.5, -0.5, 0,
	                    0.5, -0.5, 0,
	                    0,    0.5, 0 };


	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Don't need a shader or uniform, just using the default shader 0
	// which is just a passthrough vs, draw everything red fs

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);

}



