// Framebuffer objects — static helpers only. Public gl* FBO/clear APIs are in gl_impl.c.
// Amalgamated after gl_err.c / gl_internal.c / gl_tex_internal.c and before gl_impl.c.

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

static GLboolean pgl_tex_ok_color_cube(const glTexture* t)
{
	if (!t || t->deleted || t->is_depth || !t->data)
		return GL_FALSE;
	GLenum target = t->type + GL_TEXTURE_UNBOUND + 1;
	if (target != GL_TEXTURE_CUBE_MAP)
		return GL_FALSE;
	if (t->w <= 0 || t->h <= 0 || t->w != t->h || t->num_levels < 1)
		return GL_FALSE;
	return GL_TRUE;
}

static GLboolean pgl_tex_has_mip(const glTexture* t, GLint level)
{
	return t && !t->deleted && t->data && level >= 0 &&
	       level < t->num_levels && t->levels[level].data != NULL;
}

// Cube face targets offset into the 6-face pack at `level`; otherwise the level image.
static u8* pgl_tex_level_face_surf(glTexture* t, GLenum textarget, GLint level, size_t bpp)
{
	PGL_ASSERT(pgl_tex_has_mip(t, level));
	GLsizei lw = t->levels[level].w;
	GLsizei lh = t->levels[level].h;
	int face = 0;
	if (pgl_is_cube_face_target(textarget))
		face = (int)(textarget - GL_TEXTURE_CUBE_MAP_POSITIVE_X);
	return t->levels[level].data + (size_t)face * (size_t)lw * (size_t)lh * bpp;
}

#ifndef PGL_NO_DEPTH_NO_STENCIL
static u8* pgl_depth_tex_surf(glTexture* dt, GLenum textarget, GLint level, size_t* zb_out)
{
	size_t zb = dt->is_depth ? (size_t)pgl_tex_bytes_per_pixel(dt) : pgl_z_bytes_per_pixel();
	*zb_out = zb;
	return pgl_tex_level_face_surf(dt, textarget, level, zb);
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
		if (f->color[i].tex >= c->textures.size)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		glTexture* t = &c->textures.a[f->color[i].tex];
		if (pgl_is_cube_face_target(f->color[i].textarget)) {
			if (!pgl_tex_ok_color_cube(t))
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		} else if (!pgl_tex_is_2d_level0(t) || t->is_depth) {
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}
		if (!pgl_tex_has_mip(t, f->color[i].level))
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		// Drawable color: U8 RGBA, or float R/RG/RGBA (no RGB32F)
		if (t->datatype == GL_FLOAT) {
			if (t->components != 1 && t->components != 2 && t->components != 4)
				return GL_FRAMEBUFFER_UNSUPPORTED;
		} else if (t->datatype != GL_UNSIGNED_BYTE || t->components != 4) {
			return GL_FRAMEBUFFER_UNSUPPORTED;
		}
		{
			GLsizei cw = t->levels[f->color[i].level].w;
			GLsizei ch = t->levels[f->color[i].level].h;
			if (!have_size) {
				aw = cw;
				ah = ch;
				have_size = GL_TRUE;
			} else if (cw != aw || ch != ah) {
				return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
			}
		}
		n_attach++;
	}

	if (f->depth.tex || f->depth.rb) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		return GL_FRAMEBUFFER_UNSUPPORTED;
#else
		GLsizei dw = 0, dh = 0;
		if (f->depth.tex) {
			if (f->depth.tex >= c->textures.size)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			glTexture* t = &c->textures.a[f->depth.tex];
			if (pgl_is_cube_face_target(f->depth.textarget)) {
				if (!pgl_tex_ok_depth_cube(t))
					return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			} else if (!pgl_tex_ok_depth_attach(t)) {
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
			if (!pgl_tex_has_mip(t, f->depth.level))
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			dw = t->levels[f->depth.level].w;
			dh = t->levels[f->depth.level].h;
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

// Resolve mrt_color[] from FBO color attachments (texture format bpp, not pix_t).
static void pgl_apply_color_attachments(glFBO* f)
{
	for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i) {
		memset(&c->mrt_color[i], 0, sizeof(c->mrt_color[i]));
		if (!f->color[i].tex)
			continue;
		glTexture* t = &c->textures.a[f->color[i].tex];
		pgl_tex_mark_render_target(t);
		GLint lv = f->color[i].level;
		int bpp = pgl_tex_bytes_per_pixel(t);
		GLsizei lw = t->levels[lv].w;
		GLsizei lh = t->levels[lv].h;
		u8* surf = pgl_tex_level_face_surf(t, f->color[i].textarget, lv, (size_t)bpp);
		c->mrt_color[i].buf = surf;
		c->mrt_color[i].w = lw;
		c->mrt_color[i].h = lh;
		c->mrt_color[i].datatype = t->datatype;
		c->mrt_color[i].components = t->components;
		c->mrt_color[i].lastrow =
			surf + (size_t)(lh - 1) * (size_t)lw * (size_t)bpp;
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
		c->has_depth_buf = GL_TRUE;
#  ifndef PGL_NO_STENCIL
		c->has_stencil_buf = GL_TRUE;
#  else
		c->has_stencil_buf = GL_FALSE;
#  endif
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
	c->zbuf_float = GL_FALSE;
	c->has_depth_buf = GL_FALSE;
	c->has_stencil_buf = GL_FALSE;
	if (f->depth.tex) {
		glTexture* dt = &c->textures.a[f->depth.tex];
		pgl_tex_mark_render_target(dt);
		size_t zb;
		GLint lv = f->depth.level;
		u8* surf = pgl_depth_tex_surf(dt, f->depth.textarget, lv, &zb);
		GLsizei dw = dt->levels[lv].w;
		GLsizei dh = dt->levels[lv].h;
		c->zbuf.buf = surf;
		c->zbuf.w = dw;
		c->zbuf.h = dh;
		c->zbuf.lastrow = surf + (size_t)(dh - 1) * (size_t)dw * zb;
		c->has_depth_buf = GL_TRUE;
		if (dt->is_depth && dt->datatype == GL_FLOAT)
			c->zbuf_float = GL_TRUE;
#  if defined(PGL_D24S8)
		if (!c->zbuf_float && f->stencil.tex == f->depth.tex && f->depth.tex) {
			c->stencil_buf.buf = surf;
			c->stencil_buf.w = dt->w;
			c->stencil_buf.h = dt->h;
			c->stencil_buf.lastrow = c->zbuf.lastrow;
			c->has_stencil_buf = GL_TRUE;
		}
#  endif
	} else if (f->depth.rb) {
		glRenderbuffer* rb = &c->renderbuffers.a[f->depth.rb];
		c->zbuf.buf = rb->data;
		c->zbuf.w = rb->w;
		c->zbuf.h = rb->h;
		c->zbuf.lastrow = rb->lastrow;
		c->has_depth_buf = GL_TRUE;
		if (rb->internalformat == GL_DEPTH_COMPONENT32F)
			c->zbuf_float = GL_TRUE;
#  if defined(PGL_D24S8)
		if (!c->zbuf_float && f->stencil.rb == f->depth.rb) {
			c->stencil_buf = c->zbuf;
			c->has_stencil_buf = GL_TRUE;
		}
#  endif
	}
#  if !defined(PGL_NO_STENCIL) && defined(PGL_D16)
	if (f->stencil.rb && f->stencil.rb != f->depth.rb) {
		glRenderbuffer* srb = &c->renderbuffers.a[f->stencil.rb];
		c->stencil_buf.buf = srb->data;
		c->stencil_buf.w = srb->w;
		c->stencil_buf.h = srb->h;
		c->stencil_buf.lastrow = srb->lastrow;
		c->has_stencil_buf = GL_TRUE;
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

// Renderbuffers, framebuffer renderbuffer attach, read buffer/pixels

static void pgl_init_rb(glRenderbuffer* rb)
{
	memset(rb, 0, sizeof(*rb));
	rb->deleted = GL_FALSE;
	rb->user_owned = GL_FALSE;
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
	{
		GLint lv = f->color[att].level;
		int bpp = pgl_tex_bytes_per_pixel(t);
		s->buf = pgl_tex_level_face_surf(t, f->color[att].textarget, lv, (size_t)bpp);
		s->w = t->levels[lv].w;
		s->h = t->levels[lv].h;
	}
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
		GLint lv = f->depth.level;
		s->buf = pgl_depth_tex_surf(t, f->depth.textarget, lv, &zb);
		s->w = t->levels[lv].w;
		s->h = t->levels[lv].h;
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
	r = clamp_01(r);
	g = clamp_01(g);
	b = clamp_01(b);
	a = clamp_01(a);
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
	d = clamp_01(d);
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

static void pgl_fill_color_rt(pglColorRT* rt, float r, float g, float b, float a, int buf)
{
	const int nc = rt->components;
	if (rt->datatype == GL_FLOAT) {
#ifndef PGL_DISABLE_COLOR_MASK
		GLboolean* wm = c->color_writemask[buf];
		int all = wm[0] && (nc < 2 || wm[1]) && (nc < 3 || wm[2]) && (nc < 4 || wm[3]);
#endif
		if (!c->scissor_test) {
			int n = rt->w * rt->h;
			float* p = (float*)rt->buf;
			for (int i = 0; i < n; ++i) {
				float* t = p + i * nc;
#ifndef PGL_DISABLE_COLOR_MASK
				if (!all) {
					if (wm[0]) t[0] = r;
					if (nc > 1 && wm[1]) t[1] = g;
					if (nc > 2 && wm[2]) t[2] = b;
					if (nc > 3 && wm[3]) t[3] = a;
					continue;
				}
#endif
				t[0] = r;
				if (nc > 1) t[1] = g;
				if (nc > 2) t[2] = b;
				if (nc > 3) t[3] = a;
			}
		} else {
			for (int y = c->ly; y < c->uy; ++y) {
				for (int x = c->lx; x < c->ux; ++x) {
					float* t = (float*)rt->lastrow + (-y * rt->w + x) * nc;
#ifndef PGL_DISABLE_COLOR_MASK
					if (!all) {
						if (wm[0]) t[0] = r;
						if (nc > 1 && wm[1]) t[1] = g;
						if (nc > 2 && wm[2]) t[2] = b;
						if (nc > 3 && wm[3]) t[3] = a;
						continue;
					}
#endif
					t[0] = r;
					if (nc > 1) t[1] = g;
					if (nc > 2) t[2] = b;
					if (nc > 3) t[3] = a;
				}
			}
		}
	} else {
		Color col = VEC4_TO_COLOR(make_v4(clamp_01(r), clamp_01(g), clamp_01(b), clamp_01(a)));
		u32 src = *(u32*)&col;
#ifndef PGL_DISABLE_COLOR_MASK
		u32 m = c->color_mask_u8[buf];
#endif
		if (!c->scissor_test) {
			int n = rt->w * rt->h;
			u32* p = (u32*)rt->buf;
			for (int i = 0; i < n; ++i) {
#ifndef PGL_DISABLE_COLOR_MASK
				if (m != 0xFFFFFFFFu)
					p[i] = (p[i] & ~m) | (src & m);
				else
#endif
					p[i] = src;
			}
		} else {
			for (int y = c->ly; y < c->uy; ++y) {
				for (int x = c->lx; x < c->ux; ++x) {
					u32* p = (u32*)rt->lastrow + (-y * rt->w + x);
#ifndef PGL_DISABLE_COLOR_MASK
					if (m != 0xFFFFFFFFu)
						*p = (*p & ~m) | (src & m);
					else
#endif
						*p = src;
				}
			}
		}
	}
#ifdef PGL_DISABLE_COLOR_MASK
	PGL_UNUSED(buf);
#endif
}

static void pgl_fill_window_color(pix_t color)
{
#ifndef PGL_DISABLE_COLOR_MASK
	pix_t m = c->color_mask_pix[0];
	color &= m;
	pix_t clear_mask = ~m;
	pix_t tmp;
#endif
	int w = c->back_buffer.w;
	if (!c->scissor_test) {
		pix_t* buf = (pix_t*)c->back_buffer.buf;
		int n = c->back_buffer.w * c->back_buffer.h;
		for (int i = 0; i < n; ++i) {
#ifdef PGL_DISABLE_COLOR_MASK
			buf[i] = color;
#else
			tmp = buf[i];
			tmp &= clear_mask;
			buf[i] = tmp | color;
#endif
		}
	} else {
		for (int y = c->ly; y < c->uy; ++y) {
			for (int x = c->lx; x < c->ux; ++x) {
				int i = -y * w + x;
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
}

static void pgl_clear_drawbuffer_color(GLint drawbuffer, float r, float g, float b, float a, const char* api)
{
	PGL_UNUSED(api);
	PGL_ERR_NAMED(drawbuffer < 0 || drawbuffer >= GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE, api);
	if (drawbuffer >= c->num_draw_buffers)
		return;
	GLenum db = c->draw_buffers[drawbuffer];
	if (db == GL_NONE)
		return;
	if (c->fbo_color_is_rt) {
		int att = (int)(db - GL_COLOR_ATTACHMENT0);
		if (att < 0 || att >= GL_MAX_COLOR_ATTACHMENTS || !c->mrt_color[att].buf)
			return;
		pgl_fill_color_rt(&c->mrt_color[att], r, g, b, a, drawbuffer);
	} else {
		pgl_fill_window_color(RGBA_TO_PIXEL(clamp_01(r) * PGL_RMAX, clamp_01(g) * PGL_GMAX,
		                                   clamp_01(b) * PGL_BMAX, clamp_01(a) * PGL_AMAX));
	}
}

#ifndef PGL_NO_DEPTH_NO_STENCIL
static void pgl_clear_draw_depth(float d)
{
	if (!c->depth_mask || !c->has_depth_buf)
		return;
	d = clamp_01(d);
	if (c->zbuf_float) {
		if (!c->scissor_test) {
			float* z = (float*)c->zbuf.buf;
			int n = c->zbuf.w * c->zbuf.h;
			for (int i = 0; i < n; ++i)
				z[i] = d;
		} else {
			int w = c->zbuf.w;
			for (int y = c->ly; y < c->uy; ++y)
				for (int x = c->lx; x < c->ux; ++x)
					((float*)c->zbuf.lastrow)[-y * w + x] = d;
		}
		return;
	}
	int sz = c->ux * c->uy;
	u32 cd = (u32)(d * PGL_MAX_Z) << PGL_ZSHIFT;
	if (!c->scissor_test) {
		for (int i = 0; i < sz; ++i) {
			SET_Z_PRESHIFTED_TOP(i, cd);
		}
	} else {
		int w = c->back_buffer.w;
		for (int y = c->ly; y < c->uy; ++y) {
			for (int x = c->lx; x < c->ux; ++x) {
				SET_Z_PRESHIFTED(-y * w + x, cd);
			}
		}
	}
}

#  ifndef PGL_NO_STENCIL
static void pgl_clear_draw_stencil(GLint s)
{
	if (!c->has_stencil_buf)
		return;
	u8 cs = (u8)(s & PGL_STENCIL_MASK);
	int sz = c->ux * c->uy;
	if (!c->scissor_test) {
#    ifdef PGL_D16
		memset(c->stencil_buf.buf, cs, (size_t)(c->stencil_buf.w * c->stencil_buf.h));
#    else
		for (int i = 0; i < sz; ++i) {
			SET_STENCIL_TOP(i, cs);
		}
#    endif
	} else {
		int w = c->back_buffer.w;
		for (int y = c->ly; y < c->uy; ++y) {
			for (int x = c->lx; x < c->ux; ++x) {
				SET_STENCIL(-y * w + x, cs);
			}
		}
	}
}
#  endif
#endif

static void pgl_clear_buffer_fv(GLenum buffer, GLint drawbuffer, const GLfloat* value, const char* api)
{
	PGL_UNUSED(api);
	PGL_ERR_NAMED(buffer != GL_COLOR && buffer != GL_DEPTH, GL_INVALID_ENUM, api);
	PGL_ERR_NAMED(!value, GL_INVALID_VALUE, api);
	if (buffer == GL_COLOR) {
		pgl_clear_drawbuffer_color(drawbuffer, value[0], value[1], value[2], value[3], api);
		return;
	}
	PGL_ERR_NAMED(drawbuffer != 0, GL_INVALID_VALUE, api);
#ifndef PGL_NO_DEPTH_NO_STENCIL
	pgl_clear_draw_depth(value[0]);
#endif
}

static void pgl_clear_buffer_iv(GLenum buffer, GLint drawbuffer, const GLint* value, const char* api)
{
	PGL_UNUSED(api);
	PGL_ERR_NAMED(buffer != GL_COLOR && buffer != GL_STENCIL, GL_INVALID_ENUM, api);
	PGL_ERR_NAMED(!value, GL_INVALID_VALUE, api);
	if (buffer == GL_COLOR) {
		PGL_ERR_NAMED(GL_TRUE, GL_INVALID_OPERATION, api); // no integer color formats
		return;
	}
	PGL_ERR_NAMED(drawbuffer != 0, GL_INVALID_VALUE, api);
#if !defined(PGL_NO_DEPTH_NO_STENCIL) && !defined(PGL_NO_STENCIL)
	pgl_clear_draw_stencil(value[0]);
#else
	PGL_UNUSED(drawbuffer);
#endif
}

static void pgl_clear_buffer_uiv(GLenum buffer, GLint drawbuffer, const GLuint* value, const char* api)
{
	PGL_UNUSED(api);
	PGL_UNUSED(drawbuffer);
	PGL_UNUSED(value);
	PGL_ERR_NAMED(buffer != GL_COLOR, GL_INVALID_ENUM, api);
	PGL_ERR_NAMED(!value, GL_INVALID_VALUE, api);
	PGL_ERR_NAMED(drawbuffer < 0 || drawbuffer >= GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE, api);
	PGL_ERR_NAMED(GL_TRUE, GL_INVALID_OPERATION, api); // no unsigned-integer color formats
}

static void pgl_clear_buffer_fi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil, const char* api)
{
	PGL_UNUSED(api);
	PGL_ERR_NAMED(buffer != GL_DEPTH_STENCIL, GL_INVALID_ENUM, api);
	PGL_ERR_NAMED(drawbuffer != 0, GL_INVALID_VALUE, api);
#ifndef PGL_NO_DEPTH_NO_STENCIL
	pgl_clear_draw_depth(depth);
#  ifndef PGL_NO_STENCIL
	pgl_clear_draw_stencil(stencil);
#  else
	PGL_UNUSED(stencil);
#  endif
#else
	PGL_UNUSED(depth);
	PGL_UNUSED(stencil);
#endif
}

static GLboolean pgl_named_clear_setup(GLuint framebuffer, GLuint* old, const char* api)
{
	PGL_UNUSED(api);
	if (framebuffer) {
		PGL_ERR_RET_VAL_NAMED(framebuffer >= c->framebuffers.size || c->framebuffers.a[framebuffer].deleted,
		                      GL_INVALID_OPERATION, GL_FALSE, api);
	}
	PGL_ERR_RET_VAL_NAMED(!pgl_fbo_id_complete(framebuffer), GL_INVALID_FRAMEBUFFER_OPERATION, GL_FALSE, api);
	*old = c->bound_draw_framebuffer;
	if (*old != framebuffer) {
		c->bound_draw_framebuffer = framebuffer;
		pgl_apply_draw_framebuffer();
	}
	return GL_TRUE;
}
