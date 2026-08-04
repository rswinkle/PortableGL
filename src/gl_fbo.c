// Framebuffer objects (color/depth/stencil attach, MRT, renderbuffers, readback).
// Relies on amalgamation after gl_impl.c for PGL_ERR and texture RT helpers.

static void pgl_init_fbo(glFBO* f)
{
	memset(f, 0, sizeof(*f));
	f->deleted = GL_FALSE;
	f->status_dirty = GL_TRUE;
	f->status = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
	// Default draw buffer state (per-framebuffer)
	f->num_draw_buffers = 1;
	f->draw_buffers[0] = GL_COLOR_ATTACHMENT0;
	for (int i = 1; i < GL_MAX_DRAW_BUFFERS; ++i)
		f->draw_buffers[i] = GL_NONE;
	f->read_buffer = GL_COLOR_ATTACHMENT0;
}

static size_t pgl_z_bytes_per_pixel(void)
{
#ifdef PGL_NO_DEPTH_NO_STENCIL
	return 0;
#elif defined(PGL_D16)
	return sizeof(u16);
#else
	return sizeof(u32); // D24S8
#endif
}

static GLboolean pgl_tex_is_2d_level0(const glTexture* t)
{
	if (!t || t->deleted)
		return GL_FALSE;
	// type is stored as index (see glBindTexture), not raw enum
	GLenum target = t->type + GL_TEXTURE_UNBOUND + 1;
	if (target != GL_TEXTURE_2D && target != GL_TEXTURE_RECTANGLE)
		return GL_FALSE;
	if (!t->data || t->w <= 0 || t->h <= 0 || t->num_levels < 1)
		return GL_FALSE;
	return GL_TRUE;
}

// Depth attachment texture: explicit depth format, or color-sized storage large enough to alias Z.
static GLboolean pgl_tex_ok_depth_attach(const glTexture* t)
{
	if (!pgl_tex_is_2d_level0(t))
		return GL_FALSE;
	if (t->is_depth)
		return GL_TRUE;
	// Legacy: RGBA8/float buffer large enough for window Z packing
	size_t need = (size_t)t->w * (size_t)t->h * pgl_z_bytes_per_pixel();
	if (!need)
		return GL_FALSE;
	size_t have = (size_t)t->w * (size_t)t->h * (size_t)pgl_tex_bytes_per_pixel(t);
	return have >= need;
}

static GLboolean pgl_rb_ok_depth(const glRenderbuffer* rb)
{
	if (!rb || rb->deleted || !rb->data || rb->w <= 0 || rb->h <= 0)
		return GL_FALSE;
	return rb->internalformat == GL_DEPTH_COMPONENT ||
	       rb->internalformat == GL_DEPTH_COMPONENT16 ||
	       rb->internalformat == GL_DEPTH_COMPONENT24 ||
	       rb->internalformat == GL_DEPTH_COMPONENT32 ||
	       rb->internalformat == GL_DEPTH_COMPONENT32F;
}

static GLenum pgl_fbo_compute_status(glFBO* f)
{
	int n_attach = 0;
	GLsizei aw = 0, ah = 0;
	GLboolean have_size = GL_FALSE;

	for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i) {
		if (!f->color[i].tex)
			continue;
		if (f->color[i].level != 0)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		if (f->color[i].tex >= c->textures.size)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		glTexture* t = &c->textures.a[f->color[i].tex];
		if (!pgl_tex_is_2d_level0(t) || t->is_depth)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		// Drawable color: U8 RGBA, or float R/RG/RGBA (no RGB32F)
		if (t->datatype == GL_FLOAT) {
			if (t->components != 1 && t->components != 2 && t->components != 4)
				return GL_FRAMEBUFFER_UNSUPPORTED;
		} else if (t->datatype != GL_UNSIGNED_BYTE || t->components != 4) {
			return GL_FRAMEBUFFER_UNSUPPORTED;
		}
		if (!have_size) {
			aw = t->w;
			ah = t->h;
			have_size = GL_TRUE;
		} else if (t->w != aw || t->h != ah) {
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
		}
		n_attach++;
	}

	if (f->depth.tex || f->depth.rb) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		return GL_FRAMEBUFFER_UNSUPPORTED;
#else
		GLsizei dw = 0, dh = 0;
		if (f->depth.tex) {
			if (f->depth.level != 0)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			if (f->depth.tex >= c->textures.size)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			glTexture* t = &c->textures.a[f->depth.tex];
			if (!pgl_tex_ok_depth_attach(t))
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			dw = t->w;
			dh = t->h;
		} else {
			if (f->depth.rb >= c->renderbuffers.size)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			glRenderbuffer* rb = &c->renderbuffers.a[f->depth.rb];
			if (!pgl_rb_ok_depth(rb))
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			dw = rb->w;
			dh = rb->h;
		}
		if (!have_size) {
			aw = dw;
			ah = dh;
			have_size = GL_TRUE;
		} else if (dw != aw || dh != ah) {
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
		}
		n_attach++;
#endif
	}

	if (f->stencil.tex || f->stencil.rb) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		return GL_FRAMEBUFFER_UNSUPPORTED;
#else
		// Phase D middle ground: stencil RB, or packed with D24S8 depth (same buffer)
		if (f->stencil.tex) {
			// Only allow stencil tex if same as depth tex under D24S8 pack
			if (!f->depth.tex || f->stencil.tex != f->depth.tex)
				return GL_FRAMEBUFFER_UNSUPPORTED;
		} else if (f->stencil.rb) {
			if (f->stencil.rb >= c->renderbuffers.size)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			glRenderbuffer* rb = &c->renderbuffers.a[f->stencil.rb];
			if (rb->deleted || !rb->data)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
#  ifndef PGL_D16
			// D24S8: stencil may share depth RB if same object
			if (f->depth.rb && f->depth.rb == f->stencil.rb) {
				/* ok packed */
			} else if (rb->w != aw || rb->h != ah) {
				return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
			}
#  else
			if (rb->w != aw || rb->h != ah)
				return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
#  endif
		}
		n_attach++;
#endif
	}

	if (!n_attach)
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;

	// Need at least one color attachment (depth-only not useful for drawable FBOs yet)
	if (!n_attach || !have_size)
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;

	// Must have a color attachment if we counted only depth above... re-check colors
	{
		int n_color = 0;
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i)
			if (f->color[i].tex) n_color++;
		if (!n_color)
			return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
	}

	// Desktop completeness: every non-GL_NONE DRAW_BUFFERi must name a color
	// attachment that has an image (already validated above if present).
	for (GLsizei i = 0; i < f->num_draw_buffers; ++i) {
		GLenum db = f->draw_buffers[i];
		if (db == GL_NONE)
			continue;
		int att = (int)(db - GL_COLOR_ATTACHMENT0);
		if (att < 0 || att >= GL_MAX_COLOR_ATTACHMENTS || !f->color[att].tex)
			return GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER;
	}

	// Same for READ_BUFFER (GL_NONE is allowed — reads are a no-op).
	if (f->read_buffer != GL_NONE) {
		int att = (int)(f->read_buffer - GL_COLOR_ATTACHMENT0);
		if (att < 0 || att >= GL_MAX_COLOR_ATTACHMENTS || !f->color[att].tex)
			return GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER;
	}

	return GL_FRAMEBUFFER_COMPLETE;
}

static void pgl_fbo_update_status(glFBO* f)
{
	f->status = pgl_fbo_compute_status(f);
	f->status_dirty = GL_FALSE;
}

static glFBO* pgl_get_bound_user_fbo(void)
{
	if (!c->bound_framebuffer)
		return NULL;
	if (c->bound_framebuffer >= c->framebuffers.size)
		return NULL;
	glFBO* f = &c->framebuffers.a[c->bound_framebuffer];
	if (f->deleted)
		return NULL;
	return f;
}

// True if draws may proceed (default FB or complete user FBO).
static GLboolean pgl_draw_framebuffer_ok(void)
{
	if (!c->bound_framebuffer)
		return GL_TRUE;
	glFBO* f = pgl_get_bound_user_fbo();
	if (!f)
		return GL_FALSE;
	if (f->status_dirty)
		pgl_fbo_update_status(f);
	return f->status == GL_FRAMEBUFFER_COMPLETE;
}

#ifndef PGL_NO_DEPTH_NO_STENCIL
static GLboolean pgl_ensure_fbo_scratch_z(GLsizei w, GLsizei h)
{
	size_t zb = pgl_z_bytes_per_pixel();
	if (!zb || w <= 0 || h <= 0)
		return GL_FALSE;
	size_t need = (size_t)w * (size_t)h * zb;
	if (c->fbo_scratch_z.buf && c->fbo_scratch_z.w == w && c->fbo_scratch_z.h == h)
		return GL_TRUE;
	u8* p = (u8*)PGL_REALLOC(c->fbo_scratch_z.buf, need);
	if (!p)
		return GL_FALSE;
	c->fbo_scratch_z.buf = p;
	c->fbo_scratch_z.w = w;
	c->fbo_scratch_z.h = h;
	c->fbo_scratch_z.lastrow = p + (size_t)(h - 1) * (size_t)w * zb;
	return GL_TRUE;
}
#endif

// Resolve mrt_color[] from FBO color attachments (texture format bpp, not pix_t).
static void pgl_apply_color_attachments(glFBO* f)
{
	for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i) {
		memset(&c->mrt_color[i], 0, sizeof(c->mrt_color[i]));
		if (!f->color[i].tex)
			continue;
		glTexture* t = &c->textures.a[f->color[i].tex];
		pgl_tex_mark_render_target(t);
		int bpp = pgl_tex_bytes_per_pixel(t);
		c->mrt_color[i].buf = t->data;
		c->mrt_color[i].w = t->w;
		c->mrt_color[i].h = t->h;
		c->mrt_color[i].datatype = t->datatype;
		c->mrt_color[i].components = t->components;
		c->mrt_color[i].lastrow =
			t->data + (size_t)(t->h - 1) * (size_t)t->w * (size_t)bpp;
	}

	c->num_draw_buffers = f->num_draw_buffers;
	for (GLsizei i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
		c->draw_buffers[i] = (i < f->num_draw_buffers) ? f->draw_buffers[i] : (GLenum)GL_NONE;
	c->read_buffer = f->read_buffer;
	c->fbo_color_is_rt = GL_TRUE;

	// Dimension surface for scissor/viewport macros (buf may be unused for color writes)
	pglColorRT* primary = NULL;
	for (GLsizei i = 0; i < c->num_draw_buffers; ++i) {
		if (c->draw_buffers[i] == GL_NONE)
			continue;
		int att = (int)(c->draw_buffers[i] - GL_COLOR_ATTACHMENT0);
		if (att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS && c->mrt_color[att].buf) {
			primary = &c->mrt_color[att];
			break;
		}
	}
	if (!primary) {
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i) {
			if (c->mrt_color[i].buf) {
				primary = &c->mrt_color[i];
				break;
			}
		}
	}
	if (primary) {
		// Keep back_buffer dimensions in sync for raster bounds; do not use as pix_t target
		c->back_buffer.w = primary->w;
		c->back_buffer.h = primary->h;
		c->back_buffer.buf = primary->buf;
		c->back_buffer.lastrow = primary->lastrow;
	}

	{
		int n_active = 0;
		for (GLsizei i = 0; i < c->num_draw_buffers; ++i)
			if (c->draw_buffers[i] != GL_NONE)
				n_active++;
		c->mrt_active = (n_active > 1) ? GL_TRUE : GL_FALSE;
	}
}

// Point active draw surfaces at bound FBO attachments (or restore window).
static void pgl_apply_draw_framebuffer(void)
{
	if (!c->bound_framebuffer) {
		if (c->fbo_redirected) {
			c->back_buffer = c->window_back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
			c->zbuf = c->window_zbuf;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
			c->stencil_buf = c->window_stencil_buf;
#  endif
#endif
			c->fbo_redirected = GL_FALSE;
		}
		// Default FB: pix_t color, no MRT
		c->mrt_active = GL_FALSE;
		c->fbo_color_is_rt = GL_FALSE;
#ifndef PGL_NO_DEPTH_NO_STENCIL
		c->zbuf_float = GL_FALSE;
#endif
		c->num_draw_buffers = c->default_num_draw_buffers;
		for (GLsizei i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
			c->draw_buffers[i] = c->default_draw_buffers[i];
		c->read_buffer = c->default_read_buffer;
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i)
			memset(&c->mrt_color[i], 0, sizeof(c->mrt_color[i]));
		return;
	}

	glFBO* f = pgl_get_bound_user_fbo();
	if (!f)
		return;
	if (f->status_dirty)
		pgl_fbo_update_status(f);
	if (f->status != GL_FRAMEBUFFER_COMPLETE) {
		// Not drawable/readable: drop RT color routing so we never keep stale mrt_*
		// after draw/read buffer or attach changes. Window backup stays until unbind.
		c->fbo_color_is_rt = GL_FALSE;
		c->mrt_active = GL_FALSE;
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i)
			memset(&c->mrt_color[i], 0, sizeof(c->mrt_color[i]));
		return;
	}

	if (!c->fbo_redirected) {
		c->window_back_buffer = c->back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
		c->window_zbuf = c->zbuf;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
		c->window_stencil_buf = c->stencil_buf;
#  endif
#endif
		c->fbo_redirected = GL_TRUE;
	}

	pgl_apply_color_attachments(f);

#ifndef PGL_NO_DEPTH_NO_STENCIL
	GLsizei dw = c->back_buffer.w;
	GLsizei dh = c->back_buffer.h;
	c->zbuf_float = GL_FALSE;
	if (f->depth.tex) {
		glTexture* dt = &c->textures.a[f->depth.tex];
		pgl_tex_mark_render_target(dt);
		// Explicit depth textures use their own bpp; legacy "alias color storage as Z"
		// must use window Z packing (D16=2, D24S8=4), not the color texel's bpp.
		size_t zb;
		if (dt->is_depth)
			zb = (size_t)pgl_tex_bytes_per_pixel(dt);
		else
			zb = pgl_z_bytes_per_pixel();
		c->zbuf.buf = dt->data;
		c->zbuf.w = dt->w;
		c->zbuf.h = dt->h;
		c->zbuf.lastrow = dt->data + (size_t)(dt->h - 1) * (size_t)dt->w * zb;
		if (dt->is_depth && dt->datatype == GL_FLOAT)
			c->zbuf_float = GL_TRUE;
#  if defined(PGL_D24S8)
		if (!c->zbuf_float && (!f->stencil.rb || f->stencil.tex == f->depth.tex)) {
			c->stencil_buf.buf = dt->data;
			c->stencil_buf.w = dt->w;
			c->stencil_buf.h = dt->h;
			c->stencil_buf.lastrow = c->zbuf.lastrow;
		}
#  endif
	} else if (f->depth.rb) {
		glRenderbuffer* rb = &c->renderbuffers.a[f->depth.rb];
		c->zbuf.buf = rb->data;
		c->zbuf.w = rb->w;
		c->zbuf.h = rb->h;
		c->zbuf.lastrow = rb->lastrow;
		if (rb->internalformat == GL_DEPTH_COMPONENT32F)
			c->zbuf_float = GL_TRUE;
#  if defined(PGL_D24S8)
		if (!c->zbuf_float && f->stencil.rb == f->depth.rb) {
			c->stencil_buf = c->zbuf;
		}
#  endif
	} else {
		if (pgl_ensure_fbo_scratch_z(dw, dh))
			c->zbuf = c->fbo_scratch_z;
#  if defined(PGL_D24S8)
		c->stencil_buf.buf = c->zbuf.buf;
		c->stencil_buf.w = c->zbuf.w;
		c->stencil_buf.h = c->zbuf.h;
		c->stencil_buf.lastrow = c->zbuf.lastrow;
#  endif
	}
#  if !defined(PGL_NO_STENCIL) && defined(PGL_D16)
	if (f->stencil.rb && f->stencil.rb != f->depth.rb) {
		glRenderbuffer* srb = &c->renderbuffers.a[f->stencil.rb];
		c->stencil_buf.buf = srb->data;
		c->stencil_buf.w = srb->w;
		c->stencil_buf.h = srb->h;
		c->stencil_buf.lastrow = srb->lastrow;
	}
#  endif
#endif
}

static void pgl_fbo_mark_dirty(glFBO* f)
{
	f->status_dirty = GL_TRUE;
	// If this FBO is bound and was applied, re-apply after status update on next bind/check
}

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
		if (c->bound_framebuffer == id) {
			c->bound_framebuffer = 0;
			pgl_apply_draw_framebuffer();
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

	// v1: DRAW and READ share one bind
	c->bound_framebuffer = framebuffer;
	pgl_apply_draw_framebuffer();
}

PGLDEF void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                                   GLuint texture, GLint level)
{
	PGL_ERR(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER &&
	        target != GL_READ_FRAMEBUFFER, GL_INVALID_ENUM);
	PGL_ERR(!c->bound_framebuffer, GL_INVALID_OPERATION); // cannot attach to default FB

	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR(!f, GL_INVALID_OPERATION);

	if (texture != 0) {
		PGL_ERR(textarget != GL_TEXTURE_2D && textarget != GL_TEXTURE_RECTANGLE, GL_INVALID_OPERATION);
		PGL_ERR(level != 0, GL_INVALID_VALUE); // v1: level 0 only
		PGL_ERR(texture >= c->textures.size || c->textures.a[texture].deleted, GL_INVALID_VALUE);
	}

	if (attachment == GL_COLOR_ATTACHMENT0 ||
	    (attachment >= GL_COLOR_ATTACHMENT1 &&
	     attachment < GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS)) {
		int idx = (int)(attachment - GL_COLOR_ATTACHMENT0);
		f->color[idx].tex = texture;
		f->color[idx].rb = 0;
		f->color[idx].level = level;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
	} else if (attachment == GL_DEPTH_ATTACHMENT) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.tex = texture;
		f->depth.rb = 0;
		f->depth.level = level;
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
#endif
	} else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.tex = texture;
		f->depth.rb = 0;
		f->depth.level = level;
		f->stencil.tex = texture;
		f->stencil.rb = 0;
		f->stencil.level = level;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
#endif
	} else {
		PGL_ERR(1, GL_INVALID_ENUM);
	}

	pgl_fbo_mark_dirty(f);
	// Re-apply if still bound so draw targets update
	if (c->bound_framebuffer)
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

	if (!c->bound_framebuffer)
		return GL_FRAMEBUFFER_COMPLETE; // default FB always complete when context exists

	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR_RET_VAL(!f, GL_INVALID_OPERATION, 0);
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
	GLboolean seen[GL_MAX_COLOR_ATTACHMENTS];
	memset(seen, 0, sizeof(seen));

	for (GLsizei i = 0; i < n; ++i) {
		GLenum b = bufs[i];
		if (b == GL_NONE)
			continue;

		if (!c->bound_framebuffer) {
			// Default FB: only GL_BACK (and treat COLOR_ATTACHMENT0 as synonym)
			PGL_ERR(b != GL_BACK && b != GL_COLOR_ATTACHMENT0, GL_INVALID_ENUM);
			PGL_ERR(n != 1, GL_INVALID_OPERATION); // single buffer only
		} else {
			PGL_ERR(b < GL_COLOR_ATTACHMENT0 ||
			        b >= GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS, GL_INVALID_ENUM);
			int att = (int)(b - GL_COLOR_ATTACHMENT0);
			PGL_ERR(seen[att], GL_INVALID_OPERATION); // duplicate
			seen[att] = GL_TRUE;
		}
	}

	if (!c->bound_framebuffer) {
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

	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR(!f, GL_INVALID_OPERATION);

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

// Renderbuffers, framebuffer renderbuffer attach, read buffer/pixels

static void pgl_init_rb(glRenderbuffer* rb)
{
	memset(rb, 0, sizeof(*rb));
	rb->deleted = GL_FALSE;
	rb->user_owned = GL_FALSE;
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
	        internalformat != GL_STENCIL_INDEX8, GL_INVALID_ENUM);

	glRenderbuffer* rb = &c->renderbuffers.a[c->bound_renderbuffer];
	size_t bpp = pgl_z_bytes_per_pixel();
	if (internalformat == GL_DEPTH_COMPONENT32F)
		bpp = sizeof(float);
	else if (internalformat == GL_STENCIL_INDEX8)
		bpp = 1;
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
	PGL_ERR(!c->bound_framebuffer, GL_INVALID_OPERATION);

	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR(!f, GL_INVALID_OPERATION);

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
	if (c->bound_framebuffer)
		pgl_apply_draw_framebuffer();
}

PGLDEF void glReadBuffer(GLenum mode)
{
	if (!c->bound_framebuffer) {
		PGL_ERR(mode != GL_BACK && mode != GL_COLOR_ATTACHMENT0, GL_INVALID_ENUM);
		c->default_read_buffer = (mode == GL_COLOR_ATTACHMENT0) ? GL_BACK : mode;
		c->read_buffer = c->default_read_buffer;
		return;
	}
	PGL_ERR(mode != GL_NONE &&
	        (mode < GL_COLOR_ATTACHMENT0 ||
	         mode >= GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS), GL_INVALID_ENUM);
	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR(!f, GL_INVALID_OPERATION);
	f->read_buffer = mode;
	c->read_buffer = mode;
	// Read buffer affects completeness (INCOMPLETE_READ_BUFFER)
	pgl_fbo_mark_dirty(f);
	pgl_apply_draw_framebuffer();
}

// Thin glReadPixels: RGBA U8 or float RGBA/R from current read color buffer.
// (x,y) is GL bottom-left origin; converts to storage with invert for RTs.
PGLDEF void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, GLvoid* data)
{
	PGL_ERR(width < 0 || height < 0, GL_INVALID_VALUE);
	PGL_ERR(!data, GL_INVALID_VALUE);
	PGL_ERR(format != GL_RGBA && format != GL_RED, GL_INVALID_ENUM);
	PGL_ERR(type != GL_UNSIGNED_BYTE && type != GL_FLOAT, GL_INVALID_ENUM);
	// User FBO must be complete (includes DRAW_BUFFER / READ_BUFFER rules)
	PGL_ERR(c->bound_framebuffer && !pgl_draw_framebuffer_ok(),
	        GL_INVALID_FRAMEBUFFER_OPERATION);

	if (!width || !height)
		return;

	// Resolve source
	u8* src_base = NULL;
	GLsizei sw = 0, sh = 0;
	GLenum src_type = GL_UNSIGNED_BYTE;
	int src_comp = 4;
	GLboolean invert = GL_FALSE;

	if (!c->bound_framebuffer || !c->fbo_color_is_rt) {
		src_base = c->back_buffer.buf;
		sw = c->back_buffer.w;
		sh = c->back_buffer.h;
		// Window is top-down memory; GL y=0 is bottom → invert
		invert = GL_TRUE;
		src_type = GL_UNSIGNED_BYTE;
		// will convert from pix_t
	} else {
		GLenum rb = c->read_buffer;
		if (rb == GL_NONE)
			return;
		int att = (int)(rb - GL_COLOR_ATTACHMENT0);
		// Completeness guarantees a non-NONE read buffer has an attachment
		PGL_ASSERT(att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS);
		PGL_ASSERT(c->mrt_color[att].buf);
		pglColorRT* rt = &c->mrt_color[att];
		src_base = rt->buf;
		sw = rt->w;
		sh = rt->h;
		src_type = rt->datatype;
		src_comp = rt->components;
		invert = GL_TRUE; // RT lastrow / invert_y: GL y=0 = bottom
	}

	for (GLsizei row = 0; row < height; ++row) {
		for (GLsizei col = 0; col < width; ++col) {
			GLint sx = x + col;
			GLint sy_gl = y + row; // bottom-left origin
			if (sx < 0 || sy_gl < 0 || sx >= sw || sy_gl >= sh)
				continue;
			GLint sy = invert ? (sh - 1 - sy_gl) : sy_gl;
			int idx = sy * sw + sx;
			float fr = 0, fg = 0, fb = 0, fa = 1;

			if (!c->bound_framebuffer || !c->fbo_color_is_rt) {
				pix_t p = ((pix_t*)src_base)[idx];
				Color colc = PIXEL_TO_COLOR(p);
				fr = colc.r / (float)PGL_RMAX;
				fg = colc.g / (float)PGL_GMAX;
				fb = colc.b / (float)PGL_BMAX;
				fa = colc.a / (float)PGL_AMAX;
			} else if (src_type == GL_FLOAT) {
				const float* f = (const float*)src_base + idx * src_comp;
				fr = f[0];
				if (src_comp > 1) fg = f[1];
				if (src_comp > 2) fb = f[2];
				if (src_comp > 3) fa = f[3];
			} else {
				Color colc = ((Color*)src_base)[idx];
				fr = colc.r / 255.f;
				fg = colc.g / 255.f;
				fb = colc.b / 255.f;
				fa = colc.a / 255.f;
			}

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
