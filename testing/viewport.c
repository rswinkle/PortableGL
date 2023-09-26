
void test_viewport(int argc, char** argv, void* data)
{

	float points[] = { -0.5, -0.5, 0,
	                    0.5, -0.5, 0,
	                    0,    0.5, 0 };

	switch (argc) {
		case 1:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case 2:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		default:
			break;
	}

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	vec4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };

	vec4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
	vec4 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
	vec4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	
	vec4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
	vec4 Grey = { 0.5f, 0.5f, 0.5f, 0.5f };

	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_IDENTITY]);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	the_uniforms.v_color = Red;
	glViewport(0, 0, WIDTH/2, HEIGHT/2);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	the_uniforms.v_color = Green;
	glViewport(0, HEIGHT/2, WIDTH/2, HEIGHT/2);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	the_uniforms.v_color = Blue;
	glViewport(WIDTH/2, HEIGHT/2, WIDTH/2, HEIGHT/2);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	the_uniforms.v_color = Magenta;
	glViewport(WIDTH/2, 0, WIDTH/2, HEIGHT/2);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// testing going off all edges
	the_uniforms.v_color = Cyan;
	glViewport(-WIDTH/4, -HEIGHT/4, WIDTH/2, HEIGHT/2);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	the_uniforms.v_color = Yellow;
	glViewport(-WIDTH/4, 3*HEIGHT/4, WIDTH/2, HEIGHT/2);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	the_uniforms.v_color = White;
	glViewport(3*WIDTH/4, 3*HEIGHT/4, WIDTH/2, HEIGHT/2);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	the_uniforms.v_color = Grey;
	glViewport(3*WIDTH/4, -HEIGHT/4, WIDTH/2, HEIGHT/2);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}




