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
	tex->data_alloc = 0;
	tex->num_levels = 0;
	memset(tex->levels, 0, sizeof(tex->levels));
	tex->deleted = GL_FALSE;
	tex->user_owned = GL_TRUE;
	tex->datatype = GL_UNSIGNED_BYTE;
	tex->format = GL_RGBA;
	tex->components = 4;
	tex->is_depth = GL_FALSE;
	tex->is_srgb = GL_FALSE;
	tex->invert_y = GL_FALSE;
	tex->lastrow = NULL;
	tex->w = 0;
	tex->h = 0;
	tex->d = 0;

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	tex->border_color = make_v4(0,0,0,0);
#endif
}

// Dimension of mip level `level` given base size (at least 1)
static GLsizei pgl_mip_dim(GLsizei base, GLint level)
{
	GLsizei d = base >> level;
	return d > 0 ? d : 1;
}

static int pgl_tex_bytes_per_pixel(const glTexture* tex);

static size_t pgl_bytes_2d(GLsizei w, GLsizei h, int bpp)
{
	return (size_t)w * (size_t)h * (size_t)bpp;
}

static size_t pgl_bytes_1d(GLsizei w, int bpp)
{
	return (size_t)w * (size_t)bpp;
}

static size_t pgl_bytes_cube_level(GLsizei face_w, GLsizei face_h, int bpp)
{
	return pgl_bytes_2d(face_w, face_h, bpp) * 6u;
}

static size_t pgl_rgba_bytes_2d(GLsizei w, GLsizei h)
{
	return pgl_bytes_2d(w, h, 4);
}

static size_t pgl_rgba_bytes_1d(GLsizei w)
{
	return pgl_bytes_1d(w, 4);
}

static size_t pgl_chain_bytes_2d(const glTexture* tex, int nlevels)
{
	int bpp = pgl_tex_bytes_per_pixel(tex);
	size_t total = 0;
	for (int i = 0; i < nlevels; ++i)
		total += pgl_bytes_2d(pgl_mip_dim(tex->w, i), pgl_mip_dim(tex->h, i), bpp);
	return total;
}

static size_t pgl_chain_bytes_1d(const glTexture* tex, int nlevels)
{
	int bpp = pgl_tex_bytes_per_pixel(tex);
	size_t total = 0;
	for (int i = 0; i < nlevels; ++i)
		total += pgl_bytes_1d(pgl_mip_dim(tex->w, i), bpp);
	return total;
}

static size_t pgl_chain_bytes_cube(const glTexture* tex, int nlevels)
{
	int bpp = pgl_tex_bytes_per_pixel(tex);
	size_t total = 0;
	for (int i = 0; i < nlevels; ++i)
		total += pgl_bytes_cube_level(pgl_mip_dim(tex->w, i), pgl_mip_dim(tex->h, i), bpp);
	return total;
}

// Forward decl: used after realloc/bind so RT lastrow tracks tex->data
static void pgl_tex_refresh_lastrow(glTexture* tex);

// Point levels[0..nlevels) into the packed tex->data block (2D)
static void pgl_bind_level_ptrs_2d(glTexture* tex, int nlevels)
{
	int bpp = pgl_tex_bytes_per_pixel(tex);
	u8* p = tex->data;
	for (int i = 0; i < nlevels; ++i) {
		GLsizei lw = pgl_mip_dim(tex->w, i);
		GLsizei lh = pgl_mip_dim(tex->h, i);
		tex->levels[i].w = lw;
		tex->levels[i].h = lh;
		tex->levels[i].data = p;
		p += pgl_bytes_2d(lw, lh, bpp);
	}
	for (int i = nlevels; i < PGL_MAX_MIPMAP_LEVELS; ++i) {
		tex->levels[i].w = 0;
		tex->levels[i].h = 0;
		tex->levels[i].data = NULL;
	}
	tex->num_levels = nlevels;
	pgl_tex_refresh_lastrow(tex);
}

static void pgl_bind_level_ptrs_1d(glTexture* tex, int nlevels)
{
	int bpp = pgl_tex_bytes_per_pixel(tex);
	u8* p = tex->data;
	for (int i = 0; i < nlevels; ++i) {
		GLsizei lw = pgl_mip_dim(tex->w, i);
		tex->levels[i].w = lw;
		tex->levels[i].h = 1;
		tex->levels[i].data = p;
		p += pgl_bytes_1d(lw, bpp);
	}
	for (int i = nlevels; i < PGL_MAX_MIPMAP_LEVELS; ++i) {
		tex->levels[i].w = 0;
		tex->levels[i].h = 0;
		tex->levels[i].data = NULL;
	}
	tex->num_levels = nlevels;
}

// Cubemap: each level is [face0][face1]...[face5] at that face size
static void pgl_bind_level_ptrs_cube(glTexture* tex, int nlevels)
{
	int bpp = pgl_tex_bytes_per_pixel(tex);
	u8* p = tex->data;
	for (int i = 0; i < nlevels; ++i) {
		GLsizei lw = pgl_mip_dim(tex->w, i);
		GLsizei lh = pgl_mip_dim(tex->h, i);
		tex->levels[i].w = lw;
		tex->levels[i].h = lh;
		tex->levels[i].data = p;
		p += pgl_bytes_cube_level(lw, lh, bpp);
	}
	for (int i = nlevels; i < PGL_MAX_MIPMAP_LEVELS; ++i) {
		tex->levels[i].w = 0;
		tex->levels[i].h = 0;
		tex->levels[i].data = NULL;
	}
	tex->num_levels = nlevels;
}

// Bytes per texel for tightly packed L0 (color or depth).
static int pgl_tex_bytes_per_pixel(const glTexture* tex)
{
	if (tex->is_depth) {
		if (tex->datatype == GL_FLOAT)
			return (int)sizeof(float);
#ifdef PGL_D16
		return (int)sizeof(u16);
#else
		return (int)sizeof(u32); // D24S8-style pack or depth24/32
#endif
	}
	PGL_ASSERT(tex->components > 0);
	const int nc = tex->components;
	if (tex->datatype == GL_FLOAT)
		return nc * (int)sizeof(float); // R32F / RG32F / RGBA32F
	return nc; // U8 channels (RGBA8 etc.)
}

static GLint pgl_format_components(GLenum format)
{
	if (format == GL_RED || format == GL_DEPTH_COMPONENT ||
	    format == GL_DEPTH_COMPONENT16 || format == GL_DEPTH_COMPONENT24 ||
	    format == GL_DEPTH_COMPONENT32 || format == GL_DEPTH_COMPONENT32F)
		return 1;
	if (format == GL_RG)
		return 2;
	if (format == GL_RGB || format == GL_BGR)
		return 3; // not fully supported for float RT
	return 4; // RGBA / BGRA / RGBA16F / RGBA32F
}

static void pgl_tex_set_format(glTexture* tex, GLenum format, GLenum datatype)
{
	if (format == GL_RGBA16F || format == GL_RGBA32F)
		format = GL_RGBA;
	tex->format = format;
	tex->datatype = datatype;
	tex->is_depth = (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_COMPONENT16 ||
	                 format == GL_DEPTH_COMPONENT24 || format == GL_DEPTH_COMPONENT32 ||
	                 format == GL_DEPTH_COMPONENT32F) ? GL_TRUE : GL_FALSE;
	tex->components = pgl_format_components(format);
	if (tex->is_depth)
		tex->components = 1;
	tex->is_srgb = GL_FALSE;
}

static GLboolean pgl_internalformat_is_srgb(GLint ifmt)
{
	return ifmt == (GLint)GL_SRGB || ifmt == (GLint)GL_SRGB8 ||
	       ifmt == (GLint)GL_SRGB_ALPHA || ifmt == (GLint)GL_SRGB8_ALPHA8;
}

static GLboolean pgl_format_is_depth(GLenum format)
{
	return format == GL_DEPTH_COMPONENT || format == GL_DEPTH_COMPONENT16 ||
	       format == GL_DEPTH_COMPONENT24 || format == GL_DEPTH_COMPONENT32 ||
	       format == GL_DEPTH_COMPONENT32F;
}

static float pgl_srgb_decode_u8[256];

static void pgl_build_srgb_lut(void)
{
	for (int i = 0; i < 256; ++i) {
		float cs = (float)i / 255.f;
		pgl_srgb_decode_u8[i] = (cs <= 0.04045f) ? cs / 12.92f
			: powf((cs + 0.055f) / 1.055f, 2.4f);
	}
}

static u8 pgl_linear_to_srgb_u8(float cl)
{
	if (cl <= 0.f) return 0;
	if (cl >= 1.f) return 255;
	float cs = (cl <= 0.0031308f) ? 12.92f * cl
		: 1.055f * powf(cl, 1.f / 2.4f) - 0.055f;
	int v = (int)(cs * 255.f + 0.5f);
	if (v < 0) v = 0;
	if (v > 255) v = 255;
	return (u8)v;
}

// Recompute tex->lastrow from L0 data/w/h/datatype when invert_y; else NULL.
static void pgl_tex_refresh_lastrow(glTexture* tex)
{
	if (!tex->invert_y || !tex->data || tex->w <= 0 || tex->h <= 0) {
		tex->lastrow = NULL;
		return;
	}
	tex->lastrow = tex->data +
	               (size_t)(tex->h - 1) * (size_t)tex->w * (size_t)pgl_tex_bytes_per_pixel(tex);
}

// Mark texture as a render target: sample with lastrow indexing (fragCoord y=0 = bottom).
static void pgl_tex_mark_render_target(glTexture* tex)
{
	tex->invert_y = GL_TRUE;
	pgl_tex_refresh_lastrow(tex);
}

// levels[0] only; clear higher descriptors (does not free memory)
static void pgl_set_level0_desc(glTexture* tex)
{
	tex->levels[0].w = tex->w;
	tex->levels[0].h = tex->h;
	tex->levels[0].data = tex->data;
	for (int i = 1; i < PGL_MAX_MIPMAP_LEVELS; ++i) {
		tex->levels[i].w = 0;
		tex->levels[i].h = 0;
		tex->levels[i].data = NULL;
	}
	// Keep lastrow in sync if this is already an RT (remap/resize).
	pgl_tex_refresh_lastrow(tex);
}

// Free the one image allocation (all levels).  Honors user_owned.
static void pgl_free_texture_images(glTexture* tex)
{
	if (!tex->user_owned) {
		PGL_FREE(tex->data);
	}
	tex->data = NULL;
	tex->data_alloc = 0;
	tex->w = 0;
	tex->h = 0;
	tex->d = 0;
	tex->num_levels = 0;
	tex->user_owned = GL_FALSE;
	tex->invert_y = GL_FALSE;
	tex->lastrow = NULL;
	memset(tex->levels, 0, sizeof(tex->levels));
}

// Ensure a packed 2D chain of nlevels fits in one block; preserves existing prefix.
// PGL-owned storage is grown with realloc; user-owned L0 is copied into a new block.
// Returns 0 on OOM.
static int pgl_alloc_mip_chain_2d(glTexture* tex, int nlevels)
{
	if (nlevels < 1 || nlevels > PGL_MAX_MIPMAP_LEVELS)
		return 0;

	size_t need = pgl_chain_bytes_2d(tex, nlevels);

	// Already large enough (may just need more level descriptors bound)
	if (!tex->user_owned && tex->data && tex->data_alloc >= need) {
		pgl_bind_level_ptrs_2d(tex, nlevels);
		return 1;
	}

	if (!tex->user_owned && tex->data) {
		// Grow our own block in place when the allocator allows
		size_t old = tex->data_alloc;
		u8* neu = (u8*)PGL_REALLOC(tex->data, need);
		if (!neu)
			return 0;
		if (need > old)
			memset(neu + old, 0, need - old);
		tex->data = neu;
		tex->data_alloc = need;
		pgl_bind_level_ptrs_2d(tex, nlevels);
		return 1;
	}

	// user_owned (or empty): allocate a PGL-owned block; copy L0 if present
	u8* neu = (u8*)PGL_MALLOC(need);
	if (!neu)
		return 0;

	if (tex->data) {
		size_t keep = pgl_bytes_2d(tex->w, tex->h, pgl_tex_bytes_per_pixel(tex));
		if (keep > need) keep = need;
		memcpy(neu, tex->data, keep);
		if (need > keep)
			memset(neu + keep, 0, need - keep);
		// leave user memory alone
	} else {
		memset(neu, 0, need);
	}

	tex->data = neu;
	tex->data_alloc = need;
	tex->user_owned = GL_FALSE;
	pgl_bind_level_ptrs_2d(tex, nlevels);
	return 1;
}

static int pgl_alloc_mip_chain_1d(glTexture* tex, int nlevels)
{
	if (nlevels < 1 || nlevels > PGL_MAX_MIPMAP_LEVELS)
		return 0;

	size_t need = pgl_chain_bytes_1d(tex, nlevels);

	if (!tex->user_owned && tex->data && tex->data_alloc >= need) {
		pgl_bind_level_ptrs_1d(tex, nlevels);
		return 1;
	}

	if (!tex->user_owned && tex->data) {
		size_t old = tex->data_alloc;
		u8* neu = (u8*)PGL_REALLOC(tex->data, need);
		if (!neu)
			return 0;
		if (need > old)
			memset(neu + old, 0, need - old);
		tex->data = neu;
		tex->data_alloc = need;
		pgl_bind_level_ptrs_1d(tex, nlevels);
		return 1;
	}

	u8* neu = (u8*)PGL_MALLOC(need);
	if (!neu)
		return 0;

	if (tex->data) {
		size_t keep = pgl_bytes_1d(tex->w, pgl_tex_bytes_per_pixel(tex));
		if (keep > need) keep = need;
		memcpy(neu, tex->data, keep);
		if (need > keep)
			memset(neu + keep, 0, need - keep);
	} else {
		memset(neu, 0, need);
	}

	tex->data = neu;
	tex->data_alloc = need;
	tex->user_owned = GL_FALSE;
	pgl_bind_level_ptrs_1d(tex, nlevels);
	return 1;
}

// Packed cubemap chain: L0 is 6 faces (~same layout as today), then L1..Ln-1 each 6 faces.
// Preserves the full L0 pack (6 * face_w * face_h * 4), not a single face.
static int pgl_alloc_mip_chain_cube(glTexture* tex, int nlevels)
{
	if (nlevels < 1 || nlevels > PGL_MAX_MIPMAP_LEVELS)
		return 0;

	size_t need = pgl_chain_bytes_cube(tex, nlevels);

	if (!tex->user_owned && tex->data && tex->data_alloc >= need) {
		pgl_bind_level_ptrs_cube(tex, nlevels);
		return 1;
	}

	if (!tex->user_owned && tex->data) {
		size_t old = tex->data_alloc;
		u8* neu = (u8*)PGL_REALLOC(tex->data, need);
		if (!neu)
			return 0;
		if (need > old)
			memset(neu + old, 0, need - old);
		tex->data = neu;
		tex->data_alloc = need;
		pgl_bind_level_ptrs_cube(tex, nlevels);
		return 1;
	}

	u8* neu = (u8*)PGL_MALLOC(need);
	if (!neu)
		return 0;

	if (tex->data) {
		size_t keep = pgl_bytes_cube_level(tex->w, tex->h, pgl_tex_bytes_per_pixel(tex));
		if (keep > need) keep = need;
		memcpy(neu, tex->data, keep);
		if (need > keep)
			memset(neu + keep, 0, need - keep);
	} else {
		memset(neu, 0, need);
	}

	tex->data = neu;
	tex->data_alloc = need;
	tex->user_owned = GL_FALSE;
	pgl_bind_level_ptrs_cube(tex, nlevels);
	return 1;
}

// May be NULL if incomplete/empty.
static u8* pgl_tex_level_data(const glTexture* tex, GLint level)
{
	PGL_ASSERT(tex && tex->data && tex->num_levels > 0);
	PGL_ASSERT(level >= 0 && level < tex->num_levels);
	return tex->levels[level].data;
}

static void pgl_tex_level_dims(const glTexture* tex, GLint level, GLsizei* w, GLsizei* h, GLsizei* d)
{
	PGL_ASSERT(tex && tex->num_levels > 0);
	PGL_ASSERT(level >= 0 && level < tex->num_levels);

	if (w) *w = tex->levels[level].w;
	if (h) *h = tex->levels[level].h;
	if (d) *d = (level == 0) ? tex->d : 1;
}

// Box-filter one 2D RGBA8 level into the next (handles NPOT edges)
static void pgl_box_filter_2d(const u8* src, GLsizei sw, GLsizei sh, u8* dst, GLsizei dw, GLsizei dh, GLboolean srgb)
{
	for (GLsizei y = 0; y < dh; ++y) {
		GLsizei y0 = y * 2;
		GLsizei y1 = (y0 + 1 < sh) ? y0 + 1 : y0;
		for (GLsizei x = 0; x < dw; ++x) {
			GLsizei x0 = x * 2;
			GLsizei x1 = (x0 + 1 < sw) ? x0 + 1 : x0;

			int count = 0;
			u8* out = dst + ((size_t)y * (size_t)dw + (size_t)x) * 4;
			if (srgb) {
				float sumr = 0.f, sumg = 0.f, sumb = 0.f;
				unsigned suma = 0;
				for (GLsizei j = y0; j <= y1; ++j) {
					for (GLsizei i = x0; i <= x1; ++i) {
						const u8* p = src + ((size_t)j * (size_t)sw + (size_t)i) * 4;
						sumr += pgl_srgb_decode_u8[p[0]];
						sumg += pgl_srgb_decode_u8[p[1]];
						sumb += pgl_srgb_decode_u8[p[2]];
						suma += p[3];
						++count;
					}
				}
				float inv = 1.f / (float)count;
				out[0] = pgl_linear_to_srgb_u8(sumr * inv);
				out[1] = pgl_linear_to_srgb_u8(sumg * inv);
				out[2] = pgl_linear_to_srgb_u8(sumb * inv);
				out[3] = (u8)(suma / count);
			} else {
				unsigned sum[4] = {0, 0, 0, 0};
				for (GLsizei j = y0; j <= y1; ++j) {
					for (GLsizei i = x0; i <= x1; ++i) {
						const u8* p = src + ((size_t)j * (size_t)sw + (size_t)i) * 4;
						sum[0] += p[0];
						sum[1] += p[1];
						sum[2] += p[2];
						sum[3] += p[3];
						++count;
					}
				}
				out[0] = (u8)(sum[0] / count);
				out[1] = (u8)(sum[1] / count);
				out[2] = (u8)(sum[2] / count);
				out[3] = (u8)(sum[3] / count);
			}
		}
	}
}

// Same for 1D (average 2 texels, or 1 at the end)
static void pgl_box_filter_1d(const u8* src, GLsizei sw, u8* dst, GLsizei dw, GLboolean srgb)
{
	for (GLsizei x = 0; x < dw; ++x) {
		GLsizei x0 = x * 2;
		GLsizei x1 = (x0 + 1 < sw) ? x0 + 1 : x0;
		int count = 0;
		u8* out = dst + (size_t)x * 4;
		if (srgb) {
			float sumr = 0.f, sumg = 0.f, sumb = 0.f;
			unsigned suma = 0;
			for (GLsizei i = x0; i <= x1; ++i) {
				const u8* p = src + (size_t)i * 4;
				sumr += pgl_srgb_decode_u8[p[0]];
				sumg += pgl_srgb_decode_u8[p[1]];
				sumb += pgl_srgb_decode_u8[p[2]];
				suma += p[3];
				++count;
			}
			float inv = 1.f / (float)count;
			out[0] = pgl_linear_to_srgb_u8(sumr * inv);
			out[1] = pgl_linear_to_srgb_u8(sumg * inv);
			out[2] = pgl_linear_to_srgb_u8(sumb * inv);
			out[3] = (u8)(suma / count);
		} else {
			unsigned sum[4] = {0, 0, 0, 0};
			for (GLsizei i = x0; i <= x1; ++i) {
				const u8* p = src + (size_t)i * 4;
				sum[0] += p[0];
				sum[1] += p[1];
				sum[2] += p[2];
				sum[3] += p[3];
				++count;
			}
			out[0] = (u8)(sum[0] / count);
			out[1] = (u8)(sum[1] / count);
			out[2] = (u8)(sum[2] / count);
			out[3] = (u8)(sum[3] / count);
		}
	}
}

static void pgl_box_filter_2d_float(const float* src, GLsizei sw, GLsizei sh,
                                    float* dst, GLsizei dw, GLsizei dh, int nc)
{
	PGL_ASSERT(nc > 0 && nc <= 4);
	for (GLsizei y = 0; y < dh; ++y) {
		GLsizei y0 = y * 2;
		GLsizei y1 = (y0 + 1 < sh) ? y0 + 1 : y0;
		for (GLsizei x = 0; x < dw; ++x) {
			GLsizei x0 = x * 2;
			GLsizei x1 = (x0 + 1 < sw) ? x0 + 1 : x0;
			float sum[4] = {0, 0, 0, 0};
			int count = 0;
			for (GLsizei j = y0; j <= y1; ++j) {
				for (GLsizei i = x0; i <= x1; ++i) {
					const float* p = src + ((size_t)j * (size_t)sw + (size_t)i) * (size_t)nc;
					for (int k = 0; k < nc; ++k)
						sum[k] += p[k];
					++count;
				}
			}
			float* out = dst + ((size_t)y * (size_t)dw + (size_t)x) * (size_t)nc;
			float inv = 1.f / (float)count;
			for (int k = 0; k < nc; ++k)
				out[k] = sum[k] * inv;
		}
	}
}

static void pgl_box_filter_1d_float(const float* src, GLsizei sw, float* dst, GLsizei dw, int nc)
{
	PGL_ASSERT(nc > 0 && nc <= 4);
	for (GLsizei x = 0; x < dw; ++x) {
		GLsizei x0 = x * 2;
		GLsizei x1 = (x0 + 1 < sw) ? x0 + 1 : x0;
		float sum[4] = {0, 0, 0, 0};
		int count = 0;
		for (GLsizei i = x0; i <= x1; ++i) {
			const float* p = src + (size_t)i * (size_t)nc;
			for (int k = 0; k < nc; ++k)
				sum[k] += p[k];
			++count;
		}
		float* out = dst + (size_t)x * (size_t)nc;
		float inv = 1.f / (float)count;
		for (int k = 0; k < nc; ++k)
			out[k] = sum[k] * inv;
	}
}

static void pgl_filter_level_2d(const glTexture* tex, const u8* src, GLsizei sw, GLsizei sh,
                               u8* dst, GLsizei dw, GLsizei dh)
{
	if (tex->datatype == GL_FLOAT)
		pgl_box_filter_2d_float((const float*)src, sw, sh, (float*)dst, dw, dh, tex->components);
	else
		pgl_box_filter_2d(src, sw, sh, dst, dw, dh, tex->is_srgb);
}

static void pgl_filter_level_1d(const glTexture* tex, const u8* src, GLsizei sw, u8* dst, GLsizei dw)
{
	if (tex->datatype == GL_FLOAT)
		pgl_box_filter_1d_float((const float*)src, sw, (float*)dst, dw, tex->components);
	else
		pgl_box_filter_1d(src, sw, dst, dw, tex->is_srgb);
}

