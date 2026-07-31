// Framebuffer objects (Phase B) — gen/bind/delete, color0 + depth texture attach,
// completeness, redirect back_buffer/zbuf to attachments.
// Relies on amalgamation after gl_impl.c for PGL_ERR and pgl_tex_mark_render_target.

#ifndef PGL_MAX_COLOR_ATTACHMENTS
#define PGL_MAX_COLOR_ATTACHMENTS 4
#endif

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

// Depth buffer can alias texture storage when allocation is large enough for dense Z.
static GLboolean pgl_tex_can_alias_depth(const glTexture* t)
{
	if (!pgl_tex_is_2d_level0(t))
		return GL_FALSE;
	size_t need = (size_t)t->w * (size_t)t->h * pgl_z_bytes_per_pixel();
	if (!need)
		return GL_FALSE;
	// Mapped/allocated textures are RGBA U8 (4) or float (16) per pixel for L0.
	size_t have = (size_t)t->w * (size_t)t->h * (size_t)pgl_tex_bytes_per_pixel(t);
	return have >= need;
}

static GLenum pgl_fbo_compute_status(glFBO* f)
{
	int n_attach = 0;
	GLsizei aw = 0, ah = 0;
	GLboolean have_size = GL_FALSE;

	for (int i = 0; i < PGL_MAX_COLOR_ATTACHMENTS; ++i) {
		if (!f->color[i].tex)
			continue;
		if (f->color[i].level != 0)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		if (f->color[i].tex >= c->textures.size)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		glTexture* t = &c->textures.a[f->color[i].tex];
		if (!pgl_tex_is_2d_level0(t))
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		// Color write path is pix_t / U8 RGBA; float color attachments not drawable yet
		if (t->datatype == GL_FLOAT)
			return GL_FRAMEBUFFER_UNSUPPORTED;
		if (!have_size) {
			aw = t->w;
			ah = t->h;
			have_size = GL_TRUE;
		} else if (t->w != aw || t->h != ah) {
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
		}
		n_attach++;
	}

	if (f->depth.tex) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		return GL_FRAMEBUFFER_UNSUPPORTED;
#else
		if (f->depth.level != 0)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		if (f->depth.tex >= c->textures.size)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		glTexture* t = &c->textures.a[f->depth.tex];
		if (!pgl_tex_is_2d_level0(t) || !pgl_tex_can_alias_depth(t))
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		if (!have_size) {
			aw = t->w;
			ah = t->h;
			have_size = GL_TRUE;
		} else if (t->w != aw || t->h != ah) {
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
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
		for (int i = 0; i < PGL_MAX_COLOR_ATTACHMENTS; ++i)
			if (f->color[i].tex) n_color++;
		if (!n_color)
			return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
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

// Resolve mrt_color[] from FBO color attachments; set back_buffer + mrt_active.
static void pgl_apply_color_attachments(glFBO* f)
{
	GLsizei cw = 0, ch = 0;

	for (int i = 0; i < PGL_MAX_COLOR_ATTACHMENTS; ++i) {
		c->mrt_color[i].buf = NULL;
		c->mrt_color[i].lastrow = NULL;
		c->mrt_color[i].w = 0;
		c->mrt_color[i].h = 0;
		if (!f->color[i].tex)
			continue;
		glTexture* t = &c->textures.a[f->color[i].tex];
		pgl_tex_mark_render_target(t);
		c->mrt_color[i].buf = t->data;
		c->mrt_color[i].w = t->w;
		c->mrt_color[i].h = t->h;
		c->mrt_color[i].lastrow =
			t->data + (size_t)(t->h - 1) * (size_t)t->w * sizeof(pix_t);
		cw = t->w;
		ch = t->h;
	}

	// Active draw buffer list from this FBO
	c->num_draw_buffers = f->num_draw_buffers;
	for (GLsizei i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
		c->draw_buffers[i] = (i < f->num_draw_buffers) ? f->draw_buffers[i] : (GLenum)GL_NONE;

	// back_buffer = first non-NONE draw buffer's attachment (else first attached color)
	glFramebuffer* primary = NULL;
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
		for (int i = 0; i < PGL_MAX_COLOR_ATTACHMENTS; ++i) {
			if (c->mrt_color[i].buf) {
				primary = &c->mrt_color[i];
				break;
			}
		}
	}
	if (primary)
		c->back_buffer = *primary;

	// MRT path only when more than one non-NONE draw buffer (else gl_FragColor → back_buffer)
	{
		int n_active = 0;
		for (GLsizei i = 0; i < c->num_draw_buffers; ++i)
			if (c->draw_buffers[i] != GL_NONE)
				n_active++;
		c->mrt_active = (n_active > 1) ? GL_TRUE : GL_FALSE;
	}

	(void)cw;
	(void)ch;
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
		// Default FB: single color, no MRT
		c->mrt_active = GL_FALSE;
		c->num_draw_buffers = c->default_num_draw_buffers;
		for (GLsizei i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
			c->draw_buffers[i] = c->default_draw_buffers[i];
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i) {
			c->mrt_color[i].buf = NULL;
			c->mrt_color[i].w = c->mrt_color[i].h = 0;
			c->mrt_color[i].lastrow = NULL;
		}
		return;
	}

	glFBO* f = pgl_get_bound_user_fbo();
	if (!f)
		return;
	if (f->status_dirty)
		pgl_fbo_update_status(f);
	if (f->status != GL_FRAMEBUFFER_COMPLETE)
		return;

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
	if (f->depth.tex) {
		glTexture* dt = &c->textures.a[f->depth.tex];
		pgl_tex_mark_render_target(dt);
		size_t zb = pgl_z_bytes_per_pixel();
		c->zbuf.buf = dt->data;
		c->zbuf.w = dt->w;
		c->zbuf.h = dt->h;
		c->zbuf.lastrow = dt->data + (size_t)(dt->h - 1) * (size_t)dt->w * zb;
#  if defined(PGL_D24S8)
		c->stencil_buf.buf = dt->data;
		c->stencil_buf.w = dt->w;
		c->stencil_buf.h = dt->h;
		c->stencil_buf.lastrow = c->zbuf.lastrow;
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

	if (attachment == GL_COLOR_ATTACHMENT0) {
		f->color[0].tex = texture;
		f->color[0].level = level;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
	} else if (attachment >= GL_COLOR_ATTACHMENT1 &&
	           attachment < GL_COLOR_ATTACHMENT0 + PGL_MAX_COLOR_ATTACHMENTS) {
		// Accept attach for Phase C; Phase B draw uses color0 only
		int idx = (int)(attachment - GL_COLOR_ATTACHMENT0);
		f->color[idx].tex = texture;
		f->color[idx].level = level;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
	} else if (attachment == GL_DEPTH_ATTACHMENT) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.tex = texture;
		f->depth.level = level;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
#endif
	} else if (attachment == GL_STENCIL_ATTACHMENT ||
	           attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
		// Not implemented in Phase B
		PGL_ERR(1, GL_INVALID_ENUM);
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

// Phase C: select which color attachments receive FS outputs (gl_FragData[i] → bufs[i]).
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

	// Refresh back_buffer / mrt_active if this FBO is complete and bound
	pgl_apply_draw_framebuffer();
}
