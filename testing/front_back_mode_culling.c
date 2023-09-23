
void front_back_culling(int argc, char** argv, void* data)
{
	//TODO I suppose I don't really need 2 of each...I think
	//I was planning on drawing them individually, changing setting
	//for each one so I could test twice as many combinations in
	//a single frame.
	float points[] = {
		// bottom two are CCW
		-0.8, -0.8, 0,
		-0.2, -0.8, 0,
		-0.5, -0.3, 0,

		0.2, -0.8, 0,
		0.8, -0.8, 0,
		0.5, -0.3, 0,

		// top two are CW
		-0.2, 0.3, 0,
		-0.8, 0.3, 0,
		-0.5, 0.8, 0,

		0.8, 0.3, 0,
		0.2, 0.3, 0,
		0.5, 0.8, 0
	};

	switch (argc) {
		case 1:
			glEnable(GL_CULL_FACE);
			break;
		case 2:
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CW);
			break;
		case 3:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case 4:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glFrontFace(GL_CW);
			break;
		case 5:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT_AND_BACK);
			break;
		case 6:
			// front = point, back = fill
			glPolygonMode(GL_FRONT, GL_POINT);
			break;
		case 7:
			// front = fill, back = point
			glPolygonMode(GL_BACK, GL_POINT);
			break;
		case 8:
			// front = LINE, back = fill
			glPolygonMode(GL_FRONT, GL_LINE);
			break;
		case 9:
			// front = fill, back = line
			glPolygonMode(GL_BACK, GL_LINE);
			break;
		case 10:
			glPolygonMode(GL_FRONT, GL_LINE);
			glPolygonMode(GL_BACK, GL_POINT);
			break;
		case 11:
			glFrontFace(GL_CW);
			glPolygonMode(GL_FRONT, GL_LINE);
			glPolygonMode(GL_BACK, GL_POINT);
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

	//glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	//glDrawArrays(GL_TRIANGLES, 3, 3);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glDrawArrays(GL_TRIANGLES, 6, 3);
}


