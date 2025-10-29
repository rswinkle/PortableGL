
#ifndef RSW_MATSTACK
#define RSW_MATSTACK

#include "rsw_math.h"

// Not sure I want to make this a dependency
// they just wrap a call to frame.get_matrix() anyway
//#include "rsw_glframe.h"

using rsw::vec3;
using rsw::vec4;
using rsw::mat4;
using rsw::mat3;


enum MATSTACK_ERROR { MATSTACK_NOERROR = 0, MATSTACK_OVERFLOW, MATSTACK_UNDERFLOW };

struct matrix_stack
{
	int capacity;
	mat4* stack;
	int top;
	MATSTACK_ERROR last_error;

	matrix_stack(int cap = 64)
	{
		capacity = cap;
		stack = new mat4[cap];
		top = 0;
		last_error = MATSTACK_NOERROR;
	}

	~matrix_stack(void) { delete []stack; }

	void load_identity()
	{
		stack[top] = mat4();     //should I make a matrix load identity function
	}

	void load_mat(const mat4 m)
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
	void mult_mat(const mat4 m)
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
		stack[top] = stack[top] * rsw::scale_mat4(x, y, z);
	}

	void translate(float x, float y, float z)
	{
		stack[top] = stack[top] * rsw::translation_mat4(x, y, z);
	}

	void rotate(float angle, float x, float y, float z)
	{
		mat4 rotate;
		rsw::load_rotation_mat4(rotate, vec3(x, y, z), angle);
		stack[top] = stack[top] * rotate;
	}


	// I've always wanted vector versions of these
	void scale(const vec3 v)
	{
		stack[top] = stack[top] * rsw::scale_mat4(v);
	}
	void translate(const vec3 v)

	{
		stack[top] = stack[top] * rsw::translation_mat4(v);
	}


	void rotate(float angle, vec3 v)
	{
		mat4 rotate;
		rsw::load_rotation_mat4(rotate, v, angle);
		stack[top] = stack[top] * rotate;
	}

	// TODO make stack a vector?  resizable?
	void push_mat(const mat4 m)
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
	const mat4& get_matrix() { return stack[top]; } //const!
	void get_matrix(mat4 m) { m = stack[top]; }     //copy


	MATSTACK_ERROR GetLastError()
	{
		MATSTACK_ERROR ret = last_error;
		last_error = MATSTACK_NOERROR;
		return ret;
	}

};






#endif

