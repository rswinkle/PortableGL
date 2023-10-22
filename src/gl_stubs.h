
// Stubs to let real OpenGL libs compile with minimal modifications/ifdefs
// add what you need

void glGenerateMipmap(GLenum target);
void glActiveTexture(GLenum texture);

void glGetDoublev(GLenum pname, GLdouble* params);
void glGetInteger64v(GLenum pname, GLint64* params);

// Framebuffers/Renderbuffers
void glGenFramebuffers(GLsizei n, GLuint* ids);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glDeleteFramebuffers(GLsizei n, GLuint* framebuffers);
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
GLboolean glIsFramebuffer(GLuint framebuffer);

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GLboolean glIsRenderbuffer(GLuint renderbuffer);

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GLenum glCheckFramebufferStatus(GLenum target);


void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void glAttachShader(GLuint program, GLuint shader);
void glCompileShader(GLuint shader);
void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);

// use pglCreateProgram()
GLuint glCreateProgram();

void glLinkProgram(GLuint program);
void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
GLuint glCreateShader(GLenum shaderType);
void glDeleteShader(GLuint shader);
void glDetachShader(GLuint program, GLuint shader);

GLint glGetUniformLocation(GLuint program, const GLchar* name);
GLint glGetAttribLocation(GLuint program, const GLchar* name);

GLboolean glUnmapBuffer(GLenum target);
GLboolean glUnmapNamedBuffer(GLuint buffer);

void glUniform1f(GLint location, GLfloat v0);
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

void glUniform1i(GLint location, GLint v0);
void glUniform2i(GLint location, GLint v0, GLint v1);
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

void glUniform1ui(GLuint location, GLuint v0);
void glUniform2ui(GLuint location, GLuint v0, GLuint v1);
void glUniform3ui(GLuint location, GLuint v0, GLuint v1, GLuint v2);
void glUniform4ui(GLuint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

void glUniform1fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);

void glUniform1iv(GLint location, GLsizei count, const GLint* value);
void glUniform2iv(GLint location, GLsizei count, const GLint* value);
void glUniform3iv(GLint location, GLsizei count, const GLint* value);
void glUniform4iv(GLint location, GLsizei count, const GLint* value);

void glUniform1uiv(GLint location, GLsizei count, const GLuint* value);
void glUniform2uiv(GLint location, GLsizei count, const GLuint* value);
void glUniform3uiv(GLint location, GLsizei count, const GLuint* value);
void glUniform4uiv(GLint location, GLsizei count, const GLuint* value);

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);



