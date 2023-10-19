
// aggh, tried so hard to keep it out...
// maybe I should just accept it and include it in run_tests.cpp
//
// probably end up wanting rsw_math too, sigh
#include <vector>

void test_multidraw(int argc, char** argv, void* data)
{
	std::vector<vec3> tri_strips;
	std::vector<vec3> colors;
	std::vector<GLuint> strip_elems;
	vec3 offset = { 10, 10, 0 };

	int sq_dim = 20;
	std::vector<GLint> firsts;

	// needs to be the same size as a pointer so GLintptr or GLsizeiptr
	// Possibly I could remove that restriction by changing glMultiDrawElements somehow
	// but better to just match OpenGL for now
	std::vector<GLintptr> first_elems;
	std::vector<GLsizei> counts;

	const int cols = 25;
	const int rows = 25;

	for (int j=0; j<rows; j++) {
		for (int i=0; i<cols; i++) {
			firsts.push_back(tri_strips.size());

			// a byte offset into the element array buffer, which is using GLuints
			first_elems.push_back(strip_elems.size()*sizeof(GLuint));
			counts.push_back(4);

			tri_strips.push_back(make_vec3(i*(sq_dim+5),        j*(sq_dim+5),        0));
			tri_strips.push_back(make_vec3(i*(sq_dim+5),        j*(sq_dim+5)+sq_dim, 0));
			tri_strips.push_back(make_vec3(i*(sq_dim+5)+sq_dim, j*(sq_dim+5),        0));
			tri_strips.push_back(make_vec3(i*(sq_dim+5)+sq_dim, j*(sq_dim+5)+sq_dim, 0));

			colors.push_back(make_vec3(1,0,0));
			colors.push_back(make_vec3(0,1,0));
			colors.push_back(make_vec3(0,0,1));
			colors.push_back(make_vec3(0,0,0));

			strip_elems.push_back((j*cols+i)*4);
			strip_elems.push_back((j*cols+i)*4+1);
			strip_elems.push_back((j*cols+i)*4+2);
			strip_elems.push_back((j*cols+i)*4+3);
		}
	}
	for (int i=0; i<tri_strips.size(); i++) {
		tri_strips[i].x += offset.x;
		tri_strips[i].y += offset.y;
	}

	GLuint square_buf;
	glGenBuffers(1, &square_buf);
	glBindBuffer(GL_ARRAY_BUFFER, square_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*tri_strips.size(), &tri_strips[0], GL_STATIC_DRAW);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(PGL_ATTR_VERT);

	GLuint color_buf;
	glGenBuffers(1, &color_buf);
	glBindBuffer(GL_ARRAY_BUFFER, color_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*colors.size(), &colors[0], GL_STATIC_DRAW);
	glVertexAttribPointer(PGL_ATTR_COLOR, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(PGL_ATTR_COLOR);

	GLuint elem_buf;
	glGenBuffers(1, &elem_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*strip_elems.size(), &strip_elems[0], GL_STATIC_DRAW);


	GLuint std_shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(std_shaders);
	glUseProgram(std_shaders[PGL_SHADER_SHADED]);

	pgl_uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	make_orthographic_matrix(the_uniforms.mvp_mat, 0, WIDTH-1, 0, HEIGHT-1, 1, -1);

	glClear(GL_COLOR_BUFFER_BIT);

	if (!argc)
		glMultiDrawArrays(GL_TRIANGLE_STRIP, &firsts[0], &counts[0], 300);
	else {
		glMultiDrawElements(GL_TRIANGLE_STRIP, &counts[0], GL_UNSIGNED_INT, (GLvoid* const*)&first_elems[0], 625);
	}
}
