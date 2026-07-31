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

	// Phase B: require color0 for drawable FBOs (depth-only not useful yet)
	if (!f->color[0].tex)
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;

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

	glTexture* ct = &c->textures.a[f->color[0].tex];
	pgl_tex_mark_render_target(ct);
	c->back_buffer.buf = ct->data;
	c->back_buffer.w = ct->w;
	c->back_buffer.h = ct->h;
	// Draw macros use pix_t stride (U8 RGBA color attachments only).
	c->back_buffer.lastrow = ct->data + (size_t)(ct->h - 1) * (size_t)ct->w * sizeof(pix_t);

#ifndef PGL_NO_DEPTH_NO_STENCIL
	if (f->depth.tex) {
		glTexture* dt = &c->textures.a[f->depth.tex];
		pgl_tex_mark_render_target(dt);
		size_t zb = pgl_z_bytes_per_pixel();
		c->zbuf.buf = dt->data;
		c->zbuf.w = dt->w;
		c->zbuf.h = dt->h;
		c->zbuf.lastrow = dt->data + (size_t)(dt->h - 1) * (size_t)dt->w * zb;
#  if defined(PGL_D24S8)
		// Stencil lives in the packed depth buffer for D24S8
		c->stencil_buf.buf = dt->data;
		c->stencil_buf.w = dt->w;
		c->stencil_buf.h = dt->h;
		c->stencil_buf.lastrow = c->zbuf.lastrow;
#  endif
	} else {
		// Color-only: scratch Z so depth_test sizes match color (values discarded on unbind)
		if (pgl_ensure_fbo_scratch_z(ct->w, ct->h))
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
