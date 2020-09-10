#include "GLTools.h"
#include "rsw_math.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//#include <GLTriangleBatch.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#ifdef linux
#include <cstdlib>
#endif
#ifdef __APPLE__
#include <unistd.h>
#endif


using rsw::vec4;

/*
///////////////////////////////////////////////////////////////////////////////
// Get the OpenGL version number
void gltGetOpenGLVersion(GLint &nMajor, GLint &nMinor)
{
	#ifndef OPENGL_ES
	glGetIntegerv(GL_MAJOR_VERSION, &nMajor);
	glGetIntegerv(GL_MINOR_VERSION, &nMinor);
	#else
	const char *szVersionString = (const char *)glGetString(GL_VERSION);
	if(szVersionString == NULL)
		{
		nMajor = 0;
		nMinor = 0;
		return;
		}

	// Get major version number. This stops at the first non numeric character
	nMajor = atoi(szVersionString);

	// Get minor version number. Start past the first ".", atoi terminates on first non numeric char.
	nMinor = atoi(strstr(szVersionString, ".")+1);
	#endif
}


///////////////////////////////////////////////////////////////////////////////
// This function determines if the named OpenGL Extension is supported
// Returns 1 or 0
int gltIsExtSupported(const char *extension)
{
	#ifndef OPENGL_ES
	GLint nNumExtensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nNumExtensions);

	for(GLint i = 0; i < nNumExtensions; i++)
		if(strcmp(extension, (const char *)glGetStringi(GL_EXTENSIONS, i)) == 0)
		   return 1;
	#else
		GLubyte *extensions = NULL;
		const GLubyte *start;
		GLubyte *where, *terminator;

		where = (GLubyte *) strchr(extension, ' ');
		if (where || *extension == '\0')
			return 0;

		extensions = (GLubyte *)glGetString(GL_EXTENSIONS);

		start = extensions;
		for (;;)
		{
			where = (GLubyte *) strstr((const char *) start, extension);

			if (!where)
				break;

			terminator = where + strlen(extension);

			if (where == start || *(where - 1) == ' ')
			{
				if (*terminator == ' ' || *terminator == '\0')
					return 1;
			}
			start = terminator;
		}
	#endif
	return 0;
}




// Rather than malloc/free a block everytime a shader must be loaded,
// I will dedicate a single 4k page for reading in shaders. Thanks to
// modern OS design, this page will be swapped out to disk later if never
// used again after program initialization. Where-as mallocing different size
// shader blocks could lead to heap fragmentation, which would actually be worse.
//#define MAX_SHADER_LENGTH   8192  -> This is defined in gltools.h
// BTW... this does make this function unsafe to use in two threads simultaneously
// BTW BTW... personally.... I do this also to my own texture loading code - RSW
static GLubyte shaderText[MAX_SHADER_LENGTH];

//////////////////////////////////////////////////////////////////////////
// Load the shader from the source text
void gltLoadShaderSrc(const char *szShaderSrc, GLuint shader)
{
	GLchar *fsStringPtr[1];

	fsStringPtr[0] = (GLchar *)szShaderSrc;
	glShaderSource(shader, 1, (const GLchar **)fsStringPtr, NULL);
}


////////////////////////////////////////////////////////////////
// Load the shader from the specified file. Returns false if the
// shader could not be loaded
bool gltLoadShaderFile(const char *szFile, GLuint shader)
{
	GLint shaderLength = 0;
	FILE *fp;

	// Open the shader file
	fp = fopen(szFile, "r");
	if(fp != NULL)
		{
		// See how long the file is
		while (fgetc(fp) != EOF)
			shaderLength++;

		// Allocate a block of memory to send in the shader
		assert(shaderLength < MAX_SHADER_LENGTH);   // make me bigger!
		if(shaderLength > MAX_SHADER_LENGTH)
			{
			fclose(fp);
			return false;
			}

		// Go back to beginning of file
		rewind(fp);

		// Read the whole file in
		if (shaderText != NULL)
			fread(shaderText, 1, shaderLength, fp);

		// Make sure it is null terminated and close the file
		shaderText[shaderLength] = '\0';
		fclose(fp);
		}
	else
		return false;

	// Load the string
	gltLoadShaderSrc((const char *)shaderText, shader);

	return true;
}


/////////////////////////////////////////////////////////////////
// Load a pair of shaders, compile, and link together. Specify the complete
// source text for each shader. After the shader names, specify the number
// of attributes, followed by the index and attribute name of each attribute
GLuint gltLoadShaderPairWithAttributes(const char *szVertexProg, const char *szFragmentProg, ...)
	{
	// Temporary Shader objects
	GLuint hVertexShader;
	GLuint hFragmentShader;
	GLuint hReturn = 0;
	GLint testVal;

	// Create shader objects
	hVertexShader = glCreateShader(GL_VERTEX_SHADER);
	hFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load them. If fail clean up and return null
	// Vertex Program
	if(gltLoadShaderFile(szVertexProg, hVertexShader) == false)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		fprintf(stderr, "The shader at %s could ot be found.\n", szVertexProg);
		return (GLuint)NULL;
		}

	// Fragment Program
	if(gltLoadShaderFile(szFragmentProg, hFragmentShader) == false)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		fprintf(stderr,"The shader at %s  could not be found.\n", szFragmentProg);
		return (GLuint)NULL;
		}

	// Compile them both
	glCompileShader(hVertexShader);
	glCompileShader(hFragmentShader);

	// Check for errors in vertex shader
	glGetShaderiv(hVertexShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		char infoLog[1024];
		glGetShaderInfoLog(hVertexShader, 1024, NULL, infoLog);
		fprintf(stderr, "The shader at %s failed to compile with the following error:\n%s\n", szVertexProg, infoLog);
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	// Check for errors in fragment shader
	glGetShaderiv(hFragmentShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		char infoLog[1024];
		glGetShaderInfoLog(hFragmentShader, 1024, NULL, infoLog);
		fprintf(stderr, "The shader at %s failed to compile with the following error:\n%s\n", szFragmentProg, infoLog);
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	// Create the final program object, and attach the shaders
	hReturn = glCreateProgram();
	glAttachShader(hReturn, hVertexShader);
	glAttachShader(hReturn, hFragmentShader);


	// Now, we need to bind the attribute names to their specific locations
	// List of attributes
	va_list attributeList;
	va_start(attributeList, szFragmentProg);

	// Iterate over this argument list
	char *szNextArg;
	int iArgCount = va_arg(attributeList, int);	// Number of attributes
	for(int i = 0; i < iArgCount; i++)
		{
		int index = va_arg(attributeList, int);
		szNextArg = va_arg(attributeList, char*);
		glBindAttribLocation(hReturn, index, szNextArg);
		}
	va_end(attributeList);

	// Attempt to link
	glLinkProgram(hReturn);

	// These are no longer needed
	glDeleteShader(hVertexShader);
	glDeleteShader(hFragmentShader);

	// Make sure link worked too
	glGetProgramiv(hReturn, GL_LINK_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		char infoLog[1024];
		glGetProgramInfoLog(hReturn, 1024, NULL, infoLog);
		fprintf(stderr,"The programs %s and %s failed to link with the following errors:\n%s\n",
			szVertexProg, szFragmentProg, infoLog);
		glDeleteProgram(hReturn);
		return (GLuint)NULL;
		}

	// All done, return our ready to use shader program
	return hReturn;
	}

/////////////////////////////////////////////////////////////////
// Load a pair of shaders, compile, and link together. Specify the complete
// file path for each shader. Note, there is no support for
// just loading say a vertex program... you have to do both.
GLuint gltLoadShaderPair(const char *szVertexProg, const char *szFragmentProg)
	{
	// Temporary Shader objects
	GLuint hVertexShader;
	GLuint hFragmentShader;
	GLuint hReturn = 0;
	GLint testVal;

	// Create shader objects
	hVertexShader = glCreateShader(GL_VERTEX_SHADER);
	hFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load them. If fail clean up and return null
	if(gltLoadShaderFile(szVertexProg, hVertexShader) == false)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	if(gltLoadShaderFile(szFragmentProg, hFragmentShader) == false)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	// Compile them
	glCompileShader(hVertexShader);
	glCompileShader(hFragmentShader);

	// Check for errors
	glGetShaderiv(hVertexShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	glGetShaderiv(hFragmentShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	// Link them - assuming it works...
	hReturn = glCreateProgram();
	glAttachShader(hReturn, hVertexShader);
	glAttachShader(hReturn, hFragmentShader);

	glLinkProgram(hReturn);

	// These are no longer needed
	glDeleteShader(hVertexShader);
	glDeleteShader(hFragmentShader);

	// Make sure link worked too
	glGetProgramiv(hReturn, GL_LINK_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		glDeleteProgram(hReturn);
		return (GLuint)NULL;
		}

	return hReturn;
	}

/////////////////////////////////////////////////////////////////
// Load a pair of shaders, compile, and link together. Specify the complete
// file path for each shader. Note, there is no support for
// just loading say a vertex program... you have to do both.
GLuint gltLoadShaderPairSrc(const char *szVertexSrc, const char *szFragmentSrc)
	{
	// Temporary Shader objects
	GLuint hVertexShader;
	GLuint hFragmentShader;
	GLuint hReturn = 0;
	GLint testVal;

	// Create shader objects
	hVertexShader = glCreateShader(GL_VERTEX_SHADER);
	hFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load them.
	gltLoadShaderSrc(szVertexSrc, hVertexShader);
	gltLoadShaderSrc(szFragmentSrc, hFragmentShader);

	// Compile them
	glCompileShader(hVertexShader);
	glCompileShader(hFragmentShader);

	// Check for errors
	glGetShaderiv(hVertexShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	glGetShaderiv(hFragmentShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	// Link them - assuming it works...
	hReturn = glCreateProgram();
	glAttachShader(hReturn, hVertexShader);
	glAttachShader(hReturn, hFragmentShader);
	glLinkProgram(hReturn);

	// These are no longer needed
	glDeleteShader(hVertexShader);
	glDeleteShader(hFragmentShader);

	// Make sure link worked too
	glGetProgramiv(hReturn, GL_LINK_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		glDeleteProgram(hReturn);
		return (GLuint)NULL;
		}

	return hReturn;
	}

/////////////////////////////////////////////////////////////////
// Load a pair of shaders, compile, and link together. Specify the complete
// source code text for each shader. Note, there is no support for
// just loading say a vertex program... you have to do both.
GLuint gltLoadShaderPairSrcWithAttributes(const char *szVertexSrc, const char *szFragmentSrc, ...)
	{
	// Temporary Shader objects
	GLuint hVertexShader;
	GLuint hFragmentShader;
	GLuint hReturn = 0;
	GLint testVal;

	// Create shader objects
	hVertexShader = glCreateShader(GL_VERTEX_SHADER);
	hFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load them.
	gltLoadShaderSrc(szVertexSrc, hVertexShader);
	gltLoadShaderSrc(szFragmentSrc, hFragmentShader);

	// Compile them
	glCompileShader(hVertexShader);
	glCompileShader(hFragmentShader);

	// Check for errors
	glGetShaderiv(hVertexShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	glGetShaderiv(hFragmentShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
		}

	// Link them - assuming it works...
	hReturn = glCreateProgram();
	glAttachShader(hReturn, hVertexShader);
	glAttachShader(hReturn, hFragmentShader);

	// List of attributes
	va_list attributeList;
	va_start(attributeList, szFragmentSrc);

	char *szNextArg;
	int iArgCount = va_arg(attributeList, int);	// Number of attributes
	for(int i = 0; i < iArgCount; i++)
		{
		int index = va_arg(attributeList, int);
		szNextArg = va_arg(attributeList, char*);
		glBindAttribLocation(hReturn, index, szNextArg);
		}
	va_end(attributeList);


	glLinkProgram(hReturn);

	// These are no longer needed
	glDeleteShader(hVertexShader);
	glDeleteShader(hFragmentShader);

	// Make sure link worked too
	glGetProgramiv(hReturn, GL_LINK_STATUS, &testVal);
	if(testVal == GL_FALSE)
		{
		glDeleteProgram(hReturn);
		return (GLuint)NULL;
		}

	return hReturn;
	}




/////////////////////////////////////////////////////////////////
// Check for any GL errors that may affect rendering
// Check the framebuffer, the shader, and general errors
bool gltCheckErrors(GLuint progName)
{
	bool bFoundError = false;
	GLenum error = glGetError();

	if (error != GL_NO_ERROR)
	{
	    fprintf(stderr, "A GL Error has occured\n");
		bFoundError = true;
	}
#ifndef OPENGL_ES
	GLenum fboStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if(fboStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		bFoundError = true;
		fprintf(stderr,"The framebuffer is not complete - ");
		switch (fboStatus)
		{
		case GL_FRAMEBUFFER_UNDEFINED:
			// Oops, no window exists?
			fprintf(stderr, "GL_FRAMEBUFFER_UNDEFINED\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			// Check the status of each attachment
			fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			// Attach at least one buffer to the FBO
			fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			// Check that all attachments enabled via
			// glDrawBuffers exist in FBO
			fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			// Check that the buffer specified via
			// glReadBuffer exists in FBO
			fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			// Reconsider formats used for attached buffers
			fprintf(stderr, "GL_FRAMEBUFFER_UNSUPPORTED\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			// Make sure the number of samples for each
			// attachment is the same
			fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			// Make sure the number of layers for each
			// attachment is the same
			fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n");
			break;
		}
	}

#endif

	if (progName != 0)
	{
		glValidateProgram(progName);
		int iIsProgValid = 0;
		glGetProgramiv(progName, GL_VALIDATE_STATUS, &iIsProgValid);
		if(iIsProgValid == 0)
		{
			bFoundError = true;
			fprintf(stderr, "The current program(%d) is not valid\n", progName);
		}
	}
	return bFoundError;
}
*/

void check_errors(int n, const char* str)
{
	GLenum error;
	int err = 0;
	while ((error = glGetError()) != GL_NO_ERROR) {
		switch (error)
		{
		case GL_INVALID_ENUM:
			fprintf(stderr, "invalid enum\n");
			break;
		case GL_INVALID_VALUE:
			fprintf(stderr, "invalid value\n");
			break;
		case GL_INVALID_OPERATION:
			fprintf(stderr, "invalid operation\n");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			fprintf(stderr, "invalid framebuffer operation\n");
			break;
		case GL_OUT_OF_MEMORY:
			fprintf(stderr, "out of memory\n");
			break;
		default:
			fprintf(stderr, "wtf?\n");
		}
		err = 1;
	}
	if (err)
		fprintf(stderr, "%d: %s\n\n", n, str);
}

bool load_texture2D(const char* filename, GLenum minFilter, GLenum magFilter, GLenum wrapMode, bool flip)
{
	uint8* image = NULL;
	int w, h, n;
	if (!(image = stbi_load(filename, &w, &h, &n, 4)))
	{
		fprintf(stdout, "Error loading image %s: %s\n\n", filename, stbi_failure_reason());
		return false;
	}

	uint8 *flipped = NULL;

	if (flip) {
		int rowsize = w*4;
	    int imgsize = rowsize*h;

		flipped = (uint8*)malloc(imgsize);
		if (!flipped) {
			stbi_image_free(image);
			return false;
		}

		//memset(flipped, 0, imgsize);
		for (int row=0; row<h; row++) {
			memcpy(flipped + (row * rowsize), (image + imgsize) - (rowsize + (rowsize * row)), rowsize);
		}

		free(image);
		image = flipped;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	vec4 green(0, 1, 0, 0.5f);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&green);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, w, h, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, image);


	if( minFilter == GL_LINEAR_MIPMAP_LINEAR ||
		minFilter == GL_LINEAR_MIPMAP_NEAREST ||
		minFilter == GL_NEAREST_MIPMAP_LINEAR ||
		minFilter == GL_NEAREST_MIPMAP_NEAREST)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}


	free(image);

	return true;
}





bool load_texture_cubemap(const char* filename[], GLenum minFilter, GLenum magFilter, bool flip)
{
	uint8* image = NULL;
	int w, h, n;

	unsigned char *flipped = NULL;

	GLenum cube[6] = {	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
					GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
					GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
					GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
					GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
					GL_TEXTURE_CUBE_MAP_NEGATIVE_Z	};

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (int i=0; i<6; ++i) {
		if (!(image = stbi_load(filename[i], &w, &h, &n, 4)))
		{
			fprintf(stdout, "Error loading image %s\n\n", filename[i]);
			return false;
		}

		if (flip) {
			int rowsize = w*4;
	    	int imgsize = rowsize*h;

			flipped = (uint8*)malloc(imgsize);
			if (!flipped) {
				stbi_image_free(image);
				return false;
			}

			//memset(flipped, 0, imgsize);
			for (int row=0; row<h; row++) {
				memcpy(flipped + (row * rowsize), (image + imgsize) - (rowsize + (rowsize * row)), rowsize);
			}

			free(image);
			image = flipped;
		}

		glTexImage2D(cube[i], 0, GL_COMPRESSED_RGBA, w, h, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, image);

		if( minFilter == GL_LINEAR_MIPMAP_LINEAR ||
			minFilter == GL_LINEAR_MIPMAP_NEAREST ||
			minFilter == GL_NEAREST_MIPMAP_LINEAR ||
			minFilter == GL_NEAREST_MIPMAP_NEAREST)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
		free(image);
	}


	return true;
}












