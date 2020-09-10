/*
 * GLMatrixStack.h
 *
 *  Created on: Feb 25, 2011
 *      Author: robert
 */

#ifndef GLMATRIXSTACK_H_
#define GLMATRIXSTACK_H_


//#include <GLTools.h>
#include "rmath.h"
#include "GLFrame.h"

namespace rm = robert3dmath;
using rm::vec3;
using rm::vec4;
using rm::mat4;
using rm::mat3;


enum GLT_STACK_ERROR { GLT_STACK_NOERROR = 0, GLT_STACK_OVERFLOW, GLT_STACK_UNDERFLOW }; 

class GLMatrixStack
{
	public:
	GLMatrixStack(int iStackDepth = 64)
	{
		stackDepth = iStackDepth;
		pStack = new mat4[iStackDepth];	//constructor makes everyone identity matrix
		stackPointer = 0;
		lastError = GLT_STACK_NOERROR;
	}
	
	~GLMatrixStack(void) { delete [] pStack; }

	
	inline void LoadIdentity(void)
	{
		pStack[stackPointer] = mat4();	//should I make a matrix load identity function
	}
	
	inline void LoadMatrix(const mat4 mMatrix)
	{ 
		pStack[stackPointer] = mMatrix; //is this fast/right?
	}
        
    inline void LoadMatrix(GLFrame& frame)
	{
        LoadMatrix(frame.get_matrix());
	}
    
	//one of these days I should really try restrict keyword and see if it makes a significant difference
	inline void MultMatrix(const mat4 mMatrix)
	{
		pStack[stackPointer] = pStack[stackPointer]*mMatrix;

	}
        
    inline void MultMatrix(GLFrame& frame)
	{
        MultMatrix(frame.get_matrix());
	}
        				
	inline void PushMatrix(void)
	{
		if(stackPointer < (stackDepth-1)) {
			stackPointer++;
			pStack[stackPointer] = pStack[stackPointer-1];
		} else {
			lastError = GLT_STACK_OVERFLOW;
		}
	}
	
	inline void PopMatrix(void)
	{
		if(stackPointer > 0)
			stackPointer--;
		else
			lastError = GLT_STACK_UNDERFLOW;
	}
		
	void Scale(GLfloat x, GLfloat y, GLfloat z)
	{
		pStack[stackPointer] = pStack[stackPointer]*rm::ScaleMatrix44(x, y, z);
	}
		
		
	void Translate(GLfloat x, GLfloat y, GLfloat z)
	{
		pStack[stackPointer] = pStack[stackPointer]*rm::TranslationMatrix44(x, y, z);		
	}
        			
	void Rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
	{
		mat4 rotate;
		rotmat44f(rotate, vec3(x, y, z), angle);
		pStack[stackPointer] = pStack[stackPointer]*rotate;
	}
	
	
	// I've always wanted vector versions of these
	void Scalev(const vec3 vScale)
	{
		pStack[stackPointer] = pStack[stackPointer]*rm::ScaleMatrix44(vScale);
	}
		
	void Translatev(const vec3 vTranslate)
	{
		pStack[stackPointer] = pStack[stackPointer]*rm::TranslationMatrix44(vTranslate);	
	}
    
		
	void Rotatev(GLfloat angle, vec3 vAxis)
	{
		mat4 rotate;
		rotmat44f(rotate, vAxis, angle);
		pStack[stackPointer] = pStack[stackPointer]*rotate;
	}
		
	
	// I've also always wanted to be able to do this
	void PushMatrix(const mat4 mMatrix)
	{
	 	if(stackPointer < (stackDepth-1)) {
			stackPointer++;
			pStack[stackPointer] = mMatrix;
		} else {
			lastError = GLT_STACK_OVERFLOW;
		}
	}
		
    void PushMatrix(GLFrame& frame)
	{
		PushMatrix(frame.get_matrix());
	}
        
	// Two different ways to get the matrix
	const mat4& get_matrix(void) { return pStack[stackPointer]; }	//const!
	void get_matrix(mat4 mMatrix) { mMatrix = pStack[stackPointer]; }	//copy


	inline GLT_STACK_ERROR GetLastError(void) {
		GLT_STACK_ERROR retval = lastError;
		lastError = GLT_STACK_NOERROR;
		return retval; 
		}

	protected:
	GLT_STACK_ERROR		lastError;
	int					stackDepth;
	int					stackPointer;
	mat4		*pStack;
};






#endif /* GLMATRIXSTACK_H_ */
