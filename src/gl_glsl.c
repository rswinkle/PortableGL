

/*************************************
 *  GLSL(ish) functions
 *************************************/

/*
float clampf_01(float f)
{
	if (f < 0.0f) return 0.0f;
	if (f > 1.0f) return 1.0f;
	return f;
}

float clampf(float f, float min, float max)
{
	if (f < min) return min;
	if (f > max) return max;
	return f;
}

int clampi(int i, int min, int max)
{
	if (i < min) return min;
	if (i > max) return max;
	return i;
}
*/


// TODO maybe I should put this in crsw_math/rsw_math? static inline?
// guarantees positive mod result
#define positive_mod(a, b) (((a) % (b) + (b)) % (b))

// if I only wanted to support power of 2 textures...
#define positive_mod_pow_of_2(i, n) ((i) & ((n) - 1) + (n)) & ((n) - 1)

// TODO should this be in rsw_math
#define mirror(i) (i) >= 0 ? (i) : -(1 + (i))

// See page 174 of GL 3.3 core spec.
static int wrap(int i, int size, GLenum mode)
{
	switch (mode)
	{
	case GL_REPEAT:
		return positive_mod(i, size);

	// Border is too much of a pain to implement with render to
	// texture.  Trade offs in poor performance or ugly extra code
	// for a feature that almost no one actually uses and even
	// when it is used (barring rare/odd uv coordinates) it's not
	// even noticable.
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	case GL_CLAMP_TO_BORDER:
		if (i >= 0 && i < size) return i;
		return -1;
		// Would use if we went back to literally surrounding textures with a border
		//return clampi(i, -1, size);
#else
	case GL_CLAMP_TO_BORDER:  // just so stuff that uses it compiles
#endif
	case GL_CLAMP_TO_EDGE:
		return clampi(i, 0, size-1);

	case GL_MIRRORED_REPEAT: {
		int sz2 = 2*size;
		i = positive_mod(i, sz2);
		i -= size;
		i = mirror(i);
		i = size - 1 - i;
		return i;
	} break;
	default:
		//should never happen, get rid of compile warning
		assert(0);
		return 0;
	}
}
#undef imod
#undef positive_mod
#undef positive_mod_pow_of_2


// hmm should I have these take a glTexture* somehow?
// It would save the check for 0 for every single access

// used in the following texture access functions
// Not sure if it's actually necessary since wrap() clamps
#define EPSILON 0.000001

// Texture filter arithmetic: float by default (soft-float / no-double platforms).
// Define PGL_DOUBLE_TEX_FILTER before including PGL to use double for UV scaling,
// lerp weights, and the color mix — fewer off-by-one results after the truncating
// [0,1]<->[0,255] conversion (see commit f66741f5).
#ifdef PGL_DOUBLE_TEX_FILTER
typedef double pgl_texf;
#define pgl_tex_floor(x) floor(x)
#define pgl_tex_modf(x, ip) modf((x), (ip))
#else
typedef float pgl_texf;
#define pgl_tex_floor(x) floorf(x)
#define pgl_tex_modf(x, ip) modff((x), (ip))
#endif

// Map MIN_FILTER to within-level NEAREST vs LINEAR
static GLenum pgl_within_level_filter(GLenum min_filter)
{
	switch (min_filter) {
	case GL_NEAREST:
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
		return GL_NEAREST;
	default:
		return GL_LINEAR;
	}
}

// True when min filter blends between two mip levels (trilinear / "mip linear")
static int pgl_is_mip_linear_filter(GLenum min_filter)
{
	return min_filter == GL_NEAREST_MIPMAP_LINEAR ||
	       min_filter == GL_LINEAR_MIPMAP_LINEAR;
}

// Explicit λ → single integer level for *MIPMAP_NEAREST (round).
// For *MIPMAP_LINEAR the lower level is floor(lod); caller blends with floor+1.
static int pgl_lod_to_level(const glTexture* t, float lod)
{
	if (!t || t->num_levels <= 1)
		return 0;

	int max_level = t->num_levels - 1;
	int level;
	if (pgl_is_mip_linear_filter(t->min_filter))
		level = (int)floorf(lod);
	else
		level = (int)floorf(lod + 0.5f);

	if (level < 0) level = 0;
	if (level > max_level) level = max_level;
	return level;
}

static int pgl_is_mip_min_filter(GLenum min_filter)
{
	return min_filter == GL_NEAREST_MIPMAP_NEAREST ||
	       min_filter == GL_NEAREST_MIPMAP_LINEAR ||
	       min_filter == GL_LINEAR_MIPMAP_NEAREST ||
	       min_filter == GL_LINEAR_MIPMAP_LINEAR;
}

// Incomplete for mip sampling: MIN_FILTER is a *MIPMAP* mode but no chain
// (num_levels <= 1).  Under PGL_CORE_PROFILE → black; otherwise L0 fallback.
static int pgl_incomplete_mip_returns_black(const glTexture* t)
{
#ifdef PGL_CORE_PROFILE
	return pgl_is_mip_min_filter(t->min_filter) && t->num_levels <= 1;
#else
	PGL_UNUSED(t);
	return 0;
#endif
}

static vec4 pgl_lerp_v4(vec4 a, vec4 b, float t)
{
	// a*(1-t) + b*t
	a = scale_v4(a, 1.0f - t);
	b = scale_v4(b, t);
	return add_v4s(a, b);
}

// Phase 2B: λ from per-triangle UV/pixel scale and base-level size.
// ρ ≈ mip_uv_per_px * max(w,h); λ = log2(ρ).  λ<=0 => magnification.
static float pgl_auto_lod(const glTexture* t, GLsizei dim0, GLsizei dim1)
{
	float dim = (float)((dim0 > dim1) ? dim0 : dim1);
	if (dim < 1.0f)
		dim = 1.0f;
	float rho = c->mip_uv_per_px * dim;
	// Avoid -inf; tiny ρ => strong magnification (negative λ)
	if (rho < 1e-10f)
		return -16.0f;
	return log2f(rho);
}

// Sample one 1D level with NEAREST or LINEAR (filter != NEAREST => LINEAR)
static vec4 pgl_sample_1d_level(const glTexture* t, const u8* data, int w, float x, GLenum filter)
{
	int i0, i1;
	Color* texdata = (Color*)data;
	pgl_texf ww = w - EPSILON;
	pgl_texf xw = (pgl_texf)x * ww;

	if (filter == GL_NEAREST) {
		i0 = wrap((int)pgl_tex_floor(xw), w, t->wrap_s);
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if (i0 < 0) return t->border_color;
#endif
		return Color_to_v4(texdata[i0]);
	}

	// LINEAR
	// This seems right to me since pixel centers are 0.5 but
	// this isn't exactly what's described in the spec or FoCG
	i0 = wrap((int)pgl_tex_floor(xw - (pgl_texf)0.5), w, t->wrap_s);
	i1 = wrap((int)pgl_tex_floor(xw + (pgl_texf)0.499999), w, t->wrap_s);

	pgl_texf tmp2;
	pgl_texf alpha = pgl_tex_modf(xw + (pgl_texf)0.5, &tmp2);
	if (alpha < 0) ++alpha;

#ifdef PGL_HERMITE_SMOOTHING
	alpha = alpha * alpha * (3 - 2 * alpha);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	vec4 ci, ci1;
	if (i0 < 0) ci = t->border_color;
	else ci = Color_to_v4(texdata[i0]);
	if (i1 < 0) ci1 = t->border_color;
	else ci1 = Color_to_v4(texdata[i1]);
#else
	vec4 ci = Color_to_v4(texdata[i0]);
	vec4 ci1 = Color_to_v4(texdata[i1]);
#endif

#ifdef PGL_DOUBLE_TEX_FILTER
	{
		vec4 r;
		pgl_texf w0 = 1 - alpha, w1 = alpha;
		r.x = (float)(ci.x * w0 + ci1.x * w1);
		r.y = (float)(ci.y * w0 + ci1.y * w1);
		r.z = (float)(ci.z * w0 + ci1.z * w1);
		r.w = (float)(ci.w * w0 + ci1.w * w1);
		return r;
	}
#else
	ci = scale_v4(ci, (float)(1 - alpha));
	ci1 = scale_v4(ci1, (float)alpha);
	return add_v4s(ci, ci1);
#endif
}

// Sample one 2D level with NEAREST or LINEAR
static vec4 pgl_sample_2d_level(const glTexture* t, const u8* data, int w, int h, float x, float y, GLenum filter)
{
	int i0, j0, i1, j1;
	Color* texdata = (Color*)data;
	pgl_texf dw = w - EPSILON;
	pgl_texf dh = h - EPSILON;
	pgl_texf xw = (pgl_texf)x * dw;
	pgl_texf yh = (pgl_texf)y * dh;

	if (filter == GL_NEAREST) {
		i0 = wrap((int)pgl_tex_floor(xw), w, t->wrap_s);
		j0 = wrap((int)pgl_tex_floor(yh), h, t->wrap_t);
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0) < 0) return t->border_color;
#endif
		return Color_to_v4(texdata[j0 * w + i0]);
	}

	// LINEAR
	// This seems right to me since pixel centers are 0.5 but
	// this isn't exactly what's described in the spec or FoCG
	i0 = wrap((int)pgl_tex_floor(xw - (pgl_texf)0.5), w, t->wrap_s);
	j0 = wrap((int)pgl_tex_floor(yh - (pgl_texf)0.5), h, t->wrap_t);
	i1 = wrap((int)pgl_tex_floor(xw + (pgl_texf)0.499999), w, t->wrap_s);
	j1 = wrap((int)pgl_tex_floor(yh + (pgl_texf)0.499999), h, t->wrap_t);

	pgl_texf tmp2;
	pgl_texf alpha = pgl_tex_modf(xw + (pgl_texf)0.5, &tmp2);
	pgl_texf beta = pgl_tex_modf(yh + (pgl_texf)0.5, &tmp2);
	if (alpha < 0) ++alpha;
	if (beta < 0) ++beta;

	//hermite smoothing is optional
	//looks like my nvidia implementation doesn't do it
	//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
	alpha = alpha * alpha * (3 - 2 * alpha);
	beta = beta * beta * (3 - 2 * beta);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	vec4 cij, ci1j, cij1, ci1j1;
	if ((i0 | j0) < 0) cij = t->border_color;
	else cij = Color_to_v4(texdata[j0 * w + i0]);
	if ((i1 | j0) < 0) ci1j = t->border_color;
	else ci1j = Color_to_v4(texdata[j0 * w + i1]);
	if ((i0 | j1) < 0) cij1 = t->border_color;
	else cij1 = Color_to_v4(texdata[j1 * w + i0]);
	if ((i1 | j1) < 0) ci1j1 = t->border_color;
	else ci1j1 = Color_to_v4(texdata[j1 * w + i1]);
#else
	vec4 cij = Color_to_v4(texdata[j0 * w + i0]);
	vec4 ci1j = Color_to_v4(texdata[j0 * w + i1]);
	vec4 cij1 = Color_to_v4(texdata[j1 * w + i0]);
	vec4 ci1j1 = Color_to_v4(texdata[j1 * w + i1]);
#endif

#ifdef PGL_DOUBLE_TEX_FILTER
	{
		vec4 r;
		pgl_texf w00 = (1 - alpha) * (1 - beta);
		pgl_texf w10 = alpha * (1 - beta);
		pgl_texf w01 = (1 - alpha) * beta;
		pgl_texf w11 = alpha * beta;
		r.x = (float)(cij.x * w00 + ci1j.x * w10 + cij1.x * w01 + ci1j1.x * w11);
		r.y = (float)(cij.y * w00 + ci1j.y * w10 + cij1.y * w01 + ci1j1.y * w11);
		r.z = (float)(cij.z * w00 + ci1j.z * w10 + cij1.z * w01 + ci1j1.z * w11);
		r.w = (float)(cij.w * w00 + ci1j.w * w10 + cij1.w * w01 + ci1j1.w * w11);
		return r;
	}
#else
	// float path: same style as pre-mipmap texture2D (f66741f5+)
	cij = scale_v4(cij, (float)((1 - alpha) * (1 - beta)));
	ci1j = scale_v4(ci1j, (float)(alpha * (1 - beta)));
	cij1 = scale_v4(cij1, (float)((1 - alpha) * beta));
	ci1j1 = scale_v4(ci1j1, (float)(alpha * beta));

	cij = add_v4s(cij, ci1j);
	cij = add_v4s(cij, cij1);
	cij = add_v4s(cij, ci1j1);
	return cij;
#endif
}

// Sample one mip level (by index) with within-level filter from min_filter
static vec4 pgl_sample_1d_level_idx(const glTexture* t, int level, float x)
{
	u8* data = pgl_tex_level_data(t, level);
	if (!data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);
	GLsizei w;
	pgl_tex_level_dims(t, level, &w, NULL, NULL);
	return pgl_sample_1d_level(t, data, w, x, pgl_within_level_filter(t->min_filter));
}

static vec4 pgl_sample_2d_level_idx(const glTexture* t, int level, float x, float y)
{
	u8* data = pgl_tex_level_data(t, level);
	if (!data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);
	GLsizei w, h;
	pgl_tex_level_dims(t, level, &w, &h, NULL);
	return pgl_sample_2d_level(t, data, w, h, x, y, pgl_within_level_filter(t->min_filter));
}

// Minify path: *MIPMAP_NEAREST → one level; *MIPMAP_LINEAR → two levels + lerp (trilinear).
// Only used when min filter is a mip mode, chain exists, and λ/lod > 0.
static vec4 pgl_sample_1d_minify(const glTexture* t, float x, float lod)
{
	int max_level = t->num_levels - 1;
	if (max_level <= 0)
		return pgl_sample_1d_level_idx(t, 0, x);

	// Non-trilinear: single rounded/floored level (same as before)
	if (!pgl_is_mip_linear_filter(t->min_filter))
		return pgl_sample_1d_level_idx(t, pgl_lod_to_level(t, lod), x);

	// Trilinear: blend floor(lod) and floor(lod)+1
	if (lod < 0.0f)
		lod = 0.0f;
	if (lod >= (float)max_level)
		return pgl_sample_1d_level_idx(t, max_level, x);

	int l0 = (int)floorf(lod);
	float frac = lod - (float)l0;
	if (frac <= 0.0f)
		return pgl_sample_1d_level_idx(t, l0, x);
	if (frac >= 1.0f)
		return pgl_sample_1d_level_idx(t, l0 + 1, x);

	vec4 c0 = pgl_sample_1d_level_idx(t, l0, x);
	vec4 c1 = pgl_sample_1d_level_idx(t, l0 + 1, x);
	return pgl_lerp_v4(c0, c1, frac);
}

static vec4 pgl_sample_2d_minify(const glTexture* t, float x, float y, float lod)
{
	int max_level = t->num_levels - 1;
	if (max_level <= 0)
		return pgl_sample_2d_level_idx(t, 0, x, y);

	if (!pgl_is_mip_linear_filter(t->min_filter))
		return pgl_sample_2d_level_idx(t, pgl_lod_to_level(t, lod), x, y);

	if (lod < 0.0f)
		lod = 0.0f;
	if (lod >= (float)max_level)
		return pgl_sample_2d_level_idx(t, max_level, x, y);

	int l0 = (int)floorf(lod);
	float frac = lod - (float)l0;
	if (frac <= 0.0f)
		return pgl_sample_2d_level_idx(t, l0, x, y);
	if (frac >= 1.0f)
		return pgl_sample_2d_level_idx(t, l0 + 1, x, y);

	vec4 c0 = pgl_sample_2d_level_idx(t, l0, x, y);
	vec4 c1 = pgl_sample_2d_level_idx(t, l0 + 1, x, y);
	return pgl_lerp_v4(c0, c1, frac);
}

PGLDEF vec4 texture1D(GLuint tex, float x)
{
	glTexture* t;
	if (tex)
		t = &c->textures.a[tex];
	else
		t = &c->default_textures[GL_TEXTURE_1D - GL_TEXTURE_1D];

	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	// Mip path only when a chain exists and MIN_FILTER is a *MIPMAP* mode
	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter)) {
		float lambda = pgl_auto_lod(t, t->w, 1);
		if (lambda <= 0.0f)
			return pgl_sample_1d_level(t, t->data, t->w, x, t->mag_filter);
		return pgl_sample_1d_minify(t, x, lambda);
	}

	// Incomplete mip filter: Core → black; compat → L0 + mag
	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_1d_level(t, t->data, t->w, x, t->mag_filter);
}

PGLDEF vec4 texture1DLod(GLuint tex, float x, float lod)
{
	glTexture* t;
	if (tex)
		t = &c->textures.a[tex];
	else
		t = &c->default_textures[GL_TEXTURE_1D - GL_TEXTURE_1D];

	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter)) {
		if (lod <= 0.0f && !pgl_is_mip_linear_filter(t->min_filter))
			return pgl_sample_1d_level_idx(t, 0, x);
		return pgl_sample_1d_minify(t, x, lod);
	}

	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	// Non-mip or incomplete-compat: within-level filter from min_filter
	return pgl_sample_1d_level(t, t->data, t->w, x, pgl_within_level_filter(t->min_filter));
}

PGLDEF vec4 texture2D(GLuint tex, float x, float y)
{
	glTexture* t;
	if (tex)
		t = &c->textures.a[tex];
	else
		t = &c->default_textures[GL_TEXTURE_2D - GL_TEXTURE_1D];

	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter)) {
		float lambda = pgl_auto_lod(t, t->w, t->h);
		if (lambda <= 0.0f)
			return pgl_sample_2d_level(t, t->data, t->w, t->h, x, y, t->mag_filter);
		return pgl_sample_2d_minify(t, x, y, lambda);
	}

	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_2d_level(t, t->data, t->w, t->h, x, y, t->mag_filter);
}

PGLDEF vec4 texture2DLod(GLuint tex, float x, float y, float lod)
{
	glTexture* t;
	if (tex)
		t = &c->textures.a[tex];
	else
		t = &c->default_textures[GL_TEXTURE_2D - GL_TEXTURE_1D];

	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter))
		return pgl_sample_2d_minify(t, x, y, lod);

	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_2d_level(t, t->data, t->w, t->h, x, y,
	                           pgl_within_level_filter(t->min_filter));
}

PGLDEF vec4 texture3D(GLuint tex, float x, float y, float z)
{
	int i0, j0, i1, j1, k0, k1;

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_3D-GL_TEXTURE_1D];
	}
	Color* texdata = (Color*)t->data;

	float dw = t->w - EPSILON;
	float dh = t->h - EPSILON;
	float dd = t->d - EPSILON;

	int w = t->w;
	int h = t->h;
	int d = t->d;
	int plane = w * t->h;
	float xw = x * dw;
	float yh = y * dh;
	float zd = z * dd;


	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floorf(xw), w, t->wrap_s);
		j0 = wrap(floorf(yh), h, t->wrap_t);
		k0 = wrap(floorf(zd), d, t->wrap_r);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0 | k0) < 0) return t->border_color;
#endif

		return Color_to_v4(texdata[k0*plane + j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floorf(xw - 0.5f), w, t->wrap_s);
		j0 = wrap(floorf(yh - 0.5f), h, t->wrap_t);
		k0 = wrap(floorf(zd - 0.5f), d, t->wrap_r);
		i1 = wrap(floorf(xw + 0.499999f), w, t->wrap_s);
		j1 = wrap(floorf(yh + 0.499999f), h, t->wrap_t);
		k1 = wrap(floorf(zd + 0.499999f), d, t->wrap_r);

		float tmp2;
		float alpha = modff(xw+0.5f, &tmp2);
		float beta = modff(yh+0.5f, &tmp2);
		float gamma = modff(zd+0.5f, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;
		if (gamma < 0) ++gamma;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
		gamma = gamma*gamma * (3 - 2*gamma);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		vec4 cijk, ci1jk, cij1k, ci1j1k, cijk1, ci1jk1, cij1k1, ci1j1k1;
		if ((i0 | j0 | k0) < 0) cijk = t->border_color;
		else cijk = Color_to_v4(texdata[k0*plane + j0*w + i0]);

		if ((i1 | j0 | k0) < 0) ci1jk = t->border_color;
		else ci1jk = Color_to_v4(texdata[k0*plane + j0*w + i1]);

		if ((i0 | j1 | k0) < 0) cij1k = t->border_color;
		else cij1k = Color_to_v4(texdata[k0*plane + j1*w + i0]);

		if ((i1 | j1 | k0) < 0) ci1j1k = t->border_color;
		else ci1j1k = Color_to_v4(texdata[k0*plane + j1*w + i1]);

		if ((i0 | j0 | k1) < 0) cijk1 = t->border_color;
		else cijk1 = Color_to_v4(texdata[k1*plane + j0*w + i0]);

		if ((i1 | j0 | k1) < 0) ci1jk1 = t->border_color;
		else ci1jk1 = Color_to_v4(texdata[k1*plane + j0*w + i1]);

		if ((i0 | j1 | k1) < 0) cij1k1 = t->border_color;
		else cij1k1 = Color_to_v4(texdata[k1*plane + j1*w + i0]);

		if ((i1 | j1 | k1) < 0) ci1j1k1 = t->border_color;
		else ci1j1k1 = Color_to_v4(texdata[k1*plane + j1*w + i1]);
#else
		vec4 cijk = Color_to_v4(texdata[k0*plane + j0*w + i0]);
		vec4 ci1jk = Color_to_v4(texdata[k0*plane + j0*w + i1]);
		vec4 cij1k = Color_to_v4(texdata[k0*plane + j1*w + i0]);
		vec4 ci1j1k = Color_to_v4(texdata[k0*plane + j1*w + i1]);
		vec4 cijk1 = Color_to_v4(texdata[k1*plane + j0*w + i0]);
		vec4 ci1jk1 = Color_to_v4(texdata[k1*plane + j0*w + i1]);
		vec4 cij1k1 = Color_to_v4(texdata[k1*plane + j1*w + i0]);
		vec4 ci1j1k1 = Color_to_v4(texdata[k1*plane + j1*w + i1]);
#endif

		cijk = scale_v4(cijk, (1-alpha)*(1-beta)*(1-gamma));
		ci1jk = scale_v4(ci1jk, alpha*(1-beta)*(1-gamma));
		cij1k = scale_v4(cij1k, (1-alpha)*beta*(1-gamma));
		ci1j1k = scale_v4(ci1j1k, alpha*beta*(1-gamma));
		cijk1 = scale_v4(cijk1, (1-alpha)*(1-beta)*gamma);
		ci1jk1 = scale_v4(ci1jk1, alpha*(1-beta)*gamma);
		cij1k1 = scale_v4(cij1k1, (1-alpha)*beta*gamma);
		ci1j1k1 = scale_v4(ci1j1k1, alpha*beta*gamma);

		cijk = add_v4s(cijk, ci1jk);
		cijk = add_v4s(cijk, cij1k);
		cijk = add_v4s(cijk, ci1j1k);
		cijk = add_v4s(cijk, cijk1);
		cijk = add_v4s(cijk, ci1jk1);
		cijk = add_v4s(cijk, cij1k1);
		cijk = add_v4s(cijk, ci1j1k1);

		return cijk;
	}
}

// for now this should work
PGLDEF vec4 texture2DArray(GLuint tex, float x, float y, int z)
{
	int i0, j0, i1, j1;

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_2D_ARRAY-GL_TEXTURE_1D];
	}
	Color* texdata = (Color*)t->data;
	int w = t->w;
	int h = t->h;

	float dw = w - EPSILON;
	float dh = h - EPSILON;

	int plane = w * h;
	float xw = x * dw;
	float yh = y * dh;


	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floorf(xw), w, t->wrap_s);
		j0 = wrap(floorf(yh), h, t->wrap_t);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0) < 0) return t->border_color;
#endif
		return Color_to_v4(texdata[z*plane + j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floorf(xw - 0.5f), w, t->wrap_s);
		j0 = wrap(floorf(yh - 0.5f), h, t->wrap_t);
		i1 = wrap(floorf(xw + 0.499999f), w, t->wrap_s);
		j1 = wrap(floorf(yh + 0.499999f), h, t->wrap_t);

		float tmp2;
		float alpha = modff(xw+0.5f, &tmp2);
		float beta = modff(yh+0.5f, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		vec4 cij, ci1j, cij1, ci1j1;
		if ((i0 | j0) < 0) cij = t->border_color;
		else cij = Color_to_v4(texdata[z*plane + j0*w + i0]);

		if ((i1 | j0) < 0) ci1j = t->border_color;
		else ci1j = Color_to_v4(texdata[z*plane + j0*w + i1]);

		if ((i0 | j1) < 0) cij1 = t->border_color;
		else cij1 = Color_to_v4(texdata[z*plane + j1*w + i0]);

		if ((i1 | j1) < 0) ci1j1 = t->border_color;
		else ci1j1 = Color_to_v4(texdata[z*plane + j1*w + i1]);
#else
		vec4 cij = Color_to_v4(texdata[z*plane + j0*w + i0]);
		vec4 ci1j = Color_to_v4(texdata[z*plane + j0*w + i1]);
		vec4 cij1 = Color_to_v4(texdata[z*plane + j1*w + i0]);
		vec4 ci1j1 = Color_to_v4(texdata[z*plane + j1*w + i1]);
#endif

		cij = scale_v4(cij, (1-alpha)*(1-beta));
		ci1j = scale_v4(ci1j, alpha*(1-beta));
		cij1 = scale_v4(cij1, (1-alpha)*beta);
		ci1j1 = scale_v4(ci1j1, alpha*beta);

		cij = add_v4s(cij, ci1j);
		cij = add_v4s(cij, cij1);
		cij = add_v4s(cij, ci1j1);

		return cij;
	}
}

PGLDEF vec4 texture_rect(GLuint tex, float x, float y)
{
	int i0, j0, i1, j1;

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_RECTANGLE-GL_TEXTURE_1D];
	}
	Color* texdata = (Color*)t->data;

	int w = t->w;
	int h = t->h;

	float xw = x;
	float yh = y;

	//TODO don't just use mag_filter all the time?
	//is it worth bothering?
	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floorf(xw), w, t->wrap_s);
		j0 = wrap(floorf(yh), h, t->wrap_t);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0) < 0) return t->border_color;
#endif
		return Color_to_v4(texdata[j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floorf(xw - 0.5f), w, t->wrap_s);
		j0 = wrap(floorf(yh - 0.5f), h, t->wrap_t);
		i1 = wrap(floorf(xw + 0.499999f), w, t->wrap_s);
		j1 = wrap(floorf(yh + 0.499999f), h, t->wrap_t);

		float tmp2;
		float alpha = modff(xw+0.5f, &tmp2);
		float beta = modff(yh+0.5f, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		vec4 cij, ci1j, cij1, ci1j1;
		if ((i0 | j0) < 0) cij = t->border_color;
		else cij = Color_to_v4(texdata[j0*w + i0]);

		if ((i1 | j0) < 0) ci1j = t->border_color;
		else ci1j = Color_to_v4(texdata[j0*w + i1]);

		if ((i0 | j1) < 0) cij1 = t->border_color;
		else cij1 = Color_to_v4(texdata[j1*w + i0]);

		if ((i1 | j1) < 0) ci1j1 = t->border_color;
		else ci1j1 = Color_to_v4(texdata[j1*w + i1]);
#else
		vec4 cij = Color_to_v4(texdata[j0*w + i0]);
		vec4 ci1j = Color_to_v4(texdata[j0*w + i1]);
		vec4 cij1 = Color_to_v4(texdata[j1*w + i0]);
		vec4 ci1j1 = Color_to_v4(texdata[j1*w + i1]);
#endif

		cij = scale_v4(cij, (1-alpha)*(1-beta));
		ci1j = scale_v4(ci1j, alpha*(1-beta));
		cij1 = scale_v4(cij1, (1-alpha)*beta);
		ci1j1 = scale_v4(ci1j1, alpha*beta);

		cij = add_v4s(cij, ci1j);
		cij = add_v4s(cij, cij1);
		cij = add_v4s(cij, ci1j1);

		return cij;
	}
}

// Sample one face of a cubemap level (level_data points at the 6-face pack).
// face is 0..5; x,y are [0,1] face UVs.  filter is NEAREST or LINEAR.
static vec4 pgl_sample_cube_face(const glTexture* t, const u8* level_data,
                                 int w, int h, int face, float x, float y, GLenum filter)
{
	Color* texdata = (Color*)level_data;
	float dw = w - EPSILON;
	float dh = h - EPSILON;
	int plane = w * h;
	float xw = x * dw;
	float yh = y * dh;
	int i0, j0, i1, j1;

	if (filter == GL_NEAREST) {
		i0 = wrap(floorf(xw), w, t->wrap_s);
		j0 = wrap(floorf(yh), h, t->wrap_t);
		return Color_to_v4(texdata[face * plane + j0 * w + i0]);
	}

	// LINEAR
	i0 = wrap(floorf(xw - 0.5f), w, t->wrap_s);
	j0 = wrap(floorf(yh - 0.5f), h, t->wrap_t);
	i1 = wrap(floorf(xw + 0.499999f), w, t->wrap_s);
	j1 = wrap(floorf(yh + 0.499999f), h, t->wrap_t);

	float tmp2;
	float alpha = modff(xw + 0.5f, &tmp2);
	float beta = modff(yh + 0.5f, &tmp2);
	if (alpha < 0) ++alpha;
	if (beta < 0) ++beta;

#ifdef PGL_HERMITE_SMOOTHING
	alpha = alpha * alpha * (3 - 2 * alpha);
	beta = beta * beta * (3 - 2 * beta);
#endif

	vec4 cij = Color_to_v4(texdata[face * plane + j0 * w + i0]);
	vec4 ci1j = Color_to_v4(texdata[face * plane + j0 * w + i1]);
	vec4 cij1 = Color_to_v4(texdata[face * plane + j1 * w + i0]);
	vec4 ci1j1 = Color_to_v4(texdata[face * plane + j1 * w + i1]);

	cij = scale_v4(cij, (1 - alpha) * (1 - beta));
	ci1j = scale_v4(ci1j, alpha * (1 - beta));
	cij1 = scale_v4(cij1, (1 - alpha) * beta);
	ci1j1 = scale_v4(ci1j1, alpha * beta);

	cij = add_v4s(cij, ci1j);
	cij = add_v4s(cij, cij1);
	cij = add_v4s(cij, ci1j1);
	return cij;
}

static vec4 pgl_sample_cube_level_idx(const glTexture* t, int level, int face, float x, float y)
{
	u8* data = pgl_tex_level_data(t, level);
	if (!data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);
	GLsizei w, h;
	pgl_tex_level_dims(t, level, &w, &h, NULL);
	return pgl_sample_cube_face(t, data, w, h, face, x, y,
	                            pgl_within_level_filter(t->min_filter));
}

// Minify path for cubemaps (same LOD rules as 2D: nearest level or trilinear)
static vec4 pgl_sample_cube_minify(const glTexture* t, int face, float x, float y, float lod)
{
	int max_level = t->num_levels - 1;
	if (max_level <= 0)
		return pgl_sample_cube_level_idx(t, 0, face, x, y);

	if (!pgl_is_mip_linear_filter(t->min_filter))
		return pgl_sample_cube_level_idx(t, pgl_lod_to_level(t, lod), face, x, y);

	if (lod < 0.0f)
		lod = 0.0f;
	if (lod >= (float)max_level)
		return pgl_sample_cube_level_idx(t, max_level, face, x, y);

	int l0 = (int)floorf(lod);
	float frac = lod - (float)l0;
	if (frac <= 0.0f)
		return pgl_sample_cube_level_idx(t, l0, face, x, y);
	if (frac >= 1.0f)
		return pgl_sample_cube_level_idx(t, l0 + 1, face, x, y);

	vec4 c0 = pgl_sample_cube_level_idx(t, l0, face, x, y);
	vec4 c1 = pgl_sample_cube_level_idx(t, l0 + 1, face, x, y);
	return pgl_lerp_v4(c0, c1, frac);
}

PGLDEF vec4 texture_cubemap(GLuint texture, float x, float y, float z)
{
	glTexture* tex = NULL;
	if (texture) {
		tex = &c->textures.a[texture];
	} else {
		tex = &c->default_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_1D];
	}

	if (!tex->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	float x_mag = (x < 0) ? -x : x;
	float y_mag = (y < 0) ? -y : y;
	float z_mag = (z < 0) ? -z : z;

	float s, t, max;

	int p;

	//there should be a better/shorter way to do this ...
	if (x_mag > y_mag) {
		if (x_mag > z_mag) {  //x largest
			max = x_mag;
			t = -y;
			if (x_mag == x) {
				p = 0;
				s = -z;
			} else {
				p = 1;
				s = z;
			}
		} else { //z largest
			max = z_mag;
			t = -y;
			if (z_mag == z) {
				p = 4;
				s = x;
			} else {
				p = 5;
				s = -x;
			}
		}
	} else {
		if (y_mag > z_mag) {  //y largest
			max = y_mag;
			s = x;
			if (y_mag == y) {
				p = 2;
				t = z;
			} else {
				p = 3;
				t = -z;
			}
		} else { //z largest
			max = z_mag;
			t = -y;
			if (z_mag == z) {
				p = 4;
				s = x;
			} else {
				p = 5;
				s = -x;
			}
		}
	}

	// TODO As I understand this, this prevents x and y from ever being
	// outside [0, 1] so there's no need for me to put CLAMP_TO_BORDER ifdefs
	// in here, since even CLAMP_TO_EDGE should never happen.
	x = (s/max + 1.0f)/2.0f;
	y = (t/max + 1.0f)/2.0f;

	if (tex->num_levels > 1 && pgl_is_mip_min_filter(tex->min_filter)) {
		// Auto LOD from per-triangle UV scale and face base size (same ρ as 2D)
		float lambda = pgl_auto_lod(tex, tex->w, tex->h);
		if (lambda <= 0.0f)
			return pgl_sample_cube_face(tex, tex->data, tex->w, tex->h, p, x, y, tex->mag_filter);
		return pgl_sample_cube_minify(tex, p, x, y, lambda);
	}

	// Incomplete mip filter: Core → black; compat → L0 + mag
	if (pgl_incomplete_mip_returns_black(tex))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_cube_face(tex, tex->data, tex->w, tex->h, p, x, y, tex->mag_filter);
}

PGLDEF vec4 texelFetch1D(GLuint tex, int x, int lod)
{
	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_1D-GL_TEXTURE_1D];
	}

	if (lod < 0)
		lod = 0;
	u8* data = pgl_tex_level_data(t, lod);
	if (!data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	GLsizei w;
	pgl_tex_level_dims(t, lod, &w, NULL, NULL);
	if (x < 0 || x >= w)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	Color* texdata = (Color*)data;
	return Color_to_v4(texdata[x]);
}

PGLDEF vec4 texelFetch2D(GLuint tex, int x, int y, int lod)
{
	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_2D-GL_TEXTURE_1D];
	}

	if (lod < 0)
		lod = 0;
	u8* data = pgl_tex_level_data(t, lod);
	if (!data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	GLsizei w, h;
	pgl_tex_level_dims(t, lod, &w, &h, NULL);
	if (x < 0 || x >= w || y < 0 || y >= h)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	Color* texdata = (Color*)data;
	return Color_to_v4(texdata[y * w + x]);
}

PGLDEF vec4 texelFetch3D(GLuint tex, int x, int y, int z, int lod)
{
	// 3D mipmaps not supported yet; lod other than 0 still reads level 0
	PGL_UNUSED(lod);

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_3D-GL_TEXTURE_1D];
	}
	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	Color* texdata = (Color*)t->data;
	int w = t->w;
	int plane = w * t->h;
	return Color_to_v4(texdata[z*plane + y*w + x]);
}

PGLDEF ivec3 textureSize(GLuint tex, GLint lod)
{
	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_1D-GL_TEXTURE_1D];
	}

	if (lod < 0)
		lod = 0;

	// Clamp to last defined level (3D/array have only level 0 for now)
	if (t->num_levels > 0 && lod >= t->num_levels)
		lod = t->num_levels - 1;

	GLsizei w = 0, h = 0, d = 0;
	pgl_tex_level_dims(t, lod, &w, &h, &d);
	return make_iv3(w, h, d);
}

#undef EPSILON

