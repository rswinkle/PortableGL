#include <gltools.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>



#ifndef ROW_MAJOR
#define ROW_MAJOR GL_FALSE
#else
#define ROW_MAJOR GL_TRUE
#endif


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
		fprintf(stderr, "%d: %s\n\n", n, (!str)? "Errors cleared" : str);
}



int file_read(FILE* file, char** out)
{
	char* data;
	long size;

	//assert(out);

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	if (size <= 0) {
		if (size == -1)
			perror("ftell failure");
		fclose(file);
		return 0;
	}

	data = (char*)malloc(size+1);
	if (!data) {
		fclose(file);
		return 0;
	}

	rewind(file);
	if (!fread(data, size, 1, file)) {
		perror("fread failure");
		fclose(file);
		free(data);
		return 0;
	}

	data[size] = 0; /* null terminate in all cases even if reading binary data */

	*out = data;

	fclose(file);
	return size;
}

int file_open_read(const char* filename, const char* mode, char** out)
{
	FILE *file = fopen(filename, mode);
	if (!file)
		return 0;

	return file_read(file, out);
}


#define BUF_SIZE 1000

int link_program(GLuint program)
{
	glLinkProgram(program);
	int status = 0;
	char info_buf[BUF_SIZE];
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (GL_FALSE == status) {
		int len = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		if (len > 0) {
			int written = 0;
			glGetProgramInfoLog(program, len, &written, info_buf);
			printf("Link failed:\n===============\n%s\n", info_buf);
		}
		return 0;
	}

	return program;
}

int compile_shader_str(GLuint shader, const char* shader_str)
{
	glShaderSource(shader, 1, &shader_str, NULL);
	glCompileShader(shader);

	int result;
	char shader_info_buf[BUF_SIZE];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (GL_FALSE == result) {
		int length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		if (length > 0 && length <= BUF_SIZE) {
			int written = 0;
			glGetShaderInfoLog(shader, BUF_SIZE, &written, shader_info_buf);

			printf("Compile failed:\n===============\n%s\n", shader_info_buf);
		}
		return 0;
	}
	return 1;
}

#undef BUF_SIZE

GLuint load_shader_pair(const char* vert_shader_src, const char* frag_shader_src)
{
	GLuint program, vert_shader, frag_shader;

	program = glCreateProgram();
	vert_shader = glCreateShader(GL_VERTEX_SHADER);
	frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

	if (!compile_shader_str(vert_shader, vert_shader_src))
		return 0;
	if (!compile_shader_str(frag_shader, frag_shader_src))
		return 0;

	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	return link_program(program);
}

GLuint load_shader_file_pair(const char* vert_file, const char* frag_file)
{
	char *vs_str, *fs_str;

	if (!file_open_read(vert_file, "r", &vs_str))
		return 0;
	if (!file_open_read(frag_file, "r", &fs_str)) {
		free(vs_str);
		return 0;
	}

	return load_shader_pair(vs_str, fs_str);
}

void set_uniform1i(GLuint program, const char* name, int val)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform1i(loc, val);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform2i(GLuint program, const char* name, int x, int y)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform2i(loc, x, y);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform3i(GLuint program, const char* name, int x, int y, int z)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform3i(loc, x, y, z);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform4i(GLuint program, const char* name, int x, int y, int z, int w)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform4i(loc, x, y, z, w);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform1f(GLuint program, const char* name, float val)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform1f(loc, val);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform2f(GLuint program, const char* name, float x, float y)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform2f(loc, x, y);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform3f(GLuint program, const char* name, float x, float y, float z)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform3f(loc, x, y, z);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform4f(GLuint program, const char* name, float x, float y, float z, float w)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform4f(loc, x, y, z, w);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform2fv(GLuint program, const char* name, GLfloat* v)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform2fv(loc, 1, v);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform3fv(GLuint program, const char* name, GLfloat* v)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform3fv(loc, 1, v);
	else
		printf("Uniform: %s not found.\n", name);
}

void set_uniform4fv(GLuint program, const char* name, GLfloat* v)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0)
		glUniform4fv(loc, 1, v);
	else
		printf("Uniform: %s not found.\n", name);
}


void set_uniform_mat4f(GLuint program, const char* name, GLfloat* mat)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0) {
		//TODO transpose if necessary
		glUniformMatrix4fv(loc, 1, ROW_MAJOR, mat);
	} else {
		printf("Uniform: %s not found.\n", name);
	}
}

void set_uniform_mat3f(GLuint program, const char* name, GLfloat* mat)
{
	int loc = glGetUniformLocation(program, name);
	if (loc >= 0) {
		//TODO transpose if necessary
		glUniformMatrix3fv(loc, 1, ROW_MAJOR, mat);
	} else {
		printf("Uniform: %s not found.\n", name);
	}
}












GLboolean load_texture2D(const char* filename, GLenum min_filter, GLenum mag_filter, GLenum wrap_mode, GLboolean flip)
{
	GLubyte* image = NULL;
	int w, h, n;
	if (!(image = stbi_load(filename, &w, &h, &n, 4))) {
		fprintf(stdout, "Error loading image %s: %s\n\n", filename, stbi_failure_reason());
		return GL_FALSE;
	}

	GLubyte *flipped = NULL;

	if (flip) {
		int rowsize = w*4;
	    int imgsize = rowsize*h;

		flipped = (GLubyte*)malloc(imgsize);
		if (!flipped) {
			stbi_image_free(image);
			return GL_FALSE;
		}

		for (int row=0; row<h; row++) {
			memcpy(flipped + (row * rowsize), (image + imgsize) - (rowsize + (rowsize * row)), rowsize);
		}

		free(image);
		image = flipped;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);


	//TODO add parameter?
	GLfloat green[4] = { 0.0, 1.0, 0.0, 0.5f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&green);


	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, w, h, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, image);


	if( min_filter == GL_LINEAR_MIPMAP_LINEAR ||
		min_filter == GL_LINEAR_MIPMAP_NEAREST ||
		min_filter == GL_NEAREST_MIPMAP_LINEAR ||
		min_filter == GL_NEAREST_MIPMAP_NEAREST)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	free(image);

	return GL_TRUE;
}

GLboolean load_texture_cubemap(const char* filename[], GLenum min_filter, GLenum mag_filter, GLboolean flip)
{
	GLubyte* image = NULL;
	int w, h, n;

	unsigned char *flipped = NULL;

	GLenum cube[6] =
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, mag_filter);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (int i=0; i<6; ++i) {
		if (!(image = stbi_load(filename[i], &w, &h, &n, 4))) {
			fprintf(stdout, "Error loading image %s\n\n", filename[i]);
			return GL_FALSE;
		}

		if (flip) {
			int rowsize = w*4;
	    	int imgsize = rowsize*h;

			flipped = (GLubyte*)malloc(imgsize);
			if (!flipped) {
				stbi_image_free(image);
				return GL_FALSE;
			}

			for (int row=0; row<h; row++) {
				memcpy(flipped + (row * rowsize), (image + imgsize) - (rowsize + (rowsize * row)), rowsize);
			}

			free(image);
			image = flipped;
		}

		glTexImage2D(cube[i], 0, GL_COMPRESSED_RGBA, w, h, 0,
		             GL_RGBA, GL_UNSIGNED_BYTE, image);

		
		if( min_filter == GL_LINEAR_MIPMAP_LINEAR ||
			min_filter == GL_LINEAR_MIPMAP_NEAREST ||
			min_filter == GL_NEAREST_MIPMAP_LINEAR ||
			min_filter == GL_NEAREST_MIPMAP_NEAREST)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}
		free(image);
	}


	return GL_TRUE;
}

