
static glContext* c;

static Color blend_pixel(vec4 src, vec4 dst);
static void draw_pixel_vec2(vec4 cf, vec2 pos, float z);
static void draw_pixel(vec4 cf, int x, int y, float z);
static void run_pipeline(GLenum mode, GLint first, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements);

static void draw_triangle_clip(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke, int clip_bit);
static void draw_triangle_point(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_line(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_fill(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_final(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);
static void draw_triangle(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);

static void draw_line_clip(glVertex* v1, glVertex* v2);
static void draw_line_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke);
static void draw_line_smooth_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke);

/* this clip epsilon is needed to avoid some rounding errors after
   several clipping stages */

#define CLIP_EPSILON (1E-5)

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
	//TODO bug where it doesn't correctly cull if part of the triangle goes behind eye
	vec3 normal, tmpvec3 = { 0, 0, 1 };

	vec3 p0 = vec4_to_vec3h(v0->screen_space);
	vec3 p1 = vec4_to_vec3h(v1->screen_space);
	vec3 p2 = vec4_to_vec3h(v2->screen_space);

	//float a;

	//method from spec
	//a = p0.x*p1.y - p1.x*p0.y + p1.x*p2.y - p2.x*p1.y + p2.x*p0.y - p0.x*p2.y;
	//a /= 2;

	normal = cross_product(sub_vec3s(p1, p0), sub_vec3s(p2, p0));

	if (c->front_face == GL_CW) {
		//a = -a;
		normal = negate_vec3(normal);
	}

	//if (a <= 0) {
	if (dot_vec3s(normal, tmpvec3) <= 0) {
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
	c->glverts.a[vert].edge_flag = 1;

	c->glverts.a[vert].clip_code = gl_clipcode(c->builtins.gl_Position);
}


static void vertex_stage(GLint first, GLsizei count, GLsizei instance_id, GLuint base_instance, GLboolean use_elements)
{
	unsigned int i, j, vert, num_enabled;
	u8* buf_pos;

	//save checking if enabled on every loop if we build this first
	//also initialize the vertex_attrib space
	float vec4_init[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	int enabled[GL_MAX_VERTEX_ATTRIBS] = { 0 };
	glVertex_Attrib* v = c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs;
	GLuint elem_buffer = c->vertex_arrays.a[c->cur_vertex_array].element_buffer;

	for (i=0, j=0; i<GL_MAX_VERTEX_ATTRIBS; ++i) {
		memcpy(&c->vertex_attribs_vs[i], vec4_init, sizeof(vec4));

		if (v[i].enabled) {
			if (v[i].divisor == 0) {
				enabled[j++] = i;
			} else if (!(instance_id % v[i].divisor)) {   //set instanced attributes if necessary
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
static void draw_point(glVertex* vert)
{
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	vec3 point = vec4_to_vec3h(vert->screen_space);
	point.z = MAP(point.z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	//TODO not sure if I'm supposed to do this ... doesn't say to in spec but it is called depth clamping
	//but I don't do it for lines or triangles (at least in fill or line mode)
	if (c->depth_clamp)
		point.z = clampf_01(point.z);

	//TODO why not just pass vs_output directly?  hmmm...
	memcpy(fs_input, vert->vs_out, c->vs_output.size*sizeof(float));

	//accounting for pixel centers at 0.5, using truncation
	float x = point.x + 0.5f;
	float y = point.y + 0.5f;
	float p_size = c->point_size;
	float origin = (c->point_spr_origin == GL_UPPER_LEFT) ? -1.0f : 1.0f;

	// Can easily clip whole point when point size <= 1 ...
	if (p_size <= 1) {
		if (x < 0 || y < 0 || x >= c->back_buffer.w || y >= c->back_buffer.h)
			return;
	}

	for (float i = y-p_size/2; i<y+p_size/2; ++i) {
		if (i < 0 || i >= c->back_buffer.h)
			continue;

		for (float j = x-p_size/2; j<x+p_size/2; ++j) {

			if (j < 0 || j >= c->back_buffer.w)
				continue;
			
			// per page 110 of 3.3 spec
			c->builtins.gl_PointCoord.x = 0.5f + ((int)j + 0.5f - point.x)/p_size;
			c->builtins.gl_PointCoord.y = 0.5f + origin * ((int)i + 0.5f - point.y)/p_size;

			SET_VEC4(c->builtins.gl_FragCoord, j, i, point.z, 1/vert->screen_space.w);
			c->builtins.discard = GL_FALSE;
			c->builtins.gl_FragDepth = point.z;
			c->programs.a[c->cur_program].fragment_shader(fs_input, &c->builtins, c->programs.a[c->cur_program].uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, j, i, c->builtins.gl_FragDepth);
		}
	}
}

static void run_pipeline(GLenum mode, GLint first, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements)
{
	unsigned int i, vert;
	int provoke;

	assert(count <= MAX_VERTICES);

	vertex_stage(first, count, instance, base_instance, use_elements);

	//fragment portion
	if (mode == GL_POINTS) {
		for (vert=0, i=first; i<first+count; ++i, ++vert) {
			if (c->glverts.a[vert].clip_code)
				continue;

			c->glverts.a[vert].screen_space = mult_mat4_vec4(c->vp_mat, c->glverts.a[vert].clip_space);

			draw_point(&c->glverts.a[vert]);
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
	// TODO not sure if I should do this since it's supposed to prevent writing to the buffer
	// but not afaik, change the result of the test
	if (!c->depth_mask)
		return 0;

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

		//no need
		//memcpy(v1_out, v1->vs_out, c->vs_output.size*sizeof(float));
		//memcpy(v2_out, v2->vs_out, c->vs_output.size*sizeof(float));

		if (!c->line_smooth)
			draw_line_shader(t1, t2, v1->vs_out, v2->vs_out, provoke);
		else
			draw_line_smooth_shader(t1, t2, v1->vs_out, v2->vs_out, provoke);
	} else {

		d = sub_vec4s(p2, p1);

		tmin = 0;
		tmax = 1;
		if (clip_line( d.x+d.w, -p1.x-p1.w, &tmin, &tmax) &&
		    clip_line(-d.x+d.w,  p1.x-p1.w, &tmin, &tmax) &&
		    clip_line( d.y+d.w, -p1.y-p1.w, &tmin, &tmax) &&
		    clip_line(-d.y+d.w,  p1.y-p1.w, &tmin, &tmax) &&
			clip_line( d.z+d.w, -p1.z-p1.w, &tmin, &tmax) &&
			clip_line(-d.z+d.w,  p1.z-p1.w, &tmin, &tmax)) {

			//printf("%f %f\n", tmin, tmax);

			t1 = add_vec4s(p1, scale_vec4(d, tmin));
			t2 = add_vec4s(p1, scale_vec4(d, tmax));

			t1 = mult_mat4_vec4(c->vp_mat, t1);
			t2 = mult_mat4_vec4(c->vp_mat, t2);
			//print_vec4(t1, "\n");
			//print_vec4(t2, "\n");

			interpolate_clipped_line(v1, v2, v1_out, v2_out, tmin, tmax);

			if (!c->line_smooth)
				draw_line_shader(t1, t2, v1_out, v2_out, provoke);
			else
				draw_line_smooth_shader(t1, t2, v1_out, v2_out, provoke);
		}
	}
}


static void draw_line_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke)
{
	float tmp;
	float* tmp_ptr;

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

	//printf("%f %f %f %f   =\n", i_x1, i_y1, i_x2, i_y2);
	//printf("%f %f %f %f   x_min etc\n", x_min, x_max, y_min, y_max);

	z1 = MAP(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	z2 = MAP(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	//4 cases based on slope
	if (m <= -1) {     //(-infinite, -1]
		//printf("slope <= -1\n");
		for (x = x_min, y = y_max; y>=y_min && x<=x_max; --y) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
			c->builtins.discard = GL_FALSE;
			c->builtins.gl_FragDepth = z;
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth);

line_1:
			if (line_func(&line, x+0.5f, y-1) < 0) //A*(x+0.5f) + B*(y-1) + C < 0)
				++x;
		}
	} else if (m <= 0) {     //(-1, 0]
		//printf("slope = (-1, 0]\n");
		for (x = x_min, y = y_max; x<=x_max && y>=y_min; ++x) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
			c->builtins.discard = GL_FALSE;
			c->builtins.gl_FragDepth = z;
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth);

line_2:
			if (line_func(&line, x+1, y-0.5f) > 0) //A*(x+1) + B*(y-0.5f) + C > 0)
				--y;
		}
	} else if (m <= 1) {     //(0, 1]
		//printf("slope = (0, 1]\n");
		for (x = x_min, y = y_min; x <= x_max && y <= y_max; ++x) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
			c->builtins.discard = GL_FALSE;
			c->builtins.gl_FragDepth = z;
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth);

line_3:
			if (line_func(&line, x+1, y+0.5f) < 0) //A*(x+1) + B*(y+0.5f) + C < 0)
				++y;
		}

	} else {    //(1, +infinite)
		//printf("slope > 1\n");
		for (x = x_min, y = y_min; y<=y_max && x <= x_max; ++y) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
			c->builtins.discard = GL_FALSE;
			c->builtins.gl_FragDepth = z;
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth);

line_4:
			if (line_func(&line, x+0.5f, y+1) > 0) //A*(x+0.5f) + B*(y+1) + C > 0)
				++x;
		}
	}
}

// WARNING: this function is subject to serious change or removal and is currently unused (GL_LINE_SMOOTH unsupported)
// TODO do it right, handle depth test correctly since we moved it into draw_pixel
static void draw_line_smooth_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke)
{
	float tmp;
	float* tmp_ptr;

	frag_func fragment_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniform = c->programs.a[c->cur_program].uniform;
	int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;

	vec3 hp1 = vec4_to_vec3h(v1);
	vec3 hp2 = vec4_to_vec3h(v2);
	float x1 = hp1.x, x2 = hp2.x, y1 = hp1.y, y2 = hp2.y;
	float z1 = hp1.z, z2 = hp2.z;

	float w1 = v1.w;
	float w2 = v2.w;

	int x, j;

	int steep = fabsf(y2 - y1) > fabsf(x2 - x1);

	if (steep) {
		tmp = x1;
		x1 = y1;
		y1 = tmp;
		tmp = x2;
		x2 = y2;
		y2 = tmp;
	}
	if (x1 > x2) {
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

	float dx = x2 - x1;
	float dy = y2 - y1;
	float gradient = dy / dx;

	float xend = x1 + 0.5f;
	float yend = y1 + gradient * (xend - x1);

	float xgap = 1.0 - modff(x1 + 0.5, &tmp);
	float xpxl1 = xend;
	float ypxl1;
	modff(yend, &ypxl1);


	//choose to compare against just one pixel for depth test instead of both
	z1 = MAP(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	if (steep) {
		if (!c->depth_test || (!fragdepth_or_discard &&
			depthtest(z1, ((float*)c->zbuf.lastrow)[-(int)xpxl1*c->zbuf.w + (int)ypxl1]))) {

			if (!c->fragdepth_or_discard && c->depth_test) { //hate this double check but depth buf is only update if enabled
				((float*)c->zbuf.lastrow)[-(int)xpxl1*c->zbuf.w + (int)ypxl1] = z1;
				((float*)c->zbuf.lastrow)[-(int)xpxl1*c->zbuf.w + (int)(ypxl1+1)] = z1;
			}

			SET_VEC4(c->builtins.gl_FragCoord, ypxl1, xpxl1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = (1.0 - modff(yend, &tmp)) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl1, xpxl1, c->builtins.gl_FragDepth);

			SET_VEC4(c->builtins.gl_FragCoord, ypxl1+1, xpxl1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl1+1, xpxl1, c->builtins.gl_FragDepth);
		}
	} else {
		if (!c->depth_test || (!fragdepth_or_discard &&
			depthtest(z1, ((float*)c->zbuf.lastrow)[-(int)ypxl1*c->zbuf.w + (int)xpxl1]))) {

			if (!c->fragdepth_or_discard && c->depth_test) { //hate this double check but depth buf is only update if enabled
				((float*)c->zbuf.lastrow)[-(int)ypxl1*c->zbuf.w + (int)xpxl1] = z1;
				((float*)c->zbuf.lastrow)[-(int)(ypxl1+1)*c->zbuf.w + (int)xpxl1] = z1;
			}

			SET_VEC4(c->builtins.gl_FragCoord, xpxl1, ypxl1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = (1.0 - modff(yend, &tmp)) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl1, ypxl1, c->builtins.gl_FragDepth);

			SET_VEC4(c->builtins.gl_FragCoord, xpxl1, ypxl1+1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl1, ypxl1+1, c->builtins.gl_FragDepth);
		}
	}


	float intery = yend + gradient; //first y-intersection for main loop


	xend = x2 + 0.5f;
	yend = y2 + gradient * (xend - x2);

	xgap = modff(x2 + 0.5, &tmp);
	float xpxl2 = xend;
	float ypxl2;
	modff(yend, &ypxl2);

	z2 = MAP(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	if (steep) {
		if (!c->depth_test || (!fragdepth_or_discard &&
			depthtest(z2, ((float*)c->zbuf.lastrow)[-(int)xpxl2*c->zbuf.w + (int)ypxl2]))) {

			if (!c->fragdepth_or_discard && c->depth_test) {
				((float*)c->zbuf.lastrow)[-(int)xpxl2*c->zbuf.w + (int)ypxl2] = z2;
				((float*)c->zbuf.lastrow)[-(int)xpxl2*c->zbuf.w + (int)(ypxl2+1)] = z2;
			}

			SET_VEC4(c->builtins.gl_FragCoord, ypxl2, xpxl2, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = (1.0 - modff(yend, &tmp)) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl2, xpxl2, c->builtins.gl_FragDepth);

			SET_VEC4(c->builtins.gl_FragCoord, ypxl2+1, xpxl2, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl2+1, xpxl2, c->builtins.gl_FragDepth);
		}

	} else {
		if (!c->depth_test || (!fragdepth_or_discard &&
			depthtest(z2, ((float*)c->zbuf.lastrow)[-(int)ypxl2*c->zbuf.w + (int)xpxl2]))) {

			if (!c->fragdepth_or_discard && c->depth_test) {
				((float*)c->zbuf.lastrow)[-(int)ypxl2*c->zbuf.w + (int)xpxl2] = z2;
				((float*)c->zbuf.lastrow)[-(int)(ypxl2+1)*c->zbuf.w + (int)xpxl2] = z2;
			}

			SET_VEC4(c->builtins.gl_FragCoord, xpxl2, ypxl2, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = (1.0 - modff(yend, &tmp)) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl2, ypxl2, c->builtins.gl_FragDepth);

			SET_VEC4(c->builtins.gl_FragCoord, xpxl2, ypxl2+1, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl2, ypxl2+1, c->builtins.gl_FragDepth);
		}
	}

	//use the fast, inaccurate calculation of t since this algorithm is already
	//slower than the normal line drawing, pg 111 glspec if I ever want to fix it
	float range = ceil(x2-x1);
	float t, z, w;
	for (j=1, x = xpxl1 + 1; x < xpxl2; ++x, ++j, intery += gradient) {
		t = j/range;

		z = (1 - t) * z1 + t * z2;
		w = (1 - t) * w1 + t * w2;

		if (steep) {
			if (!c->fragdepth_or_discard && c->depth_test) {
				if (!depthtest(z, ((float*)c->zbuf.lastrow)[-(int)x*c->zbuf.w + (int)intery])) {
					continue;
				} else {
					((float*)c->zbuf.lastrow)[-(int)x*c->zbuf.w + (int)intery] = z;
					((float*)c->zbuf.lastrow)[-(int)x*c->zbuf.w + (int)(intery+1)] = z;
				}
			}

			SET_VEC4(c->builtins.gl_FragCoord, intery, x, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = 1.0 - modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, intery, x, c->builtins.gl_FragDepth);

			SET_VEC4(c->builtins.gl_FragCoord, intery+1, x, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, intery+1, x, c->builtins.gl_FragDepth);

		} else {
			if (!c->fragdepth_or_discard && c->depth_test) {
				if (!depthtest(z, ((float*)c->zbuf.lastrow)[-(int)intery*c->zbuf.w + (int)x])) {
					continue;
				} else {
					((float*)c->zbuf.lastrow)[-(int)intery*c->zbuf.w + (int)x] = z;
					((float*)c->zbuf.lastrow)[-(int)(intery+1)*c->zbuf.w + (int)x] = z;
				}
			}

			SET_VEC4(c->builtins.gl_FragCoord, x, intery, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = 1.0 - modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, intery, c->builtins.gl_FragDepth);

			SET_VEC4(c->builtins.gl_FragCoord, x, intery+1, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, intery+1, c->builtins.gl_FragDepth);

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
			//printf("culling back face\n");
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
	
	q->clip_code = gl_clipcode(q->clip_space);
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
		while (clip_bit < 6 && (c_or & (1 << clip_bit)) == 0)  {
			++clip_bit;
		}

		/* this test can be true only in case of rounding errors */
		if (clip_bit == 6) {
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

			tmp2.edge_flag = 1;
			tmp1.edge_flag = 0;
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

			tmp1.edge_flag = 1;
			tmp2.edge_flag = q[2]->edge_flag;
			draw_triangle_clip(q[0], &tmp1, &tmp2, provoke, clip_bit+1);
		}
	}
}

static void draw_triangle_point(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke)
{
	//TODO provoke?
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	vec3 point;
	glVertex* vert[3] = { v0, v1, v2 };

	for (int i=0; i<3; ++i) {
		if (!vert[i]->edge_flag) //TODO doesn't work
			continue;

		point = vec4_to_vec3h(vert[i]->screen_space);
		point.z = MAP(point.z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

		//TODO not sure if I'm supposed to do this ... doesn't say to in spec but it is called depth clamping
		if (c->depth_clamp)
			point.z = clampf_01(point.z);

		for (int j=0; j<c->vs_output.size; ++j) {
			if (c->vs_output.interpolation[j] != FLAT) {
				fs_input[j] = vert[i]->vs_out[j]; //would be correct from clipping
			} else {
				fs_input[j] = c->vs_output.output_buf.a[provoke*c->vs_output.size + j];
			}
		}

		c->builtins.discard = GL_FALSE;
		c->builtins.gl_FragDepth = point.z;
		c->programs.a[c->cur_program].fragment_shader(fs_input, &c->builtins, c->programs.a[c->cur_program].uniform);
		if (!c->builtins.discard)
			draw_pixel(c->builtins.gl_FragColor, point.x, point.y, c->builtins.gl_FragDepth);
	}
}

static void draw_triangle_line(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke)
{
	if (v0->edge_flag)
		draw_line_shader(v0->screen_space, v1->screen_space, v0->vs_out, v1->vs_out, provoke);
	if (v1->edge_flag)
		draw_line_shader(v1->screen_space, v2->screen_space, v1->vs_out, v2->vs_out, provoke);
	if (v2->edge_flag)
		draw_line_shader(v2->screen_space, v0->screen_space, v2->vs_out, v0->vs_out, provoke);
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
	float max_depth_slope = 0;
	float poly_offset = 0;

	if (c->poly_offset) {
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
		poly_offset = max_depth_slope * c->poly_factor + c->poly_units * SMALLEST_INCR;
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

	int ix_max = roundf(x_max);
	int iy_max = roundf(y_max);


	/*
	 * testing without this
	x_min = MAX(0, x_min);
	x_max = MIN(c->back_buffer.w-1, x_max);
	y_min = MAX(0, y_min);
	y_max = MIN(c->back_buffer.h-1, y_max);

	x_min = MAX(c->x_min, x_min);
	x_max = MIN(c->x_max, x_max);
	y_min = MAX(c->y_min, y_min);
	y_max = MIN(c->y_max, y_max);
	*/

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

	// TODO: Should I just use the glContext member like everywhere else so
	// I don't have to copy gl_InstanceID over?
	Shader_Builtins builtins;
	builtins.gl_InstanceID = c->builtins.gl_InstanceID;

	#pragma omp parallel for private(x, y, alpha, beta, gamma, z, tmp, tmp2, builtins, fs_input)
	for (int iy = y_min; iy<iy_max; ++iy) {
		y = iy + 0.5f;

		for (int ix = x_min; ix<ix_max; ++ix) {
			x = ix + 0.5f; //center of min pixel

			//see page 117 of glspec for alternative method
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

					z = MAP(z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far); //TODO move out (ie can I map hp1.z etc.)?
					z += poly_offset;

					// TODO have a macro that turns on pre-fragment shader depthtest/scissor test?

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
					c->programs.a[c->cur_program].fragment_shader(fs_input, &builtins, c->programs.a[c->cur_program].uniform);
					if (!builtins.discard) {

						draw_pixel(builtins.gl_FragColor, x, y, builtins.gl_FragDepth);
					}
				}
			}
		}
	}
}


// TODO should this be done in colors/integers not vec4/floats?
static Color blend_pixel(vec4 src, vec4 dst)
{
	vec4* cnst = &c->blend_color;
	float i = MIN(src.w, 1-dst.w);

	vec4 Cs, Cd;

	switch (c->blend_sfactor) {
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
	case GL_CONSTANT_COLOR:           Cs = *cnst;                                            break;
	case GL_ONE_MINUS_CONSTANT_COLOR: SET_VEC4(Cs, 1-cnst->x,1-cnst->y,1-cnst->z,1-cnst->w); break;
	case GL_CONSTANT_ALPHA:           SET_VEC4(Cs, cnst->w, cnst->w, cnst->w, cnst->w);      break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: SET_VEC4(Cs, 1-cnst->w,1-cnst->w,1-cnst->w,1-cnst->w); break;

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
		printf("error unrecognized blend_sfactor!\n");
		break;
	}

	switch (c->blend_dfactor) {
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
	case GL_CONSTANT_COLOR:           Cd = *cnst;                                            break;
	case GL_ONE_MINUS_CONSTANT_COLOR: SET_VEC4(Cd, 1-cnst->x,1-cnst->y,1-cnst->z,1-cnst->w); break;
	case GL_CONSTANT_ALPHA:           SET_VEC4(Cd, cnst->w, cnst->w, cnst->w, cnst->w);      break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: SET_VEC4(Cd, 1-cnst->w,1-cnst->w,1-cnst->w,1-cnst->w); break;

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
		printf("error unrecognized blend_dfactor!\n");
		break;
	}

	vec4 result;

	switch (c->blend_equation) {
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
		printf("error unrecognized blend_equation!\n");
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
TODO point size > 1
*/

	draw_pixel(cf, pos.x, pos.y, z);
}


static void draw_pixel(vec4 cf, int x, int y, float z)
{
	if (c->scissor_test) {
		if (x < c->scissor_lx || y < c->scissor_ly || x >= c->scissor_ux || y >= c->scissor_uy) {
			return;
		}
	}

	//MSAA
	
	//Stencil Test TODO have to handle when there is no stencil or depth buffer 
	//(change gl_init to make stencil and depth buffers optional)
	u8* stencil_dest = &c->stencil_buf.lastrow[-y*c->stencil_buf.w + x];
	if (c->stencil_test) {
		if (!stencil_test(*stencil_dest)) {
			stencil_op(0, 1, stencil_dest);
			return;
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
			return;
		}
		((float*)c->zbuf.lastrow)[-y*c->zbuf.w + x] = src_depth;
	} else if (c->stencil_test) {
		stencil_op(1, 1, stencil_dest);
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
	//this line neded the negation in the viewport matrix
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

