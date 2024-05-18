

#include <stdarg.h>


/******************************************
 * PORTABLEGL_IMPLEMENTATION
 ******************************************/

#include <stdio.h>
#include <float.h>

// for CHAR_BIT
#include <limits.h>



#ifdef DEBUG
#define IS_VALID(target, error, ...) is_valid(target, error, __VA_ARGS__)
#else
#define IS_VALID(target, error, ...) 1
#endif

int is_valid(GLenum target, GLenum error, int n, ...)
{
	va_list argptr;

	va_start(argptr, n);
	for (int i=0; i<n; ++i) {
		if (target == va_arg(argptr, GLenum)) {
			return 1;
		}
	}
	va_end(argptr);

	if (!c->error) {
		c->error = error;
	}
	return 0;
}


// I just set everything even if not everything applies to the type
// see section 3.8.15 pg 181 of spec for what it's supposed to be
// TODO better name and inline?
void INIT_TEX(glTexture* tex, GLenum target)
{
	tex->type = target;
	tex->mag_filter = GL_LINEAR;
	if (target != GL_TEXTURE_RECTANGLE) {
		//tex->min_filter = GL_NEAREST_MIPMAP_LINEAR;
		tex->min_filter = GL_NEAREST;
		tex->wrap_s = GL_REPEAT;
		tex->wrap_t = GL_REPEAT;
		tex->wrap_r = GL_REPEAT;
	} else {
		tex->min_filter = GL_LINEAR;
		tex->wrap_s = GL_CLAMP_TO_EDGE;
		tex->wrap_t = GL_CLAMP_TO_EDGE;
		tex->wrap_r = GL_CLAMP_TO_EDGE;
	}
	tex->data = NULL;
	tex->deleted = GL_FALSE;
	tex->user_owned = GL_TRUE;
	tex->format = GL_RGBA;
	tex->w = 0;
	tex->h = 0;
	tex->d = 0;
}

// default pass through shaders for index 0
void default_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = vertex_attribs[PGL_ATTR_VERT];
}

void default_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec4* fragcolor = &builtins->gl_FragColor;
	//wish I could use a compound literal, stupid C++ compatibility
	fragcolor->x = 1.0f;
	fragcolor->y = 0.0f;
	fragcolor->z = 0.0f;
	fragcolor->w = 1.0f;
}


void init_glVertex_Array(glVertex_Array* v)
{
	v->deleted = GL_FALSE;
	for (int i=0; i<GL_MAX_VERTEX_ATTRIBS; ++i)
		init_glVertex_Attrib(&v->vertex_attribs[i]);
}

void init_glVertex_Attrib(glVertex_Attrib* v)
{
	/*
	GLint size;      // number of components 1-4
	GLenum type;     // GL_FLOAT, default
	GLsizei stride;  //
	GLsizei offset;  //
	GLboolean normalized;
	unsigned int buf;
	GLboolean enabled;
	GLuint divisor;
*/
	v->buf = 0;
	v->enabled = 0;
	v->divisor = 0;
}


#define GET_SHIFT(mask, shift) \
	do {\
	shift = 0;\
	while ((mask & 1) == 0) {\
		mask >>= 1;\
		++shift;\
	}\
	} while (0)


int init_glContext(glContext* context, u32** back, int w, int h, int bitdepth, u32 Rmask, u32 Gmask, u32 Bmask, u32 Amask)
{
	if (bitdepth > 32 || !back)
		return 0;

	c = context;

	c->user_alloced_backbuf = *back != NULL;
	if (!*back) {
		int bytes_per_pixel = (bitdepth + CHAR_BIT-1) / CHAR_BIT;
		*back = (u32*)PGL_MALLOC(w * h * bytes_per_pixel);
		if (!*back)
			return 0;
	}

	c->zbuf.buf = (u8*)PGL_MALLOC(w*h * sizeof(float));
	if (!c->zbuf.buf) {
		if (!c->user_alloced_backbuf) {
			PGL_FREE(*back);
			*back = NULL;
		}
		return 0;
	}

	c->stencil_buf.buf = (u8*)PGL_MALLOC(w*h);
	if (!c->stencil_buf.buf) {
		if (!c->user_alloced_backbuf) {
			PGL_FREE(*back);
			*back = NULL;
		}
		PGL_FREE(c->zbuf.buf);
		return 0;
	}

	c->xmin = 0;
	c->ymin = 0;
	c->width = w;
	c->height = h;

	c->lx = 0;
	c->ly = 0;
	c->ux = w;
	c->uy = h;

	c->zbuf.w = w;
	c->zbuf.h = h;
	c->zbuf.lastrow = c->zbuf.buf + (h-1)*w*sizeof(float);

	c->stencil_buf.w = w;
	c->stencil_buf.h = h;
	c->stencil_buf.lastrow = c->stencil_buf.buf + (h-1)*w;

	c->back_buffer.w = w;
	c->back_buffer.h = h;
	c->back_buffer.buf = (u8*) *back;
	c->back_buffer.lastrow = c->back_buffer.buf + (h-1)*w*sizeof(u32);

	c->bitdepth = bitdepth; //not used yet
	c->Rmask = Rmask;
	c->Gmask = Gmask;
	c->Bmask = Bmask;
	c->Amask = Amask;

	c->red_mask = GL_TRUE;
	c->green_mask = GL_TRUE;
	c->blue_mask = GL_TRUE;
	c->alpha_mask = GL_TRUE;
	c->color_mask = Rmask | Gmask | Bmask | Amask;

	GET_SHIFT(Rmask, c->Rshift);
	GET_SHIFT(Gmask, c->Gshift);
	GET_SHIFT(Bmask, c->Bshift);
	GET_SHIFT(Amask, c->Ashift);

	//initialize all vectors
	cvec_glVertex_Array(&c->vertex_arrays, 0, 3);
	cvec_glBuffer(&c->buffers, 0, 3);
	cvec_glProgram(&c->programs, 0, 3);
	cvec_glTexture(&c->textures, 0, 1);
	cvec_glVertex(&c->glverts, 0, 10);

	//TODO might as well just set it to PGL_MAX_VERTICES * MAX_OUTPUT_COMPONENTS
	cvec_float(&c->vs_output.output_buf, 0, 0);


	c->clear_stencil = 0;
	c->clear_color = make_Color(0, 0, 0, 0);
	SET_VEC4(c->blend_color, 0, 0, 0, 0);
	c->point_size = 1.0f;
	c->line_width = 1.0f;
	c->clear_depth = 1.0f;
	c->depth_range_near = 0.0f;
	c->depth_range_far = 1.0f;
	make_viewport_matrix(c->vp_mat, 0, 0, w, h, 1);


	//set flags
	//TODO match order in structure definition
	c->provoking_vert = GL_LAST_VERTEX_CONVENTION;
	c->cull_mode = GL_BACK;
	c->cull_face = GL_FALSE;
	c->front_face = GL_CCW;
	c->depth_test = GL_FALSE;
	c->fragdepth_or_discard = GL_FALSE;
	c->depth_clamp = GL_FALSE;
	c->depth_mask = GL_TRUE;
	c->blend = GL_FALSE;
	c->logic_ops = GL_FALSE;
	c->poly_offset_pt = GL_FALSE;
	c->poly_offset_line = GL_FALSE;
	c->poly_offset_fill = GL_FALSE;
	c->scissor_test = GL_FALSE;

	c->stencil_test = GL_FALSE;
	c->stencil_writemask = -1; // all 1s for the masks
	c->stencil_writemask_back = -1;
	c->stencil_ref = 0;
	c->stencil_ref_back = 0;
	c->stencil_valuemask = -1;
	c->stencil_valuemask_back = -1;
	c->stencil_func = GL_ALWAYS;
	c->stencil_func_back = GL_ALWAYS;
	c->stencil_sfail = GL_KEEP;
	c->stencil_dpfail = GL_KEEP;
	c->stencil_dppass = GL_KEEP;
	c->stencil_sfail_back = GL_KEEP;
	c->stencil_dpfail_back = GL_KEEP;
	c->stencil_dppass_back = GL_KEEP;

	c->logic_func = GL_COPY;
	c->blend_sRGB = GL_ONE;
	c->blend_sA = GL_ONE;
	c->blend_dRGB = GL_ZERO;
	c->blend_dA = GL_ZERO;
	c->blend_eqRGB = GL_FUNC_ADD;
	c->blend_eqA = GL_FUNC_ADD;
	c->depth_func = GL_LESS;
	c->line_smooth = GL_FALSE;
	c->poly_mode_front = GL_FILL;
	c->poly_mode_back = GL_FILL;
	c->point_spr_origin = GL_UPPER_LEFT;

	c->poly_factor = 0.0f;
	c->poly_units = 0.0f;

	c->scissor_lx = 0;
	c->scissor_ly = 0;
	c->scissor_w = w;
	c->scissor_h = h;

	// According to refpages https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glPixelStore.xhtml
	c->unpack_alignment = 4;
	c->pack_alignment = 4;

	c->draw_triangle_front = draw_triangle_fill;
	c->draw_triangle_back = draw_triangle_fill;

	c->error = GL_NO_ERROR;

	//program 0 is supposed to be undefined but not invalid so I'll
	//just make it default, no transform, just draws things red
	glProgram tmp_prog = { default_vs, default_fs, NULL, 0, {0}, GL_FALSE };
	cvec_push_glProgram(&c->programs, tmp_prog);
	glUseProgram(0);

	//setup default vertex_array (vao) at position 0
	//we're like a compatibility profile for this but come on
	//no reason not to have this imo
	//https://www.opengl.org/wiki/Vertex_Specification#Vertex_Array_Object
	glVertex_Array tmp_va;
	init_glVertex_Array(&tmp_va);
	cvec_push_glVertex_Array(&c->vertex_arrays, tmp_va);
	c->cur_vertex_array = 0;

	// buffer 0 is invalid
	glBuffer tmp_buf = {0};
	tmp_buf.user_owned = GL_TRUE;
	tmp_buf.deleted = GL_FALSE;
	cvec_push_glBuffer(&c->buffers, tmp_buf);

	// texture 0 is valid/default
	glTexture tmp_tex;
	INIT_TEX(&tmp_tex, GL_TEXTURE_UNBOUND);
	cvec_push_glTexture(&c->textures, tmp_tex);

	memset(c->bound_buffers, 0, sizeof(c->bound_buffers));
	memset(c->bound_textures, 0, sizeof(c->bound_textures));

	return 1;
}

void free_glContext(glContext* ctx)
{
	int i;
	PGL_FREE(ctx->zbuf.buf);
	PGL_FREE(ctx->stencil_buf.buf);
	if (!ctx->user_alloced_backbuf) {
		PGL_FREE(ctx->back_buffer.buf);
	}

	for (i=0; i<ctx->buffers.size; ++i) {
		if (!ctx->buffers.a[i].user_owned) {
			PGL_FREE(ctx->buffers.a[i].data);
		}
	}

	for (i=0; i<ctx->textures.size; ++i) {
		if (!ctx->textures.a[i].user_owned) {
			PGL_FREE(ctx->textures.a[i].data);
		}
	}

	//free vectors
	cvec_free_glVertex_Array(&ctx->vertex_arrays);
	cvec_free_glBuffer(&ctx->buffers);
	cvec_free_glProgram(&ctx->programs);
	cvec_free_glTexture(&ctx->textures);
	cvec_free_glVertex(&ctx->glverts);

	cvec_free_float(&ctx->vs_output.output_buf);

	if (c == ctx) {
		c = NULL;
	}
}

void set_glContext(glContext* context)
{
	c = context;
}

void* pglResizeFramebuffer(size_t w, size_t h)
{
	u8* tmp;
	tmp = (u8*)PGL_REALLOC(c->zbuf.buf, w*h * sizeof(float));
	if (!tmp) {
		if (!c->error)
			c->error = GL_OUT_OF_MEMORY;
		return NULL;
	}
	c->zbuf.buf = tmp;
	c->zbuf.w = w;
	c->zbuf.h = h;
	c->zbuf.lastrow = c->zbuf.buf + (h-1)*w*sizeof(float);

	tmp = (u8*)PGL_REALLOC(c->back_buffer.buf, w*h * sizeof(u32));
	if (!tmp) {
		if (!c->error)
			c->error = GL_OUT_OF_MEMORY;
		return NULL;
	}
	c->back_buffer.buf = tmp;
	c->back_buffer.w = w;
	c->back_buffer.h = h;
	c->back_buffer.lastrow = c->back_buffer.buf + (h-1)*w*sizeof(u32);

	if (c->scissor_test) {
		int ux = c->scissor_lx+c->scissor_w;
		int uy = c->scissor_ly+c->scissor_h;

		c->lx = MAX(c->scissor_lx, 0);
		c->ly = MAX(c->scissor_ly, 0);
		c->ux = MIN(ux, w);
		c->uy = MIN(uy, h);
	} else {
		c->lx = 0;
		c->ly = 0;
		c->ux = w;
		c->uy = h;
	}

	return tmp;
}



GLubyte* glGetString(GLenum name)
{
	static GLubyte vendor[] = "Robert Winkler (robertwinkler.com)";
	static GLubyte renderer[] = "PortableGL 0.98.0";
	static GLubyte version[] = "0.98.0";
	static GLubyte shading_language[] = "C/C++";

	switch (name) {
	case GL_VENDOR:                   return vendor;
	case GL_RENDERER:                 return renderer;
	case GL_VERSION:                  return version;
	case GL_SHADING_LANGUAGE_VERSION: return shading_language;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return 0;
	}
}

GLenum glGetError()
{
	GLenum err = c->error;
	c->error = GL_NO_ERROR;
	return err;
}

void glGenVertexArrays(GLsizei n, GLuint* arrays)
{
	glVertex_Array tmp = {0};
	//init_glVertex_Array(&tmp);

	tmp.deleted = GL_FALSE;

	//fill up empty slots first
	--n;
	for (int i=1; i<c->vertex_arrays.size && n>=0; ++i) {
		if (c->vertex_arrays.a[i].deleted) {
			c->vertex_arrays.a[i] = tmp;
			arrays[n--] = i;
		}
	}

	for (; n>=0; --n) {
		cvec_push_glVertex_Array(&c->vertex_arrays, tmp);
		arrays[n] = c->vertex_arrays.size-1;
	}
}

void glDeleteVertexArrays(GLsizei n, const GLuint* arrays)
{
	for (int i=0; i<n; ++i) {
		if (!arrays[i] || arrays[i] >= c->vertex_arrays.size)
			continue;

		if (arrays[i] == c->cur_vertex_array) {
			//TODO check if memcpy isn't enough
			memcpy(&c->vertex_arrays.a[0], &c->vertex_arrays.a[arrays[i]], sizeof(glVertex_Array));
			c->cur_vertex_array = 0;
		}

		c->vertex_arrays.a[arrays[i]].deleted = GL_TRUE;
	}
}

void glGenBuffers(GLsizei n, GLuint* buffers)
{
	//fill up empty slots first
	int j = 0;
	for (int i=1; i<c->buffers.size && j<n; ++i) {
		if (c->buffers.a[i].deleted) {
			c->buffers.a[i].deleted = GL_FALSE;
			buffers[j++] = i;
		}
	}

	if (j != n) {
		int s = c->buffers.size;
		cvec_extend_glBuffer(&c->buffers, n-j);
		for (int i=s; j<n; i++) {
			c->buffers.a[i].data = NULL;
			c->buffers.a[i].deleted = GL_FALSE;
			c->buffers.a[i].user_owned = GL_FALSE;
			buffers[j++] = i;
		}
	}
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	GLenum type;
	for (int i=0; i<n; ++i) {
		if (!buffers[i] || buffers[i] >= c->buffers.size)
			continue;

		// NOTE(rswinkle): type is stored as correct index not the raw enum value so no need to
		// subtract here see glBindBuffer
		type = c->buffers.a[buffers[i]].type;
		if (buffers[i] == c->bound_buffers[type])
			c->bound_buffers[type] = 0;

		if (!c->buffers.a[buffers[i]].user_owned) {
			PGL_FREE(c->buffers.a[buffers[i]].data);
		}
		c->buffers.a[buffers[i]].data = NULL;
		c->buffers.a[buffers[i]].deleted = GL_TRUE;
		c->buffers.a[buffers[i]].user_owned = GL_FALSE;
	}
}

void glGenTextures(GLsizei n, GLuint* textures)
{
	int j = 0;
	for (int i=1; i<c->textures.size && j<n; ++i) {
		if (c->textures.a[i].deleted) {
			c->textures.a[i].deleted = GL_FALSE;
			c->textures.a[i].type = GL_TEXTURE_UNBOUND;
			textures[j++] = i;
		}
	}
	if (j != n) {
		int s = c->textures.size;
		cvec_extend_glTexture(&c->textures, n-j);
		for (int i=s; j<n; i++) {
			c->textures.a[i].deleted = GL_FALSE;
			c->textures.a[i].type = GL_TEXTURE_UNBOUND;
			c->textures.a[i].user_owned = GL_FALSE;
			textures[j++] = i;
		}
	}
}

void glCreateTextures(GLenum target, GLsizei n, GLuint* textures)
{
	if (target < GL_TEXTURE_1D || target >= GL_NUM_TEXTURE_TYPES) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	target -= GL_TEXTURE_UNBOUND + 1;
	int j = 0;
	for (int i=1; i<c->textures.size && j<n; ++i) {
		if (c->textures.a[i].deleted) {
			INIT_TEX(&c->textures.a[i], target);
			textures[j++] = i;
		}
	}
	if (j != n) {
		int s = c->textures.size;
		cvec_extend_glTexture(&c->textures, n-j);
		for (int i=s; j<n; i++) {
			INIT_TEX(&c->textures.a[i], target);
			textures[j++] = i;
		}
	}
}

void glDeleteTextures(GLsizei n, const GLuint* textures)
{
	GLenum type;
	for (int i=0; i<n; ++i) {
		if (!textures[i] || textures[i] >= c->textures.size)
			continue;

		// NOTE(rswinkle): type is stored as correct index not the raw enum value
		// so no need to subtract here see glBindTexture
		type = c->textures.a[textures[i]].type;
		if (textures[i] == c->bound_textures[type])
			c->bound_textures[type] = 0;

		if (!c->textures.a[textures[i]].user_owned) {
			PGL_FREE(c->textures.a[textures[i]].data);
		}

		c->textures.a[textures[i]].data = NULL;
		c->textures.a[textures[i]].deleted = GL_TRUE;
		c->textures.a[textures[i]].user_owned = GL_FALSE;
	}
}

void glBindVertexArray(GLuint array)
{
	if (array < c->vertex_arrays.size && c->vertex_arrays.a[array].deleted == GL_FALSE) {
		c->cur_vertex_array = array;
		c->bound_buffers[GL_ELEMENT_ARRAY_BUFFER-GL_ARRAY_BUFFER] = c->vertex_arrays.a[array].element_buffer;
	} else if (!c->error) {
		c->error = GL_INVALID_OPERATION;
	}
}

void glBindBuffer(GLenum target, GLuint buffer)
{
//GL_ARRAY_BUFFER, GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, GL_ELEMENT_ARRAY_BUFFER,
//GL_PIXEL_PACK_BUFFER, GL_PIXEL_UNPACK_BUFFER, GL_TEXTURE_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER, or GL_UNIFORM_BUFFER.
	if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	target -= GL_ARRAY_BUFFER;
	if (buffer < c->buffers.size && c->buffers.a[buffer].deleted == GL_FALSE) {
		c->bound_buffers[target] = buffer;

		// Note type isn't set till binding and we're not storing the raw
		// enum but the enum - GL_ARRAY_BUFFER so it's an index into c->bound_buffers
		// TODO need to see what's supposed to happen if you try to bind
		// a buffer to multiple targets
		c->buffers.a[buffer].type = target;

		if (target == GL_ELEMENT_ARRAY_BUFFER - GL_ARRAY_BUFFER) {
			c->vertex_arrays.a[c->cur_vertex_array].element_buffer = buffer;
		}
	} else if (!c->error) {
		c->error = GL_INVALID_OPERATION;
	}
}

void glBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage)
{
	if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//TODO check for usage later

	target -= GL_ARRAY_BUFFER;
	if (c->bound_buffers[target] == 0) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	// the spec says any pre-existing data store is deleted but there's no reason to
	// c->buffers.a[c->bound_buffers[target]].data is always NULL or valid
	if (!(c->buffers.a[c->bound_buffers[target]].data = (u8*)PGL_REALLOC(c->buffers.a[c->bound_buffers[target]].data, size))) {
		if (!c->error)
			c->error = GL_OUT_OF_MEMORY;
		// GL state is undefined from here on
		return;
	}

	if (data) {
		memcpy(c->buffers.a[c->bound_buffers[target]].data, data, size);
	}

	c->buffers.a[c->bound_buffers[target]].user_owned = GL_FALSE;
	c->buffers.a[c->bound_buffers[target]].size = size;
}

void glBufferSubData(GLenum target, GLsizei offset, GLsizei size, const GLvoid* data)
{
	if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	target -= GL_ARRAY_BUFFER;
	if (c->bound_buffers[target] == 0) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	if (offset + size > c->buffers.a[c->bound_buffers[target]].size) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	memcpy(&c->buffers.a[c->bound_buffers[target]].data[offset], data, size);
}

void glNamedBufferData(GLuint buffer, GLsizei size, const GLvoid* data, GLenum usage)
{
	//check for usage later

	if (buffer == 0 || buffer >= c->buffers.size || c->buffers.a[buffer].deleted) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	//always NULL or valid
	PGL_FREE(c->buffers.a[buffer].data);

	if (!(c->buffers.a[buffer].data = (u8*)PGL_MALLOC(size))) {
		if (!c->error)
			c->error = GL_OUT_OF_MEMORY;
		// GL state is undefined from here on
		return;
	}

	if (data) {
		memcpy(c->buffers.a[buffer].data, data, size);
	}

	c->buffers.a[buffer].user_owned = GL_FALSE;
	c->buffers.a[buffer].size = size;
}

void glNamedBufferSubData(GLuint buffer, GLsizei offset, GLsizei size, const GLvoid* data)
{
	if (buffer == 0 || buffer >= c->buffers.size || c->buffers.a[buffer].deleted) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	if (offset + size > c->buffers.a[buffer].size) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	memcpy(&c->buffers.a[buffer].data[offset], data, size);
}

void glBindTexture(GLenum target, GLuint texture)
{
	if (target < GL_TEXTURE_1D || target >= GL_NUM_TEXTURE_TYPES) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	target -= GL_TEXTURE_UNBOUND + 1;

	if (texture < c->textures.size && !c->textures.a[texture].deleted) {
		if (c->textures.a[texture].type == GL_TEXTURE_UNBOUND) {
			c->bound_textures[target] = texture;
			INIT_TEX(&c->textures.a[texture], target);
		} else if (c->textures.a[texture].type == target) {
			c->bound_textures[target] = texture;
		} else if (!c->error) {
			c->error = GL_INVALID_OPERATION;
		}
	} else if (!c->error) {
		c->error = GL_INVALID_VALUE;
	}
}

static void set_texparami(glTexture* tex, GLenum pname, GLint param)
{
	// NOTE, currently in the texture access functions
	// if it's not NEAREST, it assumes LINEAR so I could
	// just say that's good rather than these switch statements
	if (pname == GL_TEXTURE_MIN_FILTER) {
		switch (param) {
		case GL_NEAREST:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
			param = GL_NEAREST;
			break;
		case GL_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			param = GL_LINEAR;
			break;
		default:
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}
		tex->min_filter = param;
	} else if (pname == GL_TEXTURE_MAG_FILTER) {
		switch (param) {
		case GL_NEAREST:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
			param = GL_NEAREST;
			break;
		case GL_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			param = GL_LINEAR;
			break;
		default:
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}
		tex->min_filter = param;
		tex->mag_filter = param;
	} else if (pname == GL_TEXTURE_WRAP_S) {
		if (param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT) {
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}
		tex->wrap_s = param;
	} else if (pname == GL_TEXTURE_WRAP_T) {
		if (param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT) {
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}
		tex->wrap_t = param;
	} else if (pname == GL_TEXTURE_WRAP_R) {
		if (param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT) {
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}
		tex->wrap_r = param;
	} else {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	//GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_RECTANGLE, or GL_TEXTURE_CUBE_MAP.
	//will add others as they're implemented
	if (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	
	//shift to range 0 - NUM_TEXTURES-1 to access bound_textures array
	target -= GL_TEXTURE_UNBOUND + 1;

	set_texparami(&c->textures.a[c->bound_textures[target]], pname, param);
}

void glTextureParameteri(GLuint texture, GLenum pname, GLint param)
{
	if (texture >= c->textures.size) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	set_texparami(&c->textures.a[texture], pname, param);
}

void glPixelStorei(GLenum pname, GLint param)
{
	if (pname != GL_UNPACK_ALIGNMENT && pname != GL_PACK_ALIGNMENT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (param != 1 && param != 2 && param != 4 && param != 8) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	
	if (pname == GL_UNPACK_ALIGNMENT) {
		c->unpack_alignment = param;
	} else if (pname == GL_PACK_ALIGNMENT) {
		c->pack_alignment = param;
	}

}

#define CHECK_FORMAT_GET_COMP(format, components) \
	do { \
	switch (format) { \
	case GL_RED: \
	case GL_ALPHA: \
	case GL_LUMINANCE: \
	case PGL_ONE_ALPHA: \
		components = 1; \
		break; \
	case GL_RG: \
	case GL_LUMINANCE_ALPHA: \
		components = 2; \
		break; \
	case GL_RGB: \
	case GL_BGR: \
		components = 3; \
		break; \
	case GL_RGBA: \
	case GL_BGRA: \
		components = 4; \
		break; \
	default: \
		if (!c->error) \
			c->error = GL_INVALID_ENUM; \
		return; \
	} \
	} while (0)

void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	if (target != GL_TEXTURE_1D) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (border) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//ignore level and internalformat for now

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	if (format != GL_RGBA) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];
	c->textures.a[cur_tex].w = width;

	// TODO NULL or valid ... but what if user_owned?
	PGL_FREE(c->textures.a[cur_tex].data);

	//TODO hardcoded 4 till I support more than RGBA/UBYTE internally
	if (!(c->textures.a[cur_tex].data = (u8*)PGL_MALLOC(width * 4))) {
		if (!c->error)
			c->error = GL_OUT_OF_MEMORY;
		//undefined state now
		return;
	}

	u8* texdata = c->textures.a[cur_tex].data;

	if (data) {
		convert_format_to_packed_rgba(texdata, (u8*)data, width, 1, width*components, format);
	}

	c->textures.a[cur_tex].user_owned = GL_FALSE;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	// TODO GL_TEXTURE_1D_ARRAY
	if (target != GL_TEXTURE_2D &&
	    target != GL_TEXTURE_RECTANGLE &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (border) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	//ignore level for now

	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	// TODO internalformat ignored for now, always converted
	// to RGBA32 anyway

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	if (format != GL_RGBA) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	int cur_tex;

	// TODO If I ever support type other than GL_UNSIGNED_BYTE (also using for both internalformat and format)
	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	if (target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE) {
		cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

		c->textures.a[cur_tex].w = width;
		c->textures.a[cur_tex].h = height;

		// either NULL or valid
		PGL_FREE(c->textures.a[cur_tex].data);

		//TODO support other internal formats? components should be of internalformat not format hardcoded 4 until I support more than RGBA
		if (!(c->textures.a[cur_tex].data = (u8*)PGL_MALLOC(height * width*4))) {
			if (!c->error)
				c->error = GL_OUT_OF_MEMORY;
			//undefined state now
			return;
		}

		if (data) {
			convert_format_to_packed_rgba(c->textures.a[cur_tex].data, (u8*)data, width, height, padded_row_len, format);
		}

		c->textures.a[cur_tex].user_owned = GL_FALSE;

	} else {  //CUBE_MAP
		cur_tex = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1];

		// If we're reusing a texture, and we haven't already loaded
		// one of the planes of the cubemap, data is either NULL or valid
		if (!c->textures.a[cur_tex].w)
			PGL_FREE(c->textures.a[cur_tex].data);

		if (width != height) {
			//TODO spec says INVALID_VALUE, man pages say INVALID_ENUM ?
			if (!c->error)
				c->error = GL_INVALID_VALUE;
			return;
		}

		// TODO hardcoded 4 as long as we only support RGBA/UBYTES
		int mem_size = width*height*6 * 4;
		if (c->textures.a[cur_tex].w == 0) {
			c->textures.a[cur_tex].w = width;
			c->textures.a[cur_tex].h = width; //same cause square

			if (!(c->textures.a[cur_tex].data = (u8*)PGL_MALLOC(mem_size))) {
				if (!c->error)
					c->error = GL_OUT_OF_MEMORY;
				//undefined state now
				return;
			}
		} else if (c->textures.a[cur_tex].w != width) {
			//TODO spec doesn't say all sides must have same dimensions but it makes sense
			//and this site suggests it http://www.opengl.org/wiki/Cubemap_Texture
			if (!c->error)
				c->error = GL_INVALID_VALUE;
			return;
		}

		//use target as plane index
		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X;

		// TODO handle different format and internalformat
		int p = height*width*4;
		u8* texdata = c->textures.a[cur_tex].data;

		if (data) {
			convert_format_to_packed_rgba(&texdata[target*p], (u8*)data, width, height, padded_row_len, format);
		}

		c->textures.a[cur_tex].user_owned = GL_FALSE;
	} //end CUBE_MAP
}

void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (border) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	//ignore level and internalformat for now
	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	if (format != GL_RGBA) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	c->textures.a[cur_tex].w = width;
	c->textures.a[cur_tex].h = height;
	c->textures.a[cur_tex].d = depth;

	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	// NULL or valid
	PGL_FREE(c->textures.a[cur_tex].data);

	//TODO hardcoded 4 till I support more than RGBA/UBYTE internally
	if (!(c->textures.a[cur_tex].data = (u8*)PGL_MALLOC(width*height*depth * 4))) {
		if (!c->error)
			c->error = GL_OUT_OF_MEMORY;
		//undefined state now
		return;
	}

	u8* texdata = c->textures.a[cur_tex].data;

	if (data) {
		convert_format_to_packed_rgba(texdata, (u8*)data, width, height*depth, padded_row_len, format);
	}

	c->textures.a[cur_tex].user_owned = GL_FALSE;
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data)
{
	if (target != GL_TEXTURE_1D) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//ignore level for now

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	if (format != GL_RGBA) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	if (xoffset < 0 || xoffset + width > c->textures.a[cur_tex].w) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	u32* texdata = (u32*) c->textures.a[cur_tex].data;
	convert_format_to_packed_rgba((u8*)&texdata[xoffset], (u8*)data, width, 1, width*components, format);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data)
{
	// TODO GL_TEXTURE_1D_ARRAY
	if (target != GL_TEXTURE_2D &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//ignore level for now

	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	if (format != GL_RGBA) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	int cur_tex;
	u8* d = (u8*)data;

	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	if (target == GL_TEXTURE_2D) {
		cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];
		u32* texdata = (u32*) c->textures.a[cur_tex].data;

		if (xoffset < 0 || xoffset + width > c->textures.a[cur_tex].w || yoffset < 0 || yoffset + height > c->textures.a[cur_tex].h) {
			if (!c->error)
				c->error = GL_INVALID_VALUE;
			return;
		}

		int w = c->textures.a[cur_tex].w;

		// TODO maybe better to covert the whole input image if
		// necessary then do the original memcpy's even with
		// the extra alloc and free
		for (int i=0; i<height; ++i) {
			convert_format_to_packed_rgba((u8*)&texdata[(yoffset+i)*w + xoffset], &d[i*padded_row_len], width, 1, padded_row_len, format);
		}

	} else {  //CUBE_MAP
		cur_tex = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1];
		u32* texdata = (u32*) c->textures.a[cur_tex].data;

		int w = c->textures.a[cur_tex].w;

		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X; //use target as plane index

		int p = w*w;

		for (int i=0; i<height; ++i) {
			convert_format_to_packed_rgba((u8*)&texdata[p*target + (yoffset+i)*w + xoffset], &d[i*padded_row_len], width, 1, padded_row_len, format);
		}
	} //end CUBE_MAP
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data)
{
	if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//ignore level for now
	
	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	if (format != GL_RGBA) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	if (xoffset < 0 || xoffset + width > c->textures.a[cur_tex].w ||
	    yoffset < 0 || yoffset + height > c->textures.a[cur_tex].h ||
	    zoffset < 0 || zoffset + depth > c->textures.a[cur_tex].d) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	int w = c->textures.a[cur_tex].w;
	int h = c->textures.a[cur_tex].h;
	int p = w*h;
	int pp = h*padded_row_len;
	u8* d = (u8*)data;
	u32* texdata = (u32*) c->textures.a[cur_tex].data;
	u8* out;
	u8* in;

	for (int j=0; j<depth; ++j) {
		for (int i=0; i<height; ++i) {
			out = (u8*)&texdata[(zoffset+j)*p + (yoffset+i)*w + xoffset];
			in = &d[j*pp + i*padded_row_len];
			convert_format_to_packed_rgba(out, in, width, 1, padded_row_len, format);
		}
	}
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer)
{
	// core profile is error if 0 array_buffer and pointer
	// compatibility profile is if non-zero VAO and above conditions
	// to prevent client arrays.
	//
	// Technically GLES 2 doesn't even have VAOs but you can usually
	// get them as an extension with the suffix OES
	//
	// GLES 3 is the same as GL 3.3 compatibility specifically for
	// backward compatibility with GLES 2
	//
	// So for now I've decided to match the compatibility profile
	// but you can easily remove c->cur_vertex_array from the check
	// below to enable client arrays for all VAOs and it will
	// still work just fine
	if (index >= GL_MAX_VERTEX_ATTRIBS || size < 1 || size > 4 ||
	    (c->cur_vertex_array && !c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER] && pointer)) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	//TODO type Specifies the data type of each component in the array. The symbolic constants GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,
	//GL_UNSIGNED_SHORT, GL_INT, and GL_UNSIGNED_INT are accepted by both functions. Additionally GL_HALF_FLOAT, GL_FLOAT, GL_DOUBLE,
	//GL_INT_2_10_10_10_REV, and GL_UNSIGNED_INT_2_10_10_10_REV are accepted by glVertexAttribPointer. The initial value is GL_FLOAT.
	int type_sz = 4;
	switch (type) {
	case GL_BYTE:           type_sz = sizeof(GLbyte); break;
	case GL_UNSIGNED_BYTE:  type_sz = sizeof(GLubyte); break;
	case GL_SHORT:          type_sz = sizeof(GLshort); break;
	case GL_UNSIGNED_SHORT: type_sz = sizeof(GLushort); break;
	case GL_INT:            type_sz = sizeof(GLint); break;
	case GL_UNSIGNED_INT:   type_sz = sizeof(GLuint); break;

	case GL_FLOAT:  type_sz = sizeof(GLfloat); break;
	case GL_DOUBLE: type_sz = sizeof(GLdouble); break;

	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	glVertex_Attrib* v = &(c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index]);
	v->size = size;
	v->type = type;
	v->normalized = normalized;
	v->stride = (stride) ? stride : size*type_sz;

	// offset can still really a pointer if using the 0 VAO
	// and no bound ARRAY_BUFFER. !v->buf and !(buf data) see vertex_stage()
	v->offset = (GLsizeiptr)pointer;
	// I put ARRAY_BUFFER-itself instead of 0 to reinforce that bound_buffers is indexed that way, buffer type - GL_ARRAY_BUFFER
	v->buf = c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER];
}

void glEnableVertexAttribArray(GLuint index)
{
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_TRUE;
}

void glDisableVertexAttribArray(GLuint index)
{
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_FALSE;
}

void glVertexAttribDivisor(GLuint index, GLuint divisor)
{
	if (index >= GL_MAX_VERTEX_ATTRIBS) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].divisor = divisor;
}



//TODO(rswinkle): Why is first, an index, a GLint and not GLuint or GLsizei?
void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	// TODO should I just make GLsizei an uint32_t rather than int32_t?
	if (count < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count)
		return;

	run_pipeline(mode, (GLvoid*)(GLintptr)first, count, 0, 0, GL_FALSE);
}

void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (drawcount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	for (GLsizei i=0; i<drawcount; i++) {
		if (!count[i]) continue;
		run_pipeline(mode, (GLvoid*)(GLintptr)first[i], count[i], 0, 0, GL_FALSE);
	}
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//error not in the spec but says type must be one of these ... strange
	if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	// TODO should I just make GLsizei an uint32_t rather than int32_t?
	if (count < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count)
		return;

	run_pipeline(mode, indices, count, 0, 0, type);
}

// TODO fix
void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const GLvoid* const* indices, GLsizei drawcount)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (drawcount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	//error not in the spec but says type must be one of these ... strange
	if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	for (GLsizei i=0; i<drawcount; i++) {
		if (!count[i]) continue;
		run_pipeline(mode, indices[i], count[i], 0, 0, type);
	}
}

void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (count < 0 || instancecount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count || !instancecount)
		return;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, (GLvoid*)(GLintptr)first, count, instance, 0, GL_FALSE);
	}
}

void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (count < 0 || instancecount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count || !instancecount)
		return;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, (GLvoid*)(GLintptr)first, count, instance, baseinstance, GL_FALSE);
	}
}


void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	// NOTE: error not in the spec but says type must be one of these ... strange
	if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	if (count < 0 || instancecount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count || !instancecount)
		return;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, indices, count, instance, 0, type);
	}
}

void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount, GLuint baseinstance)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//error not in the spec but says type must be one of these ... strange
	if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	if (count < 0 || instancecount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count || !instancecount)
		return;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, indices, count, instance, baseinstance, GL_TRUE);
	}
}


void glViewport(int x, int y, GLsizei width, GLsizei height)
{
	if (width < 0 || height < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	// TODO: Do I need a full matrix? Also I don't actually
	// use these values anywhere else so why save them?
	make_viewport_matrix(c->vp_mat, x, y, width, height, 1);
	c->xmin = x;
	c->ymin = y;
	c->width = width;
	c->height = height;
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	red = clamp_01(red);
	green = clamp_01(green);
	blue = clamp_01(blue);
	alpha = clamp_01(alpha);

	vec4 tmp = { red, green, blue, alpha };
	c->clear_color = vec4_to_Color(tmp);
}

void glClearDepthf(GLfloat depth)
{
	c->clear_depth = clamp_01(depth);
}

void glClearDepth(GLdouble depth)
{
	c->clear_depth = clamp_01(depth);
}

void glDepthFunc(GLenum func)
{
	if (func < GL_LESS || func > GL_NEVER) {
		if (!c->error)
			c->error =GL_INVALID_ENUM;

		return;
	}

	c->depth_func = func;
}

void glDepthRangef(GLfloat nearVal, GLfloat farVal)
{
	c->depth_range_near = clamp_01(nearVal);
	c->depth_range_far = clamp_01(farVal);
}

void glDepthRange(GLdouble nearVal, GLdouble farVal)
{
	c->depth_range_near = clamp_01(nearVal);
	c->depth_range_far = clamp_01(farVal);
}

void glDepthMask(GLboolean flag)
{
	c->depth_mask = flag;
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
#ifndef PGL_DISABLE_COLOR_MASK
	// !! ensures 1 or 0
	red = !!red;
	green = !!green;
	blue = !!blue;
	alpha = !!alpha;

	c->red_mask   = red;
	c->green_mask = green;
	c->blue_mask  = blue;
	c->alpha_mask = alpha;

	// By multiplying by the masks the user gave in init_glContext I don't
	// need to shift them
	u32 rmask = red*c->Rmask;
	u32 gmask = green*c->Gmask;
	u32 bmask = blue*c->Bmask;
	u32 amask = alpha*c->Amask;
	c->color_mask = rmask | gmask | bmask | amask;
#endif
}

void glClear(GLbitfield mask)
{
	if (!(mask & (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		puts("failed to clear");
		return;
	}
	
	// NOTE: All buffers should have the same dimensions/size
	int sz = c->ux * c->uy;
	int w = c->back_buffer.w;

	Color col = c->clear_color;
	u32 color = (u32)col.a << c->Ashift | (u32)col.r << c->Rshift | (u32)col.g << c->Gshift | (u32)col.b << c->Bshift;

#ifndef PGL_DISABLE_COLOR_MASK
	// clear out channels not enabled for writing
	color &= c->color_mask;
	// used to erase channels to be written
	u32 clear_mask = ~c->color_mask;
	u32 tmp;
#endif

	float cd = c->clear_depth;
	u8 cs = c->clear_stencil;
	if (!c->scissor_test) {
		if (mask & GL_COLOR_BUFFER_BIT) {
			for (int i=0; i<sz; ++i) {
#ifdef PGL_DISABLE_COLOR_MASK
				((u32*)c->back_buffer.buf)[i] = color;
#else
				tmp = ((u32*)c->back_buffer.buf)[i];
				tmp &= clear_mask;
				((u32*)c->back_buffer.buf)[i] = tmp | color;
#endif
			}
		}
		if (mask & GL_DEPTH_BUFFER_BIT) {
			for (int i=0; i < sz; ++i) {
				((float*)c->zbuf.buf)[i] = cd;
			}
		}
		if (mask & GL_STENCIL_BUFFER_BIT) {
			memset(c->stencil_buf.buf, cs, sz);
			//for (int i=0; i < sz; ++i) {
			//	c->stencil_buf.buf[i] = cs;
			//}
		}
	} else {
		// TODO this code is correct with or without scissor
		// enabled, test performance difference with above before
		// getting rid of above
		if (mask & GL_COLOR_BUFFER_BIT) {
			for (int y=c->ly; y<c->uy; ++y) {
				for (int x=c->lx; x<c->ux; ++x) {
#ifdef PGL_DISABLE_COLOR_MASK
					((u32*)c->back_buffer.lastrow)[-y*w + x] = color;
#else
					tmp = ((u32*)c->back_buffer.lastrow)[-y*w + x];
					tmp &= clear_mask;
					((u32*)c->back_buffer.lastrow)[-y*w + x] = tmp | color;
#endif
				}
			}
		}
		if (mask & GL_DEPTH_BUFFER_BIT) {
			for (int y=c->ly; y<c->uy; ++y) {
				for (int x=c->lx; x<c->ux; ++x) {
					((float*)c->zbuf.lastrow)[-y*w + x] = cd;
				}
			}
		}
		if (mask & GL_STENCIL_BUFFER_BIT) {
			for (int y=c->ly; y<c->uy; ++y) {
				for (int x=c->lx; x<c->ux; ++x) {
					c->stencil_buf.lastrow[-y*w + x] = cs;
				}
			}
		}
	}
}

void glEnable(GLenum cap)
{
	switch (cap) {
	case GL_CULL_FACE:
		c->cull_face = GL_TRUE;
		break;
	case GL_DEPTH_TEST:
		c->depth_test = GL_TRUE;
		break;
	case GL_DEPTH_CLAMP:
		c->depth_clamp = GL_TRUE;
		break;
	case GL_LINE_SMOOTH:
		// TODO implementation needs work/upgrade
		//c->line_smooth = GL_TRUE;
		break;
	case GL_BLEND:
		c->blend = GL_TRUE;
		break;
	case GL_COLOR_LOGIC_OP:
		c->logic_ops = GL_TRUE;
		break;
	case GL_POLYGON_OFFSET_POINT:
		c->poly_offset_pt = GL_TRUE;
		break;
	case GL_POLYGON_OFFSET_LINE:
		c->poly_offset_line = GL_TRUE;
		break;
	case GL_POLYGON_OFFSET_FILL:
		c->poly_offset_fill = GL_TRUE;
		break;
	case GL_SCISSOR_TEST: {
		c->scissor_test = GL_TRUE;
		int ux = c->scissor_lx+c->scissor_w;
		int uy = c->scissor_ly+c->scissor_h;
		c->lx = MAX(c->scissor_lx, 0);
		c->ly = MAX(c->scissor_ly, 0);
		c->ux = MIN(ux, c->back_buffer.w);
		c->uy = MIN(uy, c->back_buffer.h);
	} break;
	case GL_STENCIL_TEST:
		c->stencil_test = GL_TRUE;
		break;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glDisable(GLenum cap)
{
	switch (cap) {
	case GL_CULL_FACE:
		c->cull_face = GL_FALSE;
		break;
	case GL_DEPTH_TEST:
		c->depth_test = GL_FALSE;
		break;
	case GL_DEPTH_CLAMP:
		c->depth_clamp = GL_FALSE;
		break;
	case GL_LINE_SMOOTH:
		c->line_smooth = GL_FALSE;
		break;
	case GL_BLEND:
		c->blend = GL_FALSE;
		break;
	case GL_COLOR_LOGIC_OP:
		c->logic_ops = GL_FALSE;
		break;
	case GL_POLYGON_OFFSET_POINT:
		c->poly_offset_pt = GL_FALSE;
		break;
	case GL_POLYGON_OFFSET_LINE:
		c->poly_offset_line = GL_FALSE;
		break;
	case GL_POLYGON_OFFSET_FILL:
		c->poly_offset_fill = GL_FALSE;
		break;
	case GL_SCISSOR_TEST:
		c->scissor_test = GL_FALSE;
		c->lx = 0;
		c->ly = 0;
		c->ux = c->back_buffer.w;
		c->uy = c->back_buffer.h;
		break;
	case GL_STENCIL_TEST:
		c->stencil_test = GL_FALSE;
		break;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

GLboolean glIsEnabled(GLenum cap)
{
	// make up my own enum for this?  rename member as no_early_z?
	//GLboolean fragdepth_or_discard;
	switch (cap) {
	case GL_DEPTH_TEST: return c->depth_test;
	case GL_LINE_SMOOTH: return c->line_smooth;
	case GL_CULL_FACE: return c->cull_face;
	case GL_DEPTH_CLAMP: return c->depth_clamp;
	case GL_BLEND: return c->blend;
	case GL_COLOR_LOGIC_OP: return c->logic_ops;
	case GL_POLYGON_OFFSET_POINT: return c->poly_offset_pt;
	case GL_POLYGON_OFFSET_LINE: return c->poly_offset_line;
	case GL_POLYGON_OFFSET_FILL: return c->poly_offset_fill;
	case GL_SCISSOR_TEST: return c->scissor_test;
	case GL_STENCIL_TEST: return c->stencil_test;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}

	return GL_FALSE;
}

GLboolean glIsProgram(GLuint program)
{
	if (!program || program >= c->programs.size || c->programs.a[program].deleted) {
		return GL_FALSE;
	}

	return GL_TRUE;
}

void glGetBooleanv(GLenum pname, GLboolean* data)
{
	// not sure it's worth adding every enum, spec says
	// gelGet* will convert/map types if they don't match the function
	switch (pname) {
	case GL_DEPTH_TEST:           *data = c->depth_test;       break;
	case GL_LINE_SMOOTH:          *data = c->line_smooth;      break;
	case GL_CULL_FACE:            *data = c->cull_face;        break;
	case GL_DEPTH_CLAMP:          *data = c->depth_clamp;      break;
	case GL_BLEND:                *data = c->blend;            break;
	case GL_COLOR_LOGIC_OP:       *data = c->logic_ops;        break;
	case GL_POLYGON_OFFSET_POINT: *data = c->poly_offset_pt;  break;
	case GL_POLYGON_OFFSET_LINE:  *data = c->poly_offset_line; break;
	case GL_POLYGON_OFFSET_FILL:  *data = c->poly_offset_fill; break;
	case GL_SCISSOR_TEST:         *data = c->scissor_test;     break;
	case GL_STENCIL_TEST:         *data = c->stencil_test;     break;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glGetFloatv(GLenum pname, GLfloat* data)
{
	switch (pname) {
	case GL_POLYGON_OFFSET_FACTOR: *data = c->poly_factor; break;
	case GL_POLYGON_OFFSET_UNITS:  *data = c->poly_units;  break;
	case GL_POINT_SIZE:            *data = c->point_size;  break;
	case GL_DEPTH_CLEAR_VALUE:     *data = c->clear_depth; break;
	case GL_DEPTH_RANGE:
		data[0] = c->depth_range_near;
		data[1] = c->depth_range_near;
		break;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glGetIntegerv(GLenum pname, GLint* data)
{
	// TODO maybe make all the enum/int member names match the associated ENUM?
	switch (pname) {
	case GL_STENCIL_WRITE_MASK:       data[0] = c->stencil_writemask; break;
	case GL_STENCIL_REF:              data[0] = c->stencil_ref; break;
	case GL_STENCIL_VALUE_MASK:       data[0] = c->stencil_valuemask; break;
	case GL_STENCIL_FUNC:             data[0] = c->stencil_func; break;
	case GL_STENCIL_FAIL:             data[0] = c->stencil_sfail; break;
	case GL_STENCIL_PASS_DEPTH_FAIL:  data[0] = c->stencil_dpfail; break;
	case GL_STENCIL_PASS_DEPTH_PASS:  data[0] = c->stencil_dppass; break;

	case GL_STENCIL_BACK_WRITE_MASK:      data[0] = c->stencil_writemask_back; break;
	case GL_STENCIL_BACK_REF:             data[0] = c->stencil_ref_back; break;
	case GL_STENCIL_BACK_VALUE_MASK:      data[0] = c->stencil_valuemask_back; break;
	case GL_STENCIL_BACK_FUNC:            data[0] = c->stencil_func_back; break;
	case GL_STENCIL_BACK_FAIL:            data[0] = c->stencil_sfail_back; break;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL: data[0] = c->stencil_dpfail_back; break;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS: data[0] = c->stencil_dppass_back; break;

	case GL_LOGIC_OP_MODE:             data[0] = c->logic_func; break;

	//TODO implement glBlendFuncSeparate and glBlendEquationSeparate
	case GL_BLEND_SRC_RGB:             data[0] = c->blend_sRGB; break;
	case GL_BLEND_SRC_ALPHA:           data[0] = c->blend_sA; break;
	case GL_BLEND_DST_RGB:             data[0] = c->blend_dRGB; break;
	case GL_BLEND_DST_ALPHA:           data[0] = c->blend_dA; break;

	case GL_BLEND_EQUATION_RGB:        data[0] = c->blend_eqRGB; break;
	case GL_BLEND_EQUATION_ALPHA:      data[0] = c->blend_eqA; break;

	case GL_CULL_FACE_MODE:            data[0] = c->cull_mode; break;
	case GL_FRONT_FACE:                data[0] = c->front_face; break;
	case GL_DEPTH_FUNC:                data[0] = c->depth_func; break;
	case GL_POINT_SPRITE_COORD_ORIGIN: data[0] = c->point_spr_origin; break;
	case GL_PROVOKING_VERTEX:          data[0] = c->provoking_vert; break;

	case GL_POLYGON_MODE:
		data[0] = c->poly_mode_front;
		data[1] = c->poly_mode_back;
		break;

	case GL_VIEWPORT:
		data[0] = c->xmin;
		data[1] = c->ymin;
		data[2] = c->width;
		data[3] = c->height;
		break;

	case GL_SCISSOR_BOX:
		data[0] = c->scissor_lx;
		data[1] = c->scissor_ly;
		data[2] = c->scissor_w;
		data[3] = c->scissor_h;
		break;

	// TODO decide if 3.2 is the best approximation
	case GL_MAJOR_VERSION:             data[0] = 3; break;
	case GL_MINOR_VERSION:             data[0] = 2; break;

	case GL_ARRAY_BUFFER_BINDING:
		data[0] = c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER];
		break;

	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		data[0] = c->bound_buffers[GL_ELEMENT_ARRAY_BUFFER-GL_ARRAY_BUFFER];
		break;

	case GL_VERTEX_ARRAY_BINDING:
		data[0] = c->cur_vertex_array;
		break;

	case GL_CURRENT_PROGRAM:
		data[0] = c->cur_program;
		break;


	case GL_TEXTURE_BINDING_1D:        data[0] = c->bound_textures[GL_TEXTURE_1D-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_2D:        data[0] = c->bound_textures[GL_TEXTURE_2D-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_3D:        data[0] = c->bound_textures[GL_TEXTURE_3D-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_1D_ARRAY:  data[0] = c->bound_textures[GL_TEXTURE_1D_ARRAY-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_2D_ARRAY:  data[0] = c->bound_textures[GL_TEXTURE_2D_ARRAY-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_RECTANGLE: data[0] = c->bound_textures[GL_TEXTURE_RECTANGLE-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_CUBE_MAP:  data[0] = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1]; break;

	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glCullFace(GLenum mode)
{
	if (mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	c->cull_mode = mode;
}


void glFrontFace(GLenum mode)
{
	if (mode != GL_CCW && mode != GL_CW) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	c->front_face = mode;
}

void glPolygonMode(GLenum face, GLenum mode)
{
	if ((face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) ||
	    (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL)) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (mode == GL_POINT) {
		if (face == GL_FRONT) {
			c->poly_mode_front = mode;
			c->draw_triangle_front = draw_triangle_point;
		} else if (face == GL_BACK) {
			c->poly_mode_back = mode;
			c->draw_triangle_back = draw_triangle_point;
		} else {
			c->poly_mode_front = mode;
			c->poly_mode_back = mode;
			c->draw_triangle_front = draw_triangle_point;
			c->draw_triangle_back = draw_triangle_point;
		}
	} else if (mode == GL_LINE) {
		if (face == GL_FRONT) {
			c->poly_mode_front = mode;
			c->draw_triangle_front = draw_triangle_line;
		} else if (face == GL_BACK) {
			c->poly_mode_back = mode;
			c->draw_triangle_back = draw_triangle_line;
		} else {
			c->poly_mode_front = mode;
			c->poly_mode_back = mode;
			c->draw_triangle_front = draw_triangle_line;
			c->draw_triangle_back = draw_triangle_line;
		}
	} else  {
		if (face == GL_FRONT) {
			c->poly_mode_front = mode;
			c->draw_triangle_front = draw_triangle_fill;
		} else if (face == GL_BACK) {
			c->poly_mode_back = mode;
			c->draw_triangle_back = draw_triangle_fill;
		} else {
			c->poly_mode_front = mode;
			c->poly_mode_back = mode;
			c->draw_triangle_front = draw_triangle_fill;
			c->draw_triangle_back = draw_triangle_fill;
		}
	}
}

void glLineWidth(GLfloat width)
{
	if (width <= 0.0f) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	c->line_width = width;
}

void glPointSize(GLfloat size)
{
	if (size <= 0.0f) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	c->point_size = size;
}

void glPointParameteri(GLenum pname, GLint param)
{
	//also GL_POINT_FADE_THRESHOLD_SIZE
	if (pname != GL_POINT_SPRITE_COORD_ORIGIN || (param != GL_LOWER_LEFT && param != GL_UPPER_LEFT)) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	c->point_spr_origin = param;
}


void glProvokingVertex(GLenum provokeMode)
{
	if (provokeMode != GL_FIRST_VERTEX_CONVENTION && provokeMode != GL_LAST_VERTEX_CONVENTION) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	c->provoking_vert = provokeMode;
}


// Shader functions
GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean fragdepth_or_discard)
{
	if (!vertex_shader || !fragment_shader) {
		//TODO set error? doesn't in spec but I'll think about it
		return 0;
	}

	if (n < 0 || n > GL_MAX_VERTEX_OUTPUT_COMPONENTS) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return 0;
	}

	glProgram tmp = {vertex_shader, fragment_shader, NULL, n, {0}, fragdepth_or_discard, GL_FALSE };
	for (int i=0; i<n; ++i) {
		tmp.interpolation[i] = interpolation[i];
	}

	for (int i=1; i<c->programs.size; ++i) {
		if (c->programs.a[i].deleted && i != c->cur_program) {
			c->programs.a[i] = tmp;
			return i;
		}
	}

	cvec_push_glProgram(&c->programs, tmp);
	return c->programs.size-1;
}

void glDeleteProgram(GLuint program)
{
	if (!program)
		return;

	if (program >= c->programs.size) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	c->programs.a[program].deleted = GL_TRUE;
}

void glUseProgram(GLuint program)
{
	if (program >= c->programs.size) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	c->vs_output.size = c->programs.a[program].vs_output_size;
	cvec_reserve_float(&c->vs_output.output_buf, c->vs_output.size * PGL_MAX_VERTICES);
	c->vs_output.interpolation = c->programs.a[program].interpolation;
	c->fragdepth_or_discard = c->programs.a[program].fragdepth_or_discard;

	c->cur_program = program;
}

void pglSetUniform(void* uniform)
{
	//TODO check for NULL? definitely if I ever switch to storing a local
	//copy in glProgram
	c->programs.a[c->cur_program].uniform = uniform;
}


void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	if (sfactor < GL_ZERO || sfactor >= NUM_BLEND_FUNCS || dfactor < GL_ZERO || dfactor >= NUM_BLEND_FUNCS) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	c->blend_sRGB = sfactor;
	c->blend_sA = sfactor;
	c->blend_dRGB = dfactor;
	c->blend_dA = dfactor;
}

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	if (srcRGB < GL_ZERO || srcRGB >= NUM_BLEND_FUNCS ||
	    dstRGB < GL_ZERO || dstRGB >= NUM_BLEND_FUNCS ||
	    srcAlpha < GL_ZERO || srcAlpha >= NUM_BLEND_FUNCS ||
	    dstAlpha < GL_ZERO || dstAlpha >= NUM_BLEND_FUNCS) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}
	c->blend_sRGB = srcRGB;
	c->blend_sA = srcAlpha;
	c->blend_dRGB = dstRGB;
	c->blend_dA = dstAlpha;
}

void glBlendEquation(GLenum mode)
{
	if (mode < GL_FUNC_ADD || mode >= NUM_BLEND_EQUATIONS) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	c->blend_eqRGB = mode;
	c->blend_eqA = mode;
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	if (modeRGB < GL_FUNC_ADD || modeRGB >= NUM_BLEND_EQUATIONS ||
	    modeAlpha < GL_FUNC_ADD || modeAlpha >= NUM_BLEND_EQUATIONS) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	c->blend_eqRGB = modeRGB;
	c->blend_eqA = modeAlpha;
}

void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	SET_VEC4(c->blend_color, clamp_01(red), clamp_01(green), clamp_01(blue), clamp_01(alpha));
}

void glLogicOp(GLenum opcode)
{
	if (opcode < GL_CLEAR || opcode > GL_INVERT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}
	c->logic_func = opcode;
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
	c->poly_factor = factor;
	c->poly_units = units;
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	// once again why is GLsizei not unsigned?
	if (width < 0 || height < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;

		return;
	}

	c->scissor_lx = x;
	c->scissor_ly = y;
	c->scissor_w = width;
	c->scissor_h = height;
	int ux = x+width;
	int uy = y+height;

	c->lx = MAX(x, 0);
	c->ly = MAX(y, 0);
	c->ux = MIN(ux, c->back_buffer.w);
	c->uy = MIN(uy, c->back_buffer.h);
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	if (func < GL_LESS || func > GL_NEVER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	c->stencil_func = func;
	c->stencil_func_back = func;

	// TODO clamp byte function?
	if (ref > 255)
		ref = 255;
	if (ref < 0)
		ref = 0;

	c->stencil_ref = ref;
	c->stencil_ref_back = ref;

	c->stencil_valuemask = mask;
	c->stencil_valuemask_back = mask;
}

void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	if (face < GL_FRONT || face > GL_FRONT_AND_BACK) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	if (face == GL_FRONT_AND_BACK) {
		glStencilFunc(func, ref, mask);
		return;
	}

	if (func < GL_LESS || func > GL_NEVER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	// TODO clamp byte function?
	if (ref > 255)
		ref = 255;
	if (ref < 0)
		ref = 0;

	if (face == GL_FRONT) {
		c->stencil_func = func;
		c->stencil_ref = ref;
		c->stencil_valuemask = mask;
	} else {
		c->stencil_func_back = func;
		c->stencil_ref_back = ref;
		c->stencil_valuemask_back = mask;
	}
}

void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)
{
	// TODO not sure if I should check all parameters first or
	// allow partial success?
	//
	// Also, how best to check when the enums aren't contiguous?  empty switch?
	// manually checking all enums?
	if (((sfail < GL_INVERT || sfail > GL_DECR_WRAP) && sfail != GL_ZERO) ||
	    ((dpfail < GL_INVERT || dpfail > GL_DECR_WRAP) && dpfail != GL_ZERO) ||
	    ((dppass < GL_INVERT || dppass > GL_DECR_WRAP) && dppass != GL_ZERO)) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	c->stencil_sfail = sfail;
	c->stencil_dpfail = dpfail;
	c->stencil_dppass = dppass;

	c->stencil_sfail_back = sfail;
	c->stencil_dpfail_back = dpfail;
	c->stencil_dppass_back = dppass;
}

void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
	if (face < GL_FRONT || face > GL_FRONT_AND_BACK) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	if (face == GL_FRONT_AND_BACK) {
		glStencilOp(sfail, dpfail, dppass);
		return;
	}

	if (((sfail < GL_INVERT || sfail > GL_DECR_WRAP) && sfail != GL_ZERO) ||
	    ((dpfail < GL_INVERT || dpfail > GL_DECR_WRAP) && dpfail != GL_ZERO) ||
	    ((dppass < GL_INVERT || dppass > GL_DECR_WRAP) && dppass != GL_ZERO)) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	if (face == GL_FRONT) {
		c->stencil_sfail = sfail;
		c->stencil_dpfail = dpfail;
		c->stencil_dppass = dppass;
	} else {
		c->stencil_sfail_back = sfail;
		c->stencil_dpfail_back = dpfail;
		c->stencil_dppass_back = dppass;
	}
}

void glClearStencil(GLint s)
{
	c->clear_stencil = s & PGL_STENCIL_MASK;
}

void glStencilMask(GLuint mask)
{
	c->stencil_writemask = mask;
	c->stencil_writemask_back = mask;
}

void glStencilMaskSeparate(GLenum face, GLuint mask)
{
	if (face < GL_FRONT || face > GL_FRONT_AND_BACK) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	if (face == GL_FRONT_AND_BACK) {
		glStencilMask(mask);
		return;
	}

	if (face == GL_FRONT) {
		c->stencil_writemask = mask;
	} else {
		c->stencil_writemask_back = mask;
	}
}


// Just wrap my pgl extension getter, unmap does nothing
void* glMapBuffer(GLenum target, GLenum access)
{
	if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return NULL;
	}

	if (access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return NULL;
	}

	// adjust to access bound_buffers
	target -= GL_ARRAY_BUFFER;

	void* data = NULL;
	pglGetBufferData(c->bound_buffers[target], &data);
	return data;
}

void* glMapNamedBuffer(GLuint buffer, GLenum access)
{
	// pglGetBufferData will verify buffer is valid
	if (access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return NULL;
	}

	void* data = NULL;
	pglGetBufferData(buffer, &data);
	return data;
}

