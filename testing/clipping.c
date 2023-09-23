

void clip_xy(int argc, char** argv, void* data)
{
	float points[] = {
		// top and bottom, 1 and 2, CCW and CW
		-0.7, 0.8, 0,
		-0.3, 0.8, 0,
		-0.5, 1.2, 0,

		 0.3, 1.2, 0,
		 0.7, 1.2, 0,
		 0.5, 0.8, 0,

		-0.3, -0.8, 0,
		-0.7, -0.8, 0,
		-0.5, -1.2, 0,

		 0.3, -1.2, 0,
		 0.7, -1.2, 0,
		 0.5, -0.8, 0,

		// left and right, 1 and 2, CCW and CW
		-0.8, -0.7, 0,
		-0.8, -0.3, 0,
		-1.2, -0.5, 0,

		-1.2,  0.3, 0,
		-1.2,  0.7, 0,
		-0.8,  0.5, 0,

		 0.8, -0.3, 0,
		 0.8, -0.7, 0,
		 1.2, -0.5, 0,

		 1.2,  0.7, 0,
		 1.2,  0.3, 0,
		 0.8,  0.5, 0,
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
	glDrawArrays(GL_TRIANGLES, 0, 24);

}

void clip_z(int argc, char** argv, void* data)
{
	float points[] = {
		// bottom three are CCW
		-0.9, -0.8, 0,
		-0.5, -0.8, 0,
		-0.7, -0.3, 0,

		-0.2, -0.8, 0,
		 0.2, -0.8, 0,
		 0.0, -0.3, -1.3,

		 0.5, -0.8, 1.3,
		 0.9, -0.8, 1.3,
		 0.7, -0.3, 0,

		// top three are CW
		-0.9, 0.8, 0,
		-0.5, 0.8, 0,
		-0.7, 0.3, 0,

		-0.2, 0.8, 0,
		 0.2, 0.8, 0,
		 0.0, 0.3, -1.3,

		 0.5, 0.8, 1.3,
		 0.9, 0.8, 1.3,
		 0.7, 0.3, 0,
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
	glDrawArrays(GL_TRIANGLES, 0, 18);

}

// TODO clipping GL_LINES and GL_POINTS

// TODO test clipping z after perspective projection?
// Test interaction with depth clamp?











