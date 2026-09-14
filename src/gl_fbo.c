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

static GLboolean pgl_is_cube_face_target(GLenum textarget)
{
	return textarget >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	       textarget <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
}

static GLboolean pgl_tex_ok_depth_cube(const glTexture* t)
{
	if (!t || t->deleted || !t->is_depth || !t->data)
		return GL_FALSE;
	GLenum target = t->type + GL_TEXTURE_UNBOUND + 1;
	if (target != GL_TEXTURE_CUBE_MAP)
		return GL_FALSE;
	if (t->w <= 0 || t->h <= 0 || t->w != t->h || t->num_levels < 1)
		return GL_FALSE;
	return GL_TRUE;
}

#ifndef PGL_NO_DEPTH_NO_STENCIL
static u8* pgl_depth_tex_surf(glTexture* dt, GLenum textarget, size_t* zb_out)
{
	size_t zb = dt->is_depth ? (size_t)pgl_tex_bytes_per_pixel(dt) : pgl_z_bytes_per_pixel();
	*zb_out = zb;
	int face = 0;
	if (pgl_is_cube_face_target(textarget))
		face = (int)(textarget - GL_TEXTURE_CUBE_MAP_POSITIVE_X);
	return dt->data + (size_t)face * (size_t)dt->w * (size_t)dt->h * zb;
}
#endif

static GLboolean pgl_rb_ok_depth(const glRenderbuffer* rb)
{
	if (!rb || rb->deleted || !rb->data || rb->w <= 0 || rb->h <= 0)
		return GL_FALSE;
	return rb->internalformat == GL_DEPTH_COMPONENT ||
	       rb->internalformat == GL_DEPTH_COMPONENT16 ||
	       rb->internalformat == GL_DEPTH_COMPONENT24 ||
	       rb->internalformat == GL_DEPTH_COMPONENT32 ||
	       rb->internalformat == GL_DEPTH_COMPONENT32F ||
	       rb->internalformat == GL_DEPTH24_STENCIL8;
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
			if (pgl_is_cube_face_target(f->depth.textarget)) {
				if (!pgl_tex_ok_depth_cube(t))
					return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			} else if (!pgl_tex_ok_depth_attach(t)) {
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
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

	if (!n_attach || !have_size)
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;

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

static glFBO* pgl_user_fbo(GLuint id)
{
	PGL_ASSERT(id != 0);
	PGL_ASSERT(id < c->framebuffers.size && !c->framebuffers.a[id].deleted);
	return &c->framebuffers.a[id];
}

static glFBO* pgl_draw_user_fbo(void)
{
	return pgl_user_fbo(c->bound_draw_framebuffer);
}

static GLuint pgl_fbo_id_for_target(GLenum target)
{
	if (target == GL_READ_FRAMEBUFFER)
		return c->bound_read_framebuffer;
	return c->bound_draw_framebuffer;
}

static GLboolean pgl_fbo_id_complete(GLuint id)
{
	if (!id)
		return GL_TRUE;
	glFBO* f = pgl_user_fbo(id);
	if (f->status_dirty)
		pgl_fbo_update_status(f);
	return f->status == GL_FRAMEBUFFER_COMPLETE;
}

static GLboolean pgl_draw_framebuffer_ok(void)
{
	return pgl_fbo_id_complete(c->bound_draw_framebuffer);
}

static void pgl_sync_read_state(void)
{
	if (!c->bound_read_framebuffer) {
		c->read_buffer = c->default_read_buffer;
		return;
	}
	c->read_buffer = pgl_user_fbo(c->bound_read_framebuffer)->read_buffer;
}

#ifndef PGL_NO_DEPTH_NO_STENCIL
static GLboolean pgl_ensure_fbo_scratch_z(GLsizei w, GLsizei h)
{
	size_t zb = pgl_z_bytes_per_pixel();
	PGL_ASSERT(zb && w > 0 && h > 0);
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

static int pgl_fbo_has_color(const glFBO* f)
{
	for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i)
		if (f->color[i].tex)
			return 1;
	return 0;
}

// Point active draw surfaces at bound FBO attachments (or restore window).
static void pgl_apply_draw_framebuffer(void)
{
	if (!c->bound_draw_framebuffer) {
		if (c->fbo_redirected) {
			c->back_buffer = c->window_back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
			c->zbuf = c->window_zbuf;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
			c->stencil_buf = c->window_stencil_buf;
#  elif defined(PGL_D24S8)
			c->stencil_buf = c->window_zbuf;
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
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i)
			memset(&c->mrt_color[i], 0, sizeof(c->mrt_color[i]));
		pgl_update_clip_rect();
		return;
	}

	glFBO* f = pgl_draw_user_fbo();
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
		size_t zb;
		u8* surf = pgl_depth_tex_surf(dt, f->depth.textarget, &zb);
		c->zbuf.buf = surf;
		c->zbuf.w = dt->w;
		c->zbuf.h = dt->h;
		c->zbuf.lastrow = surf + (size_t)(dt->h - 1) * (size_t)dt->w * zb;
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
	// Depth-only: no color image to size clip from — use the depth surface.
	if (!pgl_fbo_has_color(f)) {
#ifndef PGL_NO_DEPTH_NO_STENCIL
		c->back_buffer.w = c->zbuf.w;
		c->back_buffer.h = c->zbuf.h;
#else
		c->back_buffer.w = 0;
		c->back_buffer.h = 0;
#endif
		c->back_buffer.buf = NULL;
		c->back_buffer.lastrow = NULL;
	}
	pgl_update_clip_rect();
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
		PGL_ERR(level != 0, GL_INVALID_VALUE); // v1: level 0 only
		PGL_ERR(texture >= c->textures.size || c->textures.a[texture].deleted, GL_INVALID_VALUE);
		if (pgl_is_cube_face_target(textarget)) {
			PGL_ERR(attachment != GL_DEPTH_ATTACHMENT, GL_INVALID_OPERATION);
			GLenum tex_tgt = c->textures.a[texture].type + GL_TEXTURE_UNBOUND + 1;
			PGL_ERR(tex_tgt != GL_TEXTURE_CUBE_MAP, GL_INVALID_OPERATION);
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

// Default FB is mono with one color buffer. These names all mean that buffer.
static GLboolean pgl_default_fb_is_window_color(GLenum b)
{
	return b == GL_BACK || b == GL_FRONT || b == GL_LEFT ||
	       b == GL_FRONT_LEFT || b == GL_BACK_LEFT;
}

static GLboolean pgl_default_fb_missing_stereo(GLenum b)
{
	return b == GL_RIGHT || b == GL_FRONT_RIGHT || b == GL_BACK_RIGHT;
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

// Renderbuffers, framebuffer renderbuffer attach, read buffer/pixels

static void pgl_init_rb(glRenderbuffer* rb)
{
	memset(rb, 0, sizeof(*rb));
	rb->deleted = GL_FALSE;
	rb->user_owned = GL_FALSE;
}

// TODO move to gl_impl
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

typedef struct {
	u8* buf;
	GLsizei w, h;
	GLenum datatype;
	int components;
	GLboolean is_pix_t;
	GLboolean valid; // GL_FALSE = no color image (e.g. GL_NONE read/draw buffer)
} pglBlitColor;

typedef struct {
	u8* buf;
	GLsizei w, h;
	GLboolean is_float;
	u8* stencil; // D16 separate S8; NULL if none or packed in buf
	GLboolean valid; // GL_FALSE = no depth image
} pglBlitDepth;

static void pgl_resolve_window_color(pglBlitColor* s)
{
	glFramebuffer* bb = c->fbo_redirected ? &c->window_back_buffer : &c->back_buffer;
	PGL_ASSERT(bb->buf && bb->w > 0 && bb->h > 0);
	s->buf = bb->buf;
	s->w = bb->w;
	s->h = bb->h;
	s->datatype = GL_UNSIGNED_BYTE;
	s->components = 4;
	s->is_pix_t = GL_TRUE;
	s->valid = GL_TRUE;
}

static void pgl_resolve_fbo_color(glFBO* f, GLenum att_enum, pglBlitColor* s)
{
	s->valid = GL_FALSE;
	s->buf = NULL;
	if (att_enum == GL_NONE)
		return;
	int att = (int)(att_enum - GL_COLOR_ATTACHMENT0);
	PGL_ASSERT(att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS && f->color[att].tex);
	glTexture* t = &c->textures.a[f->color[att].tex];
	PGL_ASSERT(t->data);
	s->buf = t->data;
	s->w = t->w;
	s->h = t->h;
	s->datatype = t->datatype;
	s->components = t->components;
	s->is_pix_t = GL_FALSE;
	s->valid = GL_TRUE;
}

static void pgl_resolve_read_color(pglBlitColor* s)
{
	if (!c->bound_read_framebuffer) {
		pgl_resolve_window_color(s);
		return;
	}
	glFBO* f = pgl_user_fbo(c->bound_read_framebuffer);
	pgl_resolve_fbo_color(f, f->read_buffer, s);
}

#ifndef PGL_NO_DEPTH_NO_STENCIL
static void pgl_resolve_window_depth(pglBlitDepth* s)
{
	glFramebuffer* zb = c->fbo_redirected ? &c->window_zbuf : &c->zbuf;
	PGL_ASSERT(zb->buf && zb->w > 0 && zb->h > 0);
	s->buf = zb->buf;
	s->w = zb->w;
	s->h = zb->h;
	s->is_float = GL_FALSE;
	s->valid = GL_TRUE;
	s->stencil = NULL;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
	{
		glFramebuffer* sb = c->fbo_redirected ? &c->window_stencil_buf : &c->stencil_buf;
		s->stencil = sb->buf;
	}
#  endif
}

static void pgl_resolve_fbo_depth(glFBO* f, pglBlitDepth* s)
{
	s->valid = GL_FALSE;
	s->buf = NULL;
	s->stencil = NULL;
	if (!f->depth.tex && !f->depth.rb)
		return;
	if (f->depth.tex) {
		glTexture* t = &c->textures.a[f->depth.tex];
		PGL_ASSERT(t->data);
		size_t zb;
		s->buf = pgl_depth_tex_surf(t, f->depth.textarget, &zb);
		s->w = t->w;
		s->h = t->h;
		s->is_float = (t->is_depth && t->datatype == GL_FLOAT) ? GL_TRUE : GL_FALSE;
	} else {
		glRenderbuffer* rb = &c->renderbuffers.a[f->depth.rb];
		PGL_ASSERT(rb->data);
		s->buf = rb->data;
		s->w = rb->w;
		s->h = rb->h;
		s->is_float = (rb->internalformat == GL_DEPTH_COMPONENT32F) ? GL_TRUE : GL_FALSE;
	}
	s->valid = GL_TRUE;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
	if (f->stencil.rb && f->stencil.rb != f->depth.rb) {
		glRenderbuffer* srb = &c->renderbuffers.a[f->stencil.rb];
		PGL_ASSERT(srb->data);
		s->stencil = srb->data;
	}
#  endif
}

static void pgl_resolve_read_depth(pglBlitDepth* s)
{
	if (!c->bound_read_framebuffer) {
		pgl_resolve_window_depth(s);
		return;
	}
	pgl_resolve_fbo_depth(pgl_user_fbo(c->bound_read_framebuffer), s);
}

static void pgl_resolve_draw_depth(pglBlitDepth* s)
{
	if (!c->bound_draw_framebuffer) {
		pgl_resolve_window_depth(s);
		return;
	}
	pgl_resolve_fbo_depth(pgl_user_fbo(c->bound_draw_framebuffer), s);
}
#endif

static int pgl_blit_idx(GLsizei w, GLsizei h, int x, int y)
{
	return (h - 1 - y) * w + x;
}

static void pgl_blit_get_rgba(const pglBlitColor* s, int x, int y, float* r, float* g, float* b, float* a)
{
	*r = *g = *b = 0.f;
	*a = 1.f;
	if (x < 0 || y < 0 || x >= s->w || y >= s->h)
		return;
	int idx = pgl_blit_idx(s->w, s->h, x, y);
	if (s->is_pix_t) {
		Color colc = PIXEL_TO_COLOR(((pix_t*)s->buf)[idx]);
		*r = colc.r / (float)PGL_RMAX;
		*g = colc.g / (float)PGL_GMAX;
		*b = colc.b / (float)PGL_BMAX;
		*a = colc.a / (float)PGL_AMAX;
	} else if (s->datatype == GL_FLOAT) {
		const float* f = (const float*)s->buf + idx * s->components;
		*r = f[0];
		if (s->components > 1) *g = f[1];
		if (s->components > 2) *b = f[2];
		if (s->components > 3) *a = f[3];
	} else {
		Color colc = ((Color*)s->buf)[idx];
		*r = colc.r / 255.f;
		*g = colc.g / 255.f;
		*b = colc.b / 255.f;
		*a = colc.a / 255.f;
	}
}

static void pgl_blit_put_rgba(const pglBlitColor* s, int x, int y, float r, float g, float b, float a)
{
	PGL_ASSERT(x >= 0 && y >= 0 && x < s->w && y < s->h);
	int idx = pgl_blit_idx(s->w, s->h, x, y);
	if (r < 0.f) r = 0.f; if (r > 1.f) r = 1.f;
	if (g < 0.f) g = 0.f; if (g > 1.f) g = 1.f;
	if (b < 0.f) b = 0.f; if (b > 1.f) b = 1.f;
	if (a < 0.f) a = 0.f; if (a > 1.f) a = 1.f;
	if (s->is_pix_t) {
		((pix_t*)s->buf)[idx] = RGBA_TO_PIXEL(r * PGL_RMAX, g * PGL_GMAX, b * PGL_BMAX, a * PGL_AMAX);
	} else if (s->datatype == GL_FLOAT) {
		float* f = (float*)s->buf + idx * s->components;
		f[0] = r;
		if (s->components > 1) f[1] = g;
		if (s->components > 2) f[2] = b;
		if (s->components > 3) f[3] = a;
	} else {
		Color* p = (Color*)s->buf + idx;
		p->r = (u8)(r * 255.f);
		p->g = (u8)(g * 255.f);
		p->b = (u8)(b * 255.f);
		p->a = (u8)(a * 255.f);
	}
}

#ifndef PGL_NO_DEPTH_NO_STENCIL
static float pgl_blit_get_depth(const pglBlitDepth* s, int x, int y)
{
	if (x < 0 || y < 0 || x >= s->w || y >= s->h)
		return 0.f;
	int idx = pgl_blit_idx(s->w, s->h, x, y);
	if (s->is_float)
		return ((float*)s->buf)[idx];
#  if defined(PGL_D16)
	return ((u16*)s->buf)[idx] / (float)PGL_MAX_Z;
#  else
	return (((u32*)s->buf)[idx] >> PGL_ZSHIFT) / (float)PGL_MAX_Z;
#  endif
}

static void pgl_blit_put_depth(const pglBlitDepth* s, int x, int y, float d, GLboolean write_stencil, u8 stencil)
{
	PGL_ASSERT(x >= 0 && y >= 0 && x < s->w && y < s->h);
	if (d < 0.f) d = 0.f;
	if (d > 1.f) d = 1.f;
	int idx = pgl_blit_idx(s->w, s->h, x, y);
	if (s->is_float) {
		((float*)s->buf)[idx] = d;
		return;
	}
#  if defined(PGL_D16)
	((u16*)s->buf)[idx] = (u16)(d * PGL_MAX_Z);
	(void)write_stencil;
	(void)stencil;
#  else
	u32* p = (u32*)s->buf + idx;
	u32 zbits = ((u32)(d * PGL_MAX_Z)) << PGL_ZSHIFT;
	if (write_stencil)
		*p = zbits | stencil;
	else
		*p = (*p & PGL_STENCIL_MASK) | zbits;
#  endif
}

#  if !defined(PGL_NO_STENCIL)
static u8 pgl_blit_get_stencil(const pglBlitDepth* s, int x, int y)
{
	if (x < 0 || y < 0 || x >= s->w || y >= s->h)
		return 0;
	int idx = pgl_blit_idx(s->w, s->h, x, y);
#    if defined(PGL_D16)
	return s->stencil[idx];
#    else
	return (u8)(((u32*)s->buf)[idx] & PGL_STENCIL_MASK);
#    endif
}

static void pgl_blit_put_stencil(const pglBlitDepth* s, int x, int y, u8 stencil)
{
	PGL_ASSERT(x >= 0 && y >= 0 && x < s->w && y < s->h);
	int idx = pgl_blit_idx(s->w, s->h, x, y);
#    if defined(PGL_D16)
	s->stencil[idx] = stencil;
#    else
	u32* p = (u32*)s->buf + idx;
	*p = (*p & ~PGL_STENCIL_MASK) | stencil;
#    endif
}
#  endif
#endif

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

static void pgl_blit_sample_color(const pglBlitColor* src, float sx, float sy, GLenum filter,
                                  float* r, float* g, float* b, float* a)
{
	if (filter != GL_LINEAR) {
		int ix = (int)floorf(sx);
		int iy = (int)floorf(sy);
		pgl_blit_get_rgba(src, ix, iy, r, g, b, a);
		return;
	}
	int x0 = (int)floorf(sx);
	int y0 = (int)floorf(sy);
	float fx = sx - (float)x0;
	float fy = sy - (float)y0;
	float r00, g00, b00, a00, r10, g10, b10, a10, r01, g01, b01, a01, r11, g11, b11, a11;
	pgl_blit_get_rgba(src, x0, y0, &r00, &g00, &b00, &a00);
	pgl_blit_get_rgba(src, x0 + 1, y0, &r10, &g10, &b10, &a10);
	pgl_blit_get_rgba(src, x0, y0 + 1, &r01, &g01, &b01, &a01);
	pgl_blit_get_rgba(src, x0 + 1, y0 + 1, &r11, &g11, &b11, &a11);
	*r = r00 * (1 - fx) * (1 - fy) + r10 * fx * (1 - fy) + r01 * (1 - fx) * fy + r11 * fx * fy;
	*g = g00 * (1 - fx) * (1 - fy) + g10 * fx * (1 - fy) + g01 * (1 - fx) * fy + g11 * fx * fy;
	*b = b00 * (1 - fx) * (1 - fy) + b10 * fx * (1 - fy) + b01 * (1 - fx) * fy + b11 * fx * fy;
	*a = a00 * (1 - fx) * (1 - fy) + a10 * fx * (1 - fy) + a01 * (1 - fx) * fy + a11 * fx * fy;
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
