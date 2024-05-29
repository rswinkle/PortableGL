
void pglClearScreen(void);

//This isn't possible in regular OpenGL, changing the interpolation of vs output of
//an existing shader.  You'd have to switch between 2 almost identical shaders.
void pglSetInterp(GLsizei n, GLenum* interpolation);

#define pglVertexAttribPointer(index, size, type, normalized, stride, offset) \
glVertexAttribPointer(index, size, type, normalized, stride, (void*)(offset))

//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
void pglDrawFrame(void);

// TODO should these be called pglMapped* since that's what they do?  I don't think so, since it's too different from actual spec for mapped buffers
void pglBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage);
void pglTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);

void pglTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);

void pglTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data);

// I could make these return the data?
void pglGetBufferData(GLuint buffer, GLvoid** data);
void pglGetTextureData(GLuint texture, GLvoid** data);

u8* convert_format_to_packed_rgba(u8* output, u8* input, int w, int h, int pitch, GLenum format);
u8* convert_grayscale_to_rgba(u8* input, int size, u32 bg_rgba, u32 text_rgba);

void put_pixel(Color color, int x, int y);
void put_pixel_blend(vec4 src, int x, int y);

//Should I have it take a glFramebuffer as paramater?
void put_line(Color the_color, float x1, float y1, float x2, float y2);
void put_wide_line_simple(Color the_color, float width, float x1, float y1, float x2, float y2);
//void put_wide_line3(Color color1, Color color2, float width, float x1, float y1, float x2, float y2);

void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3);

void put_aa_line(vec4 c, float x1, float y1, float x2, float y2);
void put_aa_line_interp(vec4 c1, vec4 c2, float x1, float y1, float x2, float y2);

