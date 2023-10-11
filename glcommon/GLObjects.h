#pragma once
#ifndef GLOBJECTS_H
#define GLOBJECTS_H

#include "portablegl.h"

struct Buffer
{
	GLuint buf;
	
	Buffer() : buf(0) {}
	Buffer(int generate) { glGenBuffers(1, &buf); }
	
	void gen() { glGenBuffers(1, &buf); }
	void bind(GLenum target) { glBindBuffer(target, buf); }
	void del() { glDeleteBuffers(1, &buf); }
};

/*
struct Texture
{
	GLuint tex;
	
	Texture() : tex(0) {}
	Texture(int generate) { glGenTextures(1, &tex); }
	~Texture() { glDeleteTextures(1, &tex); }
	
	//how do I handle different texture types?  parameters? separate classes?
	void gen() { glGenTextures(1, &tex); }
	void bind(GLenum target) { glBindTexture(target, tex); }
	void setup_wrap_filters(GLenum minFilter, GLenum magFilter, GLenum wrapMode)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

		if( minFilter == GL_LINEAR_MIPMAP_LINEAR ||
		    minFilter == GL_LINEAR_MIPMAP_NEAREST ||
		    minFilter == GL_NEAREST_MIPMAP_LINEAR ||
		    minFilter == GL_NEAREST_MIPMAP_NEAREST)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}

	void setup_data(byte* pixel_data, int w, int h, int alignment, GLenum format)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);	//I think

	
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, w, h, 0, 
					format, GL_UNSIGNED_BYTE, pixel_data);
	}


	


};

*/

struct Vertex_Array
{
	GLuint vao;
	
	Vertex_Array() : vao(0) {}
	Vertex_Array(int generate) { glGenVertexArrays(1, &vao); }

	void gen() { glGenVertexArrays(1, &vao); }
	void bind() { glBindVertexArray(vao); }
	void del() { glDeleteVertexArrays(1, &vao); }
};


//class framebuffer class renderbuffer







#endif
