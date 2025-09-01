

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


#define imod(a, b) (a) - (b) * ((a)/(b))

static int wrap(int i, int size, GLenum mode)
{
	int tmp;
	switch (mode)
	{
	case GL_REPEAT:
		tmp = imod(i, size);
		if (tmp < 0) tmp = size + tmp;
		return tmp;

	// Border is too much of a pain to implement with render to
	// texture.  Trade offs in poor performance or ugly extra code
	// for a feature that almost no one actually uses and even
	// when it is used (barring rare/odd uv coordinates) it's not
	// even noticable.
	//case GL_CLAMP_TO_BORDER:
		//return clampi(i, -1, size);

	case GL_CLAMP_TO_BORDER:  // just so stuff that uses it compiles
	case GL_CLAMP_TO_EDGE:
		return clampi(i, 0, size-1);
	

	case GL_MIRRORED_REPEAT:
		if (i < 0) i = -i;
		tmp = i / size;
		if (tmp % 2)
			return (size-1) - (i - tmp * size);
		else
			return i - tmp * size;

		return tmp;
	default:
		//should never happen, get rid of compile warning
		assert(0);
		return 0;
	}
}
#undef imod


// used in the following 4 texture access functions
// Not sure if it's actually necessary since wrap() clamps
#define EPSILON 0.000001
PGLDEF vec4 texture1D(GLuint tex, float x)
{
	int i0, i1;

	glTexture* t = &c->textures.a[tex];
	Color* texdata = (Color*)t->data;

	double w = t->w - EPSILON;

	double xw = x * w;

	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), t->w, t->wrap_s);

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

		vec4 ci = Color_to_vec4(texdata[i0]);
		vec4 ci1 = Color_to_vec4(texdata[i1]);

		ci = scale_vec4(ci, (1-alpha));
		ci1 = scale_vec4(ci1, alpha);

		ci = add_vec4s(ci, ci1);

		return ci;
	}
}

PGLDEF vec4 texture2D(GLuint tex, float x, float y)
{
	int i0, j0, i1, j1;

	glTexture* t = &c->textures.a[tex];
	Color* texdata = (Color*)t->data;

	int w = t->w;
	int h = t->h;

	double dw = w - EPSILON;
	double dh = h - EPSILON;

	double xw = x * dw;
	double yh = y * dh;

	//TODO don't just use mag_filter all the time?
	//is it worth bothering?
	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), w, t->wrap_s);
		j0 = wrap(floor(yh), h, t->wrap_t);

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

		vec4 cij = Color_to_vec4(texdata[j0*w + i0]);
		vec4 ci1j = Color_to_vec4(texdata[j0*w + i1]);
		vec4 cij1 = Color_to_vec4(texdata[j1*w + i0]);
		vec4 ci1j1 = Color_to_vec4(texdata[j1*w + i1]);

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

	glTexture* t = &c->textures.a[tex];
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

		vec4 cijk = Color_to_vec4(texdata[k0*plane + j0*w + i0]);
		vec4 ci1jk = Color_to_vec4(texdata[k0*plane + j0*w + i1]);
		vec4 cij1k = Color_to_vec4(texdata[k0*plane + j1*w + i0]);
		vec4 ci1j1k = Color_to_vec4(texdata[k0*plane + j1*w + i1]);
		vec4 cijk1 = Color_to_vec4(texdata[k1*plane + j0*w + i0]);
		vec4 ci1jk1 = Color_to_vec4(texdata[k1*plane + j0*w + i1]);
		vec4 cij1k1 = Color_to_vec4(texdata[k1*plane + j1*w + i0]);
		vec4 ci1j1k1 = Color_to_vec4(texdata[k1*plane + j1*w + i1]);

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

	glTexture* t = &c->textures.a[tex];
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
		vec4 cij = Color_to_vec4(texdata[z*plane + j0*w + i0]);
		vec4 ci1j = Color_to_vec4(texdata[z*plane + j0*w + i1]);
		vec4 cij1 = Color_to_vec4(texdata[z*plane + j1*w + i0]);
		vec4 ci1j1 = Color_to_vec4(texdata[z*plane + j1*w + i1]);

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

	glTexture* t = &c->textures.a[tex];
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

		vec4 cij = Color_to_vec4(texdata[j0*w + i0]);
		vec4 ci1j = Color_to_vec4(texdata[j0*w + i1]);
		vec4 cij1 = Color_to_vec4(texdata[j1*w + i0]);
		vec4 ci1j1 = Color_to_vec4(texdata[j1*w + i1]);

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
	glTexture* tex = &c->textures.a[texture];
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

