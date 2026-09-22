

#include <stdarg.h>


/******************************************
 * PORTABLEGL_IMPLEMENTATION
 ******************************************/

#include <stdio.h>
#include <float.h>

// for CHAR_BIT
#include <limits.h>

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

#ifndef PGL_UNSAFE
PGLDEF void dflt_dbg_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	PGL_UNUSED(source);
	PGL_UNUSED(type);
	PGL_UNUSED(id);
	PGL_UNUSED(severity);
	PGL_UNUSED(length);
	PGL_UNUSED(userParam);

	fprintf(stderr, "%s\n", message);
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

#ifndef PGL_DISABLE_COLOR_MASK
	for (int i = 0; i < GL_MAX_DRAW_BUFFERS; ++i) {
		c->color_writemask[i][0] = GL_TRUE;
		c->color_writemask[i][1] = GL_TRUE;
		c->color_writemask[i][2] = GL_TRUE;
		c->color_writemask[i][3] = GL_TRUE;
		c->color_mask_pix[i] = (pix_t)(PGL_RMASK | PGL_GMASK | PGL_BMASK | PGL_AMASK);
		c->color_mask_u8[i] = 0xFFFFFFFFu;
	}
#endif

	//initialize all vectors
	cvec_glVertex_Array(&c->vertex_arrays, 0, 3);
	cvec_glBuffer(&c->buffers, 0, 3);
	cvec_glProgram(&c->programs, 0, 3);
	cvec_glTexture(&c->textures, 0, 1);
	cvec_glFBO(&c->framebuffers, 0, 4);
	cvec_glRenderbuffer(&c->renderbuffers, 0, 4);
	cvec_glVertex(&c->glverts, 0, 10);

	c->bound_draw_framebuffer = 0;
	c->bound_read_framebuffer = 0;
	c->bound_renderbuffer = 0;
	c->fbo_redirected = GL_FALSE;
	c->mrt_active = GL_FALSE;
	c->fbo_color_is_rt = GL_FALSE;
#ifndef PGL_NO_DEPTH_NO_STENCIL
	c->zbuf_float = GL_FALSE;
	c->has_depth_buf = GL_TRUE;
#  ifndef PGL_NO_STENCIL
	c->has_stencil_buf = GL_TRUE;
#  else
	c->has_stencil_buf = GL_FALSE;
#  endif
#endif
	c->default_num_draw_buffers = 1;
	c->default_draw_buffers[0] = GL_BACK;
	for (int i = 1; i < GL_MAX_DRAW_BUFFERS; ++i)
		c->default_draw_buffers[i] = GL_NONE;
	c->num_draw_buffers = 1;
	c->draw_buffers[0] = GL_BACK;
	for (int i = 1; i < GL_MAX_DRAW_BUFFERS; ++i)
		c->draw_buffers[i] = GL_NONE;
	c->default_read_buffer = GL_BACK;
	c->read_buffer = GL_BACK;
	pgl_build_srgb_lut();
	memset(c->mrt_color, 0, sizeof(c->mrt_color));

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
	c->logic_ops = GL_FALSE;
	c->poly_offset_pt = GL_FALSE;
	c->poly_offset_line = GL_FALSE;
	c->poly_offset_fill = GL_FALSE;
	c->scissor_test = GL_FALSE;
	c->cube_map_seamless = GL_FALSE;

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
	for (int i = 0; i < GL_MAX_DRAW_BUFFERS; ++i) {
		c->blend[i] = GL_FALSE;
		c->blend_sRGB[i] = GL_ONE;
		c->blend_sA[i] = GL_ONE;
		c->blend_dRGB[i] = GL_ZERO;
		c->blend_dA[i] = GL_ZERO;
		c->blend_eqRGB[i] = GL_FUNC_ADD;
		c->blend_eqA[i] = GL_FUNC_ADD;
	}
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
	c->dbg_output_sync = GL_FALSE;

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
	// If an FBO is bound, restore window surfaces so we free the real buffers
	if (ctx->fbo_redirected) {
		ctx->back_buffer = ctx->window_back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
		ctx->zbuf = ctx->window_zbuf;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
		ctx->stencil_buf = ctx->window_stencil_buf;
#  elif defined(PGL_D24S8)
		ctx->stencil_buf = ctx->window_zbuf;
#  endif
#endif
		ctx->fbo_redirected = GL_FALSE;
	}

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
		pgl_free_texture_images(&ctx->textures.a[i]);
	}
	for (i=0; i<GL_NUM_TEXTURE_TYPES-GL_TEXTURE_UNBOUND-1; ++i) {
		pgl_free_texture_images(&ctx->default_textures[i]);
	}

	//free vectors
	cvec_free_glVertex_Array(&ctx->vertex_arrays);
	cvec_free_glBuffer(&ctx->buffers);
	cvec_free_glProgram(&ctx->programs);
	cvec_free_glTexture(&ctx->textures);
	cvec_free_glFBO(&ctx->framebuffers);
	for (i = 0; i < ctx->renderbuffers.size; ++i) {
		if (!ctx->renderbuffers.a[i].user_owned)
			PGL_FREE(ctx->renderbuffers.a[i].data);
	}
	cvec_free_glRenderbuffer(&ctx->renderbuffers);
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

PGLDEF glContext* get_glContext(void)
{
	return c;
}

PGLDEF GLboolean pglResizeFramebuffer(GLsizei w, GLsizei h)
{
	PGL_ERR_RET_VAL((w < 0 || h < 0), GL_INVALID_VALUE, GL_FALSE);

	// Resize the *window* default surfaces, not a bound FBO's attachments.
	glFramebuffer* bb = c->fbo_redirected ? &c->window_back_buffer : &c->back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
	glFramebuffer* zb = c->fbo_redirected ? &c->window_zbuf : &c->zbuf;
#else
	glFramebuffer* zb = NULL;
	PGL_UNUSED(zb);
#endif

	// TODO C standard doesn't guarantee that passing the same size to
	// realloc is a no-op and will return the same pointer
	// NOTE checking zbuf because of the separation between pglSetBackBuffer()
	// and pglResizeFramebuffer(). If the former is called before the latter
	// backbuf dimensions would compare the same to the new size even when
	// we still need to update stencil and zbuf
#ifndef PGL_NO_DEPTH_NO_STENCIL
	if (w == zb->w && h == zb->h) {
		return GL_TRUE; // no resize necessary = success to me
	}
#else
	if (w == bb->w && h == bb->h) {
		return GL_TRUE;
	}
#endif

	u8* tmp;

	if (!c->user_alloced_backbuf) {
		tmp = (u8*)PGL_REALLOC(bb->buf, w*h * sizeof(pix_t));
		PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);
		bb->buf = tmp;
		bb->w = w;
		bb->h = h;
		bb->lastrow = bb->buf + (h-1)*w*sizeof(pix_t);
		if (!c->fbo_redirected)
			c->back_buffer = *bb;
	}

#ifdef PGL_D24S8
	tmp = (u8*)PGL_REALLOC(zb->buf, w*h * sizeof(u32));
	PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);

	zb->buf = tmp;
	zb->w = w;
	zb->h = h;
	zb->lastrow = zb->buf + (h-1)*w*sizeof(u32);
	if (!c->fbo_redirected)
		c->zbuf = *zb;

	// not checking for NO_STENCIL here because it makes no sense not to
	// have it if you're already using the space
	// D24S8: stencil is packed into the same buffer (window surfaces only)
	if (!c->fbo_redirected) {
		c->stencil_buf.buf = tmp;
		c->stencil_buf.w = w;
		c->stencil_buf.h = h;
		c->stencil_buf.lastrow = c->stencil_buf.buf + (h-1)*w*sizeof(u32);
	} else {
		// window_zbuf updated; stencil shares that storage when restored
		c->window_zbuf = *zb;
	}
#elif defined(PGL_D16)
	tmp = (u8*)PGL_REALLOC(zb->buf, w*h * sizeof(u16));
	PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);

	zb->buf = tmp;
	zb->w = w;
	zb->h = h;
	zb->lastrow = zb->buf + (h-1)*w*sizeof(u16);
	if (!c->fbo_redirected)
		c->zbuf = *zb;
	else
		c->window_zbuf = *zb;

#ifndef PGL_NO_STENCIL
	{
		glFramebuffer* sb = c->fbo_redirected ? &c->window_stencil_buf : &c->stencil_buf;
		tmp = (u8*)PGL_REALLOC(sb->buf, w*h);
		PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);
		sb->buf = tmp;
		sb->w = w;
		sb->h = h;
		sb->lastrow = sb->buf + (h-1)*w;
		if (!c->fbo_redirected)
			c->stencil_buf = *sb;
	}
#endif
#endif

	pgl_update_clip_rect();

	return GL_TRUE;
}



PGLDEF GLubyte* glGetString(GLenum name)
{
	static GLubyte vendor[] = "Robert Winkler (robertwinkler.com)";
	static GLubyte renderer[] = "PortableGL 0.101.0";
	static GLubyte version[] = "0.101.0";
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
			c->textures.a[i].data_alloc = 0;
			c->textures.a[i].num_levels = 0;
			memset(c->textures.a[i].levels, 0, sizeof(c->textures.a[i].levels));
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

		pgl_free_texture_images(&c->textures.a[textures[i]]);

		c->textures.a[textures[i]].type = GL_TEXTURE_UNBOUND;
		c->textures.a[textures[i]].deleted = GL_TRUE;
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

static void set_texparami(glTexture* tex, GLenum pname, GLint param, const char* api)
{
	PGL_UNUSED(api);
	/*
	PGL_ERR((pname != GL_TEXTURE_MIN_FILTER && pname != GL_TEXTURE_MAG_FILTER &&
	         pname != GL_TEXTURE_WRAP_S && pname != GL_TEXTURE_WRAP_T &&
	         pname != GL_TEXTURE_WRAP_R), GL_INVALID_ENUM);
	         */

	// Store full min_filter enums (including *MIPMAP*); sampling maps them to
	// within-level NEAREST/LINEAR.  texture*Lod uses them for explicit LOD.
	if (pname == GL_TEXTURE_MIN_FILTER) {
		// RECTANGLE: only NEAREST or LINEAR
		if (tex->type == GL_TEXTURE_RECTANGLE - (GL_TEXTURE_UNBOUND + 1)) {
			PGL_ERR_NAMED((param != GL_NEAREST && param != GL_LINEAR), GL_INVALID_ENUM, api);
		} else {
			switch (param) {
			case GL_NEAREST:
			case GL_LINEAR:
			case GL_NEAREST_MIPMAP_NEAREST:
			case GL_NEAREST_MIPMAP_LINEAR:
			case GL_LINEAR_MIPMAP_NEAREST:
			case GL_LINEAR_MIPMAP_LINEAR:
				break;
			default:
				PGL_SET_ERR_RET_NAMED(GL_INVALID_ENUM, api);
			}
		}
		tex->min_filter = param;
	} else if (pname == GL_TEXTURE_MAG_FILTER) {
		// Mag filter is only NEAREST or LINEAR
		PGL_ERR_NAMED((param != GL_NEAREST && param != GL_LINEAR), GL_INVALID_ENUM, api);
		tex->mag_filter = param;
	} else if (pname == GL_TEXTURE_WRAP_S) {
		PGL_ERR_NAMED((param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT), GL_INVALID_ENUM, api);
#ifdef PGL_CORE_PROFILE
		// Core: RECTANGLE wrap is only CLAMP_TO_EDGE / CLAMP_TO_BORDER
		PGL_ERR_NAMED((tex->type == GL_TEXTURE_RECTANGLE - (GL_TEXTURE_UNBOUND + 1) &&
		         param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER), GL_INVALID_ENUM, api);
#endif
		tex->wrap_s = param;
	} else if (pname == GL_TEXTURE_WRAP_T) {
		PGL_ERR_NAMED((param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT), GL_INVALID_ENUM, api);
#ifdef PGL_CORE_PROFILE
		PGL_ERR_NAMED((tex->type == GL_TEXTURE_RECTANGLE - (GL_TEXTURE_UNBOUND + 1) &&
		         param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER), GL_INVALID_ENUM, api);
#endif
		tex->wrap_t = param;
	} else if (pname == GL_TEXTURE_WRAP_R) {
		PGL_ERR_NAMED((param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT), GL_INVALID_ENUM, api);
		tex->wrap_r = param;
	} else {
		PGL_SET_ERR_NAMED(GL_INVALID_ENUM, api);
	}
}

// TODO handle ParameterI*() functions correctly
static void get_texparami(glTexture* tex, GLenum pname, GLenum type, GLvoid* params, const char* api)
{
	PGL_UNUSED(api);
	GLenum val;
	switch (pname) {
	case GL_TEXTURE_MIN_FILTER: val = tex->min_filter; break;
	case GL_TEXTURE_MAG_FILTER: val = tex->mag_filter; break;
	case GL_TEXTURE_WRAP_S:
		PGL_ERR_NAMED((pname != GL_REPEAT && pname != GL_CLAMP_TO_EDGE && pname != GL_CLAMP_TO_BORDER && pname != GL_MIRRORED_REPEAT), GL_INVALID_ENUM, api);
		val = tex->wrap_s;
		break;
	case GL_TEXTURE_WRAP_T:
		PGL_ERR_NAMED((pname != GL_REPEAT && pname != GL_CLAMP_TO_EDGE && pname != GL_CLAMP_TO_BORDER && pname != GL_MIRRORED_REPEAT), GL_INVALID_ENUM, api);
		val = tex->wrap_t;
		break;
	case GL_TEXTURE_WRAP_R:
		PGL_ERR_NAMED((pname != GL_REPEAT && pname != GL_CLAMP_TO_EDGE && pname != GL_CLAMP_TO_BORDER && pname != GL_MIRRORED_REPEAT), GL_INVALID_ENUM, api);
		val = tex->wrap_r;
		break;
	default:
		PGL_SET_ERR_RET_NAMED(GL_INVALID_ENUM, api);
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
	set_texparami(tex, pname, param, __func__);
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
	set_texparami(&c->textures.a[texture], pname, param, __func__);
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
	get_texparami(tex, pname, GL_INT, (GLvoid*)params, __func__);
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
	get_texparami(tex, pname, GL_INT, (GLvoid*)params, __func__);
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
	get_texparami(tex, pname, GL_UNSIGNED_INT, (GLvoid*)params, __func__);
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
	get_texparami(&c->textures.a[texture], pname, GL_UNSIGNED_INT, (GLvoid*)params, __func__);
}

PGLDEF void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	get_texparami(&c->textures.a[texture], pname, GL_UNSIGNED_INT, (GLvoid*)params, __func__);
}

PGLDEF void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	get_texparami(&c->textures.a[texture], pname, GL_UNSIGNED_INT, (GLvoid*)params, __func__);
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
		PGL_SET_ERR_RET(GL_INVALID_ENUM); \
	} \
	} while (0)

// Copy height rows of tightly packed dst pixels from unpack-aligned src.
static void pgl_copy_unpack_rows(u8* dst, const u8* src, int width, int height, int bpp, int src_pitch)
{
	int row_bytes = width * bpp;
	for (int y = 0; y < height; ++y)
		memcpy(dst + (size_t)y * (size_t)row_bytes, src + (size_t)y * (size_t)src_pitch, (size_t)row_bytes);
}

// True if format is valid for GL_FLOAT storage (matches pglTextureImage* matrix).
static GLboolean pgl_teximage_float_format_ok(GLenum format)
{
	return format == GL_RED || format == GL_RG || format == GL_RGBA ||
	       format == GL_RGBA16F || format == GL_RGBA32F ||
	       format == GL_DEPTH_COMPONENT;
}

PGLDEF void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(border);

	PGL_ERR(target != GL_TEXTURE_1D, GL_INVALID_ENUM);
	PGL_ERR(level < 0, GL_INVALID_VALUE);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE && type != GL_FLOAT, GL_INVALID_ENUM);

	int components;
	if (type == GL_FLOAT) {
		PGL_ERR(!pgl_teximage_float_format_ok(format), GL_INVALID_ENUM);
		components = pgl_format_components(format);
	} else {
#ifdef PGL_DONT_CONVERT_TEXTURES
		PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
		components = 4;
#else
		CHECK_FORMAT_GET_COMP(format, components);
#endif
	}

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	PGL_ERR(level >= PGL_MAX_MIPMAP_LEVELS, GL_INVALID_VALUE);

	if (level == 0) {
		if (!tex->user_owned)
			PGL_FREE(tex->data);

		tex->w = width;
		tex->h = 1;
		tex->d = 1;

		if (type == GL_FLOAT) {
			pgl_tex_set_format(tex, format, GL_FLOAT);
			int bpp = pgl_tex_bytes_per_pixel(tex);
			size_t nbytes = (size_t)width * (size_t)bpp;
			tex->data = (u8*)PGL_MALLOC(nbytes ? nbytes : 1);
			PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
			tex->data_alloc = nbytes;
			if (data) {
				int src_pitch = width * bpp; // 1D: no row padding beyond unpack for single row
				int byte_width = width * bpp;
				int pad = byte_width % c->unpack_alignment;
				if (pad)
					src_pitch = byte_width + c->unpack_alignment - pad;
				pgl_copy_unpack_rows(tex->data, (const u8*)data, width, 1, bpp, src_pitch);
			} else {
				memset(tex->data, 0, nbytes ? nbytes : 1);
			}
		} else {
			size_t nbytes = pgl_rgba_bytes_1d(width);
			tex->data = (u8*)PGL_MALLOC(nbytes);
			PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
			tex->data_alloc = nbytes;
			if (data) {
				convert_format_to_packed_rgba(tex->data, (u8*)data, width, 1, width*components, format);
			}
			pgl_tex_set_format(tex, GL_RGBA, GL_UNSIGNED_BYTE);
			tex->is_srgb = pgl_internalformat_is_srgb(internalformat);
		}

		tex->user_owned = GL_FALSE;
		tex->num_levels = 1;
		pgl_set_level0_desc(tex);
	} else {
		// Higher levels require a defined base of the same storage
		PGL_ERR(!tex->data || tex->w <= 0, GL_INVALID_OPERATION);
		if (type == GL_FLOAT) {
			PGL_ERR(tex->datatype != GL_FLOAT, GL_INVALID_OPERATION);
			PGL_ERR(tex->is_depth != pgl_format_is_depth(format), GL_INVALID_OPERATION);
			if (!tex->is_depth)
				PGL_ERR(tex->components != pgl_format_components(format), GL_INVALID_OPERATION);
		} else {
			PGL_ERR(tex->is_depth || tex->datatype != GL_UNSIGNED_BYTE || tex->components != 4,
			        GL_INVALID_OPERATION);
		}
		PGL_ERR(width != pgl_mip_dim(tex->w, level), GL_INVALID_VALUE);

		// Call alloc outside PGL_ERR (PGL_UNSAFE empties the macro and would skip alloc)
		if (!pgl_alloc_mip_chain_1d(tex, level + 1)) {
			PGL_SET_ERR_RET(GL_OUT_OF_MEMORY);
		}

		if (data) {
			if (type == GL_FLOAT) {
				int bpp = pgl_tex_bytes_per_pixel(tex);
				pgl_copy_unpack_rows(tex->levels[level].data, (const u8*)data, width, 1, bpp,
				                     width * bpp);
			} else {
				convert_format_to_packed_rgba(tex->levels[level].data, (u8*)data, width, 1, width*components, format);
			}
		}
	}
}

PGLDEF void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
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

	PGL_ERR(level < 0, GL_INVALID_VALUE);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);

	PGL_ERR(type != GL_UNSIGNED_BYTE && type != GL_FLOAT, GL_INVALID_ENUM);

	// RECTANGLE: no mip chain
	if (target == GL_TEXTURE_RECTANGLE) {
		PGL_ERR(level != 0, GL_INVALID_VALUE);
	}

	int is_cube_face = (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X);
	int components;
	if (type == GL_FLOAT) {
		PGL_ERR(!pgl_teximage_float_format_ok(format), GL_INVALID_ENUM);
		// Cubemap float: color (R/RG/RGBA32F) or depth (point shadows)
		components = pgl_format_components(format);
	} else {
		if (is_cube_face)
			PGL_ERR(pgl_format_is_depth(format), GL_INVALID_ENUM);
#ifdef PGL_DONT_CONVERT_TEXTURES
		PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
		components = 4;
#else
		CHECK_FORMAT_GET_COMP(format, components);
#endif
	}

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

	int src_bpp = (type == GL_FLOAT) ? components * (int)sizeof(float) : components;
	int byte_width = width * src_bpp;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	PGL_ERR(level >= PGL_MAX_MIPMAP_LEVELS, GL_INVALID_VALUE);

	if (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
		//target is 2D, 1D_ARRAY, or RECTANGLE
		if (level == 0) {
			if (!tex->user_owned)
				PGL_FREE(tex->data);

			tex->w = width;
			tex->h = height;
			tex->d = 1;

			if (type == GL_FLOAT) {
				pgl_tex_set_format(tex, format, GL_FLOAT);
				int bpp = pgl_tex_bytes_per_pixel(tex);
				size_t nbytes = (size_t)width * (size_t)height * (size_t)bpp;
				tex->data = (u8*)PGL_MALLOC(nbytes ? nbytes : 1);
				PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
				tex->data_alloc = nbytes;
				if (data)
					pgl_copy_unpack_rows(tex->data, (const u8*)data, width, height, bpp, padded_row_len);
				else
					memset(tex->data, 0, nbytes ? nbytes : 1);
			} else {
				size_t nbytes = pgl_rgba_bytes_2d(width, height);
				tex->data = (u8*)PGL_MALLOC(nbytes);
				PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
				tex->data_alloc = nbytes;

				if (data) {
					convert_format_to_packed_rgba(tex->data, (u8*)data, width, height, padded_row_len, format);
				}
				pgl_tex_set_format(tex, GL_RGBA, GL_UNSIGNED_BYTE);
				tex->is_srgb = pgl_internalformat_is_srgb(internalformat);
			}

			tex->user_owned = GL_FALSE;
			tex->num_levels = 1;
			pgl_set_level0_desc(tex);
		} else {
			// Higher mip levels (2D / 1D_ARRAY)
			PGL_ERR(!tex->data || tex->w <= 0 || tex->h <= 0, GL_INVALID_OPERATION);
			if (type == GL_FLOAT) {
				PGL_ERR(tex->datatype != GL_FLOAT, GL_INVALID_OPERATION);
				PGL_ERR(tex->is_depth != pgl_format_is_depth(format), GL_INVALID_OPERATION);
				if (!tex->is_depth)
					PGL_ERR(tex->components != pgl_format_components(format), GL_INVALID_OPERATION);
			} else {
				PGL_ERR(tex->is_depth || tex->datatype != GL_UNSIGNED_BYTE || tex->components != 4,
				        GL_INVALID_OPERATION);
			}
			PGL_ERR(width != pgl_mip_dim(tex->w, level) || height != pgl_mip_dim(tex->h, level), GL_INVALID_VALUE);

			if (!pgl_alloc_mip_chain_2d(tex, level + 1)) {
				PGL_SET_ERR_RET(GL_OUT_OF_MEMORY);
			}

			if (data) {
				if (type == GL_FLOAT) {
					int bpp = pgl_tex_bytes_per_pixel(tex);
					pgl_copy_unpack_rows(tex->levels[level].data, (const u8*)data, width, height, bpp, padded_row_len);
				} else {
					convert_format_to_packed_rgba(tex->levels[level].data, (u8*)data, width, height, padded_row_len, format);
				}
			}
		}

	} else {  //CUBE_MAP
		// TODO specs say INVALID_VALUE, man/ref pages say INVALID_ENUM?
		// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
		PGL_ERR(width != height, GL_INVALID_VALUE);

		GLboolean is_float = (type == GL_FLOAT);
		int face = (int)(target - GL_TEXTURE_CUBE_MAP_POSITIVE_X);

		if (level == 0) {
			// If we're reusing a texture, and we haven't already loaded
			// one of the planes of the cubemap, data is either NULL or valid
			if (!tex->w) {
				if (!tex->user_owned)
					PGL_FREE(tex->data);
				tex->data = NULL;
				tex->data_alloc = 0;
				memset(tex->levels, 0, sizeof(tex->levels));
			}

			if (tex->w == 0) {
				tex->w = width;
				tex->h = width; //same cause square
				tex->d = 1;
				if (is_float) {
					pgl_tex_set_format(tex, format, GL_FLOAT);
				} else {
					pgl_tex_set_format(tex, GL_RGBA, GL_UNSIGNED_BYTE);
					tex->is_srgb = pgl_internalformat_is_srgb(internalformat);
				}
				size_t face_bytes = (size_t)width * (size_t)height * (size_t)pgl_tex_bytes_per_pixel(tex);
				size_t mem_size = face_bytes * 6u;
				tex->data = (u8*)PGL_MALLOC(mem_size ? mem_size : 1);
				PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
				tex->data_alloc = mem_size;
				memset(tex->data, 0, mem_size ? mem_size : 1);
				tex->num_levels = 1;
				pgl_set_level0_desc(tex);
			} else if (tex->w != width) {
				//TODO spec doesn't say all sides must have same dimensions but it makes sense
				//and this site suggests it http://www.opengl.org/wiki/Cubemap_Texture
				PGL_SET_ERR_RET(GL_INVALID_VALUE);
			} else if (is_float) {
				PGL_ERR(tex->datatype != GL_FLOAT, GL_INVALID_OPERATION);
				PGL_ERR(tex->is_depth != pgl_format_is_depth(format), GL_INVALID_OPERATION);
				if (!tex->is_depth)
					PGL_ERR(tex->components != pgl_format_components(format), GL_INVALID_OPERATION);
			} else {
				PGL_ERR(tex->is_depth || tex->datatype != GL_UNSIGNED_BYTE, GL_INVALID_OPERATION);
				PGL_ERR(pgl_internalformat_is_srgb(internalformat) != tex->is_srgb, GL_INVALID_OPERATION);
			}

			size_t face_bytes = (size_t)width * (size_t)height * (size_t)pgl_tex_bytes_per_pixel(tex);
			u8* dest = tex->data + (size_t)face * face_bytes;
			if (data) {
				int bpp = pgl_tex_bytes_per_pixel(tex);
				if (is_float)
					pgl_copy_unpack_rows(dest, (const u8*)data, width, height, bpp, padded_row_len);
				else
					convert_format_to_packed_rgba(dest, (u8*)data, width, height, padded_row_len, format);
			}

			tex->user_owned = GL_FALSE;
		} else {
			PGL_ERR(!tex->data || tex->w <= 0, GL_INVALID_OPERATION);
			if (is_float) {
				PGL_ERR(tex->datatype != GL_FLOAT, GL_INVALID_OPERATION);
				PGL_ERR(tex->is_depth != pgl_format_is_depth(format), GL_INVALID_OPERATION);
				if (!tex->is_depth)
					PGL_ERR(tex->components != pgl_format_components(format), GL_INVALID_OPERATION);
			} else {
				PGL_ERR(tex->is_depth || tex->datatype != GL_UNSIGNED_BYTE, GL_INVALID_OPERATION);
			}
			PGL_ERR(width != pgl_mip_dim(tex->w, level) || height != pgl_mip_dim(tex->h, level),
			        GL_INVALID_VALUE);

			if (!pgl_alloc_mip_chain_cube(tex, level + 1)) {
				PGL_SET_ERR_RET(GL_OUT_OF_MEMORY);
			}

			size_t face_bytes = (size_t)width * (size_t)height * (size_t)pgl_tex_bytes_per_pixel(tex);
			u8* dest = tex->levels[level].data + (size_t)face * face_bytes;
			if (data) {
				int bpp = pgl_tex_bytes_per_pixel(tex);
				if (is_float)
					pgl_copy_unpack_rows(dest, (const u8*)data, width, height, bpp, padded_row_len);
				else
					convert_format_to_packed_rgba(dest, (u8*)data, width, height, padded_row_len, format);
			}
		}
	} //end CUBE_MAP
}

PGLDEF void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(level);
	PGL_UNUSED(border);

	PGL_ERR((target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY), GL_INVALID_ENUM);
	PGL_ERR(type != GL_UNSIGNED_BYTE && type != GL_FLOAT, GL_INVALID_ENUM);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((depth < 0 || depth > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);

	int components;
	if (type == GL_FLOAT) {
		PGL_ERR(!pgl_teximage_float_format_ok(format), GL_INVALID_ENUM);
		components = pgl_format_components(format);
	} else {
#ifdef PGL_DONT_CONVERT_TEXTURES
		PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
		components = 4;
#else
		CHECK_FORMAT_GET_COMP(format, components);
#endif
	}

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	// 3D mips not supported yet; level is still ignored but base is level 0
	if (!tex->user_owned)
		PGL_FREE(tex->data);

	tex->w = width;
	tex->h = height;
	tex->d = depth;

	int src_bpp = (type == GL_FLOAT) ? components * (int)sizeof(float) : components;
	int byte_width = width * src_bpp;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	if (type == GL_FLOAT) {
		pgl_tex_set_format(tex, format, GL_FLOAT);
		int bpp = pgl_tex_bytes_per_pixel(tex);
		size_t nbytes = (size_t)width * (size_t)height * (size_t)depth * (size_t)bpp;
		tex->data = (u8*)PGL_MALLOC(nbytes ? nbytes : 1);
		PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
		tex->data_alloc = nbytes;
		if (data) {
			// Treat as height*depth rows of width texels (same as U8 path)
			pgl_copy_unpack_rows(tex->data, (const u8*)data, width, height * depth, bpp, padded_row_len);
		} else {
			memset(tex->data, 0, nbytes ? nbytes : 1);
		}
	} else {
		size_t nbytes = (size_t)width * height * depth * 4;
		tex->data = (u8*)PGL_MALLOC(nbytes);
		PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
		tex->data_alloc = nbytes;

		if (data) {
			convert_format_to_packed_rgba(tex->data, (u8*)data, width, height*depth, padded_row_len, format);
		}
		pgl_tex_set_format(tex, GL_RGBA, GL_UNSIGNED_BYTE);
		tex->is_srgb = pgl_internalformat_is_srgb(internalformat);
	}

	tex->user_owned = GL_FALSE;
	tex->num_levels = 1;
	pgl_set_level0_desc(tex);
}

PGLDEF void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_ERR(target != GL_TEXTURE_1D, GL_INVALID_ENUM);
	PGL_ERR(level < 0, GL_INVALID_VALUE);
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

	PGL_ERR(level >= tex->num_levels || !tex->levels[level].data, GL_INVALID_OPERATION);

	GLsizei tw = tex->levels[level].w;
	u8* level_data = tex->levels[level].data;

	PGL_ERR((xoffset < 0 || xoffset + width > tw), GL_INVALID_VALUE);

	u32* texdata = (u32*)level_data;
	convert_format_to_packed_rgba((u8*)&texdata[xoffset], (u8*)data, width, 1, width*components, format);
}

PGLDEF void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data)
{
	// TODO GL_TEXTURE_1D_ARRAY
	PGL_ERR((target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z), GL_INVALID_ENUM);

	PGL_ERR(level < 0, GL_INVALID_VALUE);
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
		PGL_ERR(level >= tex->num_levels || !tex->levels[level].data, GL_INVALID_OPERATION);

		GLsizei tw = tex->levels[level].w;
		GLsizei th = tex->levels[level].h;
		u8* level_data = tex->levels[level].data;

		PGL_ERR((xoffset < 0 || xoffset + width > tw || yoffset < 0 || yoffset + height > th), GL_INVALID_VALUE);

		u32* texdata = (u32*)level_data;
		int w = tw;

		// TODO maybe better to covert the whole input image if
		// necessary then do the original memcpy's even with
		// the extra alloc and free
		for (int i=0; i<height; ++i) {
			convert_format_to_packed_rgba((u8*)&texdata[(yoffset+i)*w + xoffset], &d[i*padded_row_len], width, 1, padded_row_len, format);
		}

	} else {  //CUBE_MAP
		PGL_ERR(level >= tex->num_levels || !tex->levels[level].data, GL_INVALID_OPERATION);

		GLsizei tw = tex->levels[level].w;
		GLsizei th = tex->levels[level].h;
		PGL_ERR((xoffset < 0 || xoffset + width > tw || yoffset < 0 || yoffset + height > th), GL_INVALID_VALUE);

		int face = (int)(target - GL_TEXTURE_CUBE_MAP_POSITIVE_X);
		u8* dest_face = tex->levels[level].data + (size_t)face * (size_t)tw * (size_t)th * 4u;
		u32* texdata = (u32*)dest_face;

		for (int i=0; i<height; ++i) {
			convert_format_to_packed_rgba((u8*)&texdata[(yoffset+i)*tw + xoffset], &d[i*padded_row_len], width, 1, padded_row_len, format);
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

// 1D/2D/CUBE_MAP.  Builds a full box-filtered chain from level 0 into one
// contiguous allocation (U8 RGBA8 or float R/RG/RGBA).  Cubemap levels pack
// 6 faces each (~4/3 of L0 size).  3D/rectangle/depth not supported.
static void pgl_generate_mipmap_tex(glTexture* tex, GLenum target, const char* api)
{
	PGL_UNUSED(api);
	PGL_ERR_NAMED(!tex->data || tex->w <= 0, GL_INVALID_OPERATION, api);
	PGL_ERR_NAMED(tex->is_depth, GL_INVALID_OPERATION, api);
	if (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP) {
		PGL_ERR_NAMED(tex->h <= 0, GL_INVALID_OPERATION, api);
	}

	if (target == GL_TEXTURE_1D) {
		if (tex->w <= 1) {
			tex->num_levels = 1;
			pgl_set_level0_desc(tex);
			return;
		}

		int levels = 1;
		GLsizei dim = tex->w;
		while (dim > 1) {
			dim = dim / 2;
			levels++;
		}
		if (levels > PGL_MAX_MIPMAP_LEVELS)
			levels = PGL_MAX_MIPMAP_LEVELS;

		if (!pgl_alloc_mip_chain_1d(tex, levels)) {
			PGL_SET_ERR_RET_NAMED(GL_OUT_OF_MEMORY, api);
		}

		for (int level = 1; level < levels; ++level) {
			pgl_filter_level_1d(tex,
				tex->levels[level - 1].data, tex->levels[level - 1].w,
				tex->levels[level].data, tex->levels[level].w);
		}
		return;
	}

	if (target == GL_TEXTURE_CUBE_MAP) {
		// Faces are square; filter each of the 6 faces independently per level
		if (tex->w <= 1 && tex->h <= 1) {
			tex->num_levels = 1;
			pgl_set_level0_desc(tex);
			return;
		}

		int levels = 1;
		GLsizei cw = tex->w, ch = tex->h;
		while (cw > 1 || ch > 1) {
			cw = cw > 1 ? cw / 2 : 1;
			ch = ch > 1 ? ch / 2 : 1;
			levels++;
		}
		if (levels > PGL_MAX_MIPMAP_LEVELS)
			levels = PGL_MAX_MIPMAP_LEVELS;

		if (!pgl_alloc_mip_chain_cube(tex, levels)) {
			PGL_SET_ERR_RET_NAMED(GL_OUT_OF_MEMORY, api);
		}

		int bpp = pgl_tex_bytes_per_pixel(tex);
		for (int level = 1; level < levels; ++level) {
			GLsizei sw = tex->levels[level - 1].w;
			GLsizei sh = tex->levels[level - 1].h;
			GLsizei dw = tex->levels[level].w;
			GLsizei dh = tex->levels[level].h;
			size_t src_face = pgl_bytes_2d(sw, sh, bpp);
			size_t dst_face = pgl_bytes_2d(dw, dh, bpp);
			const u8* src = tex->levels[level - 1].data;
			u8* dst = tex->levels[level].data;
			for (int face = 0; face < 6; ++face) {
				pgl_filter_level_2d(tex, src + (size_t)face * src_face, sw, sh,
				                    dst + (size_t)face * dst_face, dw, dh);
			}
		}
		return;
	}

	// GL_TEXTURE_2D
	if (tex->w <= 1 && tex->h <= 1) {
		tex->num_levels = 1;
		pgl_set_level0_desc(tex);
		return;
	}

	int levels = 1;
	GLsizei cw = tex->w, ch = tex->h;
	while (cw > 1 || ch > 1) {
		cw = cw > 1 ? cw / 2 : 1;
		ch = ch > 1 ? ch / 2 : 1;
		levels++;
	}
	if (levels > PGL_MAX_MIPMAP_LEVELS)
		levels = PGL_MAX_MIPMAP_LEVELS;

	if (!pgl_alloc_mip_chain_2d(tex, levels)) {
		PGL_SET_ERR_RET_NAMED(GL_OUT_OF_MEMORY, api);
	}

	for (int level = 1; level < levels; ++level) {
		pgl_filter_level_2d(tex,
			tex->levels[level - 1].data, tex->levels[level - 1].w, tex->levels[level - 1].h,
			tex->levels[level].data, tex->levels[level].w, tex->levels[level].h);
	}
}

// DSA: texture must be a non-zero existing object (not default texture 0)
PGLDEF void glGenerateTextureMipmap(GLuint texture)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted),
	        GL_INVALID_OPERATION);

	glTexture* tex = &c->textures.a[texture];
	// type is stored as target - GL_TEXTURE_UNBOUND - 1
	GLenum target = tex->type + GL_TEXTURE_UNBOUND + 1;
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_CUBE_MAP), GL_INVALID_OPERATION);

	pgl_generate_mipmap_tex(tex, target, __func__);
}

PGLDEF void glGenerateMipmap(GLenum target)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	int target_idx = target - GL_TEXTURE_UNBOUND - 1;
	GLuint cur_tex = c->bound_textures[target_idx];
	glTexture* tex;
	if (cur_tex) {
		PGL_ERR((cur_tex >= c->textures.size || c->textures.a[cur_tex].deleted),
		        GL_INVALID_OPERATION);
		tex = &c->textures.a[cur_tex];
	} else {
		// Default texture for this target (DSA path rejects texture 0 regardless
		// of Core or Compatibility because it was defined against Core)
		//
		// Core profile removed default textures (0 is "unbound" instead)
		// Compatibility kept it but it's "Legacy"
		//
		// but since PGL is more Compatibility-ish, we need to allow it here
		// TODO PGL_CORE macro to enforce strict Core compliance?
		tex = &c->default_textures[target_idx];
	}
	pgl_generate_mipmap_tex(tex, target, __func__);
}

static int pgl_vertex_type_size(GLenum type)
{
	switch (type) {
	case GL_BYTE: case GL_UNSIGNED_BYTE: return (int)sizeof(GLbyte);
	case GL_SHORT: case GL_UNSIGNED_SHORT: return (int)sizeof(GLshort);
	case GL_INT: case GL_UNSIGNED_INT: return (int)sizeof(GLint);
	case GL_FLOAT: return (int)sizeof(GLfloat);
	case GL_DOUBLE: return (int)sizeof(GLdouble);
	default: return 0;
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

	int type_sz = pgl_vertex_type_size(type);
	PGL_ERR(!type_sz, GL_INVALID_ENUM);

	glVertex_Attrib* v = &(c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index]);
	v->size = size;
	v->type = type;
	v->normalized = normalized;
	v->stride = (stride) ? stride : size*type_sz;

	// offset can still really be a pointer if using the 0 VAO and no bound ARRAY_BUFFER.
	v->offset = (GLsizeiptr)pointer;
	v->relativeoffset = 0;
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

	c->vertex_arrays.a[vaobj].vertex_attribs[index].enabled = GL_TRUE;
}

PGLDEF void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);
	c->vertex_arrays.a[vaobj].vertex_attribs[index].enabled = GL_FALSE;
}

PGLDEF void glCreateVertexArrays(GLsizei n, GLuint* arrays)
{
	glGenVertexArrays(n, arrays);
}

PGLDEF void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
{
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);
	PGL_ERR(bindingindex >= (GLuint)GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	PGL_ERR(offset < 0 || stride < 0, GL_INVALID_VALUE);
	PGL_ERR(buffer && (buffer >= c->buffers.size || c->buffers.a[buffer].deleted), GL_INVALID_OPERATION);

	glVertex_Attrib* v = &c->vertex_arrays.a[vaobj].vertex_attribs[bindingindex];
	v->buf = buffer;
	v->offset = offset;
	if (stride)
		v->stride = stride;
	else if (v->size) {
		int ts = pgl_vertex_type_size(v->type);
		v->stride = ts ? v->size * ts : 0;
	} else {
		v->stride = 0;
	}
}

PGLDEF void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset)
{
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);
	PGL_ERR(attribindex >= (GLuint)GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	PGL_ERR((size < 1 || size > 4), GL_INVALID_VALUE);

	int type_sz = pgl_vertex_type_size(type);
	PGL_ERR(!type_sz, GL_INVALID_ENUM);

	glVertex_Attrib* v = &c->vertex_arrays.a[vaobj].vertex_attribs[attribindex];
	v->size = size;
	v->type = type;
	v->normalized = normalized;
	v->relativeoffset = relativeoffset;
	if (!v->stride)
		v->stride = size * type_sz;
}

PGLDEF void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex)
{
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);
	PGL_ERR(attribindex >= (GLuint)GL_MAX_VERTEX_ATTRIBS || bindingindex >= (GLuint)GL_MAX_VERTEX_ATTRIBS,
	        GL_INVALID_VALUE);
	PGL_ERR(attribindex != bindingindex, GL_INVALID_OPERATION);
}

PGLDEF void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer)
{
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);
	PGL_ERR(buffer && (buffer >= c->buffers.size || c->buffers.a[buffer].deleted), GL_INVALID_OPERATION);

	c->vertex_arrays.a[vaobj].element_buffer = buffer;
	if (vaobj == c->cur_vertex_array)
		c->bound_buffers[GL_ELEMENT_ARRAY_BUFFER - GL_ARRAY_BUFFER] = buffer;
}

PGLDEF void glVertexArrayAttribDivisor(GLuint vaobj, GLuint index, GLuint divisor)
{
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);
	PGL_ERR(index >= (GLuint)GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	c->vertex_arrays.a[vaobj].vertex_attribs[index].divisor = divisor;
}

PGLDEF void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags)
{
	PGL_UNUSED(flags);
	glNamedBufferData(buffer, size, data, GL_STATIC_DRAW);
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
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);

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
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);

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

PGLDEF void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled)
{
	PGL_UNUSED(source);
	PGL_UNUSED(type);
	PGL_UNUSED(severity);
	PGL_UNUSED(count);
	PGL_UNUSED(ids);
	PGL_UNUSED(enabled);

	PGL_ERR(count < 0, GL_INVALID_VALUE);
	PGL_ERR((source != GL_DONT_CARE &&
	         (source < GL_DEBUG_SOURCE_API || source > GL_DEBUG_SOURCE_OTHER)),
	        GL_INVALID_ENUM);
	PGL_ERR((type != GL_DONT_CARE &&
	         (type < GL_DEBUG_TYPE_ERROR || type > GL_DEBUG_TYPE_OTHER)),
	        GL_INVALID_ENUM);
	PGL_ERR((severity != GL_DONT_CARE &&
	         (severity < GL_DEBUG_SEVERITY_HIGH || severity > GL_DEBUG_SEVERITY_NOTIFICATION)),
	        GL_INVALID_ENUM);
	PGL_ERR((count > 0 && (source == GL_DONT_CARE || type == GL_DONT_CARE ||
	                       severity != GL_DONT_CARE)),
	        GL_INVALID_OPERATION);
	// no-op: PGL only emits API / TYPE_ERROR / SEVERITY_HIGH and does not filter
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

#ifndef PGL_DISABLE_COLOR_MASK
static void pgl_set_color_mask(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	red = !!red;
	green = !!green;
	blue = !!blue;
	alpha = !!alpha;
	c->color_writemask[buf][0] = red;
	c->color_writemask[buf][1] = green;
	c->color_writemask[buf][2] = blue;
	c->color_writemask[buf][3] = alpha;
	c->color_mask_pix[buf] = red * PGL_RMASK | green * PGL_GMASK | blue * PGL_BMASK | alpha * PGL_AMASK;
	c->color_mask_u8[buf] = red * PGL_COLOR_U8_R | green * PGL_COLOR_U8_G | blue * PGL_COLOR_U8_B | alpha * PGL_COLOR_U8_A;
}
#endif

PGLDEF void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
#ifndef PGL_DISABLE_COLOR_MASK
	for (int i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
		pgl_set_color_mask((GLuint)i, red, green, blue, alpha);
#else
	PGL_UNUSED(red);
	PGL_UNUSED(green);
	PGL_UNUSED(blue);
	PGL_UNUSED(alpha);
#endif
}

PGLDEF void glColorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	PGL_ERR(buf >= (GLuint)GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE);
#ifndef PGL_DISABLE_COLOR_MASK
	pgl_set_color_mask(buf, red, green, blue, alpha);
#else
	PGL_UNUSED(red);
	PGL_UNUSED(green);
	PGL_UNUSED(blue);
	PGL_UNUSED(alpha);
#endif
}

PGLDEF void glClear(GLbitfield mask)
{
	PGL_ERR((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)), GL_INVALID_VALUE);
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);

	if (mask & GL_COLOR_BUFFER_BIT) {
		if (c->fbo_color_is_rt) {
			Color cc = PIXEL_TO_COLOR(c->clear_color);
			float fr = cc.r / (float)PGL_RMAX, fg = cc.g / (float)PGL_GMAX;
			float fb = cc.b / (float)PGL_BMAX, fa = cc.a / (float)PGL_AMAX;
			for (GLsizei di = 0; di < c->num_draw_buffers; ++di) {
				if (c->draw_buffers[di] == GL_NONE)
					continue;
				int att = (int)(c->draw_buffers[di] - GL_COLOR_ATTACHMENT0);
				PGL_ASSERT(att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS);
				PGL_ASSERT(c->mrt_color[att].buf);
				pgl_fill_color_rt(&c->mrt_color[att], fr, fg, fb, fa, di);
			}
		} else {
			pgl_fill_window_color(c->clear_color);
		}
	}
#ifndef PGL_NO_DEPTH_NO_STENCIL
	if (mask & GL_DEPTH_BUFFER_BIT)
		pgl_clear_draw_depth(c->clear_depth);
#  ifndef PGL_NO_STENCIL
	if (mask & GL_STENCIL_BUFFER_BIT)
		pgl_clear_draw_stencil(c->clear_stencil);
#  endif
#endif
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
		for (int i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
			c->blend[i] = GL_TRUE;
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
	case GL_SCISSOR_TEST:
		c->scissor_test = GL_TRUE;
		pgl_update_clip_rect();
		break;
	case GL_STENCIL_TEST:
#ifndef PGL_NO_STENCIL
		c->stencil_test = GL_TRUE;
#endif
		break;
	case GL_DEBUG_OUTPUT:
		c->dbg_output = GL_TRUE;
		break;
	case GL_DEBUG_OUTPUT_SYNCHRONOUS:
		c->dbg_output_sync = GL_TRUE;
		break;
	case GL_TEXTURE_CUBE_MAP_SEAMLESS:
		c->cube_map_seamless = GL_TRUE;
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
		for (int i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
			c->blend[i] = GL_FALSE;
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
		pgl_update_clip_rect();
		break;
	case GL_STENCIL_TEST:
#ifndef PGL_NO_STENCIL
		c->stencil_test = GL_FALSE;
#endif
		break;
	case GL_DEBUG_OUTPUT:
		c->dbg_output = GL_FALSE;
		break;
	case GL_DEBUG_OUTPUT_SYNCHRONOUS:
		c->dbg_output_sync = GL_FALSE;
		break;
	case GL_TEXTURE_CUBE_MAP_SEAMLESS:
		c->cube_map_seamless = GL_FALSE;
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
	case GL_BLEND: return c->blend[0];
	case GL_COLOR_LOGIC_OP: return c->logic_ops;
	case GL_POLYGON_OFFSET_POINT: return c->poly_offset_pt;
	case GL_POLYGON_OFFSET_LINE: return c->poly_offset_line;
	case GL_POLYGON_OFFSET_FILL: return c->poly_offset_fill;
	case GL_SCISSOR_TEST: return c->scissor_test;
	case GL_TEXTURE_CUBE_MAP_SEAMLESS: return c->cube_map_seamless;
	case GL_DEBUG_OUTPUT: return c->dbg_output;
	case GL_DEBUG_OUTPUT_SYNCHRONOUS: return c->dbg_output_sync;
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
	case GL_BLEND:                *data = c->blend[0];            break;
	case GL_COLOR_WRITEMASK:
#ifndef PGL_DISABLE_COLOR_MASK
		data[0] = c->color_writemask[0][0];
		data[1] = c->color_writemask[0][1];
		data[2] = c->color_writemask[0][2];
		data[3] = c->color_writemask[0][3];
#else
		data[0] = data[1] = data[2] = data[3] = GL_TRUE;
#endif
		break;
	case GL_COLOR_LOGIC_OP:       *data = c->logic_ops;        break;
	case GL_POLYGON_OFFSET_POINT: *data = c->poly_offset_pt;  break;
	case GL_POLYGON_OFFSET_LINE:  *data = c->poly_offset_line; break;
	case GL_POLYGON_OFFSET_FILL:  *data = c->poly_offset_fill; break;
	case GL_SCISSOR_TEST:         *data = c->scissor_test;     break;
	case GL_TEXTURE_CUBE_MAP_SEAMLESS: *data = c->cube_map_seamless; break;
	case GL_DEBUG_OUTPUT:             *data = c->dbg_output; break;
	case GL_DEBUG_OUTPUT_SYNCHRONOUS: *data = c->dbg_output_sync; break;
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
	case GL_BLEND_SRC_RGB:             data[0] = c->blend_sRGB[0]; break;
	case GL_BLEND_SRC_ALPHA:           data[0] = c->blend_sA[0]; break;
	case GL_BLEND_DST_RGB:             data[0] = c->blend_dRGB[0]; break;
	case GL_BLEND_DST_ALPHA:           data[0] = c->blend_dA[0]; break;

	case GL_BLEND_EQUATION_RGB:        data[0] = c->blend_eqRGB[0]; break;
	case GL_BLEND_EQUATION_ALPHA:      data[0] = c->blend_eqA[0]; break;

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
#ifndef PGL_UNSAFE
	case GL_CONTEXT_FLAGS:             data[0] = (GLint)GL_CONTEXT_FLAG_DEBUG_BIT; break;
#else
	case GL_CONTEXT_FLAGS:             data[0] = 0; break;
#endif

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

	case GL_FRAMEBUFFER_BINDING:
	case GL_DRAW_FRAMEBUFFER_BINDING:
		data[0] = (GLint)c->bound_draw_framebuffer;
		break;
	case GL_READ_FRAMEBUFFER_BINDING:
		data[0] = (GLint)c->bound_read_framebuffer;
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


static void pgl_blend_func_buf(GLuint buf, GLenum sRGB, GLenum dRGB, GLenum sA, GLenum dA)
{
	c->blend_sRGB[buf] = sRGB;
	c->blend_sA[buf] = sA;
	c->blend_dRGB[buf] = dRGB;
	c->blend_dA[buf] = dA;
}

static void pgl_blend_eq_buf(GLuint buf, GLenum eqRGB, GLenum eqA)
{
	c->blend_eqRGB[buf] = eqRGB;
	c->blend_eqA[buf] = eqA;
}

PGLDEF void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	PGL_ERR((sfactor < GL_ZERO || sfactor >= NUM_BLEND_FUNCS || dfactor < GL_ZERO || dfactor >= NUM_BLEND_FUNCS), GL_INVALID_ENUM);

	for (int i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
		pgl_blend_func_buf((GLuint)i, sfactor, dfactor, sfactor, dfactor);
}

PGLDEF void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	PGL_ERR((srcRGB < GL_ZERO || srcRGB >= NUM_BLEND_FUNCS ||
	         dstRGB < GL_ZERO || dstRGB >= NUM_BLEND_FUNCS ||
	         srcAlpha < GL_ZERO || srcAlpha >= NUM_BLEND_FUNCS ||
	         dstAlpha < GL_ZERO || dstAlpha >= NUM_BLEND_FUNCS), GL_INVALID_ENUM);

	for (int i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
		pgl_blend_func_buf((GLuint)i, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

PGLDEF void glBlendEquation(GLenum mode)
{
	PGL_ERR((mode < GL_FUNC_ADD || mode >= NUM_BLEND_EQUATIONS), GL_INVALID_ENUM);

	for (int i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
		pgl_blend_eq_buf((GLuint)i, mode, mode);
}

PGLDEF void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	PGL_ERR((modeRGB < GL_FUNC_ADD || modeRGB >= NUM_BLEND_EQUATIONS ||
	    modeAlpha < GL_FUNC_ADD || modeAlpha >= NUM_BLEND_EQUATIONS), GL_INVALID_ENUM);

	for (int i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
		pgl_blend_eq_buf((GLuint)i, modeRGB, modeAlpha);
}

PGLDEF void glBlendFunci(GLuint buf, GLenum sfactor, GLenum dfactor)
{
	PGL_ERR(buf >= (GLuint)GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE);
	PGL_ERR((sfactor < GL_ZERO || sfactor >= NUM_BLEND_FUNCS || dfactor < GL_ZERO || dfactor >= NUM_BLEND_FUNCS), GL_INVALID_ENUM);
	pgl_blend_func_buf(buf, sfactor, dfactor, sfactor, dfactor);
}

PGLDEF void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	PGL_ERR(buf >= (GLuint)GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE);
	PGL_ERR((srcRGB < GL_ZERO || srcRGB >= NUM_BLEND_FUNCS ||
	         dstRGB < GL_ZERO || dstRGB >= NUM_BLEND_FUNCS ||
	         srcAlpha < GL_ZERO || srcAlpha >= NUM_BLEND_FUNCS ||
	         dstAlpha < GL_ZERO || dstAlpha >= NUM_BLEND_FUNCS), GL_INVALID_ENUM);
	pgl_blend_func_buf(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

PGLDEF void glBlendEquationi(GLuint buf, GLenum mode)
{
	PGL_ERR(buf >= (GLuint)GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE);
	PGL_ERR((mode < GL_FUNC_ADD || mode >= NUM_BLEND_EQUATIONS), GL_INVALID_ENUM);
	pgl_blend_eq_buf(buf, mode, mode);
}

PGLDEF void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha)
{
	PGL_ERR(buf >= (GLuint)GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE);
	PGL_ERR((modeRGB < GL_FUNC_ADD || modeRGB >= NUM_BLEND_EQUATIONS ||
	    modeAlpha < GL_FUNC_ADD || modeAlpha >= NUM_BLEND_EQUATIONS), GL_INVALID_ENUM);
	pgl_blend_eq_buf(buf, modeRGB, modeAlpha);
}

PGLDEF void glEnablei(GLenum cap, GLuint index)
{
	PGL_ERR(cap != GL_BLEND, GL_INVALID_ENUM);
	PGL_ERR(index >= (GLuint)GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE);
	c->blend[index] = GL_TRUE;
}

PGLDEF void glDisablei(GLenum cap, GLuint index)
{
	PGL_ERR(cap != GL_BLEND, GL_INVALID_ENUM);
	PGL_ERR(index >= (GLuint)GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE);
	c->blend[index] = GL_FALSE;
}

PGLDEF GLboolean glIsEnabledi(GLenum cap, GLuint index)
{
	PGL_ERR_RET_VAL(cap != GL_BLEND, GL_INVALID_ENUM, GL_FALSE);
	PGL_ERR_RET_VAL(index >= (GLuint)GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE, GL_FALSE);
	return c->blend[index];
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
	pgl_update_clip_rect();
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

// Framebuffer objects (public gl* APIs). Static helpers are in gl_fbo.c
// (amalgamated before this file).

PGLDEF void glGenFramebuffers(GLsizei n, GLuint* ids)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	if (!n)
		return;

	// Ensure index 0 exists as a deleted placeholder so names start at 1
	if (c->framebuffers.size == 0) {
		cvec_extend_glFBO(&c->framebuffers, 1);
		pgl_init_fbo(&c->framebuffers.a[0]);
		c->framebuffers.a[0].deleted = GL_TRUE; // never used
	}

	int j = 0;
	for (int i = 1; i < c->framebuffers.size && j < n; ++i) {
		if (c->framebuffers.a[i].deleted) {
			pgl_init_fbo(&c->framebuffers.a[i]);
			ids[j++] = (GLuint)i;
		}
	}
	if (j != n) {
		int s = (int)c->framebuffers.size;
		cvec_extend_glFBO(&c->framebuffers, n - j);
		for (int i = s; j < n; ++i) {
			pgl_init_fbo(&c->framebuffers.a[i]);
			ids[j++] = (GLuint)i;
		}
	}
}

PGLDEF void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	for (int i = 0; i < n; ++i) {
		GLuint id = framebuffers[i];
		if (!id || id >= c->framebuffers.size)
			continue;
		if (c->framebuffers.a[id].deleted)
			continue;
		if (c->bound_draw_framebuffer == id) {
			c->bound_draw_framebuffer = 0;
			pgl_apply_draw_framebuffer();
		}
		if (c->bound_read_framebuffer == id) {
			c->bound_read_framebuffer = 0;
			pgl_sync_read_state();
		}
		c->framebuffers.a[id].deleted = GL_TRUE;
	}
}

PGLDEF GLboolean glIsFramebuffer(GLuint framebuffer)
{
	if (!framebuffer || framebuffer >= c->framebuffers.size)
		return GL_FALSE;
	return !c->framebuffers.a[framebuffer].deleted;
}

PGLDEF void glBindFramebuffer(GLenum target, GLuint framebuffer)
{
	PGL_ERR(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER &&
	        target != GL_READ_FRAMEBUFFER, GL_INVALID_ENUM);

	if (framebuffer != 0) {
		PGL_ERR(framebuffer >= c->framebuffers.size ||
		        c->framebuffers.a[framebuffer].deleted, GL_INVALID_OPERATION);
	}

	GLboolean bind_draw = (target != GL_READ_FRAMEBUFFER);
	GLboolean bind_read = (target != GL_DRAW_FRAMEBUFFER);
	if (bind_draw && c->bound_draw_framebuffer != framebuffer) {
		c->bound_draw_framebuffer = framebuffer;
		pgl_apply_draw_framebuffer();
	}
	if (bind_read && c->bound_read_framebuffer != framebuffer) {
		c->bound_read_framebuffer = framebuffer;
		pgl_sync_read_state();
	}
}

PGLDEF void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                                   GLuint texture, GLint level)
{
	PGL_ERR(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER &&
	        target != GL_READ_FRAMEBUFFER, GL_INVALID_ENUM);
	GLuint fbo_id = pgl_fbo_id_for_target(target);
	PGL_ERR(!fbo_id, GL_INVALID_OPERATION); // cannot attach to default FB

	glFBO* f = pgl_user_fbo(fbo_id);

	if (texture != 0) {
		PGL_ERR(level < 0 || level >= PGL_MAX_MIPMAP_LEVELS, GL_INVALID_VALUE);
		PGL_ERR(texture >= c->textures.size || c->textures.a[texture].deleted, GL_INVALID_VALUE);
		if (pgl_is_cube_face_target(textarget)) {
			PGL_ERR(c->textures.a[texture].type + GL_TEXTURE_UNBOUND + 1 != GL_TEXTURE_CUBE_MAP,
			        GL_INVALID_OPERATION);
		} else {
			PGL_ERR(textarget != GL_TEXTURE_2D && textarget != GL_TEXTURE_RECTANGLE,
			        GL_INVALID_OPERATION);
		}
	}

	if (attachment == GL_COLOR_ATTACHMENT0 ||
	    (attachment >= GL_COLOR_ATTACHMENT1 &&
	     attachment < GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS)) {
		int idx = (int)(attachment - GL_COLOR_ATTACHMENT0);
		f->color[idx].tex = texture;
		f->color[idx].rb = 0;
		f->color[idx].level = level;
		f->color[idx].textarget = texture ? textarget : (GLenum)0;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
	} else if (attachment == GL_DEPTH_ATTACHMENT) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.tex = texture;
		f->depth.rb = 0;
		f->depth.level = level;
		f->depth.textarget = texture ? textarget : (GLenum)0;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
#endif
	} else if (attachment == GL_STENCIL_ATTACHMENT) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		// Packed path: same texture as depth (D24S8). Separate stencil textures unsupported.
		f->stencil.tex = texture;
		f->stencil.rb = 0;
		f->stencil.level = level;
		f->stencil.textarget = texture ? textarget : (GLenum)0;
#endif
	} else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.tex = texture;
		f->depth.rb = 0;
		f->depth.level = level;
		f->depth.textarget = texture ? textarget : (GLenum)0;
		f->stencil.tex = texture;
		f->stencil.rb = 0;
		f->stencil.level = level;
		f->stencil.textarget = texture ? textarget : (GLenum)0;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
#endif
	} else {
		PGL_ERR(1, GL_INVALID_ENUM);
	}

	pgl_fbo_mark_dirty(f);
	if (fbo_id == c->bound_draw_framebuffer)
		pgl_apply_draw_framebuffer();
}

// Convenience: DSA-ish path used by some ports; maps to bound-FBO style after bind.
PGLDEF void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level)
{
	// Assume 2D; validate on attach
	glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, texture, level);
}

PGLDEF GLenum glCheckFramebufferStatus(GLenum target)
{
	PGL_ERR_RET_VAL(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER &&
	                target != GL_READ_FRAMEBUFFER, GL_INVALID_ENUM, 0);

	GLuint id = pgl_fbo_id_for_target(target);
	if (!id)
		return GL_FRAMEBUFFER_COMPLETE; // default FB always complete when context exists

	glFBO* f = pgl_user_fbo(id);
	pgl_fbo_update_status(f);
	return f->status;
}
// Select which color attachments receive FS outputs (gl_FragData[i] → bufs[i]).
// State is per-framebuffer (default FB uses context default_draw_buffers).
PGLDEF void glDrawBuffers(GLsizei n, const GLenum* bufs)
{
	PGL_ERR(n < 0 || n > GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE);
	PGL_ERR(!bufs && n > 0, GL_INVALID_VALUE);

	// Validate entries; reject duplicates (except GL_NONE)
	GLboolean seen[GL_MAX_COLOR_ATTACHMENTS] = { 0 };

	for (GLsizei i = 0; i < n; ++i) {
		GLenum b = bufs[i];
		if (b == GL_NONE)
			continue;

		if (!c->bound_draw_framebuffer) {
			// Mono window: FRONT/BACK/LEFT/*_LEFT/FRONT_AND_BACK all write
			// the same pix_t buffer. RIGHT/*_RIGHT do not exist.
			PGL_ERR(pgl_default_fb_missing_stereo(b), GL_INVALID_OPERATION);
			PGL_ERR(!pgl_default_fb_is_window_color(b) && b != GL_FRONT_AND_BACK,
			        GL_INVALID_ENUM);
			PGL_ERR(n != 1, GL_INVALID_OPERATION); // single buffer only
		} else {
			PGL_ERR(b < GL_COLOR_ATTACHMENT0 ||
			        b >= GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS, GL_INVALID_ENUM);
			int att = (int)(b - GL_COLOR_ATTACHMENT0);
			PGL_ERR(seen[att], GL_INVALID_OPERATION); // duplicate
			seen[att] = GL_TRUE;
		}
	}

	if (!c->bound_draw_framebuffer) {
		c->default_num_draw_buffers = n > 0 ? n : 1;
		if (n <= 0) {
			c->default_draw_buffers[0] = GL_BACK;
		} else {
			for (GLsizei i = 0; i < n; ++i)
				c->default_draw_buffers[i] = bufs[i];
			for (GLsizei i = n; i < GL_MAX_DRAW_BUFFERS; ++i)
				c->default_draw_buffers[i] = GL_NONE;
		}
		c->num_draw_buffers = c->default_num_draw_buffers;
		for (GLsizei i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
			c->draw_buffers[i] = c->default_draw_buffers[i];
		c->mrt_active = GL_FALSE;
		return;
	}

	glFBO* f = pgl_draw_user_fbo();

	f->num_draw_buffers = n > 0 ? n : 1;
	if (n <= 0) {
		f->draw_buffers[0] = GL_COLOR_ATTACHMENT0;
		for (int i = 1; i < GL_MAX_DRAW_BUFFERS; ++i)
			f->draw_buffers[i] = GL_NONE;
	} else {
		for (GLsizei i = 0; i < n; ++i)
			f->draw_buffers[i] = bufs[i];
		for (GLsizei i = n; i < GL_MAX_DRAW_BUFFERS; ++i)
			f->draw_buffers[i] = GL_NONE;
	}

	// Draw buffers affect completeness (INCOMPLETE_DRAW_BUFFER)
	pgl_fbo_mark_dirty(f);
	// Refresh back_buffer / mrt_active if this FBO is complete and bound
	pgl_apply_draw_framebuffer();
}

PGLDEF void glDrawBuffer(GLenum buf)
{
	glDrawBuffers(1, &buf);
}

PGLDEF void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	if (!n) return;

	if (c->renderbuffers.size == 0) {
		cvec_extend_glRenderbuffer(&c->renderbuffers, 1);
		pgl_init_rb(&c->renderbuffers.a[0]);
		c->renderbuffers.a[0].deleted = GL_TRUE;
	}

	int j = 0;
	for (int i = 1; i < c->renderbuffers.size && j < n; ++i) {
		if (c->renderbuffers.a[i].deleted) {
			pgl_init_rb(&c->renderbuffers.a[i]);
			renderbuffers[j++] = (GLuint)i;
		}
	}
	if (j != n) {
		int s = (int)c->renderbuffers.size;
		cvec_extend_glRenderbuffer(&c->renderbuffers, n - j);
		for (int i = s; j < n; ++i) {
			pgl_init_rb(&c->renderbuffers.a[i]);
			renderbuffers[j++] = (GLuint)i;
		}
	}
}

PGLDEF void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	for (int i = 0; i < n; ++i) {
		GLuint id = renderbuffers[i];
		if (!id || id >= c->renderbuffers.size) continue;
		glRenderbuffer* rb = &c->renderbuffers.a[id];
		if (rb->deleted) continue;
		if (!rb->user_owned)
			PGL_FREE(rb->data);
		if (c->bound_renderbuffer == id)
			c->bound_renderbuffer = 0;
		pgl_init_rb(rb);
		rb->deleted = GL_TRUE;
	}
}

PGLDEF GLboolean glIsRenderbuffer(GLuint renderbuffer)
{
	if (!renderbuffer || renderbuffer >= c->renderbuffers.size)
		return GL_FALSE;
	return !c->renderbuffers.a[renderbuffer].deleted;
}

PGLDEF void glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
	PGL_ERR(target != GL_RENDERBUFFER, GL_INVALID_ENUM);
	if (renderbuffer != 0) {
		PGL_ERR(renderbuffer >= c->renderbuffers.size ||
		        c->renderbuffers.a[renderbuffer].deleted, GL_INVALID_OPERATION);
	}
	c->bound_renderbuffer = renderbuffer;
}

PGLDEF void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	PGL_ERR(target != GL_RENDERBUFFER, GL_INVALID_ENUM);
	PGL_ERR(!c->bound_renderbuffer, GL_INVALID_OPERATION);
	PGL_ERR(width < 0 || height < 0, GL_INVALID_VALUE);
	PGL_ERR(internalformat != GL_DEPTH_COMPONENT &&
	        internalformat != GL_DEPTH_COMPONENT16 &&
	        internalformat != GL_DEPTH_COMPONENT24 &&
	        internalformat != GL_DEPTH_COMPONENT32 &&
	        internalformat != GL_DEPTH_COMPONENT32F &&
	        internalformat != GL_DEPTH24_STENCIL8 &&
	        internalformat != GL_STENCIL_INDEX8, GL_INVALID_ENUM);
	// Packed DS is the D24S8 integer layout; D16 has no room in the depth word.
#if !defined(PGL_D24S8)
	PGL_ERR(internalformat == GL_DEPTH24_STENCIL8, GL_INVALID_ENUM);
#endif

	glRenderbuffer* rb = &c->renderbuffers.a[c->bound_renderbuffer];
	size_t bpp = pgl_z_bytes_per_pixel();
	if (internalformat == GL_DEPTH_COMPONENT32F)
		bpp = sizeof(float);
	else if (internalformat == GL_STENCIL_INDEX8)
		bpp = 1;
	else if (internalformat == GL_DEPTH24_STENCIL8)
		bpp = sizeof(u32);
	else if (!bpp)
		bpp = sizeof(u32);

	size_t need = (size_t)width * (size_t)height * bpp;
	if (!rb->user_owned)
		PGL_FREE(rb->data);
	rb->data = (u8*)PGL_MALLOC(need ? need : 1);
	PGL_ERR(!rb->data, GL_OUT_OF_MEMORY);
	memset(rb->data, 0, need ? need : 1);
	rb->data_alloc = need;
	rb->user_owned = GL_FALSE;
	rb->w = width;
	rb->h = height;
	rb->internalformat = internalformat;
	rb->lastrow = rb->data + (size_t)(height > 0 ? height - 1 : 0) * (size_t)width * bpp;
}

PGLDEF void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	PGL_ERR(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER &&
	        target != GL_READ_FRAMEBUFFER, GL_INVALID_ENUM);
	PGL_ERR(renderbuffertarget != GL_RENDERBUFFER, GL_INVALID_ENUM);
	GLuint fbo_id = pgl_fbo_id_for_target(target);
	PGL_ERR(!fbo_id, GL_INVALID_OPERATION);

	glFBO* f = pgl_user_fbo(fbo_id);

	if (renderbuffer != 0) {
		PGL_ERR(renderbuffer >= c->renderbuffers.size ||
		        c->renderbuffers.a[renderbuffer].deleted, GL_INVALID_OPERATION);
	}

	if (attachment == GL_DEPTH_ATTACHMENT) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.rb = renderbuffer;
		f->depth.tex = 0;
		f->depth.level = 0;
#endif
	} else if (attachment == GL_STENCIL_ATTACHMENT) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->stencil.rb = renderbuffer;
		f->stencil.tex = 0;
		f->stencil.level = 0;
#endif
	} else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.rb = renderbuffer;
		f->depth.tex = 0;
		f->stencil.rb = renderbuffer;
		f->stencil.tex = 0;
#endif
	} else {
		// Color renderbuffers not implemented (use textures)
		PGL_ERR(1, GL_INVALID_ENUM);
	}

	pgl_fbo_mark_dirty(f);
	if (fbo_id == c->bound_draw_framebuffer)
		pgl_apply_draw_framebuffer();
}

PGLDEF void glReadBuffer(GLenum mode)
{
	if (!c->bound_read_framebuffer) {
		PGL_ERR(pgl_default_fb_missing_stereo(mode), GL_INVALID_OPERATION);
		PGL_ERR(!pgl_default_fb_is_window_color(mode), GL_INVALID_ENUM);
		c->default_read_buffer = GL_BACK;
		c->read_buffer = GL_BACK;
		return;
	}
	PGL_ERR(mode != GL_NONE &&
	        (mode < GL_COLOR_ATTACHMENT0 ||
	         mode >= GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS), GL_INVALID_ENUM);
	glFBO* f = pgl_user_fbo(c->bound_read_framebuffer);
	f->read_buffer = mode;
	c->read_buffer = mode;
	pgl_fbo_mark_dirty(f);
	if (c->bound_read_framebuffer == c->bound_draw_framebuffer)
		pgl_apply_draw_framebuffer();
}

// Thin glReadPixels: RGBA U8 or float RGBA/R from the *read* color buffer.
PGLDEF void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, GLvoid* data)
{
	PGL_ERR(width < 0 || height < 0, GL_INVALID_VALUE);
	PGL_ERR(!data, GL_INVALID_VALUE);
	PGL_ERR(format != GL_RGBA && format != GL_RED, GL_INVALID_ENUM);
	PGL_ERR(type != GL_UNSIGNED_BYTE && type != GL_FLOAT, GL_INVALID_ENUM);
	PGL_ERR(!pgl_fbo_id_complete(c->bound_read_framebuffer),
	        GL_INVALID_FRAMEBUFFER_OPERATION);

	if (!width || !height)
		return;

	pglBlitColor src;
	pgl_resolve_read_color(&src);
	if (!src.valid)
		return;

	for (GLsizei row = 0; row < height; ++row) {
		for (GLsizei col = 0; col < width; ++col) {
			float fr, fg, fb, fa;
			pgl_blit_get_rgba(&src, x + col, y + row, &fr, &fg, &fb, &fa);
			size_t out_i = (size_t)row * (size_t)width + (size_t)col;
			if (type == GL_UNSIGNED_BYTE) {
				u8* o = (u8*)data;
				if (format == GL_RED) {
					o[out_i] = (u8)(fr * 255.f);
				} else {
					o[out_i * 4 + 0] = (u8)(fr * 255.f);
					o[out_i * 4 + 1] = (u8)(fg * 255.f);
					o[out_i * 4 + 2] = (u8)(fb * 255.f);
					o[out_i * 4 + 3] = (u8)(fa * 255.f);
				}
			} else {
				float* o = (float*)data;
				if (format == GL_RED) {
					o[out_i] = fr;
				} else {
					o[out_i * 4 + 0] = fr;
					o[out_i * 4 + 1] = fg;
					o[out_i * 4 + 2] = fb;
					o[out_i * 4 + 3] = fa;
				}
			}
		}
	}
}
PGLDEF void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                              GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                              GLbitfield mask, GLenum filter)
{
	PGL_ERR(filter != GL_NEAREST && filter != GL_LINEAR, GL_INVALID_ENUM);
	PGL_ERR(mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT),
	        GL_INVALID_VALUE);
	if ((mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) && filter == GL_LINEAR)
		PGL_ERR(1, GL_INVALID_OPERATION);
	PGL_ERR(!pgl_fbo_id_complete(c->bound_read_framebuffer) ||
	        !pgl_fbo_id_complete(c->bound_draw_framebuffer),
	        GL_INVALID_FRAMEBUFFER_OPERATION);

	GLint src_w = srcX1 - srcX0;
	GLint src_h = srcY1 - srcY0;
	GLint dst_w = dstX1 - dstX0;
	GLint dst_h = dstY1 - dstY0;
	if (!src_w || !src_h || !dst_w || !dst_h)
		return;

	int dst_x0 = dstX0 < dstX1 ? dstX0 : dstX1;
	int dst_x1 = dstX0 < dstX1 ? dstX1 : dstX0;
	int dst_y0 = dstY0 < dstY1 ? dstY0 : dstY1;
	int dst_y1 = dstY0 < dstY1 ? dstY1 : dstY0;

	if (mask & GL_COLOR_BUFFER_BIT) {
		pglBlitColor src;
		pgl_resolve_read_color(&src);
		if (src.valid) {
			pglBlitColor dsts[GL_MAX_COLOR_ATTACHMENTS];
			int n_dst = 0;
			if (!c->bound_draw_framebuffer) {
				pgl_resolve_window_color(&dsts[0]);
				n_dst = 1;
			} else {
				glFBO* df = pgl_draw_user_fbo();
				for (GLsizei i = 0; i < df->num_draw_buffers; ++i) {
					if (df->draw_buffers[i] == GL_NONE)
						continue;
					pgl_resolve_fbo_color(df, df->draw_buffers[i], &dsts[n_dst]);
					n_dst++;
				}
			}
			if (n_dst) {
				int x0 = dst_x0, y0 = dst_y0, x1 = dst_x1, y1 = dst_y1;
				if (x0 < c->lx) x0 = c->lx;
				if (y0 < c->ly) y0 = c->ly;
				if (x1 > c->ux) x1 = c->ux;
				if (y1 > c->uy) y1 = c->uy;
				if (x1 > dsts[0].w) x1 = dsts[0].w;
				if (y1 > dsts[0].h) y1 = dsts[0].h;
				if (x0 < 0) x0 = 0;
				if (y0 < 0) y0 = 0;
				for (int y = y0; y < y1; ++y) {
					for (int x = x0; x < x1; ++x) {
						float tx = ((x - dstX0) + 0.5f) / (float)dst_w;
						float ty = ((y - dstY0) + 0.5f) / (float)dst_h;
						float sx = srcX0 + tx * (float)src_w - (filter == GL_LINEAR ? 0.5f : 0.f);
						float sy = srcY0 + ty * (float)src_h - (filter == GL_LINEAR ? 0.5f : 0.f);
						float r, g, b, a;
						pgl_blit_sample_color(&src, sx, sy, filter, &r, &g, &b, &a);
						for (int i = 0; i < n_dst; ++i)
							pgl_blit_put_rgba(&dsts[i], x, y, r, g, b, a);
					}
				}
			}
		}
	}

#ifndef PGL_NO_DEPTH_NO_STENCIL
	if (mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
		pglBlitDepth src, dst;
		pgl_resolve_read_depth(&src);
		pgl_resolve_draw_depth(&dst);
		GLboolean do_z = (mask & GL_DEPTH_BUFFER_BIT) && src.valid && dst.valid;
		GLboolean do_s = GL_FALSE;
#  if !defined(PGL_NO_STENCIL)
#    if defined(PGL_D16)
		do_s = (mask & GL_STENCIL_BUFFER_BIT) && src.stencil && dst.stencil;
#    else
		do_s = (mask & GL_STENCIL_BUFFER_BIT) && src.valid && dst.valid &&
		       !src.is_float && !dst.is_float;
#    endif
#  endif
		if (do_z || do_s) {
			int x0 = dst_x0, y0 = dst_y0, x1 = dst_x1, y1 = dst_y1;
			if (x0 < c->lx) x0 = c->lx;
			if (y0 < c->ly) y0 = c->ly;
			if (x1 > c->ux) x1 = c->ux;
			if (y1 > c->uy) y1 = c->uy;
			if (x1 > dst.w) x1 = dst.w;
			if (y1 > dst.h) y1 = dst.h;
			if (x0 < 0) x0 = 0;
			if (y0 < 0) y0 = 0;
			for (int y = y0; y < y1; ++y) {
				for (int x = x0; x < x1; ++x) {
					float tx = ((x - dstX0) + 0.5f) / (float)dst_w;
					float ty = ((y - dstY0) + 0.5f) / (float)dst_h;
					int sx = (int)floorf(srcX0 + tx * (float)src_w);
					int sy = (int)floorf(srcY0 + ty * (float)src_h);
					if (do_z) {
						float d = pgl_blit_get_depth(&src, sx, sy);
						u8 st = 0;
#  if !defined(PGL_NO_STENCIL)
						if (do_s)
							st = pgl_blit_get_stencil(&src, sx, sy);
#  endif
						pgl_blit_put_depth(&dst, x, y, d, do_s, st);
					}
#  if !defined(PGL_NO_STENCIL)
					else if (do_s) {
						pgl_blit_put_stencil(&dst, x, y, pgl_blit_get_stencil(&src, sx, sy));
					}
#  endif
				}
			}
		}
	}
#endif
}
PGLDEF void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value)
{
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);
	pgl_clear_buffer_fv(buffer, drawbuffer, value, __func__);
}

PGLDEF void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value)
{
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);
	pgl_clear_buffer_iv(buffer, drawbuffer, value, __func__);
}

PGLDEF void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value)
{
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);
	pgl_clear_buffer_uiv(buffer, drawbuffer, value, __func__);
}

PGLDEF void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);
	pgl_clear_buffer_fi(buffer, drawbuffer, depth, stencil, __func__);
}

PGLDEF void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value)
{
	GLuint old;
	if (!pgl_named_clear_setup(framebuffer, &old, __func__))
		return;
	pgl_clear_buffer_fv(buffer, drawbuffer, value, __func__);
	if (c->bound_draw_framebuffer != old) {
		c->bound_draw_framebuffer = old;
		pgl_apply_draw_framebuffer();
	}
}

PGLDEF void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value)
{
	GLuint old;
	if (!pgl_named_clear_setup(framebuffer, &old, __func__))
		return;
	pgl_clear_buffer_iv(buffer, drawbuffer, value, __func__);
	if (c->bound_draw_framebuffer != old) {
		c->bound_draw_framebuffer = old;
		pgl_apply_draw_framebuffer();
	}
}

PGLDEF void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value)
{
	GLuint old;
	if (!pgl_named_clear_setup(framebuffer, &old, __func__))
		return;
	pgl_clear_buffer_uiv(buffer, drawbuffer, value, __func__);
	if (c->bound_draw_framebuffer != old) {
		c->bound_draw_framebuffer = old;
		pgl_apply_draw_framebuffer();
	}
}

PGLDEF void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
	GLuint old;
	if (!pgl_named_clear_setup(framebuffer, &old, __func__))
		return;
	pgl_clear_buffer_fi(buffer, drawbuffer, depth, stencil, __func__);
	if (c->bound_draw_framebuffer != old) {
		c->bound_draw_framebuffer = old;
		pgl_apply_draw_framebuffer();
	}
}
