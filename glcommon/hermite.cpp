#include "hermite.h"





hermite::hermite()
{
	float m[16] = { 2, -2, 1, 1,
					-3, 3, -2, -1,
					0, 0, 1, 0,
					1, 0, 0, 0	};
	
	memcpy(hermite_matrix.matrix, m, sizeof(float)*16);
}



void hermite::reset(int num_controls, int segments, vec3 min, vec3 max, float tan_weight)
{
	srand(time(NULL));
	
	controls.clear();
	tangents.clear();
	glpoints.clear();
	coefficients.clear();
	
	float xrange = max.x-min.x;
	float yrange = max.y-min.y;
	float zrange = max.z-min.z;

	vec3 tempvec;
	
	for(int i=0; i<num_controls; i++) {
		//make random control points
		tempvec.x = (float(rand())/float(RAND_MAX-1))* xrange + min.x;
		tempvec.y = (float(rand())/float(RAND_MAX-1))* yrange + min.y;
		tempvec.z = (float(rand())/float(RAND_MAX-1))* zrange + min.z;
				
				
		controls.push_back(tempvec);

		//make tangents
		tempvec.x = (float(rand())/float(RAND_MAX-1));
		tempvec.y = (float(rand())/float(RAND_MAX-1));
		tempvec.z = (float(rand())/float(RAND_MAX-1));

		tempvec = tempvec.norm();

		tempvec *= tan_weight;


		tangents.push_back(tempvec);
		tangent_lines.push_back(controls[i]);
		tangent_lines.push_back(controls[i] + tangents[i]*0.1);
	}

	float a = float(1)/segments;

	vec3 temp;

	for(int i=0; i<controls.size()-1; i++) {
		for(int j=0; j<segments+1; j++) {

			P_u(j*a, i, temp);
			glpoints.push_back(temp);
		}
	}
}



//just generate the visual of the hermite (line segments and tangents)
void hermite::reset(int segments)
{
	srand(time(NULL));
	
	glpoints.clear();
	tangent_lines.clear();
	coefficients.clear();
	
	for(int i=0; i<controls.size(); i++) {
		//make random control points
		tangent_lines.push_back(controls[i]);
		tangent_lines.push_back(controls[i] + tangents[i]*0.1);
	}

	float a = float(1)/segments;

	vec3 temp;

	for(int i=0; i<controls.size()-1; i++) {
		for(int j=0; j<segments+1; j++) {

			P_u(j*a, i, temp);
			glpoints.push_back(temp);
		}
	}
}





void hermite::P_u(float u, int k, vec3 &pu)
{
	vec4 U(pow(u, 3), pow(u, 2), u, 1);
	
	if( coefficients.size() == k*4 ) {
		vec3 a, b, c, d;

		vec4 colx(controls[k].x, controls[k+1].x, tangents[k].x, tangents[k+1].x);
		vec4 coly(controls[k].y, controls[k+1].y, tangents[k].y, tangents[k+1].y);
		vec4 colz(controls[k].z, controls[k+1].z, tangents[k].z, tangents[k+1].z);

		a.x = hermite_matrix.x()*colx;
		a.y = hermite_matrix.x()*coly;
		a.z = hermite_matrix.x()*colz;

		b.x = hermite_matrix.y()*colx;
		b.y = hermite_matrix.y()*coly;
		b.z = hermite_matrix.y()*colz;
		
		c.x = hermite_matrix.z()*colx;
		c.y = hermite_matrix.z()*coly;
		c.z = hermite_matrix.z()*colz;
		
		d.x = hermite_matrix.w()*colx;
		d.y = hermite_matrix.w()*coly;
		d.z = hermite_matrix.w()*colz;

		coefficients.push_back(a);
		coefficients.push_back(b);
		coefficients.push_back(c);
		coefficients.push_back(d);
	}


	pu.x = U.x*coefficients[k*4].x + U.y*coefficients[k*4+1].x + U.z*coefficients[k*4+2].x + U.w*coefficients[k*4+3].x;
	pu.y = U.x*coefficients[k*4].y + U.y*coefficients[k*4+1].y + U.z*coefficients[k*4+2].y + U.w*coefficients[k*4+3].y;
	pu.z = U.x*coefficients[k*4].z + U.y*coefficients[k*4+1].z + U.z*coefficients[k*4+2].z + U.w*coefficients[k*4+3].z;

	return;
}


void hermite::get_tangent(float u, int k, vec3 &tan)
{
	vec4 U(3*pow(u, 2), 2*u, 1, 0);

	tan.x = U.x*coefficients[k*4].x + U.y*coefficients[k*4+1].x + U.z*coefficients[k*4+2].x + U.w*coefficients[k*4+3].x;
	tan.y = U.x*coefficients[k*4].y + U.y*coefficients[k*4+1].y + U.z*coefficients[k*4+2].y + U.w*coefficients[k*4+3].y;
	tan.z = U.x*coefficients[k*4].z + U.y*coefficients[k*4+1].z + U.z*coefficients[k*4+2].z + U.w*coefficients[k*4+3].z;

	tan = tan.norm();


	return;
}


void hermite::end()
{
	GLfloat sizes[2];
	GLfloat step;

	glGetFloatv(GL_POINT_SIZE_RANGE, sizes);
	glGetFloatv(GL_POINT_SIZE_GRANULARITY, &step);

	glGenBuffers(3, buffer_objects);

	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);

	glBindBuffer(GL_ARRAY_BUFFER, buffer_objects[0]);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*controls.size()*3, &controls[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	
	glGenVertexArrays(1, &line);
	glBindVertexArray(line);

	glBindBuffer(GL_ARRAY_BUFFER, buffer_objects[1]);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*glpoints.size()*3, &glpoints[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	glGenVertexArrays(1, &tangent_array);
	glBindVertexArray(tangent_array);

	glBindBuffer(GL_ARRAY_BUFFER, buffer_objects[2]);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*tangent_lines.size()*3, &tangent_lines[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


	glBindVertexArray(0);
	return;
}




void hermite::draw(mat4 mvp, int shader)
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
	glBindVertexArray(vertex_array);

	glPointSize(7.0f);
	glDrawArrays(GL_POINTS, 0, controls.size());
	glBindVertexArray(0);

	glBindVertexArray(tangent_array);
	glUniform4fv(locColor, 1, (GLfloat*)&green.x);
	glLineWidth(3.0f);

	glDrawArrays(GL_LINES, 0, tangent_lines.size());
	glBindVertexArray(0);

	glUseProgram(old_program);
	glLineWidth(1.0f);
	glPointSize(2.0f);

	return;
}

