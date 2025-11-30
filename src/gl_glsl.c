

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

// used in the following 4 texture access functions
// Not sure if it's actually necessary since wrap() clamps
#define EPSILON 0.000001
PGLDEF vec4 texture1D(GLuint tex, float x)
{
	int i0, i1;

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_1D-GL_TEXTURE_1D];
	}
	Color* texdata = (Color*)t->data;

	double w = t->w - EPSILON;

	double xw = x * w;

	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), t->w, t->wrap_s);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if (i0 < 0) return t->border_color;
#endif

		return Color_to_vec4(texdata[i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), t->w, t->wrap_s);
		i1 = wrap(floor(xw + 0.499999), t->w, t->wrap_s);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		if (alpha < 0) ++alpha;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		vec4 ci, ci1;
		if (i0 < 0) ci = t->border_color;
		else ci = Color_to_vec4(texdata[i0]);

		if (i1 < 0) ci1 = t->border_color;
		else ci1 = Color_to_vec4(texdata[i1]);
#else
		vec4 ci = Color_to_vec4(texdata[i0]);
		vec4 ci1 = Color_to_vec4(texdata[i1]);
#endif

		ci = scale_vec4(ci, (1-alpha));
		ci1 = scale_vec4(ci1, alpha);

		ci = add_vec4s(ci, ci1);

		return ci;
	}
}

PGLDEF vec4 texture2D(GLuint tex, float x, float y)
{
	int i0, j0, i1, j1;

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_2D-GL_TEXTURE_1D];
	}
	Color* texdata = (Color*)t->data;

	int w = t->w;
	int h = t->h;

	double dw = w - EPSILON;
	double dh = h - EPSILON;

	double xw = x * dw;
	double yh = y * dh;

	// TODO don't just use mag_filter all the time?
	// is it worth bothering?
	// Or maybe it makes more sense to use min_filter all the time
	// since that defaults to NEAREST?
	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), w, t->wrap_s);
		j0 = wrap(floor(yh), h, t->wrap_t);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0) < 0) return t->border_color;
#endif
		return Color_to_vec4(texdata[j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), w, t->wrap_s);
		j0 = wrap(floor(yh - 0.5), h, t->wrap_t);
		i1 = wrap(floor(xw + 0.499999), w, t->wrap_s);
		j1 = wrap(floor(yh + 0.499999), h, t->wrap_t);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		double beta = modf(yh+0.5, &tmp2);
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
		else cij = Color_to_vec4(texdata[j0*w + i0]);

		if ((i1 | j0) < 0) ci1j = t->border_color;
		else ci1j = Color_to_vec4(texdata[j0*w + i1]);

		if ((i0 | j1) < 0) cij1 = t->border_color;
		else cij1 = Color_to_vec4(texdata[j1*w + i0]);

		if ((i1 | j1) < 0) ci1j1 = t->border_color;
		else ci1j1 = Color_to_vec4(texdata[j1*w + i1]);
#else
		vec4 cij = Color_to_vec4(texdata[j0*w + i0]);
		vec4 ci1j = Color_to_vec4(texdata[j0*w + i1]);
		vec4 cij1 = Color_to_vec4(texdata[j1*w + i0]);
		vec4 ci1j1 = Color_to_vec4(texdata[j1*w + i1]);
#endif

		cij = scale_vec4(cij, (1-alpha)*(1-beta));
		ci1j = scale_vec4(ci1j, alpha*(1-beta));
		cij1 = scale_vec4(cij1, (1-alpha)*beta);
		ci1j1 = scale_vec4(ci1j1, alpha*beta);

		cij = add_vec4s(cij, ci1j);
		cij = add_vec4s(cij, cij1);
		cij = add_vec4s(cij, ci1j1);

		return cij;
	}
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

	double dw = t->w - EPSILON;
	double dh = t->h - EPSILON;
	double dd = t->d - EPSILON;

	int w = t->w;
	int h = t->h;
	int d = t->d;
	int plane = w * t->h;
	double xw = x * dw;
	double yh = y * dh;
	double zd = z * dd;


	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), w, t->wrap_s);
		j0 = wrap(floor(yh), h, t->wrap_t);
		k0 = wrap(floor(zd), d, t->wrap_r);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0 | k0) < 0) return t->border_color;
#endif

		return Color_to_vec4(texdata[k0*plane + j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), w, t->wrap_s);
		j0 = wrap(floor(yh - 0.5), h, t->wrap_t);
		k0 = wrap(floor(zd - 0.5), d, t->wrap_r);
		i1 = wrap(floor(xw + 0.499999), w, t->wrap_s);
		j1 = wrap(floor(yh + 0.499999), h, t->wrap_t);
		k1 = wrap(floor(zd + 0.499999), d, t->wrap_r);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		double beta = modf(yh+0.5, &tmp2);
		double gamma = modf(zd+0.5, &tmp2);
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
		else cijk = Color_to_vec4(texdata[k0*plane + j0*w + i0]);

		if ((i1 | j0 | k0) < 0) ci1jk = t->border_color;
		else ci1jk = Color_to_vec4(texdata[k0*plane + j0*w + i1]);

		if ((i0 | j1 | k0) < 0) cij1k = t->border_color;
		else cij1k = Color_to_vec4(texdata[k0*plane + j1*w + i0]);

		if ((i1 | j1 | k0) < 0) ci1j1k = t->border_color;
		else ci1j1k = Color_to_vec4(texdata[k0*plane + j1*w + i1]);

		if ((i0 | j0 | k1) < 0) cijk1 = t->border_color;
		else cijk1 = Color_to_vec4(texdata[k1*plane + j0*w + i0]);

		if ((i1 | j0 | k1) < 0) ci1jk1 = t->border_color;
		else ci1jk1 = Color_to_vec4(texdata[k1*plane + j0*w + i1]);

		if ((i0 | j1 | k1) < 0) cij1k1 = t->border_color;
		else cij1k1 = Color_to_vec4(texdata[k1*plane + j1*w + i0]);

		if ((i1 | j1 | k1) < 0) ci1j1k1 = t->border_color;
		else ci1j1k1 = Color_to_vec4(texdata[k1*plane + j1*w + i1]);
#else
		vec4 cijk = Color_to_vec4(texdata[k0*plane + j0*w + i0]);
		vec4 ci1jk = Color_to_vec4(texdata[k0*plane + j0*w + i1]);
		vec4 cij1k = Color_to_vec4(texdata[k0*plane + j1*w + i0]);
		vec4 ci1j1k = Color_to_vec4(texdata[k0*plane + j1*w + i1]);
		vec4 cijk1 = Color_to_vec4(texdata[k1*plane + j0*w + i0]);
		vec4 ci1jk1 = Color_to_vec4(texdata[k1*plane + j0*w + i1]);
		vec4 cij1k1 = Color_to_vec4(texdata[k1*plane + j1*w + i0]);
		vec4 ci1j1k1 = Color_to_vec4(texdata[k1*plane + j1*w + i1]);
#endif

		cijk = scale_vec4(cijk, (1-alpha)*(1-beta)*(1-gamma));
		ci1jk = scale_vec4(ci1jk, alpha*(1-beta)*(1-gamma));
		cij1k = scale_vec4(cij1k, (1-alpha)*beta*(1-gamma));
		ci1j1k = scale_vec4(ci1j1k, alpha*beta*(1-gamma));
		cijk1 = scale_vec4(cijk1, (1-alpha)*(1-beta)*gamma);
		ci1jk1 = scale_vec4(ci1jk1, alpha*(1-beta)*gamma);
		cij1k1 = scale_vec4(cij1k1, (1-alpha)*beta*gamma);
		ci1j1k1 = scale_vec4(ci1j1k1, alpha*beta*gamma);

		cijk = add_vec4s(cijk, ci1jk);
		cijk = add_vec4s(cijk, cij1k);
		cijk = add_vec4s(cijk, ci1j1k);
		cijk = add_vec4s(cijk, cijk1);
		cijk = add_vec4s(cijk, ci1jk1);
		cijk = add_vec4s(cijk, cij1k1);
		cijk = add_vec4s(cijk, ci1j1k1);

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

	double dw = w - EPSILON;
	double dh = h - EPSILON;

	int plane = w * h;
	double xw = x * dw;
	double yh = y * dh;


	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), w, t->wrap_s);
		j0 = wrap(floor(yh), h, t->wrap_t);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0) < 0) return t->border_color;
#endif
		return Color_to_vec4(texdata[z*plane + j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), w, t->wrap_s);
		j0 = wrap(floor(yh - 0.5), h, t->wrap_t);
		i1 = wrap(floor(xw + 0.499999), w, t->wrap_s);
		j1 = wrap(floor(yh + 0.499999), h, t->wrap_t);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		double beta = modf(yh+0.5, &tmp2);
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
		else cij = Color_to_vec4(texdata[z*plane + j0*w + i0]);

		if ((i1 | j0) < 0) ci1j = t->border_color;
		else ci1j = Color_to_vec4(texdata[z*plane + j0*w + i1]);

		if ((i0 | j1) < 0) cij1 = t->border_color;
		else cij1 = Color_to_vec4(texdata[z*plane + j1*w + i0]);

		if ((i1 | j1) < 0) ci1j1 = t->border_color;
		else ci1j1 = Color_to_vec4(texdata[z*plane + j1*w + i1]);
#else
		vec4 cij = Color_to_vec4(texdata[z*plane + j0*w + i0]);
		vec4 ci1j = Color_to_vec4(texdata[z*plane + j0*w + i1]);
		vec4 cij1 = Color_to_vec4(texdata[z*plane + j1*w + i0]);
		vec4 ci1j1 = Color_to_vec4(texdata[z*plane + j1*w + i1]);
#endif

		cij = scale_vec4(cij, (1-alpha)*(1-beta));
		ci1j = scale_vec4(ci1j, alpha*(1-beta));
		cij1 = scale_vec4(cij1, (1-alpha)*beta);
		ci1j1 = scale_vec4(ci1j1, alpha*beta);

		cij = add_vec4s(cij, ci1j);
		cij = add_vec4s(cij, cij1);
		cij = add_vec4s(cij, ci1j1);

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

	double xw = x;
	double yh = y;

	//TODO don't just use mag_filter all the time?
	//is it worth bothering?
	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), w, t->wrap_s);
		j0 = wrap(floor(yh), h, t->wrap_t);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0) < 0) return t->border_color;
#endif
		return Color_to_vec4(texdata[j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), w, t->wrap_s);
		j0 = wrap(floor(yh - 0.5), h, t->wrap_t);
		i1 = wrap(floor(xw + 0.499999), w, t->wrap_s);
		j1 = wrap(floor(yh + 0.499999), h, t->wrap_t);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		double beta = modf(yh+0.5, &tmp2);
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
		else cij = Color_to_vec4(texdata[j0*w + i0]);

		if ((i1 | j0) < 0) ci1j = t->border_color;
		else ci1j = Color_to_vec4(texdata[j0*w + i1]);

		if ((i0 | j1) < 0) cij1 = t->border_color;
		else cij1 = Color_to_vec4(texdata[j1*w + i0]);

		if ((i1 | j1) < 0) ci1j1 = t->border_color;
		else ci1j1 = Color_to_vec4(texdata[j1*w + i1]);
#else
		vec4 cij = Color_to_vec4(texdata[j0*w + i0]);
		vec4 ci1j = Color_to_vec4(texdata[j0*w + i1]);
		vec4 cij1 = Color_to_vec4(texdata[j1*w + i0]);
		vec4 ci1j1 = Color_to_vec4(texdata[j1*w + i1]);
#endif

		cij = scale_vec4(cij, (1-alpha)*(1-beta));
		ci1j = scale_vec4(ci1j, alpha*(1-beta));
		cij1 = scale_vec4(cij1, (1-alpha)*beta);
		ci1j1 = scale_vec4(ci1j1, alpha*beta);

		cij = add_vec4s(cij, ci1j);
		cij = add_vec4s(cij, cij1);
		cij = add_vec4s(cij, ci1j1);

		return cij;
	}
}

PGLDEF vec4 texture_cubemap(GLuint texture, float x, float y, float z)
{
	glTexture* tex = NULL;
	if (texture) {
		tex = &c->textures.a[texture];
	} else {
		tex = &c->default_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_1D];
	}
	Color* texdata = (Color*)tex->data;

	float x_mag = (x < 0) ? -x : x;
	float y_mag = (y < 0) ? -y : y;
	float z_mag = (z < 0) ? -z : z;

	float s, t, max;

	int p, i0, j0, i1, j1;

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

	int w = tex->w;
	int h = tex->h;

	double dw = w - EPSILON;
	double dh = h - EPSILON;

	int plane = w*w;
	double xw = x * dw;
	double yh = y * dh;

	if (tex->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), w, tex->wrap_s);
		j0 = wrap(floor(yh), h, tex->wrap_t);

		vec4 tmpvec4 = Color_to_vec4(texdata[p*plane + j0*w + i0]);
		return tmpvec4;

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), tex->w, tex->wrap_s);
		j0 = wrap(floor(yh - 0.5), tex->h, tex->wrap_t);
		i1 = wrap(floor(xw + 0.499999), tex->w, tex->wrap_s);
		j1 = wrap(floor(yh + 0.499999), tex->h, tex->wrap_t);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		double beta = modf(yh+0.5, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
#endif

		vec4 cij = Color_to_vec4(texdata[p*plane + j0*w + i0]);
		vec4 ci1j = Color_to_vec4(texdata[p*plane + j0*w + i1]);
		vec4 cij1 = Color_to_vec4(texdata[p*plane + j1*w + i0]);
		vec4 ci1j1 = Color_to_vec4(texdata[p*plane + j1*w + i1]);

		cij = scale_vec4(cij, (1-alpha)*(1-beta));
		ci1j = scale_vec4(ci1j, alpha*(1-beta));
		cij1 = scale_vec4(cij1, (1-alpha)*beta);
		ci1j1 = scale_vec4(ci1j1, alpha*beta);

		cij = add_vec4s(cij, ci1j);
		cij = add_vec4s(cij, cij1);
		cij = add_vec4s(cij, ci1j1);

		return cij;
	}
}

#undef EPSILON

