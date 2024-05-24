
#ifndef PGL_EXCLUDE_STUBS

// Stubs to let real OpenGL libs compile with minimal modifications/ifdefs
// add what you need

const GLubyte* glGetStringi(GLenum name, GLuint index) { return NULL; }

void glColorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}

void glGenerateMipmap(GLenum target)
{
	//TODO not implemented, not sure it's worth it.
	//For example mipmap generation code see
	//https://github.com/thebeast33/cro_lib/blob/master/cro_mipmap.h
}

void glGetDoublev(GLenum pname, GLdouble* params) { }
void glGetInteger64v(GLenum pname, GLint64* params) { }

// Drawbuffers
void glDrawBuffers(GLsizei n, const GLenum* bufs) {}
void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum* bufs) {}

// Framebuffers/Renderbuffers
void glGenFramebuffers(GLsizei n, GLuint* ids) {}
void glBindFramebuffer(GLenum target, GLuint framebuffer) {}
void glDeleteFramebuffers(GLsizei n, GLuint* framebuffers) {}
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {}
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) {}
GLboolean glIsFramebuffer(GLuint framebuffer) { return GL_FALSE; }

void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {}
void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) {}

void glReadBuffer(GLenum mode) {}
void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode) {}

void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}
void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}

void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {}
void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {}
void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) {}
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {}
GLboolean glIsRenderbuffer(GLuint renderbuffer) { return GL_FALSE; }

void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}
// Could also return GL_FRAMEBUFFER_UNDEFINED, but then I'd have to add all
// those enums and really 0 signaling an error makes more sense
GLenum glCheckFramebufferStatus(GLenum target) { return 0; }

void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value) {}
void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value) {}
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value) {}
void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {}
void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value) {}
void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value) {}
void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value) {}
void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {}

void glGetProgramiv(GLuint program, GLenum pname, GLint* params) { }
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog) { }
void glAttachShader(GLuint program, GLuint shader) { }
void glCompileShader(GLuint shader) { }
void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog) { }
void glLinkProgram(GLuint program) { }
void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length) { }
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) { }
void glDeleteShader(GLuint shader) { }
void glDetachShader(GLuint program, GLuint shader) { }

GLuint glCreateProgram(void) { return 0; }
GLuint glCreateShader(GLenum shaderType) { return 0; }
GLint glGetUniformLocation(GLuint program, const GLchar* name) { return 0; }
GLint glGetAttribLocation(GLuint program, const GLchar* name) { return 0; }

GLboolean glUnmapBuffer(GLenum target) { return GL_TRUE; }
GLboolean glUnmapNamedBuffer(GLuint buffer) { return GL_TRUE; }

// TODO

void glActiveTexture(GLenum texture) { }
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {}
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) {}
void glTexParameteriv(GLenum target, GLenum pname, const GLint* params) {}

void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {}
void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* params) {}
void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* params) {}

// TODO what the heck are these?
void glTexParameterliv(GLenum target, GLenum pname, const GLint* params) {}
void glTexParameterluiv(GLenum target, GLenum pname, const GLuint* params) {}

void glTextureParameterliv(GLuint texture, GLenum pname, const GLint* params) {}
void glTextureParameterluiv(GLuint texture, GLenum pname, const GLuint* params) {}

void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data) {}
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data) {}
void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data) {}

void glUniform1f(GLint location, GLfloat v0) { }
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) { }
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) { }
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) { }

void glUniform1i(GLint location, GLint v0) { }
void glUniform2i(GLint location, GLint v0, GLint v1) { }
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) { }
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) { }

void glUniform1ui(GLuint location, GLuint v0) { }
void glUniform2ui(GLuint location, GLuint v0, GLuint v1) { }
void glUniform3ui(GLuint location, GLuint v0, GLuint v1, GLuint v2) { }
void glUniform4ui(GLuint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) { }

void glUniform1fv(GLint location, GLsizei count, const GLfloat* value) { }
void glUniform2fv(GLint location, GLsizei count, const GLfloat* value) { }
void glUniform3fv(GLint location, GLsizei count, const GLfloat* value) { }
void glUniform4fv(GLint location, GLsizei count, const GLfloat* value) { }

void glUniform1iv(GLint location, GLsizei count, const GLint* value) { }
void glUniform2iv(GLint location, GLsizei count, const GLint* value) { }
void glUniform3iv(GLint location, GLsizei count, const GLint* value) { }
void glUniform4iv(GLint location, GLsizei count, const GLint* value) { }

void glUniform1uiv(GLint location, GLsizei count, const GLuint* value) { }
void glUniform2uiv(GLint location, GLsizei count, const GLuint* value) { }
void glUniform3uiv(GLint location, GLsizei count, const GLuint* value) { }
void glUniform4uiv(GLint location, GLsizei count, const GLuint* value) { }

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }

#endif
