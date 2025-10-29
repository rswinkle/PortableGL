#ifndef GLM_MATSTACK_H
#define GLM_MATSTACK_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Not sure I want to make this a dependency
// they just wrap a call to frame.get_matrix() anyway
//#include "glm_glframe.h"

enum MATSTACK_ERROR { MATSTACK_NOERROR = 0, MATSTACK_OVERFLOW, MATSTACK_UNDERFLOW };

struct matrix_stack
{
	int capacity;
	glm::mat4* stack;
	int top;
	MATSTACK_ERROR last_error;

	matrix_stack(int cap = 64)
	{
		capacity = cap;
		stack = new glm::mat4[cap];
		top = 0;
		last_error = MATSTACK_NOERROR;
	}

	~matrix_stack(void) { delete []stack; }

	void load_identity()
	{
		stack[top] = glm::mat4(1);   // (1) vs () vs identity()
	}

	void load_mat(const glm::mat4 m)
	{
		stack[top] = m;
	}

    /*
    void load_mat(GLFrame& frame)
	{
        load_mat(frame.get_matrix());
	}
	*/

	//one of these days I should really try restrict keyword and see if it makes a significant difference
	void multiply(const glm::mat4 m)
	{
		stack[top] = stack[top] * m;
	}

    /*
    void mult_mat(GLFrame& frame)
	{
        mult_mat(frame.get_matrix());
	}
	*/

	void push()
	{
		if (top < (capacity-1)) {
			top++;
			stack[top] = stack[top-1];
		} else {
			last_error = MATSTACK_OVERFLOW;
		}
	}

	void pop()
	{
		if (top > 0)
			top--;
		else
			last_error = MATSTACK_UNDERFLOW;
	}

	void scale(float x, float y, float z)
	{
		stack[top] = stack[top] * glm::scale(glm::mat4(1), glm::vec3(x, y, z));
	}

	void translate(float x, float y, float z)
	{
		stack[top] = stack[top] * glm::translate(glm::mat4(1), glm::vec3(x, y, z));
	}

	void rotate(float angle, float x, float y, float z)
	{
		stack[top] = stack[top] * glm::rotate(glm::mat4(1), angle, glm::vec3(x, y, z));
	}


	// I've always wanted vector versions of these
	void scale(const glm::vec3 v)
	{
		stack[top] = stack[top] * glm::scale(glm::mat4(1), v);
	}
	void translate(const glm::vec3 v)
	{
		stack[top] = stack[top] * glm::translate(glm::mat4(1), v);
	}


	void rotate(float angle, glm::vec3 v)
	{
		stack[top] = stack[top] * glm::rotate(glm::mat4(1), angle, v);
	}

	// TODO make stack a vector?  resizable?
	void push_mat(const glm::mat4 m)
	{
	 	if(top < (capacity-1)) {
			top++;
			stack[top] = m;
		} else {
			last_error = MATSTACK_OVERFLOW;
		}
	}

	/*
    void push_mat(GLFrame& frame)
	{
		push_mat(frame.get_matrix());
	}
	*/

	// Two different ways to get the matrix
	const glm::mat4& get_matrix() { return stack[top]; } //const!

	// TODO why is this not a reference for out? mistake in porting?
	void get_matrix(glm::mat4 m) { m = stack[top]; }     //copy


	MATSTACK_ERROR GetLastError()
	{
		MATSTACK_ERROR ret = last_error;
		last_error = MATSTACK_NOERROR;
		return ret;
	}

};






#endif


