
static glContext* c;

static Color blend_pixel(vec4 src, vec4 dst);
static int fragment_processing(int x, int y, float z);
static void draw_pixel_vec2(vec4 cf, vec2 pos, float z);
static void draw_pixel(vec4 cf, int x, int y, float z, int do_frag_processing);
static void run_pipeline(GLenum mode, GLuint first, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements);

static float calc_poly_offset(vec3 hp0, vec3 hp1, vec3 hp2);

static void draw_triangle_clip(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke, int clip_bit);
static void draw_triangle_point(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_line(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_fill(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_final(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);
static void draw_triangle(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);

static void draw_line_clip(glVertex* v1, glVertex* v2);
static void draw_line_shader(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset);
static void draw_thick_line_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke);

/* this clip epsilon is needed to avoid some rounding errors after
   several clipping stages */

#define CLIP_EPSILON (1E-5)
#define CLIPZ_MASK 0x3
#define CLIPXY_TEST(x, y) (x >= c->lx && x < c->ux && y >= c->ly && y < c->uy)


static inline int gl_clipcode(vec4 pt)
{
	float w;

	w = pt.w * (1.0 + CLIP_EPSILON);
	return
		(((pt.z < -w) |
		 ((pt.z >  w) << 1)) &
		 (!c->depth_clamp |
		  !c->depth_clamp << 1)) |

		((pt.x < -w) << 2) |
		((pt.x >  w) << 3) |
		((pt.y < -w) << 4) |
		((pt.y >  w) << 5);

}




static int is_front_facing(glVertex* v0, glVertex* v1, glVertex* v2)
{
	//according to docs culling is done based on window coordinates
	//See page 3.6.1 page 116 of glspec33.core for more on rasterization, culling etc.
	//
	//TODO See if there's a way to determine front facing before
	// clipping the near plane (vertex behind the eye seems to mess
	// up winding).  If yes, can refactor to cull early and handle
	// line and point modes separately
	vec3 p0 = vec4_to_vec3h(v0->screen_space);
	vec3 p1 = vec4_to_vec3h(v1->screen_space);
	vec3 p2 = vec4_to_vec3h(v2->screen_space);

	float a;

	//method from spec
	a = p0.x*p1.y - p1.x*p0.y + p1.x*p2.y - p2.x*p1.y + p2.x*p0.y - p0.x*p2.y;
	//a /= 2;

	if (c->front_face == GL_CW) {
		a = -a;
	}

	if (a <= 0) {
		return 0;
	}

	return 1;
}


static void do_vertex(glVertex_Attrib* v, int* enabled, unsigned int num_enabled, unsigned int i, unsigned int vert)
{
	GLuint buf;
	u8* buf_pos;
	vec4 tmpvec4;

	// copy/prep vertex attributes from buffers into appropriate positions for vertex shader to access
	for (int j=0; j<num_enabled; ++j) {
		buf = v[enabled[j]].buf;

		buf_pos = (u8*)c->buffers.a[buf].data + v[enabled[j]].offset + v[enabled[j]].stride*i;

		SET_VEC4(tmpvec4, 0.0f, 0.0f, 0.0f, 1.0f);
		memcpy(&tmpvec4, buf_pos, sizeof(float)*v[enabled[j]].size);

		c->vertex_attribs_vs[enabled[j]] = tmpvec4;
	}

	float* vs_out = &c->vs_output.output_buf.a[vert*c->vs_output.size];
	c->programs.a[c->cur_program].vertex_shader(vs_out, c->vertex_attribs_vs, &c->builtins, c->programs.a[c->cur_program].uniform);

	c->glverts.a[vert].vs_out = vs_out;
	c->glverts.a[vert].clip_space = c->builtins.gl_Position;

	// no use setting here because of TRIANGLE_STRIP
	// and TRIANGLE_FAN. While I don't properly
	// generate "primitives", I do expand create unique vertices
	// to process when the user uses an element (index) buffer.
	// Strip
	//
	// so it's done in draw_triangle()
	//c->glverts.a[vert].edge_flag = 1;

	c->glverts.a[vert].clip_code = gl_clipcode(c->builtins.gl_Position);
}


static void vertex_stage(GLuint first, GLsizei count, GLsizei instance_id, GLuint base_instance, GLboolean use_elements)
{
	unsigned int i, j, vert, num_enabled;
	u8* buf_pos;

	//save checking if enabled on every loop if we build this first
	//also initialize the vertex_attrib space
	vec4 vec4_init = { 0.0f, 0.0f, 0.0f, 1.0f };
	int enabled[GL_MAX_VERTEX_ATTRIBS] = { 0 };
	glVertex_Attrib* v = c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs;
	GLuint elem_buffer = c->vertex_arrays.a[c->cur_vertex_array].element_buffer;

	for (i=0, j=0; i<GL_MAX_VERTEX_ATTRIBS; ++i) {
		if (v[i].enabled) {
			if (v[i].divisor == 0) {
				// no need to set to defalt vector here because it's handled in do_vertex()
				enabled[j++] = i;
			} else if (!(instance_id % v[i].divisor)) {   //set instanced attributes if necessary
				// only reset to default vector right before updating, because
				// it has to stay the same across multiple instances for divisors > 1
				memcpy(&c->vertex_attribs_vs[i], &vec4_init, sizeof(vec4));

				int n = instance_id/v[i].divisor + base_instance;
				buf_pos = (u8*)c->buffers.a[v[i].buf].data + v[i].offset + v[i].stride*n;

				memcpy(&c->vertex_attribs_vs[i], buf_pos, sizeof(float)*v[i].size);
			}
		}
	}
	num_enabled = j;

	cvec_reserve_glVertex(&c->glverts, count);
	c->builtins.gl_InstanceID = instance_id;

	if (!use_elements) {
		for (vert=0, i=first; i<first+count; ++i, ++vert) {
			do_vertex(v, enabled, num_enabled, i, vert);
		}
	} else {
		GLuint* uint_array = (GLuint*) c->buffers.a[elem_buffer].data;
		GLushort* ushort_array = (GLushort*) c->buffers.a[elem_buffer].data;
		GLubyte* ubyte_array = (GLubyte*) c->buffers.a[elem_buffer].data;
		if (c->buffers.a[elem_buffer].type == GL_UNSIGNED_BYTE) {
			for (vert=0, i=first; i<first+count; ++i, ++vert) {
				do_vertex(v, enabled, num_enabled, ubyte_array[i], vert);
			}
		} else if (c->buffers.a[elem_buffer].type == GL_UNSIGNED_SHORT) {
			for (vert=0, i=first; i<first+count; ++i, ++vert) {
				do_vertex(v, enabled, num_enabled, ushort_array[i], vert);
			}
		} else {
			for (vert=0, i=first; i<first+count; ++i, ++vert) {
				do_vertex(v, enabled, num_enabled, uint_array[i], vert);
			}
		}
	}
}


//TODO make fs_input static?  or a member of glContext?
static void draw_point(glVertex* vert, float poly_offset)
{
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	vec3 point = vec4_to_vec3h(vert->screen_space);
	point.z += poly_offset; // couldn't this put it outside of [-1,1]?
	point.z = MAP(point.z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	// TODO necessary for non-perspective?
	//if (c->depth_clamp)
	//	clampf(point.z, c->depth_range_near, c->depth_range_far);

	Shader_Builtins builtins;
	// spec pg 110 r,q are supposed to be replaced with 0 and 1...but PointCoord is a vec2
	// not worth making it a vec4 for something unlikely to be used
	//builtins.gl_PointCoord.z = 0;
	//builtins.gl_PointCoord.w = 1;
	int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;

	//TODO why not just pass vs_output directly?  hmmm...
	memcpy(fs_input, vert->vs_out, c->vs_output.size*sizeof(float));

	//accounting for pixel centers at 0.5, using truncation
	float x = point.x + 0.5f;
	float y = point.y + 0.5f;
	float p_size = c->point_size;
	float origin = (c->point_spr_origin == GL_UPPER_LEFT) ? -1.0f : 1.0f;
	// NOTE/TODO, According to the spec if the clip coordinate, ie the
	// center of the point is outside the clip volume, you're supposed to
	// clip the whole thing, but some vendors don't do that because it's
	// not what most people want.

	// Can easily clip whole point when point size <= 1
	if (p_size <= 1.0f) {
		if (x < c->lx || y < c->ly || x >= c->ux || y >= c->uy)
			return;
	}

	for (float i = y-p_size/2; i<y+p_size/2; ++i) {
		if (i < c->ly || i >= c->uy)
			continue;

		for (float j = x-p_size/2; j<x+p_size/2; ++j) {

			if (j < c->lx || j >= c->ux)
				continue;

			if (!fragdepth_or_discard && !fragment_processing(j, i, point.z)) {
				continue;
			}

			// per page 110 of 3.3 spec (x,y are s,t)
			builtins.gl_PointCoord.x = 0.5f + ((int)j + 0.5f - point.x)/p_size;
			builtins.gl_PointCoord.y = 0.5f + origin * ((int)i + 0.5f - point.y)/p_size;

			SET_VEC4(builtins.gl_FragCoord, j, i, point.z, 1/vert->screen_space.w);
			builtins.discard = GL_FALSE;
			builtins.gl_FragDepth = point.z;
			c->programs.a[c->cur_program].fragment_shader(fs_input, &builtins, c->programs.a[c->cur_program].uniform);
			if (!builtins.discard)
				draw_pixel(builtins.gl_FragColor, j, i, builtins.gl_FragDepth, fragdepth_or_discard);
		}
	}
}

static void run_pipeline(GLenum mode, GLuint first, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements)
{
	unsigned int i, vert;
	int provoke;

	PGL_ASSERT(count <= MAX_VERTICES);

	vertex_stage(first, count, instance, base_instance, use_elements);

	//fragment portion
	if (mode == GL_POINTS) {
		for (vert=0, i=first; i<first+count; ++i, ++vert) {
			// clip only z and let partial points (size > 1)
			// show even if the center would have been clipped
			if (c->glverts.a[vert].clip_code & CLIPZ_MASK)
				continue;

			c->glverts.a[vert].screen_space = mult_mat4_vec4(c->vp_mat, c->glverts.a[vert].clip_space);

			draw_point(&c->glverts.a[vert], 0.0f);
		}
	} else if (mode == GL_LINES) {
		for (vert=0, i=first; i<first+count-1; i+=2, vert+=2) {
			draw_line_clip(&c->glverts.a[vert], &c->glverts.a[vert+1]);
		}
	} else if (mode == GL_LINE_STRIP) {
		for (vert=0, i=first; i<first+count-1; i++, vert++) {
			draw_line_clip(&c->glverts.a[vert], &c->glverts.a[vert+1]);
		}
	} else if (mode == GL_LINE_LOOP) {
		for (vert=0, i=first; i<first+count-1; i++, vert++) {
			draw_line_clip(&c->glverts.a[vert], &c->glverts.a[vert+1]);
		}
		//draw ending line from last to first point
		draw_line_clip(&c->glverts.a[count-1], &c->glverts.a[0]);

	} else if (mode == GL_TRIANGLES) {
		provoke = (c->provoking_vert == GL_LAST_VERTEX_CONVENTION) ? 2 : 0;

		for (vert=0, i=first; i<first+count-2; i+=3, vert+=3) {
			draw_triangle(&c->glverts.a[vert], &c->glverts.a[vert+1], &c->glverts.a[vert+2], vert+provoke);
		}

	} else if (mode == GL_TRIANGLE_STRIP) {
		unsigned int a=0, b=1, toggle = 0;
		provoke = (c->provoking_vert == GL_LAST_VERTEX_CONVENTION) ? 0 : -2;

		for (vert=2; vert<count; ++vert) {
			draw_triangle(&c->glverts.a[a], &c->glverts.a[b], &c->glverts.a[vert], vert+provoke);

			if (!toggle)
				a = vert;
			else
				b = vert;

			toggle = !toggle;
		}
	} else if (mode == GL_TRIANGLE_FAN) {
		provoke = (c->provoking_vert == GL_LAST_VERTEX_CONVENTION) ? 0 : -1;

		for (vert=2; vert<count; ++vert) {
			draw_triangle(&c->glverts.a[0], &c->glverts.a[vert-1], &c->glverts.a[vert], vert+provoke);
		}
	}
}


static int depthtest(float zval, float zbufval)
{
	switch (c->depth_func) {
	case GL_LESS:
		return zval < zbufval;
	case GL_LEQUAL:
		return zval <= zbufval;
	case GL_GREATER:
		return zval > zbufval;
	case GL_GEQUAL:
		return zval >= zbufval;
	case GL_EQUAL:
		return zval == zbufval;
	case GL_NOTEQUAL:
		return zval != zbufval;
	case GL_ALWAYS:
		return 1;
	case GL_NEVER:
		return 0;
	}
	return 0; //get rid of compile warning
}


static void setup_fs_input(float t, float* v1_out, float* v2_out, float wa, float wb, unsigned int provoke)
{
	float* vs_output = &c->vs_output.output_buf.a[0];

	float inv_wa = 1.0/wa;
	float inv_wb = 1.0/wb;

	for (int i=0; i<c->vs_output.size; ++i) {
		if (c->vs_output.interpolation[i] == SMOOTH) {
			c->fs_input[i] = (v1_out[i]*inv_wa + t*(v2_out[i]*inv_wb - v1_out[i]*inv_wa)) / (inv_wa + t*(inv_wb - inv_wa));

		} else if (c->vs_output.interpolation[i] == NOPERSPECTIVE) {
			c->fs_input[i] = v1_out[i] + t*(v2_out[i] - v1_out[i]);
		} else {
			c->fs_input[i] = vs_output[provoke*c->vs_output.size + i];
		}
	}

	c->builtins.discard = GL_FALSE;
}

/* Line Clipping algorithm from 'Computer Graphics', Principles and
   Practice */
static inline int clip_line(float denom, float num, float* tmin, float* tmax)
{
	float t;

	if (denom > 0) {
		t = num / denom;
		if (t > *tmax) return 0;
		if (t > *tmin) {
			*tmin = t;
			//printf("t > *tmin %f\n", t);
		}
	} else if (denom < 0) {
		t = num / denom;
		if (t < *tmin) return 0;
		if (t < *tmax) {
			*tmax = t;
			//printf("t < *tmax %f\n", t);
		}
	} else if (num > 0) return 0;
	return 1;
}


static void interpolate_clipped_line(glVertex* v1, glVertex* v2, float* v1_out, float* v2_out, float tmin, float tmax)
{
	for (int i=0; i<c->vs_output.size; ++i) {
		v1_out[i] = v1->vs_out[i] + (v2->vs_out[i] - v1->vs_out[i])*tmin;
		v2_out[i] = v1->vs_out[i] + (v2->vs_out[i] - v1->vs_out[i])*tmax;

		//v2_out[i] = (1 - tmax)*v1->vs_out[i] + tmax*v2->vs_out[i];
	}
}



static void draw_line_clip(glVertex* v1, glVertex* v2)
{
	int cc1, cc2;
	vec4 d, p1, p2, t1, t2;
	float tmin, tmax;

	cc1 = v1->clip_code;
	cc2 = v2->clip_code;

	p1 = v1->clip_space;
	p2 = v2->clip_space;
	
	float v1_out[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	float v2_out[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	//TODO ponder this
	unsigned int provoke;
	if (c->provoking_vert == GL_LAST_VERTEX_CONVENTION)
		provoke = (v2 - c->glverts.a)/sizeof(glVertex);
	else
		provoke = (v1 - c->glverts.a)/sizeof(glVertex);

	if (cc1 & cc2) {
		return;
	} else if ((cc1 | cc2) == 0) {
		t1 = mult_mat4_vec4(c->vp_mat, p1);
		t2 = mult_mat4_vec4(c->vp_mat, p2);

		if (c->line_width < 1.5f)
			draw_line_shader(vec4_to_vec3h(t1), vec4_to_vec3h(t2), t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
		else
			draw_thick_line_shader(t1, t2, v1->vs_out, v2->vs_out, provoke);
	} else {

		d = sub_vec4s(p2, p1);

		tmin = 0;
		tmax = 1;
		if (clip_line( d.z+d.w, -p1.z-p1.w, &tmin, &tmax) &&
		    clip_line(-d.z+d.w,  p1.z-p1.w, &tmin, &tmax)) {

			//printf("%f %f\n", tmin, tmax);

			t1 = add_vec4s(p1, scale_vec4(d, tmin));
			t2 = add_vec4s(p1, scale_vec4(d, tmax));

			t1 = mult_mat4_vec4(c->vp_mat, t1);
			t2 = mult_mat4_vec4(c->vp_mat, t2);
			//print_vec4(t1, "\n");
			//print_vec4(t2, "\n");

			interpolate_clipped_line(v1, v2, v1_out, v2_out, tmin, tmax);

			if (c->line_width < 1.5f)
				draw_line_shader(vec4_to_vec3h(t1), vec4_to_vec3h(t2), t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
			else
				draw_thick_line_shader(t1, t2, v1->vs_out, v2->vs_out, provoke);
		}
	}
}


static void draw_line_shader(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
{
	float tmp;
	float* tmp_ptr;

	//print_vec3(hp1, "\n");
	//print_vec3(hp2, "\n");

	float x1 = hp1.x, x2 = hp2.x, y1 = hp1.y, y2 = hp2.y;
	float z1 = hp1.z, z2 = hp2.z;

	//always draw from left to right
	if (x2 < x1) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;

		tmp = z1;
		z1 = z2;
		z2 = tmp;

		tmp = w1;
		w1 = w2;
		w2 = tmp;

		tmp_ptr = v1_out;
		v1_out = v2_out;
		v2_out = tmp_ptr;
	}

	//calculate slope and implicit line parameters once
	//could just use my Line type/constructor as in draw_triangle
	float m = (y2-y1)/(x2-x1);
	Line line = make_Line(x1, y1, x2, y2);

	float t, x, y, z, w;

	vec2 p1 = { x1, y1 }, p2 = { x2, y2 };
	vec2 pr, sub_p2p1 = sub_vec2s(p2, p1);
	float line_length_squared = length_vec2(sub_p2p1);
	line_length_squared *= line_length_squared;

	frag_func fragment_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniform = c->programs.a[c->cur_program].uniform;
	int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;

	float i_x1, i_y1, i_x2, i_y2;
	i_x1 = floor(p1.x) + 0.5;
	i_y1 = floor(p1.y) + 0.5;
	i_x2 = floor(p2.x) + 0.5;
	i_y2 = floor(p2.y) + 0.5;

	float x_min, x_max, y_min, y_max;
	x_min = i_x1;
	x_max = i_x2; //always left to right;
	if (m <= 0) {
		y_min = i_y2;
		y_max = i_y1;
	} else {
		y_min = i_y1;
		y_max = i_y2;
	}

	// TODO should be done for each fragment, after poly_offset is added?
	z1 = MAP(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	z2 = MAP(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	//4 cases based on slope
	if (m <= -1) {     //(-infinite, -1]
		//printf("slope <= -1\n");
		for (x = x_min, y = y_max; y>=y_min && x<=x_max; --y) {
			if (CLIPXY_TEST(x, y)) {
				pr.x = x;
				pr.y = y;
				t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

				z = (1 - t) * z1 + t * z2;
				z += poly_offset;
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					w = (1 - t) * w1 + t * w2;

					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard)
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);

				}
			}
			if (line_func(&line, x+0.5f, y-1) < 0) //A*(x+0.5f) + B*(y-1) + C < 0)
				++x;
		}
	} else if (m <= 0) {     //(-1, 0]
		//printf("slope = (-1, 0]\n");
		for (x = x_min, y = y_max; x<=x_max && y>=y_min; ++x) {
			if (CLIPXY_TEST(x, y)) {
				pr.x = x;
				pr.y = y;
				t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

				z = (1 - t) * z1 + t * z2;
				z += poly_offset;
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					w = (1 - t) * w1 + t * w2;

					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard)
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}
			if (line_func(&line, x+1, y-0.5f) > 0) //A*(x+1) + B*(y-0.5f) + C > 0)
				--y;
		}
	} else if (m <= 1) {     //(0, 1]
		//printf("slope = (0, 1]\n");
		for (x = x_min, y = y_min; x <= x_max && y <= y_max; ++x) {
			if (CLIPXY_TEST(x, y)) {
				pr.x = x;
				pr.y = y;
				t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

				z = (1 - t) * z1 + t * z2;
				z += poly_offset;
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					w = (1 - t) * w1 + t * w2;

					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard)
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}

			if (line_func(&line, x+1, y+0.5f) < 0) //A*(x+1) + B*(y+0.5f) + C < 0)
				++y;
		}

	} else {    //(1, +infinite)
		//printf("slope > 1\n");
		for (x = x_min, y = y_min; y<=y_max && x <= x_max; ++y) {
			if (CLIPXY_TEST(x, y)) {
				pr.x = x;
				pr.y = y;
				t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

				z = (1 - t) * z1 + t * z2;
				z += poly_offset;
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					w = (1 - t) * w1 + t * w2;

					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard)
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}

			if (line_func(&line, x+0.5f, y+1) > 0) //A*(x+0.5f) + B*(y+1) + C > 0)
				++x;
		}
	}
}

static int draw_perp_line(float m, float x1, float y1, float x2, float y2)
{
	// Assume that caller (draw_thick_line_shader) always arranged x1 < x2
	Line line = make_Line(x1, y1, x2, y2);

	frag_func fragment_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniform = c->programs.a[c->cur_program].uniform;
	// TODO use
	//int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;

	float i_x1, i_y1, i_x2, i_y2;
	i_x1 = floor(x1) + 0.5;
	i_y1 = floor(y1) + 0.5;
	i_x2 = floor(x2) + 0.5;
	i_y2 = floor(y2) + 0.5;

	// TODO the central ideal lines are clipped but perpendiculars of wide lines
	// could go off the edge
	float x_min, x_max, y_min, y_max;
	x_min = i_x1;
	x_max = i_x2; //always left to right;
	if (m <= 0) {
		y_min = i_y2;
		y_max = i_y1;
	} else {
		y_min = i_y1;
		y_max = i_y2;
	}

	float x, y;
	// same z for whole line was already set in caller
	float z = c->builtins.gl_FragCoord.z;

	int first_is_diag = GL_FALSE;

	//4 cases based on slope
	if (m <= -1) {     //(-infinite, -1]
		//NOTE(rswinkle): double checking the first step but better than duplicating
		// so much code imo
		if (line_func(&line, x_min+0.5f, y_max-1) < 0) {
			first_is_diag = GL_TRUE;
		}
		for (x = x_min, y = y_max; y>=y_min && x<=x_max; --y) {
			if (CLIPXY_TEST(x, y)) {
				c->builtins.gl_FragCoord.x = x;
				c->builtins.gl_FragCoord.y = y;
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				fragment_shader(c->fs_input, &c->builtins, uniform);

				if (!c->builtins.discard)
					draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, GL_TRUE);
			}

			if (line_func(&line, x+0.5f, y-1) < 0) //A*(x+0.5f) + B*(y-1) + C < 0)
				++x;
		}
	} else if (m <= 0) {     //(-1, 0]
		if (line_func(&line, x_min+1, y_max-0.5f) > 0) {
			first_is_diag = GL_TRUE;
		}
		for (x = x_min, y = y_max; x<=x_max && y>=y_min; ++x) {
			if (CLIPXY_TEST(x, y)) {
				c->builtins.gl_FragCoord.x = x;
				c->builtins.gl_FragCoord.y = y;
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				fragment_shader(c->fs_input, &c->builtins, uniform);

				if (!c->builtins.discard)
					draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, GL_TRUE);
			}

			if (line_func(&line, x+1, y-0.5f) > 0) //A*(x+1) + B*(y-0.5f) + C > 0)
				--y;
		}
	} else if (m <= 1) {     //(0, 1]
		if (line_func(&line, x_min+1, y_min+0.5f) < 0) {
			first_is_diag = GL_TRUE;
		}
		for (x = x_min, y = y_min; x <= x_max && y <= y_max; ++x) {
			if (CLIPXY_TEST(x, y)) {
				c->builtins.gl_FragCoord.x = x;
				c->builtins.gl_FragCoord.y = y;
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				fragment_shader(c->fs_input, &c->builtins, uniform);

				if (!c->builtins.discard)
					draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, GL_TRUE);
			}
			if (line_func(&line, x+1, y+0.5f) < 0) //A*(x+1) + B*(y+0.5f) + C < 0)
				++y;
		}
	} else {    //(1, +infinite)
		if (line_func(&line, x_min+0.5f, y_min+1) > 0) {
			first_is_diag = GL_TRUE;
		}
		for (x = x_min, y = y_min; y<=y_max && x <= x_max; ++y) {
			if (CLIPXY_TEST(x, y)) {
				c->builtins.gl_FragCoord.x = x;
				c->builtins.gl_FragCoord.y = y;
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				fragment_shader(c->fs_input, &c->builtins, uniform);

				if (!c->builtins.discard)
					draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, GL_TRUE);
			}
			if (line_func(&line, x+0.5f, y+1) > 0) //A*(x+0.5f) + B*(y+1) + C > 0)
				++x;
		}
	}
	return first_is_diag;
}

static void draw_thick_line_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke)
{
	float tmp;
	float* tmp_ptr;
	// TODO add poly_offset to parameters and use

	vec3 hp1 = vec4_to_vec3h(v1);
	vec3 hp2 = vec4_to_vec3h(v2);

	//print_vec3(hp1, "\n");
	//print_vec3(hp2, "\n");

	float w1 = v1.w;
	float w2 = v2.w;

	float x1 = hp1.x, x2 = hp2.x, y1 = hp1.y, y2 = hp2.y;
	float z1 = hp1.z, z2 = hp2.z;

	//always draw from left to right
	if (x2 < x1) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;

		tmp = z1;
		z1 = z2;
		z2 = tmp;

		tmp = w1;
		w1 = w2;
		w2 = tmp;

		tmp_ptr = v1_out;
		v1_out = v2_out;
		v2_out = tmp_ptr;
	}

	//calculate slope and implicit line parameters once
	//could just use my Line type/constructor as in draw_triangle
	float m = (y2-y1)/(x2-x1);
	Line line = make_Line(x1, y1, x2, y2);
	vec2 ab = { line.A, line.B };
	normalize_vec2(&ab);
	ab = scale_vec2(ab, c->line_width/2.0f);

	float t, x, y, z, w;

	vec2 p1 = { x1, y1 }, p2 = { x2, y2 };
	vec2 pr, sub_p2p1 = sub_vec2s(p2, p1);
	float line_length_squared = length_vec2(sub_p2p1);
	line_length_squared *= line_length_squared;

	float i_x1, i_y1, i_x2, i_y2;
	i_x1 = floor(p1.x) + 0.5;
	i_y1 = floor(p1.y) + 0.5;
	i_x2 = floor(p2.x) + 0.5;
	i_y2 = floor(p2.y) + 0.5;

	float x_min, x_max, y_min, y_max;
	x_min = i_x1;
	x_max = i_x2; //always left to right;
	if (m <= 0) {
		y_min = i_y2;
		y_max = i_y1;
	} else {
		y_min = i_y1;
		y_max = i_y2;
	}

	//printf("%f %f %f %f   =\n", i_x1, i_y1, i_x2, i_y2);
	//printf("%f %f %f %f   x_min etc\n", x_min, x_max, y_min, y_max);

	z1 = MAP(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	z2 = MAP(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	int diag;

	//4 cases based on slope
	if (m <= -1) {     //(-infinite, -1]
		for (x = x_min, y = y_max; y>=y_min && x<=x_max; --y) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			// These are constant for the whole perpendicular
			// I'm pretending the special gap perpendiculars get the
			// same values for the sake of simplicity
			c->builtins.gl_FragCoord.z = z;
			c->builtins.gl_FragCoord.w = 1/w;

			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			diag = draw_perp_line(-1/m, x-ab.x, y-ab.y, x+ab.x, y+ab.y);
			if (line_func(&line, x+0.5f, y-1) < 0) {
				if (diag) {
					draw_perp_line(-1/m, x-ab.x, y-1-ab.y, x+ab.x, y-1+ab.y);
				}
				++x;
			}
		}
	} else if (m <= 0) {     //(-1, 0]
		float inv_m = m ? -1/m : INFINITY;
		for (x = x_min, y = y_max; x<=x_max && y>=y_min; ++x) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			c->builtins.gl_FragCoord.z = z;
			c->builtins.gl_FragCoord.w = 1/w;

			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			diag = draw_perp_line(inv_m, x-ab.x, y-ab.y, x+ab.x, y+ab.y);
			if (line_func(&line, x+1, y-0.5f) > 0) {
				if (diag) {
					draw_perp_line(inv_m, x+1-ab.x, y-ab.y, x+1+ab.x, y+ab.y);
				}
				--y;
			}
		}
	} else if (m <= 1) {     //(0, 1]
		for (x = x_min, y = y_min; x <= x_max && y <= y_max; ++x) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			c->builtins.gl_FragCoord.z = z;
			c->builtins.gl_FragCoord.w = 1/w;

			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			diag = draw_perp_line(-1/m, x+ab.x, y+ab.y, x-ab.x, y-ab.y);

			if (line_func(&line, x+1, y+0.5f) < 0) {
				if (diag) {
					draw_perp_line(-1/m, x+1+ab.x, y+ab.y, x+1-ab.x, y-ab.y);
				}
				++y;
			}
		}

	} else {    //(1, +infinite)
		for (x = x_min, y = y_min; y<=y_max && x <= x_max; ++y) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			c->builtins.gl_FragCoord.z = z;
			c->builtins.gl_FragCoord.w = 1/w;

			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			diag = draw_perp_line(-1/m, x+ab.x, y+ab.y, x-ab.x, y-ab.y);

			if (line_func(&line, x+0.5f, y+1) > 0) {
				if (diag) {
					draw_perp_line(-1/m, x+ab.x, y+1+ab.y, x-ab.x, y+1-ab.y);
				}
				++x;
			}
		}
	}
}

static void draw_triangle(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke)
{
	int c_or, c_and;
	c_and = v0->clip_code & v1->clip_code & v2->clip_code;
	if (c_and != 0) {
		//printf("triangle outside\n");
		return;
	}

	// have to set here because we can re use vertices
	// for multiple triangles in STRIP and FAN
	v0->edge_flag = v1->edge_flag = v2->edge_flag = 1;

	v0->clip_code &= CLIPZ_MASK;
	v1->clip_code &= CLIPZ_MASK;
	v2->clip_code &= CLIPZ_MASK;
	c_or = v0->clip_code | v1->clip_code | v2->clip_code;
	if (c_or == 0) {
		draw_triangle_final(v0, v1, v2, provoke);
	} else {
		draw_triangle_clip(v0, v1, v2, provoke, 0);
	}
}

static void draw_triangle_final(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke)
{
	int front_facing;
	v0->screen_space = mult_mat4_vec4(c->vp_mat, v0->clip_space);
	v1->screen_space = mult_mat4_vec4(c->vp_mat, v1->clip_space);
	v2->screen_space = mult_mat4_vec4(c->vp_mat, v2->clip_space);

	front_facing = is_front_facing(v0, v1, v2);
	if (c->cull_face) {
		if (c->cull_mode == GL_FRONT_AND_BACK)
			return;
		if (c->cull_mode == GL_BACK && !front_facing) {
			//puts("culling back face");
			return;
		}
		if (c->cull_mode == GL_FRONT && front_facing)
			return;
	}

	c->builtins.gl_FrontFacing = front_facing;

	if (front_facing) {
		c->draw_triangle_front(v0, v1, v2, provoke);
	} else {
		c->draw_triangle_back(v0, v1, v2, provoke);
	}
}


/* We clip the segment [a,b] against the 6 planes of the normal volume.
 * We compute the point 'c' of intersection and the value of the parameter 't'
 * of the intersection if x=a+t(b-a).
 */

#define clip_func(name, sign, dir, dir1, dir2) \
static float name(vec4 *c, vec4 *a, vec4 *b) \
{\
	float t, dx, dy, dz, dw, den;\
	dx = (b->x - a->x);\
	dy = (b->y - a->y);\
	dz = (b->z - a->z);\
	dw = (b->w - a->w);\
	den = -(sign d ## dir) + dw;\
	if (den == 0) t=0;\
	else t = ( sign a->dir - a->w) / den;\
	c->dir1 = a->dir1 + t * d ## dir1;\
	c->dir2 = a->dir2 + t * d ## dir2;\
	c->w = a->w + t * dw;\
	c->dir = sign c->w;\
	return t;\
}


clip_func(clip_xmin, -, x, y, z)

clip_func(clip_xmax, +, x, y, z)

clip_func(clip_ymin, -, y, x, z)

clip_func(clip_ymax, +, y, x, z)

clip_func(clip_zmin, -, z, x, y)

clip_func(clip_zmax, +, z, x, y)


static float (*clip_proc[6])(vec4 *, vec4 *, vec4 *) = {
	clip_zmin, clip_zmax,
	clip_xmin, clip_xmax,
	clip_ymin, clip_ymax
};

static inline void update_clip_pt(glVertex *q, glVertex *v0, glVertex *v1, float t)
{
	for (int i=0; i<c->vs_output.size; ++i) {
		//why is this correct for both SMOOTH and NOPERSPECTIVE?
		q->vs_out[i] = v0->vs_out[i] + (v1->vs_out[i] - v0->vs_out[i]) * t;

		//FLAT should be handled indirectly by the provoke index
		//nothing to do here unless I change that
	}
	
	q->clip_code = gl_clipcode(q->clip_space) & CLIPZ_MASK;
	/*
	 * this is done in draw_triangle currently ...
	q->screen_space = mult_mat4_vec4(c->vp_mat, q->clip_space);
	if (q->clip_code == 0)
		q->screen_space = mult_mat4_vec4(c->vp_mat, q->clip_space);
		*/
}




static void draw_triangle_clip(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke, int clip_bit)
{
	int c_or, c_and, c_ex_or, cc[3], edge_flag_tmp, clip_mask;
	glVertex tmp1, tmp2, *q[3];
	float tt;

	//quite a bit of stack if there's a lot of clipping ...
	float tmp1_out[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	float tmp2_out[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	tmp1.vs_out = tmp1_out;
	tmp2.vs_out = tmp2_out;

	cc[0] = v0->clip_code;
	cc[1] = v1->clip_code;
	cc[2] = v2->clip_code;
	/*
	printf("in draw_triangle_clip\n");
	print_vec4(v0->clip_space, "\n");
	print_vec4(v1->clip_space, "\n");
	print_vec4(v2->clip_space, "\n");
	printf("tmp_out tmp2_out = %p %p\n\n", tmp1_out, tmp2_out);
	*/


	c_or = cc[0] | cc[1] | cc[2];
	if (c_or == 0) {
		draw_triangle_final(v0, v1, v2, provoke);
	} else {
		c_and = cc[0] & cc[1] & cc[2];
		/* the triangle is completely outside */
		if (c_and != 0) {
			//printf("triangle outside\n");
			return;
		}

		/* find the next direction to clip */
		// Changed 6 to 2 to only clip z planes
		while (clip_bit < 2 && (c_or & (1 << clip_bit)) == 0)  {
			++clip_bit;
		}

		/* this test can be true only in case of rounding errors */
		if (clip_bit == 2) {
#if 1
			printf("Clipping error:\n");
			print_vec4(v0->clip_space, "\n");
			print_vec4(v1->clip_space, "\n");
			print_vec4(v2->clip_space, "\n");
#endif
			return;
		}

		clip_mask = 1 << clip_bit;
		c_ex_or = (cc[0] ^ cc[1] ^ cc[2]) & clip_mask;

		if (c_ex_or)  {
			/* one point outside */

			if (cc[0] & clip_mask) { q[0]=v0; q[1]=v1; q[2]=v2; }
			else if (cc[1] & clip_mask) { q[0]=v1; q[1]=v2; q[2]=v0; }
			else { q[0]=v2; q[1]=v0; q[2]=v1; }

			tt = clip_proc[clip_bit](&tmp1.clip_space, &q[0]->clip_space, &q[1]->clip_space);
			update_clip_pt(&tmp1, q[0], q[1], tt);

			tt = clip_proc[clip_bit](&tmp2.clip_space, &q[0]->clip_space, &q[2]->clip_space);
			update_clip_pt(&tmp2, q[0], q[2], tt);

			tmp1.edge_flag = q[0]->edge_flag;
			edge_flag_tmp = q[2]->edge_flag;
			q[2]->edge_flag = 0;
			draw_triangle_clip(&tmp1, q[1], q[2], provoke, clip_bit+1);

			tmp2.edge_flag = 0;
			tmp1.edge_flag = 0; // fixed from TinyGL, was 1
			q[2]->edge_flag = edge_flag_tmp;
			draw_triangle_clip(&tmp2, &tmp1, q[2], provoke, clip_bit+1);
		} else {
			/* two points outside */

			if ((cc[0] & clip_mask) == 0) { q[0]=v0; q[1]=v1; q[2]=v2; }
			else if ((cc[1] & clip_mask) == 0) { q[0]=v1; q[1]=v2; q[2]=v0; }
			else { q[0]=v2; q[1]=v0; q[2]=v1; }

			tt = clip_proc[clip_bit](&tmp1.clip_space, &q[0]->clip_space, &q[1]->clip_space);
			update_clip_pt(&tmp1, q[0], q[1], tt);

			tt = clip_proc[clip_bit](&tmp2.clip_space, &q[0]->clip_space, &q[2]->clip_space);
			update_clip_pt(&tmp2, q[0], q[2], tt);

			tmp1.edge_flag = 0; // fixed from TinyGL, was 1
			tmp2.edge_flag = q[2]->edge_flag;
			draw_triangle_clip(q[0], &tmp1, &tmp2, provoke, clip_bit+1);
		}
	}
}

static void draw_triangle_point(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke)
{
	//TODO use provoke?
	glVertex* vert[3] = { v0, v1, v2 };
	vec3 hp[3];
	hp[0] = vec4_to_vec3h(v0->screen_space);
	hp[1] = vec4_to_vec3h(v1->screen_space);
	hp[2] = vec4_to_vec3h(v2->screen_space);

	float poly_offset = 0;
	if (c->poly_offset_pt) {
		poly_offset = calc_poly_offset(hp[0], hp[1], hp[2]);
	}

	// TODO TinyGL uses edge_flags to determine whether to draw
	// a point here...but it doesn't work and there's no way
	// to make it work as far as I can tell.  There are hacks
	// I can do to get proper behavior but for now...meh
	for (int i=0; i<3; ++i) {
		draw_point(vert[i], poly_offset);
	}
}

static void draw_triangle_line(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke)
{
	// TODO early return if no edge_flags
	vec4 s0 = v0->screen_space;
	vec4 s1 = v1->screen_space;
	vec4 s2 = v2->screen_space;

	// TODO remove redundant calc in thick_line_shader
	vec3 hp0 = vec4_to_vec3h(s0);
	vec3 hp1 = vec4_to_vec3h(s1);
	vec3 hp2 = vec4_to_vec3h(s2);
	float w0 = v0->screen_space.w;
	float w1 = v1->screen_space.w;
	float w2 = v2->screen_space.w;

	float poly_offset = 0;
	if (c->poly_offset_line) {
		poly_offset = calc_poly_offset(hp0, hp1, hp2);
	}

	if (c->line_width < 1.5f) {
		if (v0->edge_flag)
			draw_line_shader(hp0, hp1, w0, w1, v0->vs_out, v1->vs_out, provoke, poly_offset);
		if (v1->edge_flag)
			draw_line_shader(hp1, hp2, w1, w2, v1->vs_out, v2->vs_out, provoke, poly_offset);
		if (v2->edge_flag)
			draw_line_shader(hp2, hp0, w2, w0, v2->vs_out, v0->vs_out, provoke, poly_offset);
	} else {
		if (v0->edge_flag)
			draw_thick_line_shader(s0, s1, v0->vs_out, v1->vs_out, provoke);
		if (v1->edge_flag)
			draw_thick_line_shader(s1, s2, v1->vs_out, v2->vs_out, provoke);
		if (v2->edge_flag)
			draw_thick_line_shader(s2, s0, v2->vs_out, v0->vs_out, provoke);
	}
}

// TODO make macro or inline?
static float calc_poly_offset(vec3 hp0, vec3 hp1, vec3 hp2)
{
	float max_depth_slope = 0;
	float dzxy[6];
	dzxy[0] = fabsf((hp1.z - hp0.z)/(hp1.x - hp0.x));
	dzxy[1] = fabsf((hp1.z - hp0.z)/(hp1.y - hp0.y));
	dzxy[2] = fabsf((hp2.z - hp1.z)/(hp2.x - hp1.x));
	dzxy[3] = fabsf((hp2.z - hp1.z)/(hp2.y - hp1.y));
	dzxy[4] = fabsf((hp0.z - hp2.z)/(hp0.x - hp2.x));
	dzxy[5] = fabsf((hp0.z - hp2.z)/(hp0.y - hp2.y));

	max_depth_slope = dzxy[0];
	for (int i=1; i<6; ++i) {
		if (dzxy[i] > max_depth_slope)
			max_depth_slope = dzxy[i];
	}

#define SMALLEST_INCR 0.000001;
	return max_depth_slope * c->poly_factor + c->poly_units * SMALLEST_INCR;
#undef SMALLEST_INCR
}

static void draw_triangle_fill(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke)
{
	vec4 p0 = v0->screen_space;
	vec4 p1 = v1->screen_space;
	vec4 p2 = v2->screen_space;

	vec3 hp0 = vec4_to_vec3h(p0);
	vec3 hp1 = vec4_to_vec3h(p1);
	vec3 hp2 = vec4_to_vec3h(p2);

	// TODO even worth calculating or just some constant?
	float poly_offset = 0;

	if (c->poly_offset_fill) {
		poly_offset = calc_poly_offset(hp0, hp1, hp2);
	}

	/*
	print_vec4(hp0, "\n");
	print_vec4(hp1, "\n");
	print_vec4(hp2, "\n");

	printf("%f %f %f\n", p0.w, p1.w, p2.w);
	print_vec3(hp0, "\n");
	print_vec3(hp1, "\n");
	print_vec3(hp2, "\n\n");
	*/

	//can't think of a better/cleaner way to do this than these 8 lines
	float x_min = MIN(hp0.x, hp1.x);
	float x_max = MAX(hp0.x, hp1.x);
	float y_min = MIN(hp0.y, hp1.y);
	float y_max = MAX(hp0.y, hp1.y);

	x_min = MIN(hp2.x, x_min);
	x_max = MAX(hp2.x, x_max);
	y_min = MIN(hp2.y, y_min);
	y_max = MAX(hp2.y, y_max);

	// clipping/scissoring against side planes here
	x_min = MAX(c->lx, x_min);
	x_max = MIN(c->ux, x_max);
	y_min = MAX(c->ly, y_min);
	y_max = MIN(c->uy, y_max);
	// end clipping

	// TODO is there any point to having an int index?
	// I think I did it for OpenMP
	int ix_max = roundf(x_max);
	int iy_max = roundf(y_max);

	//form implicit lines
	Line l01 = make_Line(hp0.x, hp0.y, hp1.x, hp1.y);
	Line l12 = make_Line(hp1.x, hp1.y, hp2.x, hp2.y);
	Line l20 = make_Line(hp2.x, hp2.y, hp0.x, hp0.y);

	float alpha, beta, gamma, tmp, tmp2, z;
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	float perspective[GL_MAX_VERTEX_OUTPUT_COMPONENTS*3];
	float* vs_output = &c->vs_output.output_buf.a[0];

	for (int i=0; i<c->vs_output.size; ++i) {
		perspective[i] = v0->vs_out[i]/p0.w;
		perspective[GL_MAX_VERTEX_OUTPUT_COMPONENTS + i] = v1->vs_out[i]/p1.w;
		perspective[2*GL_MAX_VERTEX_OUTPUT_COMPONENTS + i] = v2->vs_out[i]/p2.w;
	}
	float inv_w0 = 1/p0.w;  //is this worth it?  faster than just dividing by w down below?
	float inv_w1 = 1/p1.w;
	float inv_w2 = 1/p2.w;

	float x, y;

	int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;
	Shader_Builtins builtins;

	#pragma omp parallel for private(x, y, alpha, beta, gamma, z, tmp, tmp2, builtins, fs_input)
	for (int iy = y_min; iy<iy_max; ++iy) {
		y = iy + 0.5f;

		for (int ix = x_min; ix<ix_max; ++ix) {
			x = ix + 0.5f; //center of min pixel

			// page 117 of glspec describes calculating using areas of triangles but that
			// simplifies (b*h_1/2)/(b*h_2/2) = h_1/h_2 hence the implicit line equations
			// See FoCG pg 34-5 and 167
			gamma = line_func(&l01, x, y)/line_func(&l01, hp2.x, hp2.y);
			beta = line_func(&l20, x, y)/line_func(&l20, hp1.x, hp1.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -2.5
				//this is a deterministic way of choosing which triangle gets a pixel for triangles that share
				//edges (see commit message for e87e324)
				if ((alpha > 0 || line_func(&l12, hp0.x, hp0.y) * line_func(&l12, -1, -2.5) > 0) &&
				    (beta  > 0 || line_func(&l20, hp1.x, hp1.y) * line_func(&l20, -1, -2.5) > 0) &&
				    (gamma > 0 || line_func(&l01, hp2.x, hp2.y) * line_func(&l01, -1, -2.5) > 0)) {
					//calculate interoplation here
					tmp2 = alpha*inv_w0 + beta*inv_w1 + gamma*inv_w2;

					z = alpha * hp0.z + beta * hp1.z + gamma * hp2.z;

					z += poly_offset;
					z = MAP(z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far); //TODO move out (ie can I map hp1.z etc.)?

					// early testing if shader doesn't use fragdepth or discard
					if (!fragdepth_or_discard && !fragment_processing(x, y, z)) {
						continue;
					}

					for (int i=0; i<c->vs_output.size; ++i) {
						if (c->vs_output.interpolation[i] == SMOOTH) {
							tmp = alpha*perspective[i] + beta*perspective[GL_MAX_VERTEX_OUTPUT_COMPONENTS + i] + gamma*perspective[2*GL_MAX_VERTEX_OUTPUT_COMPONENTS + i];

							fs_input[i] = tmp/tmp2;

						} else if (c->vs_output.interpolation[i] == NOPERSPECTIVE) {
							fs_input[i] = alpha * v0->vs_out[i] + beta * v1->vs_out[i] + gamma * v2->vs_out[i];
						} else { // == FLAT
							fs_input[i] = vs_output[provoke*c->vs_output.size + i];
						}
					}

					// tmp2 is 1/w interpolated... I now do that everywhere (draw_line, draw_point)
					SET_VEC4(builtins.gl_FragCoord, x, y, z, tmp2);
					builtins.discard = GL_FALSE;
					builtins.gl_FragDepth = z;

					// have to do this here instead of outside the loop because somehow openmp messes it up
					// TODO probably some way to prevent that but it's just copying an int so no big deal
					builtins.gl_InstanceID = c->builtins.gl_InstanceID;

					c->programs.a[c->cur_program].fragment_shader(fs_input, &builtins, c->programs.a[c->cur_program].uniform);
					if (!builtins.discard) {

						draw_pixel(builtins.gl_FragColor, x, y, builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}
		}
	}
}


// TODO should this be done in colors/integers not vec4/floats?
// and if it's done in Colors/integers what's the performance difference?
static Color blend_pixel(vec4 src, vec4 dst)
{
	vec4 bc = c->blend_color;
	float i = MIN(src.w, 1-dst.w); // in colors this would be min(src.a, 255-dst.a)/255

	vec4 Cs, Cd;

	switch (c->blend_sRGB) {
	case GL_ZERO:                     SET_VEC4(Cs, 0,0,0,0);                                 break;
	case GL_ONE:                      SET_VEC4(Cs, 1,1,1,1);                                 break;
	case GL_SRC_COLOR:                Cs = src;                                              break;
	case GL_ONE_MINUS_SRC_COLOR:      SET_VEC4(Cs, 1-src.x,1-src.y,1-src.z,1-src.w);         break;
	case GL_DST_COLOR:                Cs = dst;                                              break;
	case GL_ONE_MINUS_DST_COLOR:      SET_VEC4(Cs, 1-dst.x,1-dst.y,1-dst.z,1-dst.w);         break;
	case GL_SRC_ALPHA:                SET_VEC4(Cs, src.w, src.w, src.w, src.w);              break;
	case GL_ONE_MINUS_SRC_ALPHA:      SET_VEC4(Cs, 1-src.w,1-src.w,1-src.w,1-src.w);         break;
	case GL_DST_ALPHA:                SET_VEC4(Cs, dst.w, dst.w, dst.w, dst.w);              break;
	case GL_ONE_MINUS_DST_ALPHA:      SET_VEC4(Cs, 1-dst.w,1-dst.w,1-dst.w,1-dst.w);         break;
	case GL_CONSTANT_COLOR:           Cs = bc;                                               break;
	case GL_ONE_MINUS_CONSTANT_COLOR: SET_VEC4(Cs, 1-bc.x,1-bc.y,1-bc.z,1-bc.w);             break;
	case GL_CONSTANT_ALPHA:           SET_VEC4(Cs, bc.w, bc.w, bc.w, bc.w);                  break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: SET_VEC4(Cs, 1-bc.w,1-bc.w,1-bc.w,1-bc.w);             break;

	case GL_SRC_ALPHA_SATURATE:       SET_VEC4(Cs, i, i, i, 1);                              break;
	/*not implemented yet
	 * won't be until I implement dual source blending/dual output from frag shader
	 *https://www.opengl.org/wiki/Blending#Dual_Source_Blending
	case GL_SRC1_COLOR:               Cs =  break;
	case GL_ONE_MINUS_SRC1_COLOR:     Cs =  break;
	case GL_SRC1_ALPHA:               Cs =  break;
	case GL_ONE_MINUS_SRC1_ALPHA:     Cs =  break;
	*/
	default:
		//should never get here
		puts("error unrecognized blend_sRGB!");
		break;
	}

	switch (c->blend_dRGB) {
	case GL_ZERO:                     SET_VEC4(Cd, 0,0,0,0);                                 break;
	case GL_ONE:                      SET_VEC4(Cd, 1,1,1,1);                                 break;
	case GL_SRC_COLOR:                Cd = src;                                              break;
	case GL_ONE_MINUS_SRC_COLOR:      SET_VEC4(Cd, 1-src.x,1-src.y,1-src.z,1-src.w);         break;
	case GL_DST_COLOR:                Cd = dst;                                              break;
	case GL_ONE_MINUS_DST_COLOR:      SET_VEC4(Cd, 1-dst.x,1-dst.y,1-dst.z,1-dst.w);         break;
	case GL_SRC_ALPHA:                SET_VEC4(Cd, src.w, src.w, src.w, src.w);              break;
	case GL_ONE_MINUS_SRC_ALPHA:      SET_VEC4(Cd, 1-src.w,1-src.w,1-src.w,1-src.w);         break;
	case GL_DST_ALPHA:                SET_VEC4(Cd, dst.w, dst.w, dst.w, dst.w);              break;
	case GL_ONE_MINUS_DST_ALPHA:      SET_VEC4(Cd, 1-dst.w,1-dst.w,1-dst.w,1-dst.w);         break;
	case GL_CONSTANT_COLOR:           Cd = bc;                                               break;
	case GL_ONE_MINUS_CONSTANT_COLOR: SET_VEC4(Cd, 1-bc.x,1-bc.y,1-bc.z,1-bc.w);             break;
	case GL_CONSTANT_ALPHA:           SET_VEC4(Cd, bc.w, bc.w, bc.w, bc.w);                  break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: SET_VEC4(Cd, 1-bc.w,1-bc.w,1-bc.w,1-bc.w);             break;

	case GL_SRC_ALPHA_SATURATE:       SET_VEC4(Cd, i, i, i, 1);                              break;
	/*not implemented yet
	case GL_SRC_ALPHA_SATURATE:       Cd =  break;
	case GL_SRC1_COLOR:               Cd =  break;
	case GL_ONE_MINUS_SRC1_COLOR:     Cd =  break;
	case GL_SRC1_ALPHA:               Cd =  break;
	case GL_ONE_MINUS_SRC1_ALPHA:     Cd =  break;
	*/
	default:
		//should never get here
		puts("error unrecognized blend_dRGB!");
		break;
	}

	// TODO simplify combine redundancies
	switch (c->blend_sA) {
	case GL_ZERO:                     Cs.w = 0;              break;
	case GL_ONE:                      Cs.w = 1;              break;
	case GL_SRC_COLOR:                Cs.w = src.w;          break;
	case GL_ONE_MINUS_SRC_COLOR:      Cs.w = 1-src.w;        break;
	case GL_DST_COLOR:                Cs.w = dst.w;          break;
	case GL_ONE_MINUS_DST_COLOR:      Cs.w = 1-dst.w;        break;
	case GL_SRC_ALPHA:                Cs.w = src.w;          break;
	case GL_ONE_MINUS_SRC_ALPHA:      Cs.w = 1-src.w;        break;
	case GL_DST_ALPHA:                Cs.w = dst.w;          break;
	case GL_ONE_MINUS_DST_ALPHA:      Cs.w = 1-dst.w;        break;
	case GL_CONSTANT_COLOR:           Cs.w = bc.w;           break;
	case GL_ONE_MINUS_CONSTANT_COLOR: Cs.w = 1-bc.w;         break;
	case GL_CONSTANT_ALPHA:           Cs.w = bc.w;           break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: Cs.w = 1-bc.w;         break;

	case GL_SRC_ALPHA_SATURATE:       Cs.w = 1;              break;
	/*not implemented yet
	 * won't be until I implement dual source blending/dual output from frag shader
	 *https://www.opengl.org/wiki/Blending#Dual_Source_Blending
	case GL_SRC1_COLOR:               Cs =  break;
	case GL_ONE_MINUS_SRC1_COLOR:     Cs =  break;
	case GL_SRC1_ALPHA:               Cs =  break;
	case GL_ONE_MINUS_SRC1_ALPHA:     Cs =  break;
	*/
	default:
		//should never get here
		puts("error unrecognized blend_sA!");
		break;
	}

	switch (c->blend_dA) {
	case GL_ZERO:                     Cd.w = 0;              break;
	case GL_ONE:                      Cd.w = 1;              break;
	case GL_SRC_COLOR:                Cd.w = src.w;          break;
	case GL_ONE_MINUS_SRC_COLOR:      Cd.w = 1-src.w;        break;
	case GL_DST_COLOR:                Cd.w = dst.w;          break;
	case GL_ONE_MINUS_DST_COLOR:      Cd.w = 1-dst.w;        break;
	case GL_SRC_ALPHA:                Cd.w = src.w;          break;
	case GL_ONE_MINUS_SRC_ALPHA:      Cd.w = 1-src.w;        break;
	case GL_DST_ALPHA:                Cd.w = dst.w;          break;
	case GL_ONE_MINUS_DST_ALPHA:      Cd.w = 1-dst.w;        break;
	case GL_CONSTANT_COLOR:           Cd.w = bc.w;           break;
	case GL_ONE_MINUS_CONSTANT_COLOR: Cd.w = 1-bc.w;         break;
	case GL_CONSTANT_ALPHA:           Cd.w = bc.w;           break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: Cd.w = 1-bc.w;         break;

	case GL_SRC_ALPHA_SATURATE:       Cd.w = 1;              break;
	/*not implemented yet
	case GL_SRC_ALPHA_SATURATE:       Cd =  break;
	case GL_SRC1_COLOR:               Cd =  break;
	case GL_ONE_MINUS_SRC1_COLOR:     Cd =  break;
	case GL_SRC1_ALPHA:               Cd =  break;
	case GL_ONE_MINUS_SRC1_ALPHA:     Cd =  break;
	*/
	default:
		//should never get here
		puts("error unrecognized blend_dA!");
		break;
	}

	vec4 result;

	// TODO eliminate function calls to avoid alpha component calculations?
	switch (c->blend_eqRGB) {
	case GL_FUNC_ADD:
		result = add_vec4s(mult_vec4s(Cs, src), mult_vec4s(Cd, dst));
		break;
	case GL_FUNC_SUBTRACT:
		result = sub_vec4s(mult_vec4s(Cs, src), mult_vec4s(Cd, dst));
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		result = sub_vec4s(mult_vec4s(Cd, dst), mult_vec4s(Cs, src));
		break;
	case GL_MIN:
		SET_VEC4(result, MIN(src.x, dst.x), MIN(src.y, dst.y), MIN(src.z, dst.z), MIN(src.w, dst.w));
		break;
	case GL_MAX:
		SET_VEC4(result, MAX(src.x, dst.x), MAX(src.y, dst.y), MAX(src.z, dst.z), MAX(src.w, dst.w));
		break;
	default:
		//should never get here
		puts("error unrecognized blend_eqRGB!");
		break;
	}

	switch (c->blend_eqA) {
	case GL_FUNC_ADD:
		result.w = Cs.w*src.w + Cd.w*dst.w;
		break;
	case GL_FUNC_SUBTRACT:
		result.w = Cs.w*src.w - Cd.w*dst.w;
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		result.w = Cd.w*dst.w - Cs.w*src.w;
		break;
	case GL_MIN:
		result.w = MIN(src.w, dst.w);
		break;
	case GL_MAX:
		result.w = MAX(src.w, dst.w);
		break;
	default:
		//should never get here
		puts("error unrecognized blend_eqRGB!");
		break;
	}

	return vec4_to_Color(result);
}

// source and destination colors
static Color logic_ops_pixel(Color s, Color d)
{
	switch (c->logic_func) {
	case GL_CLEAR:
		return make_Color(0,0,0,0);
	case GL_SET:
		return make_Color(255,255,255,255);
	case GL_COPY:
		return s;
	case GL_COPY_INVERTED:
		return make_Color(~s.r, ~s.g, ~s.b, ~s.a);
	case GL_NOOP:
		return d;
	case GL_INVERT:
		return make_Color(~d.r, ~d.g, ~d.b, ~d.a);
	case GL_AND:
		return make_Color(s.r & d.r, s.g & d.g, s.b & d.b, s.a & d.a);
	case GL_NAND:
		return make_Color(~(s.r & d.r), ~(s.g & d.g), ~(s.b & d.b), ~(s.a & d.a));
	case GL_OR:
		return make_Color(s.r | d.r, s.g | d.g, s.b | d.b, s.a | d.a);
	case GL_NOR:
		return make_Color(~(s.r | d.r), ~(s.g | d.g), ~(s.b | d.b), ~(s.a | d.a));
	case GL_XOR:
		return make_Color(s.r ^ d.r, s.g ^ d.g, s.b ^ d.b, s.a ^ d.a);
	case GL_EQUIV:
		return make_Color(~(s.r ^ d.r), ~(s.g ^ d.g), ~(s.b ^ d.b), ~(s.a ^ d.a));
	case GL_AND_REVERSE:
		return make_Color(s.r & ~d.r, s.g & ~d.g, s.b & ~d.b, s.a & ~d.a);
	case GL_AND_INVERTED:
		return make_Color(~s.r & d.r, ~s.g & d.g, ~s.b & d.b, ~s.a & d.a);
	case GL_OR_REVERSE:
		return make_Color(s.r | ~d.r, s.g | ~d.g, s.b | ~d.b, s.a | ~d.a);
	case GL_OR_INVERTED:
		return make_Color(~s.r | d.r, ~s.g | d.g, ~s.b | d.b, ~s.a | d.a);
	default:
		puts("Unrecognized logic op!, defaulting to GL_COPY");
		return s;
	}

}

static int stencil_test(u8 stencil)
{
	int func, ref, mask;
	// TODO what about non-triangles, should use front values, so need to make sure that's set?
	if (c->builtins.gl_FrontFacing) {
		func = c->stencil_func;
		ref = c->stencil_ref;
		mask = c->stencil_valuemask;
	} else {
		func = c->stencil_func_back;
		ref = c->stencil_ref_back;
		mask = c->stencil_valuemask_back;
	}
	switch (func) {
	case GL_NEVER:    return 0;
	case GL_LESS:     return (ref & mask) < (stencil & mask);
	case GL_LEQUAL:   return (ref & mask) <= (stencil & mask);
	case GL_GREATER:  return (ref & mask) > (stencil & mask);
	case GL_GEQUAL:   return (ref & mask) >= (stencil & mask);
	case GL_EQUAL:    return (ref & mask) == (stencil & mask);
	case GL_NOTEQUAL: return (ref & mask) != (stencil & mask);
	case GL_ALWAYS:   return 1;
	default:
		puts("Error: unrecognized stencil function!");
		return 0;
	}

}

static void stencil_op(int stencil, int depth, u8* dest)
{
	int op, ref, mask;
	// make them proper arrays in gl_context?
	GLenum* ops;
	// TODO what about non-triangles, should use front values, so need to make sure that's set?
	if (c->builtins.gl_FrontFacing) {
		ops = &c->stencil_sfail;
		ref = c->stencil_ref;
		mask = c->stencil_writemask;
	} else {
		ops = &c->stencil_sfail_back;
		ref = c->stencil_ref_back;
		mask = c->stencil_writemask_back;
	}
	op = (!stencil) ? ops[0] : ((!depth) ? ops[1] : ops[2]);

	u8 val = *dest;
	switch (op) {
	case GL_KEEP: return;
	case GL_ZERO: val = 0; break;
	case GL_REPLACE: val = ref; break;
	case GL_INCR: if (val < 255) val++; break;
	case GL_INCR_WRAP: val++; break;
	case GL_DECR: if (val > 0) val--; break;
	case GL_DECR_WRAP: val--; break;
	case GL_INVERT: val = ~val;
	}

	*dest = val & mask;

}

static void draw_pixel_vec2(vec4 cf, vec2 pos, float z)
{
/*
 * spec pg 110:
Point rasterization produces a fragment for each framebuffer pixel whose center
lies inside a square centered at the point’s (x w , y w ), with side length equal to the
current point size.

for a 1 pixel size point there are only 3 edge cases where more than 1 pixel center (0.5, 0.5)
would fall on the very edge of a 1 pixel square.  I think just drawing the upper or upper
corner pixel in these cases is fine and makes sense since width and height are actually 0.01 less
than full, see make_viewport_matrix
*/

	draw_pixel(cf, pos.x, pos.y, z, GL_TRUE);
}

static int fragment_processing(int x, int y, float z)
{
	// TODO only clip z planes, just factor in scissor values into
	// min/maxing the boundaries of rasterization, maybe do it always
	// even if scissoring is disabled? (could cause problems if
	// they're turning it on and off with non-standard scissor bounds)
	/*
	// Now handled by "always-on" scissoring/guardband clipping earlier
	if (c->scissor_test) {
		if (x < c->scissor_lx || y < c->scissor_ly || x >= c->scissor_ux || y >= c->scissor_uy) {
			return 0;
		}
	}
	*/

	//MSAA
	
	//Stencil Test TODO have to handle when there is no stencil or depth buffer 
	//(change gl_init to make stencil and depth buffers optional)
	u8* stencil_dest = &c->stencil_buf.lastrow[-y*c->stencil_buf.w + x];
	if (c->stencil_test) {
		if (!stencil_test(*stencil_dest)) {
			stencil_op(0, 1, stencil_dest);
			return 0;
		}
	}

	//Depth test if necessary
	if (c->depth_test) {
		// I made gl_FragDepth read/write, ie same == to gl_FragCoord.z going into the shader
		// so I can just always use gl_FragDepth here
		float dest_depth = ((float*)c->zbuf.lastrow)[-y*c->zbuf.w + x];
		float src_depth = z;  //c->builtins.gl_FragDepth;  // pass as parameter?

		int depth_result = depthtest(src_depth, dest_depth);

		if (c->stencil_test) {
			stencil_op(1, depth_result, stencil_dest);
		}
		if (!depth_result) {
			return 0;
		}
		if (c->depth_mask) {
			((float*)c->zbuf.lastrow)[-y*c->zbuf.w + x] = src_depth;
		}
	} else if (c->stencil_test) {
		stencil_op(1, 1, stencil_dest);
	}
	return 1;
}


static void draw_pixel(vec4 cf, int x, int y, float z, int do_frag_processing)
{
	if (do_frag_processing && !fragment_processing(x, y, z)) {
		return;
	}

	//Blending
	Color dest_color, src_color;
	u32* dest = &((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	dest_color = make_Color((*dest & c->Rmask) >> c->Rshift, (*dest & c->Gmask) >> c->Gshift, (*dest & c->Bmask) >> c->Bshift, (*dest & c->Amask) >> c->Ashift);

	if (c->blend) {
		//TODO clamp in blend_pixel?  return the vec4 and clamp?
		src_color = blend_pixel(cf, Color_to_vec4(dest_color));
	} else {
		cf.x = clampf_01(cf.x);
		cf.y = clampf_01(cf.y);
		cf.z = clampf_01(cf.z);
		cf.w = clampf_01(cf.w);
		src_color = vec4_to_Color(cf);
	}
	//this line needed the negation in the viewport matrix
	//((u32*)c->back_buffer.buf)[y*buf.w+x] = c.a << 24 | c.c << 16 | c.g << 8 | c.b;

	//Logic Ops
	if (c->logic_ops) {
		src_color = logic_ops_pixel(src_color, dest_color);
	}


	//Dithering

	//((u32*)c->back_buffer.buf)[(buf.h-1-y)*buf.w + x] = c.a << 24 | c.c << 16 | c.g << 8 | c.b;
	//((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x] = c.a << 24 | c.c << 16 | c.g << 8 | c.b;
	*dest = (u32)src_color.a << c->Ashift | (u32)src_color.r << c->Rshift | (u32)src_color.g << c->Gshift | (u32)src_color.b << c->Bshift;
}

