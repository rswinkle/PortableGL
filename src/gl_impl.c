

#include <stdarg.h>


/******************************************
 * PORTABLEGL_IMPLEMENTATION
 ******************************************/

#include <stdio.h>
#include <float.h>

// for CHAR_BIT
#include <limits.h>

// TODO different name? NO_ERROR_CHECKING? LOOK_MA_NO_HANDS?
#ifdef PGL_UNSAFE
#define PGL_SET_ERR(err)
#define PGL_ERR(check, err)
#define PGL_ERR_RET_VAL(check, err, ret)
#define PGL_LOG(err)
#else
#define PGL_LOG(err) \
	do { \
		if (c->dbg_output && c->dbg_callback) { \
			int len = snprintf(c->dbg_msg_buf, PGL_MAX_DEBUG_MESSAGE_LENGTH, "%s in %s()", pgl_err_strs[err-GL_NO_ERROR], __func__); \
			c->dbg_callback(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, len, c->dbg_msg_buf, c->dbg_userparam); \
		} \
	} while (0)

#define PGL_SET_ERR(err) \
	do { \
		if (!c->error) c->error = err; \
		PGL_LOG(err); \
	} while (0)

#define PGL_ERR(check, err) \
	do { \
		if (check) {  \
			if (!c->error) c->error = err; \
			PGL_LOG(err); \
			return; \
		} \
	} while (0)

#define PGL_ERR_RET_VAL(check, err, ret) \
	do { \
		if (check) {  \
			if (!c->error) c->error = err; \
			PGL_LOG(err); \
			return ret; \
		} \
	} while (0)
#endif

// I just set everything even if not everything applies to the type
// see section 3.8.15 pg 181 of spec for what it's supposed to be
// TODO better name and inline?
static void INIT_TEX(glTexture* tex, GLenum target)
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

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	tex->border_color = make_v4(0,0,0,0);
#endif
}

// default pass through shaders for index 0
PGLDEF void default_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	PGL_UNUSED(uniforms);

	builtins->gl_Position = vertex_attribs[PGL_ATTR_VERT];
}

PGLDEF void default_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	PGL_UNUSED(uniforms);

	vec4* fragcolor = &builtins->gl_FragColor;
	//wish I could use a compound literal, stupid C++ compatibility
	fragcolor->x = 1.0f;
	fragcolor->y = 0.0f;
	fragcolor->z = 0.0f;
	fragcolor->w = 1.0f;
}

// TODO Where to put this and what to name it? move this and all static functions to gl_internal.c?
#ifndef PGL_UNSAFE
static const char* pgl_err_strs[] =
{
	"GL_NO_ERROR",
	"GL_INVALID_ENUM",
	"GL_INVALID_VALUE",
	"GL_INVALID_OPERATION",
	"GL_INVALID_FRAMEBUFFER_OPERATION",
	"GL_OUT_OF_MEMORY"
};

PGLDEF void dflt_dbg_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	PGL_UNUSED(source);
	PGL_UNUSED(type);
	PGL_UNUSED(id);
	PGL_UNUSED(severity);
	PGL_UNUSED(length);
	PGL_UNUSED(userParam);

	printf("%s\n", message);
}
#endif

static void init_glVertex_Attrib(glVertex_Attrib* v)
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

// TODO these are currently equivalent to memset(0) or = {0}...
static void init_glVertex_Array(glVertex_Array* v)
{
	v->deleted = GL_FALSE;
	for (int i=0; i<GL_MAX_VERTEX_ATTRIBS; ++i)
		init_glVertex_Attrib(&v->vertex_attribs[i]);
}

#define GET_SHIFT(mask, shift) \
	do {\
	shift = 0;\
	while ((mask & 1) == 0) {\
		mask >>= 1;\
		++shift;\
	}\
	} while (0)


PGLDEF GLboolean init_glContext(glContext* context, pix_t** back, GLsizei w, GLsizei h)
{
	PGL_ERR_RET_VAL(!back, GL_INVALID_VALUE, GL_FALSE);
	PGL_ERR_RET_VAL((w < 0 || h < 0), GL_INVALID_VALUE, GL_FALSE);

	c = context;
	memset(c, 0, sizeof(glContext));

	if (w && h && *back != NULL) {
		c->user_alloced_backbuf = GL_TRUE;
		c->back_buffer.buf = (u8*)*back;
		c->back_buffer.w = w;
		c->back_buffer.h = h;
		c->back_buffer.lastrow = c->back_buffer.buf + (h-1)*w*sizeof(pix_t);
	}

	c->xmin = 0;
	c->ymin = 0;
	c->width = w;
	c->height = h;

	c->color_mask = ~0;

	//initialize all vectors
	cvec_glVertex_Array(&c->vertex_arrays, 0, 3);
	cvec_glBuffer(&c->buffers, 0, 3);
	cvec_glProgram(&c->programs, 0, 3);
	cvec_glTexture(&c->textures, 0, 1);
	cvec_glVertex(&c->glverts, 0, 10);

	// If not pre-allocating max, need to track size and edit glUseProgram and pglSetInterp
	c->vs_output.output_buf = (float*)PGL_MALLOC(PGL_MAX_VERTICES * GL_MAX_VERTEX_OUTPUT_COMPONENTS * sizeof(float));
	PGL_ERR_RET_VAL(!c->vs_output.output_buf, GL_OUT_OF_MEMORY, GL_FALSE);

	c->clear_color = 0;
	SET_V4(c->blend_color, 0, 0, 0, 0);
	c->point_size = 1.0f;
	c->line_width = 1.0f;
	c->clear_depth = 1.0f;
	c->depth_range_near = 0.0f;
	c->depth_range_far = 1.0f;
	make_viewport_m4(c->vp_mat, 0, 0, w, h, 1);

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

#ifndef PGL_NO_STENCIL
	c->clear_stencil = 0;

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
#endif

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
#ifndef PGL_UNSAFE
	c->dbg_callback = dflt_dbg_callback;
	c->dbg_output = GL_TRUE;
#else
	c->dbg_callback = NULL;
	c->dbg_output = GL_FALSE;
#endif

	// program 0 is supposed to be undefined but not invalid so I'll
	// just make it default, no transform, just draws things red
	glProgram tmp_prog = { default_vs, default_fs, NULL, 0, {0}, GL_FALSE, GL_FALSE };
	cvec_push_glProgram(&c->programs, tmp_prog);
	glUseProgram(0);

	// setup default vertex_array (vao) at position 0
	// we're like a compatibility profile for this but come on
	// no reason not to have this imo
	// https://www.opengl.org/wiki/Vertex_Specification#Vertex_Array_Object
	glVertex_Array tmp_va;
	init_glVertex_Array(&tmp_va);
	cvec_push_glVertex_Array(&c->vertex_arrays, tmp_va);
	c->cur_vertex_array = 0;

	// buffer 0 is invalid
	glBuffer tmp_buf = {0};
	tmp_buf.user_owned = GL_TRUE;
	tmp_buf.deleted = GL_FALSE;
	cvec_push_glBuffer(&c->buffers, tmp_buf);

	// From glBindTexture():
	// "The value zero is reserved to represent the default texture for each texture target."
	// "In effect, the texture targets become aliases for the textures currently bound to them, and the texture name zero refers to the default textures that were bound to them at initialization."
	//
	// ... which means we can't use the 0 index at all as it can obviously only
	// be one type/target at a time and it would be a pain regardless
	// Still we might as well initialize it since something has to be there
	glTexture tmp_tex;
	INIT_TEX(&tmp_tex, GL_TEXTURE_UNBOUND);
	cvec_push_glTexture(&c->textures, tmp_tex);

	// Initialize the actual default textures..
	// TODO Should I initialize them as their actual types? no
	// Should I do the non-spec white pixel thing?
	for (int i=0; i<GL_NUM_TEXTURE_TYPES-GL_TEXTURE_UNBOUND-1; i++) {
		INIT_TEX(&c->default_textures[i], GL_TEXTURE_UNBOUND);
	}

	// default texture (0) is bound to all targets initially
	memset(c->bound_textures, 0, sizeof(c->bound_textures));

	// invalid buffer (0) bound initially
	memset(c->bound_buffers, 0, sizeof(c->bound_buffers));

	// DRY, do all buffer allocs/init in here
	if (w && h && !pglResizeFramebuffer(w, h)) {
#ifndef PGL_NO_DEPTH_NO_STENCIL
		PGL_FREE(c->zbuf.buf);
#if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
		PGL_FREE(c->stencil_buf.buf);
#endif
#endif
		if (!c->user_alloced_backbuf) {
			PGL_FREE(c->back_buffer.buf);
		}
		return GL_FALSE;
	}

	*back = (pix_t*)c->back_buffer.buf;

	return GL_TRUE;
}

PGLDEF void free_glContext(glContext* ctx)
{
	int i;
#ifndef PGL_NO_DEPTH_NO_STENCIL
	PGL_FREE(ctx->zbuf.buf);
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
	PGL_FREE(ctx->stencil_buf.buf);
#  endif
#endif
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

	PGL_FREE(ctx->vs_output.output_buf);

	if (c == ctx) {
		c = NULL;
	}
}

PGLDEF void set_glContext(glContext* context)
{
	c = context;
}

PGLDEF GLboolean pglResizeFramebuffer(GLsizei w, GLsizei h)
{
	PGL_ERR_RET_VAL((w < 0 || h < 0), GL_INVALID_VALUE, GL_FALSE);

	// TODO C standard doesn't guarantee that passing the same size to
	// realloc is a no-op and will return the same pointer
	// NOTE checking zbuf because of the separation between pglSetBackBuffer()
	// and pglResizeFramebuffer(). If the former is called before the latter
	// backbuf dimensions would compare the same to the new size even when
	// we still need to update stencil and zbuf
	if (w == c->zbuf.w && h == c->zbuf.h) {
		return GL_TRUE; // no resize necessary = success to me
	}

	u8* tmp;

	if (!c->user_alloced_backbuf) {
		tmp = (u8*)PGL_REALLOC(c->back_buffer.buf, w*h * sizeof(pix_t));
		PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);
		c->back_buffer.buf = tmp;
		c->back_buffer.w = w;
		c->back_buffer.h = h;
		c->back_buffer.lastrow = c->back_buffer.buf + (h-1)*w*sizeof(pix_t);
	}

#ifdef PGL_D24S8
	tmp = (u8*)PGL_REALLOC(c->zbuf.buf, w*h * sizeof(u32));
	PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);

	c->zbuf.buf = tmp;
	c->zbuf.w = w;
	c->zbuf.h = h;
	c->zbuf.lastrow = c->zbuf.buf + (h-1)*w*sizeof(u32);

	// not checking for NO_STENCIL here because it makes no sense not to
	// have it if you're already using the space
	c->stencil_buf.buf = tmp;
	c->stencil_buf.w = w;
	c->stencil_buf.h = h;
	c->stencil_buf.lastrow = c->stencil_buf.buf + (h-1)*w*sizeof(u32);
#elif defined(PGL_D16)
	tmp = (u8*)PGL_REALLOC(c->zbuf.buf, w*h * sizeof(u16));
	PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);

	c->zbuf.buf = tmp;
	c->zbuf.w = w;
	c->zbuf.h = h;
	c->zbuf.lastrow = c->zbuf.buf + (h-1)*w*sizeof(u16);

#ifndef PGL_NO_STENCIL
	tmp = (u8*)PGL_REALLOC(c->stencil_buf.buf, w*h);
	PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);
	c->stencil_buf.buf = tmp;
	c->stencil_buf.w = w;
	c->stencil_buf.h = h;
	c->stencil_buf.lastrow = c->stencil_buf.buf + (h-1)*w;
#endif
#endif

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

	return GL_TRUE;
}



PGLDEF GLubyte* glGetString(GLenum name)
{
	static GLubyte vendor[] = "Robert Winkler (robertwinkler.com)";
	static GLubyte renderer[] = "PortableGL 0.100.0";
	static GLubyte version[] = "0.100.0";
	static GLubyte shading_language[] = "C/C++";

	switch (name) {
	case GL_VENDOR:                   return vendor;
	case GL_RENDERER:                 return renderer;
	case GL_VERSION:                  return version;
	case GL_SHADING_LANGUAGE_VERSION: return shading_language;
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
		return NULL;
	}
}

PGLDEF GLenum glGetError(void)
{
	GLenum err = c->error;
	c->error = GL_NO_ERROR;
	return err;
}

PGLDEF void glGenVertexArrays(GLsizei n, GLuint* arrays)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);

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

PGLDEF void glDeleteVertexArrays(GLsizei n, const GLuint* arrays)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	for (int i=0; i<n; ++i) {
		if (!arrays[i] || arrays[i] >= c->vertex_arrays.size)
			continue;

		// NOTE/TODO: This is non-standard behavior even in a compatibility profile but it
		// is similar to (from the user's perspective) how GL handles DeleteProgram called on
		// the active program.  So instead of getting a blank screen immediately, you just
		// free up the name moving the current vao to the default 0. Of course if you're switching
		// between VAOs and bind to the old name, you will get a GL error even if it still works
		// (because VAOS are POD and I don't overwrite it)... so maybe I should just have an
		// error here
		if (arrays[i] == c->cur_vertex_array) {
			memcpy(&c->vertex_arrays.a[0], &c->vertex_arrays.a[arrays[i]], sizeof(glVertex_Array));
			c->cur_vertex_array = 0;
		}

		c->vertex_arrays.a[arrays[i]].deleted = GL_TRUE;
	}
}

PGLDEF void glGenBuffers(GLsizei n, GLuint* buffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
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

PGLDEF void glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
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

PGLDEF void glGenTextures(GLsizei n, GLuint* textures)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
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
			c->textures.a[i].data = NULL;
			textures[j++] = i;
		}
	}
}

PGLDEF void glCreateTextures(GLenum target, GLsizei n, GLuint* textures)
{
	PGL_ERR((target < GL_TEXTURE_1D || target >= GL_NUM_TEXTURE_TYPES), GL_INVALID_ENUM);
	PGL_ERR(n < 0, GL_INVALID_VALUE);

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

PGLDEF void glDeleteTextures(GLsizei n, const GLuint* textures)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
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

		c->textures.a[textures[i]].type = GL_TEXTURE_UNBOUND;
		c->textures.a[textures[i]].data = NULL;
		c->textures.a[textures[i]].deleted = GL_TRUE;
		c->textures.a[textures[i]].user_owned = GL_FALSE;
	}
}

PGLDEF void glBindVertexArray(GLuint array)
{
	PGL_ERR((array >= c->vertex_arrays.size || c->vertex_arrays.a[array].deleted), GL_INVALID_OPERATION);

	c->cur_vertex_array = array;
	c->bound_buffers[GL_ELEMENT_ARRAY_BUFFER-GL_ARRAY_BUFFER] = c->vertex_arrays.a[array].element_buffer;
}

PGLDEF void glBindBuffer(GLenum target, GLuint buffer)
{
	PGL_ERR(target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER, GL_INVALID_ENUM);

	PGL_ERR((buffer >= c->buffers.size || c->buffers.a[buffer].deleted), GL_INVALID_OPERATION);

	target -= GL_ARRAY_BUFFER;
	c->bound_buffers[target] = buffer;

	// Note type isn't set till binding and we're not storing the raw
	// enum but the enum - GL_ARRAY_BUFFER so it's an index into c->bound_buffers
	// TODO need to see what's supposed to happen if you try to bind
	// a buffer to multiple targets
	c->buffers.a[buffer].type = target;

	if (target == GL_ELEMENT_ARRAY_BUFFER - GL_ARRAY_BUFFER) {
		c->vertex_arrays.a[c->cur_vertex_array].element_buffer = buffer;
	}
}

// TODO reuse code, call glNamedBufferData() internally, remove duplicated error checks?
PGLDEF void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	//TODO check for usage later
	PGL_UNUSED(usage);

	PGL_ERR((target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER), GL_INVALID_ENUM);
	PGL_ERR(size < 0, GL_INVALID_VALUE);

	target -= GL_ARRAY_BUFFER;
	PGL_ERR(!c->bound_buffers[target], GL_INVALID_OPERATION);

	// the spec says any pre-existing data store is deleted but there's no reason to
	// c->buffers.a[c->bound_buffers[target]].data is always NULL or valid
	u8* tmp = (u8*)PGL_REALLOC(c->buffers.a[c->bound_buffers[target]].data, size);
	PGL_ERR(!tmp, GL_OUT_OF_MEMORY);

	c->buffers.a[c->bound_buffers[target]].data = tmp;

	if (data) {
		memcpy(c->buffers.a[c->bound_buffers[target]].data, data, size);
	}

	c->buffers.a[c->bound_buffers[target]].user_owned = GL_FALSE;
	c->buffers.a[c->bound_buffers[target]].size = size;
}

PGLDEF void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	PGL_ERR(target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER, GL_INVALID_ENUM);
	PGL_ERR((offset < 0 || size < 0), GL_INVALID_VALUE);

	target -= GL_ARRAY_BUFFER;

	PGL_ERR(!c->bound_buffers[target], GL_INVALID_OPERATION);
	PGL_ERR((offset + size > c->buffers.a[c->bound_buffers[target]].size), GL_INVALID_VALUE);

	memcpy(&c->buffers.a[c->bound_buffers[target]].data[offset], data, size);
}

PGLDEF void glNamedBufferData(GLuint buffer, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	//check for usage later
	PGL_UNUSED(usage);

	PGL_ERR((!buffer || buffer >= c->buffers.size || c->buffers.a[buffer].deleted), GL_INVALID_OPERATION);
	PGL_ERR(size < 0, GL_INVALID_VALUE);

	//always NULL or valid
	PGL_FREE(c->buffers.a[buffer].data);

	c->buffers.a[buffer].data = (u8*)PGL_MALLOC(size);
	PGL_ERR(!c->buffers.a[buffer].data, GL_OUT_OF_MEMORY);

	if (data) {
		memcpy(c->buffers.a[buffer].data, data, size);
	}

	c->buffers.a[buffer].user_owned = GL_FALSE;
	c->buffers.a[buffer].size = size;
}

PGLDEF void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	PGL_ERR((!buffer || buffer >= c->buffers.size || c->buffers.a[buffer].deleted), GL_INVALID_OPERATION);
	PGL_ERR((offset < 0 || size < 0), GL_INVALID_VALUE);
	PGL_ERR((offset + size > c->buffers.a[buffer].size), GL_INVALID_VALUE);

	memcpy(&c->buffers.a[buffer].data[offset], data, size);
}

// TODO see page 136-7 of spec
PGLDEF void glBindTexture(GLenum target, GLuint texture)
{
	PGL_ERR((target < GL_TEXTURE_1D || target >= GL_NUM_TEXTURE_TYPES), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;

	PGL_ERR((texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_VALUE);

	if (texture) {
		GLenum type = c->textures.a[texture].type;
		PGL_ERR((type != GL_TEXTURE_UNBOUND && type != target), GL_INVALID_OPERATION);

		if (type == GL_TEXTURE_UNBOUND) {
			INIT_TEX(&c->textures.a[texture], target);
		}
	}
	c->bound_textures[target] = texture;
}

static void set_texparami(glTexture* tex, GLenum pname, GLint param)
{
	/*
	PGL_ERR((pname != GL_TEXTURE_MIN_FILTER && pname != GL_TEXTURE_MAG_FILTER &&
	         pname != GL_TEXTURE_WRAP_S && pname != GL_TEXTURE_WRAP_T &&
	         pname != GL_TEXTURE_WRAP_R), GL_INVALID_ENUM);
	         */

	// NOTE, currently in the texture access functions
	// if it's not NEAREST, it assumes LINEAR so I could
	// just say that's good rather than these switch statements
	//
	// TODO compress this code
	if (pname == GL_TEXTURE_MIN_FILTER) {
		// TODO technically GL_TEXTURE_RECTANGLE can only have NEAREST OR LINEAR, no mipmapping
		// but since we don't actually do mipmaping or use min filter at all...
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
			PGL_SET_ERR(GL_INVALID_ENUM);
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
			PGL_SET_ERR(GL_INVALID_ENUM);
			return;
		}
		tex->min_filter = param;
		tex->mag_filter = param;
	} else if (pname == GL_TEXTURE_WRAP_S) {
		PGL_ERR((param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);

		// TODO This is in the standard but I don't really see the point, it costs nothing to support it,
		// maybe I'll make a PGL_WARN() macro or something
		//PGL_ERR((tex->type == GL_TEXTURE_RECTANGLE && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER), GL_INVALID_ENUM);
		tex->wrap_s = param;
	} else if (pname == GL_TEXTURE_WRAP_T) {
		PGL_ERR((param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);

		//PGL_ERR((tex->type == GL_TEXTURE_RECTANGLE && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER), GL_INVALID_ENUM);

		tex->wrap_t = param;
	} else if (pname == GL_TEXTURE_WRAP_R) {
		PGL_ERR((param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
		tex->wrap_r = param;
	} else {
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

// TODO handle ParameterI*() functions correctly
static void get_texparami(glTexture* tex, GLenum pname, GLenum type, GLvoid* params)
{
	GLenum val;
	switch (pname) {
	case GL_TEXTURE_MIN_FILTER: val = tex->min_filter; break;
	case GL_TEXTURE_MAG_FILTER: val = tex->mag_filter; break;
	case GL_TEXTURE_WRAP_S:
		PGL_ERR((pname != GL_REPEAT && pname != GL_CLAMP_TO_EDGE && pname != GL_CLAMP_TO_BORDER && pname != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
		val = tex->wrap_s;
		break;
	case GL_TEXTURE_WRAP_T:
		PGL_ERR((pname != GL_REPEAT && pname != GL_CLAMP_TO_EDGE && pname != GL_CLAMP_TO_BORDER && pname != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
		val = tex->wrap_t;
		break;
	case GL_TEXTURE_WRAP_R:
		PGL_ERR((pname != GL_REPEAT && pname != GL_CLAMP_TO_EDGE && pname != GL_CLAMP_TO_BORDER && pname != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
		val = tex->wrap_r;
		break;
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
		return;
	}

	if (type == GL_INT) {
		*(GLint*)params = val;
	} else {
		*(GLuint*)params = val;
	}
}

PGLDEF void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	//shift to range 0 - NUM_TEXTURES-1 to access bound_textures array
	target -= GL_TEXTURE_UNBOUND + 1;

	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	set_texparami(tex, pname, param);
}

PGLDEF void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	PGL_ERR((pname != GL_TEXTURE_BORDER_COLOR), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;
	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	memcpy(&tex->border_color, params, sizeof(GLfloat)*4);
#endif
}
PGLDEF void glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	PGL_ERR((pname != GL_TEXTURE_BORDER_COLOR), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;
	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}

	tex->border_color.x = (2*params[0] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.y = (2*params[1] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.z = (2*params[2] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.w = (2*params[3] + 1)/(UINT32_MAX - 1.0f);
#endif
}

// NOTE: I added the !texture checks to the glTextureParameter*() functions
// even though it's not in the spec because there's no way to know which
// default texture (0) target you're referring to
PGLDEF void glTextureParameteri(GLuint texture, GLenum pname, GLint param)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	set_texparami(&c->textures.a[texture], pname, param);
}

PGLDEF void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	memcpy(&c->textures.a[texture].border_color, params, sizeof(GLfloat)*4);
#endif
}

PGLDEF void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);

	glTexture* tex = &c->textures.a[texture];
	tex->border_color.x = (2*params[0] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.y = (2*params[1] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.z = (2*params[2] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.w = (2*params[3] + 1)/(UINT32_MAX - 1.0f);
#endif
}

PGLDEF void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	PGL_ERR((pname != GL_TEXTURE_BORDER_COLOR), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;
	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	memcpy(params, &tex->border_color, sizeof(GLfloat)*4);
#endif
}

PGLDEF void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;

	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	get_texparami(tex, pname, GL_INT, (GLvoid*)params);
}

PGLDEF void glGetTexParameterIiv(GLenum target, GLenum pname, GLint* params)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;

	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	get_texparami(tex, pname, GL_INT, (GLvoid*)params);
}

PGLDEF void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint* params)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;

	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	get_texparami(tex, pname, GL_UNSIGNED_INT, (GLvoid*)params);
}

PGLDEF void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	memcpy(params, &c->textures.a[texture].border_color, sizeof(GLfloat)*4);
#endif
}

PGLDEF void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	get_texparami(&c->textures.a[texture], pname, GL_UNSIGNED_INT, (GLvoid*)params);
}

PGLDEF void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	get_texparami(&c->textures.a[texture], pname, GL_UNSIGNED_INT, (GLvoid*)params);
}

PGLDEF void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	get_texparami(&c->textures.a[texture], pname, GL_UNSIGNED_INT, (GLvoid*)params);
}


PGLDEF void glPixelStorei(GLenum pname, GLint param)
{
	PGL_ERR((pname != GL_UNPACK_ALIGNMENT && pname != GL_PACK_ALIGNMENT), GL_INVALID_ENUM);

	PGL_ERR((param != 1 && param != 2 && param != 4 && param != 8), GL_INVALID_VALUE);

	// TODO eliminate branch? or use PGL_SET_ERR in else
	if (pname == GL_UNPACK_ALIGNMENT) {
		c->unpack_alignment = param;
	} else if (pname == GL_PACK_ALIGNMENT) {
		c->pack_alignment = param;
	}

}

// TODO check preprocessor output
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
		PGL_SET_ERR(GL_INVALID_ENUM); \
		return; \
	} \
	} while (0)

PGLDEF void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(level);
	PGL_UNUSED(internalformat);
	PGL_UNUSED(border);

	PGL_ERR(target != GL_TEXTURE_1D, GL_INVALID_ENUM);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	tex->w = width;
	tex->h = 1;
	tex->d = 1;

	// TODO NULL or valid ... but what if user_owned?
	PGL_FREE(tex->data);

	//TODO hardcoded 4 till I support more than RGBA/UBYTE internally
	tex->data = (u8*)PGL_MALLOC(width * 4);
	PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);

	u8* texdata = tex->data;

	if (data) {
		convert_format_to_packed_rgba(texdata, (u8*)data, width, 1, width*components, format);
	}

	tex->user_owned = GL_FALSE;
}

PGLDEF void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(level);
	PGL_UNUSED(internalformat);
	PGL_UNUSED(border);

	// TODO GL_TEXTURE_1D_ARRAY
	PGL_ERR((target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_1D_ARRAY &&
	         target != GL_TEXTURE_RECTANGLE &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z), GL_INVALID_ENUM);

	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);

	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	// Have to handle cubemaps specially since they have 1 real target
	// and 6 pseudo targets
	int target_idx;
	if (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
		//target is 2D, 1D_ARRAY, or RECTANGLE
		target_idx = target-GL_TEXTURE_UNBOUND-1;
	} else {
		target_idx = GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1;
	}
	int cur_tex_i = c->bound_textures[target_idx];

	// Have to handle 0 specially as well
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	// TODO If I ever support type other than GL_UNSIGNED_BYTE (also using for both internalformat and format)
	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	if (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
		//target is 2D, 1D_ARRAY, or RECTANGLE
		tex->w = width;
		tex->h = height;
		tex->d = 1;

		// either NULL or valid
		PGL_FREE(tex->data);

		//TODO support other internal formats? components should be of internalformat not format hardcoded 4 until I support more than RGBA
		tex->data = (u8*)PGL_MALLOC(height * width*4);
		PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);

		if (data) {
			convert_format_to_packed_rgba(tex->data, (u8*)data, width, height, padded_row_len, format);
		}

		tex->user_owned = GL_FALSE;

	} else {  //CUBE_MAP
		// If we're reusing a texture, and we haven't already loaded
		// one of the planes of the cubemap, data is either NULL or valid
		if (!tex->w)
			PGL_FREE(tex->data);

		// TODO specs say INVALID_VALUE, man/ref pages say INVALID_ENUM?
		// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
		PGL_ERR(width != height, GL_INVALID_VALUE);

		// TODO hardcoded 4 as long as we only support RGBA/UBYTES
		int mem_size = width*height*6 * 4;
		if (tex->w == 0) {
			tex->w = width;
			tex->h = width; //same cause square
			tex->d = 1;

			tex->data = (u8*)PGL_MALLOC(mem_size);
			PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
		} else if (tex->w != width) {
			//TODO spec doesn't say all sides must have same dimensions but it makes sense
			//and this site suggests it http://www.opengl.org/wiki/Cubemap_Texture
			PGL_SET_ERR(GL_INVALID_VALUE);
			return;
		}

		//use target as plane index
		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X;

		// TODO handle different format and internalformat
		int p = height*width*4;
		u8* texdata = tex->data;

		if (data) {
			convert_format_to_packed_rgba(&texdata[target*p], (u8*)data, width, height, padded_row_len, format);
		}

		tex->user_owned = GL_FALSE;
	} //end CUBE_MAP
}

PGLDEF void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(level);
	PGL_UNUSED(internalformat);
	PGL_UNUSED(border);

	PGL_ERR((target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY), GL_INVALID_ENUM);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((depth < 0 || depth > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	tex->w = width;
	tex->h = height;
	tex->d = depth;

	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	// NULL or valid
	PGL_FREE(tex->data);

	//TODO hardcoded 4 till I support more than RGBA/UBYTE internally
	tex->data = (u8*)PGL_MALLOC(width*height*depth * 4);
	PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);

	u8* texdata = tex->data;

	if (data) {
		convert_format_to_packed_rgba(texdata, (u8*)data, width, height*depth, padded_row_len, format);
	}

	tex->user_owned = GL_FALSE;
}

PGLDEF void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(level);

	PGL_ERR(target != GL_TEXTURE_1D, GL_INVALID_ENUM);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	PGL_ERR((xoffset < 0 || xoffset + width > tex->w), GL_INVALID_VALUE);

	u32* texdata = (u32*)tex->data;
	convert_format_to_packed_rgba((u8*)&texdata[xoffset], (u8*)data, width, 1, width*components, format);
}

PGLDEF void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(level);

	// TODO GL_TEXTURE_1D_ARRAY
	PGL_ERR((target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z), GL_INVALID_ENUM);

	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	// Have to handle cubemaps specially since they have 1 real target
	// and 6 pseudo targets
	int target_idx;
	if (target == GL_TEXTURE_2D) {
		target_idx = target-GL_TEXTURE_UNBOUND-1;
	} else {
		target_idx = GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1;
	}
	int cur_tex_i = c->bound_textures[target_idx];

	// Have to handle 0 specially as well
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	u8* d = (u8*)data;

	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	if (target == GL_TEXTURE_2D) {
		u32* texdata = (u32*)tex->data;

		PGL_ERR((xoffset < 0 || xoffset + width > tex->w || yoffset < 0 || yoffset + height > tex->h), GL_INVALID_VALUE);

		int w = tex->w;

		// TODO maybe better to covert the whole input image if
		// necessary then do the original memcpy's even with
		// the extra alloc and free
		for (int i=0; i<height; ++i) {
			convert_format_to_packed_rgba((u8*)&texdata[(yoffset+i)*w + xoffset], &d[i*padded_row_len], width, 1, padded_row_len, format);
		}

	} else {  //CUBE_MAP
		u32* texdata = (u32*)tex->data;

		int w = tex->w;

		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X; //use target as plane index

		int p = w*w;

		for (int i=0; i<height; ++i) {
			convert_format_to_packed_rgba((u8*)&texdata[p*target + (yoffset+i)*w + xoffset], &d[i*padded_row_len], width, 1, padded_row_len, format);
		}
	} //end CUBE_MAP
}

PGLDEF void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(level);

	PGL_ERR((target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY), GL_INVALID_ENUM);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((depth < 0 || depth > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	PGL_ERR((xoffset < 0 || xoffset + width > tex->w ||
	         yoffset < 0 || yoffset + height > tex->h ||
	         zoffset < 0 || zoffset + depth > tex->d), GL_INVALID_VALUE);

	int w = tex->w;
	int h = tex->h;
	int p = w*h;
	int pp = h*padded_row_len;
	u8* d = (u8*)data;
	u32* texdata = (u32*)tex->data;
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

PGLDEF void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer)
{
	// See Section 2.8 pages 37-38 of 3.3 compatiblity spec
	//
	// Compare with Section 2.8 page 29 of 3.3 core spec
	// plus section E.2.2, pg 344 (VAOs required for everything, no default/0 VAO)
	//
	// GLES 2 and 3 match 3.3 compatibility profile
	//
	// Basically, core got rid of client arrays entirely, while compatibility
	// allows them for the default/0 VAO.
	//
	// So for now I've decided to match the compatibility profile
	// but you can easily remove c->cur_vertex_array from the check
	// below to enable client arrays for all VAOs; there's not really
	// any downside in PGL, it's all RAM.
	PGL_ERR((c->cur_vertex_array && !c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER] && pointer),
	        GL_INVALID_OPERATION);

	PGL_ERR(stride < 0, GL_INVALID_VALUE);
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	PGL_ERR((size < 1 || size > 4), GL_INVALID_VALUE);

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
		PGL_SET_ERR(GL_INVALID_ENUM);
		return;
	}

	glVertex_Attrib* v = &(c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index]);
	v->size = size;
	v->type = type;
	v->normalized = normalized;
	v->stride = (stride) ? stride : size*type_sz;

	// offset can still really be a pointer if using the 0 VAO and no bound ARRAY_BUFFER.
	v->offset = (GLsizeiptr)pointer;
	// I put ARRAY_BUFFER-itself instead of 0 to reinforce that bound_buffers is indexed that way, buffer type - GL_ARRAY_BUFFER
	v->buf = c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER];
}

PGLDEF void glEnableVertexAttribArray(GLuint index)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_TRUE;
}

PGLDEF void glDisableVertexAttribArray(GLuint index)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_FALSE;
}

PGLDEF void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);

	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_TRUE;
}

PGLDEF void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_FALSE;
}

PGLDEF void glVertexAttribDivisor(GLuint index, GLuint divisor)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);

	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].divisor = divisor;
}



//TODO(rswinkle): Why is first, an index, a GLint and not GLuint or GLsizei?
PGLDEF void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR(count < 0, GL_INVALID_VALUE);

	if (!count)
		return;

	run_pipeline(mode, (GLvoid*)(GLintptr)first, count, 0, 0, GL_FALSE);
}

PGLDEF void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR(drawcount < 0, GL_INVALID_VALUE);

	for (GLsizei i=0; i<drawcount; i++) {
		if (!count[i]) continue;
		run_pipeline(mode, (GLvoid*)(GLintptr)first[i], count[i], 0, 0, GL_FALSE);
	}
}

PGLDEF void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR(count < 0, GL_INVALID_VALUE);

	// TODO error not in the spec but says type must be one of these ... strange
	PGL_ERR((type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT), GL_INVALID_ENUM);

	if (!count)
		return;

	run_pipeline(mode, indices, count, 0, 0, type);
}

// TODO fix
PGLDEF void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const GLvoid* const* indices, GLsizei drawcount)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR(drawcount < 0, GL_INVALID_VALUE);

	// TODO error not in the spec but says type must be one of these ... strange
	PGL_ERR((type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT), GL_INVALID_ENUM);

	for (GLsizei i=0; i<drawcount; i++) {
		if (!count[i]) continue;
		run_pipeline(mode, indices[i], count[i], 0, 0, type);
	}
}

PGLDEF void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR((count < 0 || instancecount < 0), GL_INVALID_VALUE);

	if (!count || !instancecount)
		return;

	for (GLsizei instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, (GLvoid*)(GLintptr)first, count, instance, 0, GL_FALSE);
	}
}

PGLDEF void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR((count < 0 || instancecount < 0), GL_INVALID_VALUE);

	if (!count || !instancecount)
		return;

	for (GLsizei instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, (GLvoid*)(GLintptr)first, count, instance, baseinstance, GL_FALSE);
	}
}


PGLDEF void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR((count < 0 || instancecount < 0), GL_INVALID_VALUE);

	// NOTE: error not in the spec but says type must be one of these ... strange
	PGL_ERR((type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT), GL_INVALID_ENUM);

	if (!count || !instancecount)
		return;

	for (GLsizei instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, indices, count, instance, 0, type);
	}
}

PGLDEF void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount, GLuint baseinstance)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR((count < 0 || instancecount < 0), GL_INVALID_VALUE);

	//error not in the spec but says type must be one of these ... strange
	PGL_ERR((type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT), GL_INVALID_ENUM);

	if (!count || !instancecount)
		return;

	for (GLsizei instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, indices, count, instance, baseinstance, GL_TRUE);
	}
}

PGLDEF void glDebugMessageCallback(GLDEBUGPROC callback, void* userParam)
{
	c->dbg_callback = callback;
	c->dbg_userparam = userParam;
}

PGLDEF void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	PGL_ERR((width < 0 || height < 0), GL_INVALID_VALUE);

	// TODO: Do I need a full matrix? Also I don't actually
	// use these values anywhere else so why save them?  See ref pages or TinyGL for alternative
	make_viewport_m4(c->vp_mat, x, y, width, height, 1);
	c->xmin = x;
	c->ymin = y;
	c->width = width;
	c->height = height;
}

PGLDEF void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	red = clamp_01(red);
	green = clamp_01(green);
	blue = clamp_01(blue);
	alpha = clamp_01(alpha);

	//vec4 tmp = { red, green, blue, alpha };
	//c->clear_color = vec4_to_Color(tmp);
	c->clear_color = RGBA_TO_PIXEL(red*PGL_RMAX, green*PGL_GMAX, blue*PGL_BMAX, alpha*PGL_AMAX);
}

PGLDEF void glClearDepthf(GLfloat depth)
{
	c->clear_depth = clamp_01(depth);
}

PGLDEF void glClearDepth(GLdouble depth)
{
	c->clear_depth = clamp_01(depth);
}

PGLDEF void glDepthFunc(GLenum func)
{
	PGL_ERR((func < GL_LESS || func > GL_NEVER), GL_INVALID_ENUM);

	c->depth_func = func;
}

PGLDEF void glDepthRangef(GLfloat nearVal, GLfloat farVal)
{
	c->depth_range_near = clamp_01(nearVal);
	c->depth_range_far = clamp_01(farVal);
}

PGLDEF void glDepthRange(GLdouble nearVal, GLdouble farVal)
{
	c->depth_range_near = clamp_01(nearVal);
	c->depth_range_far = clamp_01(farVal);
}

PGLDEF void glDepthMask(GLboolean flag)
{
	c->depth_mask = flag;
}

PGLDEF void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
#ifndef PGL_DISABLE_COLOR_MASK
	// !! ensures 1 or 0
	red = !!red;
	green = !!green;
	blue = !!blue;
	alpha = !!alpha;

	// By multiplying by the pixel format masks there's no need to shift them
	pix_t rmask = red*PGL_RMASK;
	pix_t gmask = green*PGL_GMASK;
	pix_t bmask = blue*PGL_BMASK;
	pix_t amask = alpha*PGL_AMASK;
	c->color_mask = rmask | gmask | bmask | amask;
#endif
}

PGLDEF void glClear(GLbitfield mask)
{
	// TODO: If a buffer is not present, then a glClear directed at that buffer has no effect.
	// right now they're all always present

	PGL_ERR((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)), GL_INVALID_VALUE);

	// NOTE: All buffers should have the same dimensions/size
	int sz = c->ux * c->uy;
	int w = c->back_buffer.w;

	pix_t color = c->clear_color;

#ifndef PGL_DISABLE_COLOR_MASK
	// clear out channels not enabled for writing
	// TODO are these casts really necessary?
	color &= (pix_t)c->color_mask;
	// used to erase channels to be written
	pix_t clear_mask = ~((pix_t)c->color_mask);
	pix_t tmp;
#endif

#ifndef PGL_NO_DEPTH_NO_STENCIL
	u32 cd = (u32)(c->clear_depth * PGL_MAX_Z) << PGL_ZSHIFT;
#  ifndef PGL_NO_STENCIL
    u8 cs = c->clear_stencil;
#  endif
#endif
	if (!c->scissor_test) {
		if (mask & GL_COLOR_BUFFER_BIT) {
			for (int i=0; i<sz; ++i) {
#ifdef PGL_DISABLE_COLOR_MASK
				((pix_t*)c->back_buffer.buf)[i] = color;
#else
				tmp = ((pix_t*)c->back_buffer.buf)[i];
				tmp &= clear_mask;
				((pix_t*)c->back_buffer.buf)[i] = tmp | color;
#endif
			}
		}
#ifndef PGL_NO_DEPTH_NO_STENCIL
		if (mask & GL_DEPTH_BUFFER_BIT && c->depth_mask) {
			for (int i=0; i < sz; ++i) {
				SET_Z_PRESHIFTED_TOP(i, cd);
			}
		}

#ifndef PGL_NO_STENCIL
		if (mask & GL_STENCIL_BUFFER_BIT) {
#  ifdef PGL_D16
			memset(c->stencil_buf.buf, cs, sz);
#  else
			for (int i=0; i < sz; ++i) {
				SET_STENCIL_TOP(i, cs);
			}
#  endif
		}
#  endif
#endif
	} else {
		// TODO this code is correct with or without scissor
		// enabled, test performance difference with above before
		// getting rid of above
		if (mask & GL_COLOR_BUFFER_BIT) {
			for (int y=c->ly; y<c->uy; ++y) {
				for (int x=c->lx; x<c->ux; ++x) {
					int i = -y*w + x;
#ifdef PGL_DISABLE_COLOR_MASK
					((pix_t*)c->back_buffer.lastrow)[i] = color;
#else
					tmp = ((pix_t*)c->back_buffer.lastrow)[i];
					tmp &= clear_mask;
					((pix_t*)c->back_buffer.lastrow)[i] = tmp | color;
#endif
				}
			}
		}
#ifndef PGL_NO_DEPTH_NO_STENCIL
		if (mask & GL_DEPTH_BUFFER_BIT && c->depth_mask) {
			for (int y=c->ly; y<c->uy; ++y) {
				for (int x=c->lx; x<c->ux; ++x) {
					int i = -y*w + x;
					SET_Z_PRESHIFTED(i, cd);
				}
			}
		}
#  ifndef PGL_NO_STENCIL
		if (mask & GL_STENCIL_BUFFER_BIT) {
			for (int y=c->ly; y<c->uy; ++y) {
				for (int x=c->lx; x<c->ux; ++x) {
					int i = -y*w + x;
					SET_STENCIL(i, cs);
				}
			}
		}
#  endif
#endif
	}
}

PGLDEF void glEnable(GLenum cap)
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
		c->line_smooth = GL_TRUE;
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
#ifndef PGL_NO_STENCIL
		c->stencil_test = GL_TRUE;
#endif
		break;
	case GL_DEBUG_OUTPUT:
		c->dbg_output = GL_TRUE;
		break;
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF void glDisable(GLenum cap)
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
#ifndef PGL_NO_STENCIL
		c->stencil_test = GL_FALSE;
#endif
		break;
	case GL_DEBUG_OUTPUT:
		c->dbg_output = GL_FALSE;
		break;
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF GLboolean glIsEnabled(GLenum cap)
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
#ifndef PGL_NO_STENCIL
	case GL_STENCIL_TEST: return c->stencil_test;
#endif
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}

	return GL_FALSE;
}

PGLDEF GLboolean glIsProgram(GLuint program)
{
	if (!program || program >= c->programs.size || c->programs.a[program].deleted) {
		return GL_FALSE;
	}

	return GL_TRUE;
}

PGLDEF void glGetBooleanv(GLenum pname, GLboolean* data)
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
#ifndef PGL_NO_STENCIL
	case GL_STENCIL_TEST:         *data = c->stencil_test;     break;
#endif
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF void glGetFloatv(GLenum pname, GLfloat* data)
{
	switch (pname) {
	case GL_POLYGON_OFFSET_FACTOR:         *data = c->poly_factor;         break;
	case GL_POLYGON_OFFSET_UNITS:          *data = c->poly_units;          break;
	case GL_POINT_SIZE:                    *data = c->point_size;          break;
	case GL_LINE_WIDTH:                    *data = c->line_width;          break;
	case GL_DEPTH_CLEAR_VALUE:             *data = c->clear_depth;         break;
	case GL_SMOOTH_LINE_WIDTH_GRANULARITY: *data = PGL_SMOOTH_GRANULARITY; break;

	case GL_MAX_TEXTURE_SIZE:         *data = PGL_MAX_TEXTURE_SIZE;         break;
	case GL_MAX_3D_TEXTURE_SIZE:      *data = PGL_MAX_3D_TEXTURE_SIZE;      break;
	case GL_MAX_ARRAY_TEXTURE_LAYERS: *data = PGL_MAX_ARRAY_TEXTURE_LAYERS; break;

	case GL_ALIASED_LINE_WIDTH_RANGE:
		data[0] = 1.0f;
		data[1] = PGL_MAX_ALIASED_WIDTH;
		break;

	case GL_SMOOTH_LINE_WIDTH_RANGE:
		data[0] = 1.0f;
		data[1] = PGL_MAX_SMOOTH_WIDTH;
		break;

	case GL_DEPTH_RANGE:
		data[0] = c->depth_range_near;
		data[1] = c->depth_range_near;
		break;
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF void glGetIntegerv(GLenum pname, GLint* data)
{
	// TODO maybe make all the enum/int member names match the associated ENUM?
	switch (pname) {
#ifndef PGL_NO_STENCIL
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
#endif

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

	case GL_MAX_TEXTURE_SIZE:         data[0] = PGL_MAX_TEXTURE_SIZE;         break;
	case GL_MAX_3D_TEXTURE_SIZE:      data[0] = PGL_MAX_3D_TEXTURE_SIZE;      break;
	case GL_MAX_ARRAY_TEXTURE_LAYERS: data[0] = PGL_MAX_ARRAY_TEXTURE_LAYERS; break;

	case GL_MAX_DEBUG_MESSAGE_LENGTH: data[0] = PGL_MAX_DEBUG_MESSAGE_LENGTH; break;

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
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF void glCullFace(GLenum mode)
{
	PGL_ERR((mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK), GL_INVALID_ENUM);
	c->cull_mode = mode;
}

PGLDEF void glFrontFace(GLenum mode)
{
	PGL_ERR((mode != GL_CCW && mode != GL_CW), GL_INVALID_ENUM);
	c->front_face = mode;
}

PGLDEF void glPolygonMode(GLenum face, GLenum mode)
{
	// TODO only support FRONT_AND_BACK like OpenGL 3/4 and OpenGL ES 2/3 ...
	// or keep support for FRONT and BACK like OpenGL 1 and 2?
	// Make final decision before version 1.0.0
	PGL_ERR(((face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) ||
	         (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL)), GL_INVALID_ENUM);

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

PGLDEF void glLineWidth(GLfloat width)
{
	PGL_ERR(width <= 0.0f, GL_INVALID_VALUE);
	c->line_width = width;
}

PGLDEF void glPointSize(GLfloat size)
{
	PGL_ERR(size <= 0.0f, GL_INVALID_VALUE);
	c->point_size = size;
}

PGLDEF void glPointParameteri(GLenum pname, GLint param)
{
	//also GL_POINT_FADE_THRESHOLD_SIZE
	PGL_ERR((pname != GL_POINT_SPRITE_COORD_ORIGIN ||
	        (param != GL_LOWER_LEFT && param != GL_UPPER_LEFT)), GL_INVALID_ENUM);

	c->point_spr_origin = param;
}

PGLDEF void glProvokingVertex(GLenum provokeMode)
{
	PGL_ERR((provokeMode != GL_FIRST_VERTEX_CONVENTION && provokeMode != GL_LAST_VERTEX_CONVENTION), GL_INVALID_ENUM);

	c->provoking_vert = provokeMode;
}


// Shader functions
PGLDEF GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean fragdepth_or_discard)
{
	// Using glAttachShader error if shader is not a shader object which
	// is the closest analog
	PGL_ERR_RET_VAL((!vertex_shader || !fragment_shader), GL_INVALID_OPERATION, 0);

	PGL_ERR_RET_VAL((n < 0 || n > GL_MAX_VERTEX_OUTPUT_COMPONENTS), GL_INVALID_VALUE, 0);

	glProgram tmp = {vertex_shader, fragment_shader, NULL, n, {0}, fragdepth_or_discard, GL_FALSE };
	for (int i=0; i<n; ++i) {
		tmp.interpolation[i] = interpolation[i];
	}

	for (int i=1; i<c->programs.size; ++i) {
		if (c->programs.a[i].deleted && (GLuint)i != c->cur_program) {
			c->programs.a[i] = tmp;
			return i;
		}
	}

	cvec_push_glProgram(&c->programs, tmp);
	return c->programs.size-1;
}

// Doesn't really do anything except mark for re-use, you
// could still use it even if it wasn't current as long as
// no new program get's assigned to the same spot
PGLDEF void glDeleteProgram(GLuint program)
{
	// This check isn't really necessary since "deleting" only marks it
	// and CreateProgram will never overwrite the 0/default shader
	if (!program)
		return;

	PGL_ERR(program >= c->programs.size, GL_INVALID_VALUE);

	c->programs.a[program].deleted = GL_TRUE;
}

PGLDEF void glUseProgram(GLuint program)
{
	// Not a problem if program is marked "deleted" already
	PGL_ERR(program >= c->programs.size, GL_INVALID_VALUE);

	c->vs_output.size = c->programs.a[program].vs_output_size;
	// c->vs_output.output_buf was pre-allocated to max size needed in init_glContext
	// otherwise would need to assure it's at least
	// c->vs_output_size * PGL_MAX_VERTS * sizeof(float) right here
	c->vs_output.interpolation = c->programs.a[program].interpolation;
	c->fragdepth_or_discard = c->programs.a[program].fragdepth_or_discard;

	c->cur_program = program;
}

PGLDEF void pglSetUniform(void* uniform)
{
	//TODO check for NULL? definitely if I ever switch to storing a local
	//copy in glProgram
	c->programs.a[c->cur_program].uniform = uniform;
}

PGLDEF void pglSetProgramUniform(GLuint program, void* uniform)
{
	// can set uniform for a "deleted" program ... but maybe I should still check and just
	// make an exception if it's the current program?
	PGL_ERR(program >= c->programs.size, GL_INVALID_OPERATION);

	c->programs.a[program].uniform = uniform;
}


PGLDEF void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	PGL_ERR((sfactor < GL_ZERO || sfactor >= NUM_BLEND_FUNCS || dfactor < GL_ZERO || dfactor >= NUM_BLEND_FUNCS), GL_INVALID_ENUM);

	c->blend_sRGB = sfactor;
	c->blend_sA = sfactor;
	c->blend_dRGB = dfactor;
	c->blend_dA = dfactor;
}

PGLDEF void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	PGL_ERR((srcRGB < GL_ZERO || srcRGB >= NUM_BLEND_FUNCS ||
	         dstRGB < GL_ZERO || dstRGB >= NUM_BLEND_FUNCS ||
	         srcAlpha < GL_ZERO || srcAlpha >= NUM_BLEND_FUNCS ||
	         dstAlpha < GL_ZERO || dstAlpha >= NUM_BLEND_FUNCS), GL_INVALID_ENUM);

	c->blend_sRGB = srcRGB;
	c->blend_sA = srcAlpha;
	c->blend_dRGB = dstRGB;
	c->blend_dA = dstAlpha;
}

PGLDEF void glBlendEquation(GLenum mode)
{
	PGL_ERR((mode < GL_FUNC_ADD || mode >= NUM_BLEND_EQUATIONS), GL_INVALID_ENUM);

	c->blend_eqRGB = mode;
	c->blend_eqA = mode;
}

PGLDEF void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	PGL_ERR((modeRGB < GL_FUNC_ADD || modeRGB >= NUM_BLEND_EQUATIONS ||
	    modeAlpha < GL_FUNC_ADD || modeAlpha >= NUM_BLEND_EQUATIONS), GL_INVALID_ENUM);

	c->blend_eqRGB = modeRGB;
	c->blend_eqA = modeAlpha;
}

PGLDEF void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	SET_V4(c->blend_color, clamp_01(red), clamp_01(green), clamp_01(blue), clamp_01(alpha));
}

PGLDEF void glLogicOp(GLenum opcode)
{
	PGL_ERR((opcode < GL_CLEAR || opcode > GL_INVERT), GL_INVALID_ENUM);
	c->logic_func = opcode;
}

PGLDEF void glPolygonOffset(GLfloat factor, GLfloat units)
{
	c->poly_factor = factor;
	c->poly_units = units;
}

PGLDEF void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	PGL_ERR((width < 0 || height < 0), GL_INVALID_VALUE);

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

#ifndef PGL_NO_STENCIL
PGLDEF void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	PGL_ERR((func < GL_LESS || func > GL_NEVER), GL_INVALID_ENUM);

	c->stencil_func = func;
	c->stencil_func_back = func;

	// TODO clamp byte function?
	clampi(ref, 0, 255);

	c->stencil_ref = ref;
	c->stencil_ref_back = ref;

	c->stencil_valuemask = mask;
	c->stencil_valuemask_back = mask;
}

PGLDEF void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	PGL_ERR((face < GL_FRONT || face > GL_FRONT_AND_BACK), GL_INVALID_ENUM);
	PGL_ERR((func < GL_LESS || func > GL_NEVER), GL_INVALID_ENUM);

	// TODO clamp byte function?
	clampi(ref, 0, 255);

	// Any better way to do this? I don't call glStencilFunc in case
	// I ever want/need debugging/logging info to show the function call
	if (face == GL_FRONT) {
		c->stencil_func = func;
		c->stencil_ref = ref;
		c->stencil_valuemask = mask;
	} else if (face == GL_BACK) {
		c->stencil_func_back = func;
		c->stencil_ref_back = ref;
		c->stencil_valuemask_back = mask;
	} else {
		c->stencil_func = func;
		c->stencil_ref = ref;
		c->stencil_valuemask = mask;

		c->stencil_func_back = func;
		c->stencil_ref_back = ref;
		c->stencil_valuemask_back = mask;
	}
}

PGLDEF void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)
{
	PGL_ERR((((sfail < GL_INVERT || sfail > GL_DECR_WRAP) && sfail != GL_ZERO) ||
	        ((dpfail < GL_INVERT || dpfail > GL_DECR_WRAP) && dpfail != GL_ZERO) ||
	        ((dppass < GL_INVERT || dppass > GL_DECR_WRAP) && dppass != GL_ZERO)), GL_INVALID_ENUM);

	c->stencil_sfail = sfail;
	c->stencil_dpfail = dpfail;
	c->stencil_dppass = dppass;

	c->stencil_sfail_back = sfail;
	c->stencil_dpfail_back = dpfail;
	c->stencil_dppass_back = dppass;
}

PGLDEF void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
	PGL_ERR((face < GL_FRONT || face > GL_FRONT_AND_BACK), GL_INVALID_ENUM);

	PGL_ERR((((sfail < GL_INVERT || sfail > GL_DECR_WRAP) && sfail != GL_ZERO) ||
	        ((dpfail < GL_INVERT || dpfail > GL_DECR_WRAP) && dpfail != GL_ZERO) ||
	        ((dppass < GL_INVERT || dppass > GL_DECR_WRAP) && dppass != GL_ZERO)), GL_INVALID_ENUM);


	if (face == GL_FRONT) {
		c->stencil_sfail = sfail;
		c->stencil_dpfail = dpfail;
		c->stencil_dppass = dppass;
	} else if (face == GL_BACK) {
		c->stencil_sfail_back = sfail;
		c->stencil_dpfail_back = dpfail;
		c->stencil_dppass_back = dppass;
	} else {
		c->stencil_sfail = sfail;
		c->stencil_dpfail = dpfail;
		c->stencil_dppass = dppass;

		c->stencil_sfail_back = sfail;
		c->stencil_dpfail_back = dpfail;
		c->stencil_dppass_back = dppass;
	}
}

PGLDEF void glClearStencil(GLint s)
{
	c->clear_stencil = s & PGL_STENCIL_MASK;
}

PGLDEF void glStencilMask(GLuint mask)
{
	c->stencil_writemask = mask;
	c->stencil_writemask_back = mask;
}

PGLDEF void glStencilMaskSeparate(GLenum face, GLuint mask)
{
	PGL_ERR((face < GL_FRONT || face > GL_FRONT_AND_BACK), GL_INVALID_ENUM);

	if (face == GL_FRONT) {
		c->stencil_writemask = mask;
	} else if (face == GL_BACK) {
		c->stencil_writemask_back = mask;
	} else {
		c->stencil_writemask = mask;
		c->stencil_writemask_back = mask;
	}
}
#endif


// Just wrap my pgl extension getter, unmap does nothing
PGLDEF void* glMapBuffer(GLenum target, GLenum access)
{
	PGL_ERR_RET_VAL((target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER), GL_INVALID_ENUM, NULL);

	PGL_ERR_RET_VAL((access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE), GL_INVALID_ENUM, NULL);

	// adjust to access bound_buffers
	target -= GL_ARRAY_BUFFER;

	void* data = NULL;
	pglGetBufferData(c->bound_buffers[target], &data);
	return data;
}

PGLDEF void* glMapNamedBuffer(GLuint buffer, GLenum access)
{
	// TODO pglGetBufferData will verify buffer is valid, hmm
	PGL_ERR_RET_VAL((access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE), GL_INVALID_ENUM, NULL);

	void* data = NULL;
	pglGetBufferData(buffer, &data);
	return data;
}

