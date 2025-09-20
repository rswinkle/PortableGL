
//Raw draw functions that bypass the OpenGL pipeline and draw
//points/lines/triangles directly to the framebuffer, modify as needed.
//
//Example modifications:
//add the blending part of OpenGL to put_pixel
//change them to take vec4's instead of Color's
//change put_triangle to draw all one color or have a separate path/function
//that draws a single color triangle faster (no need to blend)
//
//pass the framebuffer in instead of drawing to c->back_buffer so 
//you can use it elsewhere, independently of a glContext
//etc.
//
PGLDEF void pglClearScreen(void)
{
	memset(c->back_buffer.buf, 255, c->back_buffer.w * c->back_buffer.h * 4);
}

PGLDEF void pglSetInterp(GLsizei n, GLenum* interpolation)
{
	c->programs.a[c->cur_program].vs_output_size = n;
	c->vs_output.size = n;

	memcpy(c->programs.a[c->cur_program].interpolation, interpolation, n*sizeof(GLenum));

	// c->vs_output.output_buf was pre-allocated to max size needed in init_glContext
	// otherwise would need to assure it's at least
	// c->vs_output_size * PGL_MAX_VERTS * sizeof(float) right here

	//vs_output.interpolation would be already pointing at current program's array
	//unless the programs array was realloced since the last glUseProgram because
	//they've created a bunch of programs.  Unlikely they'd be changing a shader
	//before creating all their shaders but whatever.
	c->vs_output.interpolation = c->programs.a[c->cur_program].interpolation;
}




//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
PGLDEF void pglDrawFrame(void)
{
	frag_func frag_shader = c->programs.a[c->cur_program].fragment_shader;

	Shader_Builtins builtins;
	#pragma omp parallel for private(builtins)
	for (int y=0; y<c->back_buffer.h; ++y) {
		for (int x=0; x<c->back_buffer.w; ++x) {

			//ignore z and w components
			builtins.gl_FragCoord.x = x + 0.5f;
			builtins.gl_FragCoord.y = y + 0.5f;

			builtins.discard = GL_FALSE;
			frag_shader(NULL, &builtins, c->programs.a[c->cur_program].uniform);
			if (!builtins.discard)
				draw_pixel(builtins.gl_FragColor, x, y, 0.0f, GL_FALSE);  //scissor/stencil/depth aren't used for pglDrawFrame
		}
	}

}

PGLDEF void pglBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage)
{
	//TODO check for usage later
	PGL_UNUSED(usage);

	PGL_ERR((target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER), GL_INVALID_ENUM);

	target -= GL_ARRAY_BUFFER;
	PGL_ERR(!c->bound_buffers[target], GL_INVALID_OPERATION);

	// data can't be null for user_owned data
	PGL_ERR(!data, GL_INVALID_VALUE);

	// TODO Should I change this in spec functions too?  Or just say don't mix them
	// otherwise bad things/undefined behavior??
	if (!c->buffers.a[c->bound_buffers[target]].user_owned) {
		free(c->buffers.a[c->bound_buffers[target]].data);
	}

	// user_owned buffer, just assign the pointer, will not free
	c->buffers.a[c->bound_buffers[target]].data = (u8*)data;

	c->buffers.a[c->bound_buffers[target]].user_owned = GL_TRUE;
	c->buffers.a[c->bound_buffers[target]].size = size;

	if (target == GL_ELEMENT_ARRAY_BUFFER) {
		c->vertex_arrays.a[c->cur_vertex_array].element_buffer = c->bound_buffers[target];
	}
}

// TODO/NOTE
// All pglTexImage* functions expect the user to pass in packed GL_RGBA
// data. Unlike glTexImage*, no conversion is done, and format != GL_RGBA
// is an INVALID_ENUM error
//
// At least the latter part will change if I ever expand internal format
// support
PGLDEF void pglTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	// ignore level and internalformat for now
	// (the latter is always converted to RGBA32 anyway)
	PGL_UNUSED(level);
	PGL_UNUSED(internalformat);

	PGL_ERR(target != GL_TEXTURE_1D, GL_INVALID_ENUM);
	PGL_ERR(border, GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);

	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);

	// data can't be null for user_owned data
	PGL_ERR(!data, GL_INVALID_VALUE);

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	c->textures.a[cur_tex].w = width;

	// TODO see pglBufferData
	if (!c->textures.a[cur_tex].user_owned)
		free(c->textures.a[cur_tex].data);

	//TODO support other internal formats? components should be of internalformat not format
	c->textures.a[cur_tex].data = (u8*)data;
	c->textures.a[cur_tex].user_owned = GL_TRUE;
}

PGLDEF void pglTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	// ignore level and internalformat for now
	// (the latter is always converted to RGBA32 anyway)
	PGL_UNUSED(level);
	PGL_UNUSED(internalformat);

	// TODO handle cubemap properly
	PGL_ERR((target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_RECTANGLE &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z), GL_INVALID_ENUM);

	PGL_ERR(border, GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);

	// data can't be null for user_owned data
	PGL_ERR(!data, GL_INVALID_VALUE);

	int cur_tex;

	if (target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE) {
		cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

		c->textures.a[cur_tex].w = width;
		c->textures.a[cur_tex].h = height;

		// TODO see pglBufferData
		if (!c->textures.a[cur_tex].user_owned)
			free(c->textures.a[cur_tex].data);

		// If you're using these pgl mapped functions, it assumes you are respecting
		// your own current unpack alignment settings already
		c->textures.a[cur_tex].data = (u8*)data;
		c->textures.a[cur_tex].user_owned = GL_TRUE;

	} else {  //CUBE_MAP
		/*
		 * TODO, doesn't make sense to call this six times when mapping, you'd set
		 * them all up beforehand and set the pointer once...so change this or
		 * make a pglCubeMapData() function?
		 *
		cur_tex = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1];

		// TODO see pglBufferData
		if (!c->textures.a[cur_tex].user_owned)
			free(c->textures.a[cur_tex].data);

		//TODO spec says INVALID_VALUE, man pages say INVALID_ENUM ?
		PGL_ERR(width != height, GL_INVALID_VALUE);

		int mem_size = width*height*6 * components;
		if (c->textures.a[cur_tex].w == 0) {
			c->textures.a[cur_tex].w = width;
			c->textures.a[cur_tex].h = width; //same cause square

		} else if (c->textures.a[cur_tex].w != width) {
			//TODO spec doesn't say all sides must have same dimensions but it makes sense
			//and this site suggests it http://www.opengl.org/wiki/Cubemap_Texture
			PGL_SET_ERR(GL_INVALID_VALUE);
			return;
		}

		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X; //use target as plane index

		c->textures.a[cur_tex].data = (u8*)data;
		c->textures.a[cur_tex].user_owned = GL_TRUE;
		*/

	} //end CUBE_MAP
}

PGLDEF void pglTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	// ignore level and internalformat for now
	// (the latter is always converted to RGBA32 anyway)
	PGL_UNUSED(level);
	PGL_UNUSED(internalformat);

	PGL_ERR((target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY), GL_INVALID_ENUM);
	PGL_ERR(border, GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);

	// data can't be null for user_owned data
	PGL_ERR(!data, GL_INVALID_VALUE);

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	c->textures.a[cur_tex].w = width;
	c->textures.a[cur_tex].h = height;
	c->textures.a[cur_tex].d = depth;

	// TODO see pglBufferData
	if (!c->textures.a[cur_tex].user_owned)
		free(c->textures.a[cur_tex].data);

	c->textures.a[cur_tex].data = (u8*)data;
	c->textures.a[cur_tex].user_owned = GL_TRUE;
}


PGLDEF void pglGetBufferData(GLuint buffer, GLvoid** data)
{
	// why'd you even call it?
	PGL_ERR(!data, GL_INVALID_VALUE);

	// matching error code of binding invalid buffecr
	PGL_ERR((!buffer || buffer >= c->buffers.size || c->buffers.a[buffer].deleted),
	        GL_INVALID_OPERATION);

	*data = c->buffers.a[buffer].data;
}

PGLDEF void pglGetTextureData(GLuint texture, GLvoid** data)
{
	// why'd you even call it?
	PGL_ERR(!data, GL_INVALID_VALUE);

	// TODO texture 0?
	PGL_ERR((texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);

	*data = c->textures.a[texture].data;
}

// TODO hmm, void*, or u8*, or GLvoid*?
GLvoid* pglGetBackBuffer(void)
{
	return c->back_buffer.buf;
}

// Assumes buf is the same size/shape as existing buffer (or at least
// sufficiently large to not cause problems
PGLDEF void pglSetBackBuffer(GLvoid* backbuf)
{
	int w = c->back_buffer.w;
	int h = c->back_buffer.h;
	c->back_buffer.buf = (u8*)backbuf;
	c->back_buffer.lastrow = c->back_buffer.buf + (h-1)*w*sizeof(pix_t);
}


// Not sure where else to put these two functions, they're helper/stopgap
// measures to deal with PGL only supporting RGBA but they're
// also useful functions on their own and not really "extensions"
// so I don't feel right putting them here or giving them a pgl prefix.
//
// Takes an image with GL_UNSIGNED_BYTE channels in
// a format other than packed GL_RGBA and returns it in (tightly packed) GL_RGBA
// (with the same rules as GLSL texture access for filling the other channels).
// See section 3.6.2 page 65 of the OpenGL ES 2.0.25 spec pdf
//
// IOW this creates an image that will give you the same values in the
// shader that you would have gotten had you used the unsupported
// format.  Passing in a GL_RGBA where pitch == w*4 reduces to a single memcpy
//
// If output is NULL, it will allocate the output image for you
// pitch is the length of a row in bytes.
//
// Returns the resulting packed RGBA image
PGLDEF u8* convert_format_to_packed_rgba(u8* output, u8* input, int w, int h, int pitch, GLenum format)
{
	int i, j, size = w*h;
	int rb = pitch;
	u8* out = output;
	if (!out) {
		out = (u8*)PGL_MALLOC(size*4);
	}
	memset(out, 0, size*4);

	u8* p = out;

	if (format == PGL_ONE_ALPHA) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = UINT8_MAX;
				p[1] = UINT8_MAX;
				p[2] = UINT8_MAX;
				p[3] = input[i*rb+j];
			}
		}
	} else if (format == GL_ALPHA) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[3] = input[i*rb+j];
			}
		}
	} else if (format == GL_LUMINANCE) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j];
				p[1] = input[i*rb+j];
				p[2] = input[i*rb+j];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_RED) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_LUMINANCE_ALPHA) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*2];
				p[1] = input[i*rb+j*2];
				p[2] = input[i*rb+j*2];
				p[3] = input[i*rb+j*2+1];
			}
		}
	} else if (format == GL_RG) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*2];
				p[1] = input[i*rb+j*2+1];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_RGB) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*3];
				p[1] = input[i*rb+j*3+1];
				p[2] = input[i*rb+j*3+2];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_BGR) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*3+2];
				p[1] = input[i*rb+j*3+1];
				p[2] = input[i*rb+j*3];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_BGRA) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*4+2];
				p[1] = input[i*rb+j*4+1];
				p[2] = input[i*rb+j*4];
				p[3] = input[i*rb+j*4+3];
			}
		}
	} else if (format == GL_RGBA) {
		if (pitch == w*4) {
			// Just a plain copy
			memcpy(out, input, w*h*4);
		} else {
			// get rid of row padding
			int bw = w*4;
			for (i=0; i<h; ++i) {
				memcpy(&out[i*bw], &input[i*rb], bw);
			}
		}
	} else {
		puts("Unrecognized or unsupported input format!");
		free(out);
		out = NULL;
	}
	return out;
}

// pass in packed single channel 8 bit image where background=0, foreground=255
// and get a packed 4-channel rgba image using the colors provided
PGLDEF u8* convert_grayscale_to_rgba(u8* input, int size, u32 bg_rgba, u32 text_rgba)
{
	float rb, gb, bb, ab, rt, gt, bt, at;

	u8* tmp = (u8*)&bg_rgba;
	rb = tmp[0];
	gb = tmp[1];
	bb = tmp[2];
	ab = tmp[3];

	tmp = (u8*)&text_rgba;
	rt = tmp[0];
	gt = tmp[1];
	bt = tmp[2];
	at = tmp[3];

	//printf("background = (%f, %f, %f, %f)\ntext = (%f, %f, %f, %f)\n", rb, gb, bb, ab, rt, gt, bt, at);

	u8* color_image = (u8*)PGL_MALLOC(size * 4);
	float t;
	for (int i=0; i<size; ++i) {
		t = (input[i] - 0) / 255.0;
		color_image[i*4] = rt * t + rb * (1 - t);
		color_image[i*4+1] = gt * t + gb * (1 - t);
		color_image[i*4+2] = bt * t + bb * (1 - t);
		color_image[i*4+3] = at * t + ab * (1 - t);
	}


	return color_image;
}


PGLDEF void put_pixel(Color color, int x, int y)
{
	//u32* dest = &((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	pix_t* dest = &((pix_t*)c->back_buffer.buf)[y*c->back_buffer.w + x];
	//*dest = (u32)color.a << PGL_ASHIFT | (u32)color.r << PGL_RSHIFT | (u32)color.g << PGL_GSHIFT | (u32)color.b << PGL_BSHIFT;
	*dest = RGBA_TO_PIXEL(color.r, color.g, color.b, color.a);
}

PGLDEF void put_pixel_blend(vec4 src, int x, int y)
{
	//u32* dest = &((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	u32* dest = &((u32*)c->back_buffer.buf)[y*c->back_buffer.w + x];

	//Color dest_color = make_Color((*dest & PGL_RMASK) >> PGL_RSHIFT, (*dest & PGL_GMASK) >> PGL_GSHIFT, (*dest & PGL_BMASK) >> PGL_BSHIFT, (*dest & PGL_AMASK) >> PGL_ASHIFT);
	Color dest_color = PIXEL_TO_COLOR(*dest);

	vec4 dst = Color_to_vec4(dest_color);

	// standard alpha blending xyzw = rgba
	vec4 final;
	final.x = src.x * src.w + dst.x * (1.0f - src.w);
	final.y = src.y * src.w + dst.y * (1.0f - src.w);
	final.z = src.z * src.w + dst.z * (1.0f - src.w);
	final.w = src.w + dst.w * (1.0f - src.w);

	Color color = vec4_to_Color(final);
	//*dest = (u32)color.a << PGL_ASHIFT | (u32)color.r << PGL_RSHIFT | (u32)color.g << PGL_GSHIFT | (u32)color.b << PGL_BSHIFT;
	*dest = RGBA_TO_PIXEL(color.r, color.g, color.b, color.a);
}

PGLDEF void put_wide_line_simple(Color the_color, float width, float x1, float y1, float x2, float y2)
{
	float tmp;

	//always draw from left to right
	if (x2 < x1) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	//calculate slope and implicit line parameters once
	float m = (y2-y1)/(x2-x1);
	Line line = make_Line(x1, y1, x2, y2);

	vec2 ab = make_vec2(line.A, line.B);
	normalize_vec2(&ab);

	int x, y;

	float x_min = MAX(0, MIN(x1, x2));
	float x_max = MIN(c->back_buffer.w-1, MAX(x1, x2));
	float y_min = MAX(0, MIN(y1, y2));
	float y_max = MIN(c->back_buffer.h-1, MAX(y1, y2));

	//4 cases based on slope
	if (m <= -1) {           //(-infinite, -1]
		x = x1;
		for (y=y_max; y>=y_min; --y) {
			for (float j=x-width/2; j<x+width/2; j++) {
				put_pixel(the_color, j, y);
			}
			if (line_func(&line, x+0.5f, y-1) < 0)
				x++;
		}
	} else if (m <= 0) {     //(-1, 0]
		y = y1;
		for (x=x_min; x<=x_max; ++x) {
			for (float j=y-width/2; j<y+width/2; j++) {
				put_pixel(the_color, x, j);
			}
			if (line_func(&line, x+1, y-0.5f) > 0)
				y--;
		}
	} else if (m <= 1) {     //(0, 1]
		y = y1;
		for (x=x_min; x<=x_max; ++x) {
			for (float j=y-width/2; j<y+width/2; j++) {
				put_pixel(the_color, x, j);
			}

			//put_pixel(the_color, x, y);
			if (line_func(&line, x+1, y+0.5f) < 0)
				y++;
		}

	} else {                 //(1, +infinite)
		x = x1;
		for (y=y_min; y<=y_max; ++y) {
			for (float j=x-width/2; j<x+width/2; j++) {
				put_pixel(the_color, j, y);
			}
			if (line_func(&line, x+0.5f, y+1) > 0)
				x++;
		}
	}
}

PGLDEF void put_wide_line(Color color1, Color color2, float width, float x1, float y1, float x2, float y2)
{
	vec2 a = { x1, y1 };
	vec2 b = { x2, y2 };
	vec2 tmp;
	Color tmpc;

	if (x2 < x1) {
		tmp = a;
		a = b;
		b = tmp;
		tmpc = color1;
		color1 = color2;
		color2 = tmpc;
	}

	vec4 c1 = Color_to_vec4(color1);
	vec4 c2 = Color_to_vec4(color2);

	// need half the width to calculate
	width /= 2.0f;

	float m = (y2-y1)/(x2-x1);
	Line line = make_Line(x1, y1, x2, y2);
	normalize_line(&line);
	vec2 c;

	vec2 ab = sub_vec2s(b, a);
	vec2 ac;

	float dot_abab = dot_vec2s(ab, ab);

	float x_min = floor(a.x - width) + 0.5f;
	float x_max = floor(b.x + width) + 0.5f;
	float y_min, y_max;
	if (m <= 0) {
		y_min = floor(b.y - width) + 0.5f;
		y_max = floor(a.y + width) + 0.5f;
	} else {
		y_min = floor(a.y - width) + 0.5f;
		y_max = floor(b.y + width) + 0.5f;
	}

	float x, y, e, dist, t;
	float w2 = width*width;
	//int last = 1;
	Color out_c;

	for (y = y_min; y <= y_max; ++y) {
		c.y = y;
		for (x = x_min; x <= x_max; x++) {
			// TODO optimize
			c.x = x;
			ac = sub_vec2s(c, a);
			e = dot_vec2s(ac, ab);
			
			// c lies past the ends of the segment ab
			if (e <= 0.0f || e >= dot_abab) {
				continue;
			}

			// can do this because we normalized the line equation
			// TODO square or fabsf?
			dist = line_func(&line, c.x, c.y);
			if (dist*dist < w2) {
				t = e / dot_abab;
				out_c = vec4_to_Color(mixf_vec4(c1, c2, t));
				put_pixel(out_c, x, y);
			}
		}
	}
}

//Should I have it take a glFramebuffer as paramater?
PGLDEF void put_line(Color the_color, float x1, float y1, float x2, float y2)
{
	float tmp;

	//always draw from left to right
	if (x2 < x1) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	//calculate slope and implicit line parameters once
	float m = (y2-y1)/(x2-x1);
	Line line = make_Line(x1, y1, x2, y2);

	int x, y;

	float x_min = MAX(0, MIN(x1, x2));
	float x_max = MIN(c->back_buffer.w-1, MAX(x1, x2));
	float y_min = MAX(0, MIN(y1, y2));
	float y_max = MIN(c->back_buffer.h-1, MAX(y1, y2));

	x_min = floorf(x_min) + 0.5f;
	x_max = floorf(x_max) + 0.5f;
	y_min = floorf(y_min) + 0.5f;
	y_max = floorf(y_max) + 0.5f;

	//4 cases based on slope
	if (m <= -1) {           //(-infinite, -1]
		x = x_min;
		for (y=y_max; y>=y_min; --y) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+0.5f, y-1) < 0)
				x++;
		}
	} else if (m <= 0) {     //(-1, 0]
		y = y_max;
		for (x=x_min; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+1, y-0.5f) > 0)
				y--;
		}
	} else if (m <= 1) {     //(0, 1]
		y = y_min;
		for (x=x_min; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+1, y+0.5f) < 0)
				y++;
		}

	} else {                 //(1, +infinite)
		x = x_min;
		for (y=y_min; y<=y_max; ++y) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+0.5f, y+1) > 0)
				x++;
		}
	}
}


// can't think of a better/cleaner way to do this than these lines
#define CLIP_TRIANGLE() \
	do { \
	x_min = MIN(p1.x, p2.x); \
	x_max = MAX(p1.x, p2.x); \
	y_min = MIN(p1.y, p2.y); \
	y_max = MAX(p1.y, p2.y); \
 \
	x_min = MIN(p3.x, x_min); \
	x_max = MAX(p3.x, x_max); \
	y_min = MIN(p3.y, y_min); \
	y_max = MAX(p3.y, y_max); \
 \
	x_min = MAX(c->lx, x_min); \
	x_max = MIN(c->ux, x_max); \
	y_min = MAX(c->ly, y_min); \
	y_max = MIN(c->uy, y_max); \
	} while (0)

#define MAKE_IMPLICIT_LINES() \
	do { \
	l12 = make_Line(p1.x, p1.y, p2.x, p2.y); \
	l23 = make_Line(p2.x, p2.y, p3.x, p3.y); \
	l31 = make_Line(p3.x, p3.y, p1.x, p1.y); \
	} while (0)

#define ANY_COLORS_NOT_WHITE(c) \
	(c0.r != 255 || c1.r != 255 || c2.r != 255 || \
	c0.g != 255 || c1.g != 255 || c2.g != 255 || \
	c0.b != 255 || c1.b != 255 || c2.b != 255)

PGLDEF void put_triangle_uniform(vec4 color, vec2 p1, vec2 p2, vec2 p3)
{
	float x_min,x_max,y_min,y_max;
	Line l12, l23, l31;
	float alpha, beta, gamma;

	CLIP_TRIANGLE();
	MAKE_IMPLICIT_LINES();

	x_min = floorf(x_min) + 0.5f;
	y_min = floorf(y_min) + 0.5f;

	for (float y=y_min; y<y_max; ++y) {
		for (float x=x_min; x<x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
				    (beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					// blend
					put_pixel_blend(color, x, y);
					//put_pixel(color, x, y);
				}
			}
		}
	}
}

PGLDEF void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3)
{
	float x_min,x_max,y_min,y_max;
	Line l12, l23, l31;
	float alpha, beta, gamma;
	Color col;
	col.a = 255; // hmm

	CLIP_TRIANGLE();
	MAKE_IMPLICIT_LINES();

	x_min = floorf(x_min) + 0.5f;
	y_min = floorf(y_min) + 0.5f;

	for (float y=y_min; y<y_max; ++y) {
		for (float x=x_min; x<x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
				    (beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					//calculate interoplation here
					col.r = alpha*c1.r + beta*c2.r + gamma*c3.r;
					col.g = alpha*c1.g + beta*c2.g + gamma*c3.g;
					col.b = alpha*c1.b + beta*c2.b + gamma*c3.b;
					//col.a = alpha*c1.a + beta*c2.a + gamma*c3.a;
					//put_pixel_blend(c, x, y);
					put_pixel(col, x, y);
				}
			}
		}
	}
}

PGLDEF void put_triangle_tex(int tex, vec2 uv1, vec2 uv2, vec2 uv3, vec2 p1, vec2 p2, vec2 p3)
{
	float x_min,x_max,y_min,y_max;
	Line l12, l23, l31;
	float alpha, beta, gamma;

	CLIP_TRIANGLE();
	MAKE_IMPLICIT_LINES();

#if 0
	print_vec2(p1, " p1\n");
	print_vec2(p2, " p2\n");
	print_vec2(p3, " p3\n");
	print_vec2(uv1, " uv1\n");
	print_vec2(uv2, " uv2\n");
	print_vec2(uv3, " uv3\n");
#endif

	x_min = floorf(x_min) + 0.5f;
	y_min = floorf(y_min) + 0.5f;
	vec2 uv;

	for (float y=y_min; y<y_max; ++y) {
		for (float x=x_min; x<x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
				    (beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					//calculate interoplation here
					uv = add_vec2s(scale_vec2(uv1, alpha), scale_vec2(uv2, beta));
					uv = add_vec2s(uv, scale_vec2(uv3, gamma));
					put_pixel_blend(texture2D(tex, uv.x, uv.y), x, y);
				}
			}
		}
	}
}

PGLDEF void put_triangle_tex_modulate(int tex, vec2 uv1, vec2 uv2, vec2 uv3, vec2 p1, vec2 p2, vec2 p3, Color c1, Color c2, Color c3)
{
	float x_min,x_max,y_min,y_max;
	Line l12, l23, l31;
	float alpha, beta, gamma;
	Color col;

	CLIP_TRIANGLE();
	MAKE_IMPLICIT_LINES();

#if 0
	print_vec2(p1, " p1\n");
	print_vec2(p2, " p2\n");
	print_vec2(p3, " p3\n");
	print_vec2(uv1, " uv1\n");
	print_vec2(uv2, " uv2\n");
	print_vec2(uv3, " uv3\n");
	print_Color(c1, " c1\n");
	print_Color(c2, " c2\n");
	print_Color(c3, " c3\n");
#endif

	x_min = floorf(x_min) + 0.5f;
	y_min = floorf(y_min) + 0.5f;
	vec2 uv;

	for (float y=y_min; y<y_max; ++y) {
		for (float x=x_min; x<x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
				    (beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					//calculate interoplation here
					uv = add_vec2s(scale_vec2(uv1, alpha), scale_vec2(uv2, beta));
					uv = add_vec2s(uv, scale_vec2(uv3, gamma));

					col.r = alpha*c1.r + beta*c2.r + gamma*c3.r;
					col.g = alpha*c1.g + beta*c2.g + gamma*c3.g;
					col.b = alpha*c1.b + beta*c2.b + gamma*c3.b;
					col.a = alpha*c1.a + beta*c2.a + gamma*c3.a;
					vec4 cv = Color_to_vec4(col);
					vec4 texcolor = texture2D(tex, uv.x, uv.y);
					
					put_pixel_blend(mult_vec4s(cv, texcolor), x, y);
				}
			}
		}
	}
}

#define COLOR_EQ(c1, c2) ((c1).r == (c2).r && (c1).g == (c2).g && (c1).b == (c2).b && (c1).a == (c2).a)


// TODO Color* or vec4*? float* for xy/uv or vec2*?
PGLDEF void pgl_draw_geometry_raw(int tex, const float* xy, int xy_stride, const Color* color, int color_stride, const float* uv, int uv_stride, int n_verts, const void* indices, int n_indices, int sz_indices)
{
	int i,j;
	float* x;
	float* u;
	int count = indices ? n_indices : n_verts;

	// TODO make PGL_INVALID_VALUE et all?
	PGL_ERR(!xy, GL_INVALID_VALUE);

	// Matching SDL_RenderGeometryRaw but I feel like they should be able to pass
	// NULL and just use the texture
	PGL_ERR(!color, GL_INVALID_VALUE);


	PGL_ERR(count % 3, GL_INVALID_VALUE);
	PGL_ERR(!(sz_indices==1 || sz_indices==2 || sz_indices==4), GL_INVALID_VALUE);

	if (n_verts < 3) return;

	PGL_ASSERT((PGL_MAX_VERTICES * GL_MAX_VERTEX_OUTPUT_COMPONENTS * sizeof(float))/sizeof(pgl_copy_data) >= (size_t)count);
	// Allow default texture 0?  many implementations return black (0,0,0,1) when sampling
	// tex 0
	if (tex > 0) {
		PGL_ERR(!uv, GL_INVALID_VALUE);

		PGL_ERR((tex >= c->textures.size || c->textures.a[tex].deleted), GL_INVALID_VALUE);

		PGL_ERR(c->textures.a[tex].type != GL_TEXTURE_2D-(GL_TEXTURE_UNBOUND+1), GL_INVALID_OPERATION);

		pgl_copy_data* verts = (pgl_copy_data*)&c->vs_output.output_buf[0];
		for (i=0; i<count; ++i) {
			if (sz_indices == 1) j = ((GLubyte*)indices)[i];
			else if (sz_indices == 2) j = ((GLushort*)indices)[i];
			else if (sz_indices == 4) j = ((GLuint*)indices)[i];
			else j = i;

			x = (float*)((u8*)xy + j*xy_stride);
			u = (float*)((u8*)uv + j*uv_stride);

			verts[i].c = *(Color*)((u8*)color + j*color_stride);

			// TODO convert to ints here for efficiency or leave floats for
			// flexibility/subpixel accuracy?
			verts[i].src.x = u[0];
			verts[i].src.y = u[1];

			// scale?
			verts[i].dst.x = x[0];
			verts[i].dst.y = x[1];
		}

		vec4 tex_color;
		pgl_copy_data* p = verts;
		int is_uniform = GL_FALSE;
		int has_modulation = GL_FALSE;
		int tex_uniform = GL_FALSE;
		for (i=0; i<count; i+=3, p+=3) {
			is_uniform = (COLOR_EQ(p[0].c, p[1].c) && COLOR_EQ(p[1].c, p[2].c));
			if (is_uniform) {
				has_modulation = (p[0].c.r != 255 || p[0].c.g != 255 || p[0].c.b != 255 || p[0].c.a != 255);
			} else {
				has_modulation = GL_TRUE;
			}
			tex_uniform = (equal_vec2s(p[0].src, p[1].src) && equal_vec2s(p[1].src, p[2].src));
			if (tex_uniform) tex_color = texture2D(tex, p[0].src.x, p[0].src.y);

			if (has_modulation) {
				if (is_uniform) {
					if (tex_uniform) {
						// uniform color triangle, likely uniform color rect
						vec4 color = mult_vec4s(tex_color, Color_to_vec4(p[0].c));
						put_triangle_uniform(color, p[0].dst, p[1].dst, p[2].dst);
					} else {
						// need another variant that takes a single color so only
						// interpolates uv
						put_triangle_tex_modulate(tex, p[0].src, p[1].src, p[2].src, p[0].dst, p[1].dst, p[2].dst, p[0].c, p[1].c, p[2].c);
					}
				} else {
					put_triangle_tex_modulate(tex, p[0].src, p[1].src, p[2].src, p[0].dst, p[1].dst, p[2].dst, p[0].c, p[1].c, p[2].c);
				}
			} else {
				if (tex_uniform) {
					// uniform color triangle, likely uniform color rect
					put_triangle_uniform(tex_color, p[0].dst, p[1].dst, p[2].dst);
				} else {
					put_triangle_tex(tex, p[0].src, p[1].src, p[2].src, p[0].dst, p[1].dst, p[2].dst);
				}
			}
		}
	} else {
		pgl_fill_data* verts = (pgl_fill_data*)&c->vs_output.output_buf[0];
		for (i=0; i<count; ++i) {
			if (sz_indices == 1) j = ((GLubyte*)indices)[i];
			else if (sz_indices == 2) j = ((GLushort*)indices)[i];
			else if (sz_indices == 4) j = ((GLuint*)indices)[i];
			else j = i;

			x = (float*)((u8*)xy + j*xy_stride);
			verts[i].c = *(Color*)((u8*)color + j*color_stride);

			// scale?
			verts[i].dst.x = x[0];
			verts[i].dst.y = x[1];
		}

		pgl_fill_data* p = verts;
		for (i=0; i<count; i+=3, p+=3) {
			put_triangle(p[0].c, p[1].c, p[2].c, p[0].dst, p[1].dst, p[2].dst);
		}
	}
}



#define plot(X,Y,D) do{ c.w = (D); put_pixel_blend(c, X, Y); } while (0)

#define ipart_(X) ((int)(X))
#define round_(X) ((int)(((float)(X))+0.5f))
#define fpart_(X) (((float)(X))-(float)ipart_(X))
#define rfpart_(X) (1.0f-fpart_(X))

#define swap_(a, b) do{ __typeof__(a) tmp;  tmp = a; a = b; b = tmp; } while(0)
PGLDEF void put_aa_line(vec4 c, float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	if (fabs(dx) > fabs(dy)) {
		if (x2 < x1) {
			swap_(x1, x2);
			swap_(y1, y2);
		}
		float gradient = dy / dx;
		float xend = round_(x1);
		float yend = y1 + gradient*(xend - x1);
		float xgap = rfpart_(x1 + 0.5);
		int xpxl1 = xend;
		int ypxl1 = ipart_(yend);
		plot(xpxl1, ypxl1, rfpart_(yend)*xgap);
		plot(xpxl1, ypxl1+1, fpart_(yend)*xgap);
		printf("xgap = %f\n", xgap);
		printf("%f %f\n", rfpart_(yend), fpart_(yend));
		printf("%f %f\n", rfpart_(yend)*xgap, fpart_(yend)*xgap);
		float intery = yend + gradient;

		xend = round_(x2);
		yend = y2 + gradient*(xend - x2);
		xgap = fpart_(x2+0.5);
		int xpxl2 = xend;
		int ypxl2 = ipart_(yend);
		plot(xpxl2, ypxl2, rfpart_(yend) * xgap);
		plot(xpxl2, ypxl2 + 1, fpart_(yend) * xgap);

		int x;
		for(x=xpxl1+1; x < xpxl2; x++) {
			plot(x, ipart_(intery), rfpart_(intery));
			plot(x, ipart_(intery) + 1, fpart_(intery));
			intery += gradient;
		}
	} else {
		if ( y2 < y1 ) {
			swap_(x1, x2);
			swap_(y1, y2);
		}
		float gradient = dx / dy;
		float yend = round_(y1);
		float xend = x1 + gradient*(yend - y1);
		float ygap = rfpart_(y1 + 0.5);
		int ypxl1 = yend;
		int xpxl1 = ipart_(xend);
		plot(xpxl1, ypxl1, rfpart_(xend)*ygap);
		plot(xpxl1 + 1, ypxl1, fpart_(xend)*ygap);
		float interx = xend + gradient;

		yend = round_(y2);
		xend = x2 + gradient*(yend - y2);
		ygap = fpart_(y2+0.5);
		int ypxl2 = yend;
		int xpxl2 = ipart_(xend);
		plot(xpxl2, ypxl2, rfpart_(xend) * ygap);
		plot(xpxl2 + 1, ypxl2, fpart_(xend) * ygap);

		int y;
		for(y=ypxl1+1; y < ypxl2; y++) {
			plot(ipart_(interx), y, rfpart_(interx));
			plot(ipart_(interx) + 1, y, fpart_(interx));
			interx += gradient;
		}
	}
}


PGLDEF void put_aa_line_interp(vec4 c1, vec4 c2, float x1, float y1, float x2, float y2)
{
	vec4 c;
	float t;

	float dx = x2 - x1;
	float dy = y2 - y1;

	if (fabs(dx) > fabs(dy)) {
		if (x2 < x1) {
			swap_(x1, x2);
			swap_(y1, y2);
			swap_(c1, c2);
		}

		vec2 p1 = { x1, y1 }, p2 = { x2, y2 };
		vec2 pr, sub_p2p1 = sub_vec2s(p2, p1);
		float line_length_squared = length_vec2(sub_p2p1);
		line_length_squared *= line_length_squared;

		c = c1;

		float gradient = dy / dx;
		float xend = round_(x1);
		float yend = y1 + gradient*(xend - x1);
		float xgap = rfpart_(x1 + 0.5);
		int xpxl1 = xend;
		int ypxl1 = ipart_(yend);
		plot(xpxl1, ypxl1, rfpart_(yend)*xgap);
		plot(xpxl1, ypxl1+1, fpart_(yend)*xgap);
		printf("xgap = %f\n", xgap);
		printf("%f %f\n", rfpart_(yend), fpart_(yend));
		printf("%f %f\n", rfpart_(yend)*xgap, fpart_(yend)*xgap);
		float intery = yend + gradient;

		c = c2;
		xend = round_(x2);
		yend = y2 + gradient*(xend - x2);
		xgap = fpart_(x2+0.5);
		int xpxl2 = xend;
		int ypxl2 = ipart_(yend);
		plot(xpxl2, ypxl2, rfpart_(yend) * xgap);
		plot(xpxl2, ypxl2 + 1, fpart_(yend) * xgap);

		int x;
		for(x=xpxl1+1; x < xpxl2; x++) {
			pr.x = x;
			pr.y = intery;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;
			c = mixf_vec4(c1, c2, t);

			plot(x, ipart_(intery), rfpart_(intery));
			plot(x, ipart_(intery) + 1, fpart_(intery));
			intery += gradient;
		}
	} else {
		if ( y2 < y1 ) {
			swap_(x1, x2);
			swap_(y1, y2);
			swap_(c1, c2);
		}

		vec2 p1 = { x1, y1 }, p2 = { x2, y2 };
		vec2 pr, sub_p2p1 = sub_vec2s(p2, p1);
		float line_length_squared = length_vec2(sub_p2p1);
		line_length_squared *= line_length_squared;

		c = c1;

		float gradient = dx / dy;
		float yend = round_(y1);
		float xend = x1 + gradient*(yend - y1);
		float ygap = rfpart_(y1 + 0.5);
		int ypxl1 = yend;
		int xpxl1 = ipart_(xend);
		plot(xpxl1, ypxl1, rfpart_(xend)*ygap);
		plot(xpxl1 + 1, ypxl1, fpart_(xend)*ygap);
		float interx = xend + gradient;


		c = c2;
		yend = round_(y2);
		xend = x2 + gradient*(yend - y2);
		ygap = fpart_(y2+0.5);
		int ypxl2 = yend;
		int xpxl2 = ipart_(xend);
		plot(xpxl2, ypxl2, rfpart_(xend) * ygap);
		plot(xpxl2 + 1, ypxl2, fpart_(xend) * ygap);

		int y;
		for(y=ypxl1+1; y < ypxl2; y++) {
			pr.x = interx;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;
			c = mixf_vec4(c1, c2, t);

			plot(ipart_(interx), y, rfpart_(interx));
			plot(ipart_(interx) + 1, y, fpart_(interx));
			interx += gradient;
		}
	}
}


#undef swap_
#undef plot
#undef ipart_
#undef fpart_
#undef round_
#undef rfpart_


