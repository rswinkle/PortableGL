
void clipping_xy(int argc, char** argv, void* data)
{
	float points[] = {
		// top and bottom CCW and CW
		-0.5, 0.8, 0,
		 0.5, 0.8, 0,
		 0.0, 1.2, 0,

		-0.5, -0.8, 0,
		 0.5, -0.8, 0,
		 0.0, -1.2, 0,

		// left and right CCW and CW
		-0.8, -0.3, 0,
		-0.8,  0.3, 0,
		-1.2,  0.0, 0,

		 0.8, -0.3, 0,
		 0.8,  0.3, 0,
		 1.2,  0.0, 0,
	};

	switch (argc) {
		case 1:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 2:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case 3:
			glLineWidth(8);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 4:
			glPointSize(8);
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case 5:
			glLineWidth(32);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 6:
			glPointSize(32);
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		default:
			break;
	}


	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Don't need a shader or uniform, just using the default shader 0
	// which is just a passthrough vs, draw everything red fs

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 12);

}











