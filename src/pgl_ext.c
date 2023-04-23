
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
void pglClearScreen()
{
	memset(c->back_buffer.buf, 255, c->back_buffer.w * c->back_buffer.h * 4);
}

void pglSetInterp(GLsizei n, GLenum* interpolation)
{
	c->programs.a[c->cur_program].vs_output_size = n;
	c->vs_output.size = n;

	memcpy(c->programs.a[c->cur_program].interpolation, interpolation, n*sizeof(GLenum));
	cvec_reserve_float(&c->vs_output.output_buf, n * MAX_VERTICES);

	//vs_output.interpolation would be already pointing at current program's array
	//unless the programs array was realloced since the last glUseProgram because
	//they've created a bunch of programs.  Unlikely they'd be changing a shader
	//before creating all their shaders but whatever.
	c->vs_output.interpolation = c->programs.a[c->cur_program].interpolation;
}




//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
void pglDrawFrame()
{
	frag_func frag_shader = c->programs.a[c->cur_program].fragment_shader;

	Shader_Builtins builtins;
	int y;
	#pragma omp parallel for private(builtins)
	for (y=0; y<c->back_buffer.h; ++y) {
		for (int x=0; x<c->back_buffer.w; ++x) {

			//ignore z and w components
			builtins.gl_FragCoord.x = x + 0.5f;
			builtins.gl_FragCoord.y = y + 0.5f;

			builtins.discard = GL_FALSE;
			frag_shader(NULL, &builtins, c->programs.a[c->cur_program].uniform);
			if (!builtins.discard)
				draw_pixel(builtins.gl_FragColor, x, y, 0.0f);  //depth isn't used for pglDrawFrame
		}
	}

}

void pglBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage)
{
	if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//check for usage later

	target -= GL_ARRAY_BUFFER;
	if (c->bound_buffers[target] == 0) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	// data can't be null for user_owned data
	if (!data) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

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


void pglTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	if (target != GL_TEXTURE_1D) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (border) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	// data can't be null for user_owned data
	if (!data) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	//ignore level for now

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	c->textures.a[cur_tex].w = width;

	if (type != GL_UNSIGNED_BYTE) {

		return;
	}

	int components;
	if (format == GL_RED) components = 1;
	else if (format == GL_RG) components = 2;
	else if (format == GL_RGB || format == GL_BGR) components = 3;
	else if (format == GL_RGBA || format == GL_BGRA) components = 4;
	else {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	// TODO see pglBufferData
	if (!c->textures.a[cur_tex].user_owned)
		free(c->textures.a[cur_tex].data);

	//TODO support other internal formats? components should be of internalformat not format
	c->textures.a[cur_tex].data = (u8*)data;
	c->textures.a[cur_tex].user_owned = GL_TRUE;

	//TODO
	//assume for now always RGBA coming in and that's what I'm storing it as
}

void pglTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	//GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_RECTANGLE, or GL_TEXTURE_CUBE_MAP.
	//will add others as they're implemented
	if (target != GL_TEXTURE_2D &&
	    target != GL_TEXTURE_RECTANGLE &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (border) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	// data can't be null for user_owned data
	if (!data) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	//ignore level for now

	//TODO support other types?
	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	// TODO I don't actually support anything other than GL_RGBA for input or
	// internal format ... so I should probably make the others errors and
	// I'm not even checking internalFormat currently..
	int components;
	if (format == GL_RED) components = 1;
	else if (format == GL_RG) components = 2;
	else if (format == GL_RGB || format == GL_BGR) components = 3;
	else if (format == GL_RGBA || format == GL_BGRA) components = 4;
	else {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	int cur_tex;

	if (target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE) {
		cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

		c->textures.a[cur_tex].w = width;
		c->textures.a[cur_tex].h = height;


		// TODO see pglBufferData
		if (!c->textures.a[cur_tex].user_owned)
			free(c->textures.a[cur_tex].data);

		//TODO support other internal formats? components should be of internalformat not format
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

		if (width != height) {
			//TODO spec says INVALID_VALUE, man pages say INVALID_ENUM ?
			if (!c->error)
				c->error = GL_INVALID_VALUE;
			return;
		}

		int mem_size = width*height*6 * components;
		if (c->textures.a[cur_tex].w == 0) {
			c->textures.a[cur_tex].w = width;
			c->textures.a[cur_tex].h = width; //same cause square

		} else if (c->textures.a[cur_tex].w != width) {
			//TODO spec doesn't say all sides must have same dimensions but it makes sense
			//and this site suggests it http://www.opengl.org/wiki/Cubemap_Texture
			if (!c->error)
				c->error = GL_INVALID_VALUE;
			return;
		}

		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X; //use target as plane index

		c->textures.a[cur_tex].data = (u8*)data;
		c->textures.a[cur_tex].user_owned = GL_TRUE;
		*/

	} //end CUBE_MAP
}

void pglTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (border) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	// data can't be null for user_owned data
	if (!data) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	//ignore level for now

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	c->textures.a[cur_tex].w = width;
	c->textures.a[cur_tex].h = height;
	c->textures.a[cur_tex].d = depth;

	if (type != GL_UNSIGNED_BYTE) {
		// TODO
		return;
	}

	// TODO add error?  only support GL_RGBA for now
	int components;
	if (format == GL_RED) components = 1;
	else if (format == GL_RG) components = 2;
	else if (format == GL_RGB || format == GL_BGR) components = 3;
	else if (format == GL_RGBA || format == GL_BGRA) components = 4;
	else {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	// TODO see pglBufferData
	if (!c->textures.a[cur_tex].user_owned)
		free(c->textures.a[cur_tex].data);

	//TODO support other internal formats? components should be of internalformat not format
	c->textures.a[cur_tex].data = (u8*)data;
	c->textures.a[cur_tex].user_owned = GL_TRUE;

	//TODO
	//assume for now always RGBA coming in and that's what I'm storing it as
}


void pglGetBufferData(GLuint buffer, GLvoid** data)
{
	// why'd you even call it?
	if (!data) {
		if (!c->error) {
			c->error = GL_INVALID_VALUE;
		}
		return;
	}

	if (buffer && buffer < c->buffers.size && !c->buffers.a[buffer].deleted) {
		*data = c->buffers.a[buffer].data;
	} else if (!c->error) {
		c->error = GL_INVALID_OPERATION; // matching error code of binding invalid buffer
	}
}

void pglGetTextureData(GLuint texture, GLvoid** data)
{
	// why'd you even call it?
	if (!data) {
		if (!c->error) {
			c->error = GL_INVALID_VALUE;
		}
		return;
	}

	if (texture < c->textures.size && !c->textures.a[texture].deleted) {
		*data = c->textures.a[texture].data;
	} else if (!c->error) {
		c->error = GL_INVALID_OPERATION; // matching error code of binding invalid buffer
	}
}


void put_pixel(Color color, int x, int y)
{
	u32* dest = &((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	*dest = color.a << c->Ashift | color.r << c->Rshift | color.g << c->Gshift | color.b << c->Bshift;
}

void put_wide_line_simple(Color the_color, float width, float x1, float y1, float x2, float y2)
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

// TODO add variable width version?  Or programmable width, user function?
void put_wide_line2(Color the_color, float width, float x1, float y1, float x2, float y2)
{
	if (width < 1.5f) {
		put_line(the_color, x1, y1, x2, y2);
		return;
	}
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
	ab = scale_vec2(ab, width/2.0f);

	float x, y;

	float x_min = MAX(0, x1);
	float x_max = MIN(c->back_buffer.w-1, MAX(x1, x2));
	float y_min = MAX(0, MIN(y1, y2));
	float y_max = MIN(c->back_buffer.h-1, MAX(y1, y2));

	// use pixel centers at 0.5f to match OpenGL line drawing
	x_min += 0.5f;
	x_max += 0.5f;
	y_min += 0.5f;
	y_max += 0.5f;

	int diag;

	//4 cases based on slope
	if (m <= -1) {           //(-infinite, -1]
		x = x1;
		for (y=y_max; y>=y_min; --y) {
			diag = put_line(the_color, x-ab.x, y-ab.y, x+ab.x, y+ab.y);
			if (line_func(&line, x+0.5f, y-1) < 0) {
				if (diag) {
					put_line(the_color, x-ab.x, y-1-ab.y, x+ab.x, y-1+ab.y);
				}
				x++;
			}
		}
	} else if (m <= 0) {     //(-1, 0]
		y = y1;
		for (x=x_min; x<=x_max; ++x) {
			diag = put_line(the_color, x-ab.x, y-ab.y, x+ab.x, y+ab.y);
			if (line_func(&line, x+1, y-0.5f) > 0) {
				if (diag) {
					put_line(the_color, x+1-ab.x, y-ab.y, x+1+ab.x, y+ab.y);
				}
				y--;
			}
		}
	} else if (m <= 1) {     //(0, 1]
		y = y1;
		for (x=x_min; x<=x_max; ++x) {
			diag = put_line(the_color, x-ab.x, y-ab.y, x+ab.x, y+ab.y);
			if (line_func(&line, x+1, y+0.5f) < 0) {
				if (diag) {
					put_line(the_color, x+1-ab.x, y-ab.y, x+1+ab.x, y+ab.y);
				}
				y++;
			}
		}

	} else {                 //(1, +infinite)
		x = x1;
		for (y=y_min; y<=y_max; ++y) {
			diag = put_line(the_color, x-ab.x, y-ab.y, x+ab.x, y+ab.y);
			if (line_func(&line, x+0.5f, y+1) > 0) {
				if (diag) {
					put_line(the_color, x-ab.x, y+1-ab.y, x+ab.x, y+1+ab.y);
				}
				x++;
			}
		}
	}
}

//Should I have it take a glFramebuffer as paramater?
int put_line(Color the_color, float x1, float y1, float x2, float y2)
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

	int first_is_diag = GL_FALSE;

	//4 cases based on slope
	if (m <= -1) {           //(-infinite, -1]
		x = x1;
		put_pixel(the_color, x, y_max);
		if (line_func(&line, x+0.5f, y-1) < 0) {
			x++;
			first_is_diag = GL_TRUE;
		}
		for (y=y_max-1; y>=y_min; --y) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+0.5f, y-1) < 0)
				x++;
		}
	} else if (m <= 0) {     //(-1, 0]
		y = y1;
		put_pixel(the_color, x_min, y);
		if (line_func(&line, x+1, y-0.5f) > 0) {
			y--;
			first_is_diag = GL_TRUE;
		}
		for (x=x_min+1; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+1, y-0.5f) > 0)
				y--;
		}
	} else if (m <= 1) {     //(0, 1]
		y = y1;
		put_pixel(the_color, x_min, y);
		if (line_func(&line, x+1, y+0.5f) < 0) {
			y++;
			first_is_diag = GL_TRUE;
		}
		for (x=x_min+1; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+1, y+0.5f) < 0)
				y++;
		}

	} else {                 //(1, +infinite)
		x = x1;
		put_pixel(the_color, x, y_min);
		if (line_func(&line, x+0.5f, y+1) > 0) {
			x++;
			first_is_diag = GL_TRUE;
		}
		for (y=y_min+1; y<=y_max; ++y) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+0.5f, y+1) > 0)
				x++;
		}
	}

	return first_is_diag;
}

void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3)
{
	//can't think of a better/cleaner way to do this than these 8 lines
	float x_min = MIN(floor(p1.x), floor(p2.x));
	float x_max = MAX(ceil(p1.x), ceil(p2.x));
	float y_min = MIN(floor(p1.y), floor(p2.y));
	float y_max = MAX(ceil(p1.y), ceil(p2.y));

	x_min = MIN(floor(p3.x), x_min);
	x_max = MAX(ceil(p3.x),  x_max);
	y_min = MIN(floor(p3.y), y_min);
	y_max = MAX(ceil(p3.y),  y_max);

	x_min = MAX(0, x_min);
	x_max = MIN(c->back_buffer.w-1, x_max);
	y_min = MAX(0, y_min);
	y_max = MIN(c->back_buffer.h-1, y_max);

	//form implicit lines
	Line l12 = make_Line(p1.x, p1.y, p2.x, p2.y);
	Line l23 = make_Line(p2.x, p2.y, p3.x, p3.y);
	Line l31 = make_Line(p3.x, p3.y, p1.x, p1.y);

	float alpha, beta, gamma;
	Color c;

	float x, y;
	//y += 0.5f; //center of pixel

	// TODO(rswinkle): floor(  + 0.5f) like draw_triangle?
	for (y=y_min; y<=y_max; ++y) {
		for (x=x_min; x<=x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0)
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
				    (beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					//calculate interoplation here
						c.r = alpha*c1.r + beta*c2.r + gamma*c3.r;
						c.g = alpha*c1.g + beta*c2.g + gamma*c3.g;
						c.b = alpha*c1.b + beta*c2.b + gamma*c3.b;
						put_pixel(c, x, y);
				}
		}
	}
}


