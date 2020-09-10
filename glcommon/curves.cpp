#include "curves.h"
#include <iostream>


void bezier3D::end()
{
	glGenBuffers(2, buffer_objects);

	glGenVertexArrays(1, &gl_controls);
	glBindVertexArray(gl_controls);

	glBindBuffer(GL_ARRAY_BUFFER, buffer_objects[0]);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*controls.size()*3, &controls[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	glBindVertexArray(0);
	
	glGenVertexArrays(1, &line);
	glBindVertexArray(line);

	glBindBuffer(GL_ARRAY_BUFFER, buffer_objects[1]);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*glpoints.size()*3, &glpoints[0], GL_STATIC_DRAW);	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);
	return;
}



void bezier3D::draw(mat4 mvp, int shader, bool draw_controls)
{
	int old_program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &old_program);

	glUseProgram(shader);

	vec4 white(1, 1, 1);
	vec4 blue(0, 0, 1);
	vec4 green(0, 1, 0);

	GLint locColor = glGetUniformLocation(shader, "vColor");
	glUniform4fv(locColor, 1, (GLfloat*)&white.x);
	GLint locMVP = glGetUniformLocation(shader, "mvpMatrix");
	glUniformMatrix4fv(locMVP, 1, GL_TRUE, (GLfloat*)mvp.matrix);


	glBindVertexArray(0);
	glBindVertexArray(line);
	glLineWidth(2.0f);
	//glEnable(GL_LINE_SMOOTH);

	glDrawArrays(GL_LINE_STRIP, 0, glpoints.size());

	glUniform4fv(locColor, 1, (GLfloat*)&blue.x);

	glBindVertexArray(0);
	
	if (draw_controls) {
		glBindVertexArray(gl_controls);

		glPointSize(7.0f);
		glDrawArrays(GL_POINTS, 0, controls.size());
		
		glUniform4fv(locColor, 1, (GLfloat*)&green.x);
		//change color, draw line connecting control points?
		glDrawArrays(GL_LINE_STRIP, 0, controls.size());
		
		
		glBindVertexArray(0);
	}
	
	glUseProgram(old_program);
	glLineWidth(1.0f);
	glPointSize(2.0f);

	return;
}



void bezier3D::compute_glpoints(unsigned int segments)
{
	glpoints.clear();
	
	float a = 1/float(segments);
	
	for(int i=0; i<segments+1; ++i) {
		glpoints.push_back(de_casteljau(controls, i*a));
	}
}






//could just create a static structure of first 20 levels of pascals triangle or something
unsigned int fact(unsigned int n)
{
	unsigned int result = 1;
	while (n) {
		result *= n--;
	}
	return result;
}

// float fact(unsigned int n)
// {
// 	unsigned int result = 1;
// 	while (n) {
// 		result *= n--;
// 	}
// 	return float(result);
// }




float bernstein(unsigned int n, unsigned int i, float t)
{
	int coef = fact(n)/(fact(i)*fact(n-i));
	
	float tmp = pow(1-t, n-i)*pow(t, i);
	
	return coef*tmp;
}


//I meant to copy controls.  don't want a reference
vec3 de_casteljau(vector<vec3> controls, float t, vec3* tangent)
{
	int n = controls.size();
	vec3 *b = &controls[0];

	//std::cout<<"\nt = "<<t<<"\n";

	for(int r=1; r<=n; ++r) {
// 		for (int k=0; k<=n-r; ++k)
// 			std::cout<<b[k]<<" ";
// 		
// 		std::cout<<"\n\n";
		for(int i=0; i<n-r; ++i) {
			b[i] = (1-t)*b[i] + t*b[i+1];
		}
	}
	
	if (tangent)
		*tangent = b[1] - b[0];
	
	return (1-t)*b[0] + t*b[1];
}
