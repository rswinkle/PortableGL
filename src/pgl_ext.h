
// Modeled after SDL for RenderGeometry
// Color c like SDL or vec4 c?
typedef struct pgl_vertex
{
	vec2 pos;
	Color color;
	vec2 tex_coord;
} pgl_vertex;


// TODO use ints like SDL or keep floats?
typedef struct pgl_fill_data
{
	vec2 dst;
	Color c;
} pgl_fill_data;

typedef struct pgl_copy_data
{
	vec2 src;
	vec2 dst;
	Color c;
} pgl_copy_data;

PGLDEF void pglClearScreen(void);

//This isn't possible in regular OpenGL, changing the interpolation of vs output of
//an existing shader.  You'd have to switch between 2 almost identical shaders.
PGLDEF void pglSetInterp(GLsizei n, GLenum* interpolation);

#define pglVertexAttribPointer(index, size, type, normalized, stride, offset) \
glVertexAttribPointer(index, size, type, normalized, stride, (void*)(offset))


PGLDEF GLuint pglCreateFragProgram(frag_func fragment_shader, GLboolean fragdepth_or_discard);

//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
PGLDEF void pglDrawFrame(void);
PGLDEF void pglDrawFrame2(frag_func frag_shader, void* uniforms);

// TODO should these be called pglMapped* since that's what they do?  I don't think so, since it's too different from actual spec for mapped buffers
PGLDEF void pglBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage);
PGLDEF void pglTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);

PGLDEF void pglTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);

PGLDEF void pglTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data);

// I could make these return the data?
PGLDEF void pglGetBufferData(GLuint buffer, GLvoid** data);
PGLDEF void pglGetTextureData(GLuint texture, GLvoid** data);

GLvoid* pglGetBackBuffer(void);
PGLDEF void pglSetBackBuffer(GLvoid* backbuf, GLsizei width, GLsizei height);

PGLDEF u8* convert_format_to_packed_rgba(u8* output, u8* input, int w, int h, int pitch, GLenum format);
PGLDEF u8* convert_grayscale_to_rgba(u8* input, int size, u32 bg_rgba, u32 text_rgba);

PGLDEF void put_pixel(Color color, int x, int y);
PGLDEF void put_pixel_blend(vec4 src, int x, int y);

//Should I have it take a glFramebuffer as paramater?
PGLDEF void put_line(Color the_color, float x1, float y1, float x2, float y2);
PGLDEF void put_wide_line_simple(Color the_color, float width, float x1, float y1, float x2, float y2);
PGLDEF void put_wide_line(Color color1, Color color2, float width, float x1, float y1, float x2, float y2);

PGLDEF void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3);
PGLDEF void put_triangle_tex(int tex, vec2 uv1, vec2 uv2, vec2 uv3, vec2 p1, vec2 p2, vec2 p3);
PGLDEF void pgl_draw_geometry_raw(int tex, const float* xy, int xy_stride, const Color* color, int color_stride, const float* uv, int uv_stride, int n_verts, const void* indices, int n_indices, int sz_indices);

PGLDEF void put_aa_line(vec4 c, float x1, float y1, float x2, float y2);
PGLDEF void put_aa_line_interp(vec4 c1, vec4 c2, float x1, float y1, float x2, float y2);

