
void pglClearScreen();

//This isn't possible in regular OpenGL, changing the interpolation of vs output of
//an existing shader.  You'd have to switch between 2 almost identical shaders.
void pglSetInterp(GLsizei n, GLenum* interpolation);

#define pglVertexAttribPointer(index, size, type, normalized, stride, offset) \
glVertexAttribPointer(index, size, type, normalized, stride, (void*)(offset))

//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
void pglDrawFrame();

// TODO should these be called pglMapped* since that's what they do?  I don't think so, since it's too different from actual spec for mapped buffers
void pglBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage);
void pglTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);

void pglTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);

void pglTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data);

// I could make these return the data?
void pglGetBufferData(GLuint buffer, GLvoid** data);
void pglGetTextureData(GLuint texture, GLvoid** data);


void put_pixel(Color color, int x, int y);

//Should I have it take a glFramebuffer as paramater?
int put_line(Color the_color, float x1, float y1, float x2, float y2);
void put_wide_line_simple(Color the_color, float width, float x1, float y1, float x2, float y2);
void put_wide_line2(Color the_color, float width, float x1, float y1, float x2, float y2);

void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3);

