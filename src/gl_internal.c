
static glContext* c;

static Color blend_pixel(vec4 src, vec4 dst);
static int fragment_processing(int x, int y, float z);
static void draw_pixel(vec4 cf, int x, int y, float z, int do_frag_processing);
static void run_pipeline(GLenum mode, const GLvoid* indices, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements);

static float calc_poly_offset(vec3 hp0, vec3 hp1, vec3 hp2);

static void draw_triangle_clip(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke, int clip_bit);
static void draw_triangle_point(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_line(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_fill(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_final(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);
static void draw_triangle(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);

static void draw_line_clip(glVertex* v1, glVertex* v2);

// This is the prototype for either implementation; only one is defined based on
// whether PGL_BETTER_THICK_LINES is defined
static void draw_thick_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset);

// Only width 1 supported for now
static void draw_aa_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset);

/* this clip epsilon is needed to avoid some rounding errors after
   several clipping stages */

#define CLIP_EPSILON (1E-5)
#define CLIPZ_MASK 0x3
#define CLIPX_TEST(x) (x >= c->lx && x < c->ux)
#define CLIPY_TEST(y) (y >= c->ly && y < c->uy)
#define CLIPXY_TEST(x, y) (x >= c->lx && x < c->ux && y >= c->ly && y < c->uy)


static inline int gl_clipcode(vec4 pt)
{
	float w;

	w = pt.w * (1.0 + CLIP_EPSILON);
	return
		(((pt.z < -w) |
		 ((pt.z >  w) << 1)) &
		 ((!c->depth_clamp) |
		  (!c->depth_clamp) << 1)) |

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

// TODO make a config macro that turns this into an inline function/macro that
// only supports float for a small perf boost
static vec4 get_v_attrib(glVertex_Attrib* v, GLsizei i)
{
	// v->buf will be 0 for a client array and buf[0].data
	// is always NULL so this works for both but we have to cast
	// the pointer to GLsizeiptr because adding an offset to a NULL pointer
	// is undefined.  So, do the math as numbers and convert back to a pointer
	GLsizeiptr buf_data = (GLsizeiptr)c->buffers.a[v->buf].data;
	u8* u8p = (u8*)(buf_data + v->offset + v->stride*i);

	i8* i8p = (i8*)u8p;
	u16* u16p = (u16*)u8p;
	i16* i16p = (i16*)u8p;
	u32* u32p = (u32*)u8p;
	i32* i32p = (i32*)u8p;

	vec4 tmpvec4 = { 0.0f, 0.0f, 0.0f, 1.0f };
	float* tv = (float*)&tmpvec4;
	GLenum type = v->type;

	if (type < GL_FLOAT) {
		for (int i=0; i<v->size; i++) {
			if (v->normalized) {
				switch (type) {
				case GL_BYTE:           tv[i] = rsw_mapf(i8p[i], INT8_MIN, INT8_MAX, -1.0f, 1.0f); break;
				case GL_UNSIGNED_BYTE:  tv[i] = rsw_mapf(u8p[i], 0, UINT8_MAX, 0.0f, 1.0f); break;
				case GL_SHORT:          tv[i] = rsw_mapf(i16p[i], INT16_MIN,INT16_MAX, 0.0f, 1.0f); break;
				case GL_UNSIGNED_SHORT: tv[i] = rsw_mapf(u16p[i], 0, UINT16_MAX, 0.0f, 1.0f); break;
				case GL_INT:            tv[i] = rsw_mapf(i32p[i], INT32_MIN, INT32_MAX, 0.0f, 1.0f); break;
				case GL_UNSIGNED_INT:   tv[i] = rsw_mapf(u32p[i], 0, UINT32_MAX, 0.0f, 1.0f); break;
				}
			} else {
				switch (type) {
				case GL_BYTE:           tv[i] = i8p[i]; break;
				case GL_UNSIGNED_BYTE:  tv[i] = u8p[i]; break;
				case GL_SHORT:          tv[i] = i16p[i]; break;
				case GL_UNSIGNED_SHORT: tv[i] = u16p[i]; break;
				case GL_INT:            tv[i] = i32p[i]; break;
				case GL_UNSIGNED_INT:   tv[i] = u32p[i]; break;
				}
			}
		}
	} else {
		// TODO support GL_DOUBLE

		memcpy(tv, u8p, sizeof(float)*v->size);
	}

	//c->cur_vertex_array->vertex_attribs[enabled[j]].buf->data;
	return tmpvec4;
}

// TODO Possibly split for optimization and future parallelization, prep all verts first then do all shader calls at once
// Will need num_verts * vertex_attribs_vs[] space rather than a single attribute staging area...
static void do_vertex(glVertex_Attrib* v, int* enabled, int num_enabled, int i, int vert)
{
	// copy/prep vertex attributes from buffers into appropriate positions for vertex shader to access
	for (int j=0; j<num_enabled; ++j) {
		c->vertex_attribs_vs[enabled[j]] = get_v_attrib(&v[enabled[j]], i);
	}

	float* vs_out = &c->vs_output.output_buf[vert*c->vs_output.size];
	c->programs.a[c->cur_program].vertex_shader(vs_out, c->vertex_attribs_vs, &c->builtins, c->programs.a[c->cur_program].uniform);

	c->glverts.a[vert].vs_out = vs_out;
	c->glverts.a[vert].clip_space = c->builtins.gl_Position;

	// no use setting here because of TRIANGLE_STRIP
	// and TRIANGLE_FAN. While I don't properly
	// generate "primitives", I do expand create unique vertices
	// to process when the user uses an element (index) buffer.
	//
	// so it's done in draw_triangle()
	//c->glverts.a[vert].edge_flag = 1;

	c->glverts.a[vert].clip_code = gl_clipcode(c->builtins.gl_Position);
}

// TODO naming issue/refactor?
// When used with Draw*Arrays* indices is really the index of the first vertex to be used
// When used for Draw*Elements* indices is either a byte offset of the first index or
// an actual pointer to the array of indices depending on whether an ELEMENT_ARRAY_BUFFER is bound
//
// use_elems_type is either 0/false or one of GL_UNSIGNED_BYTE/SHORT/INT
// so used as a boolean and an enum
static void vertex_stage(const GLvoid* indices, GLsizei count, GLsizei instance_id, GLuint base_instance, GLenum use_elems_type)
{
	int i, j, vert, num_enabled;

	glVertex_Attrib* v = c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs;
	GLuint elem_buffer = c->vertex_arrays.a[c->cur_vertex_array].element_buffer;

	//save checking if enabled on every loop if we build this first
	//also initialize the vertex_attrib space
	// TODO does creating enabled array actually help perf?  At what number
	// of GL_MAX_VERTEX_ATTRIBS and vertices does it become a benefit?
	int enabled[GL_MAX_VERTEX_ATTRIBS] = { 0 };
	for (i=0, j=0; i<GL_MAX_VERTEX_ATTRIBS; ++i) {
		if (v[i].enabled) {
			if (v[i].divisor == 0) {
				enabled[j++] = i;
			} else if (!(instance_id % v[i].divisor)) {
				//set instanced attributes if necessary
				int n = instance_id/v[i].divisor + base_instance;
				c->vertex_attribs_vs[i] = get_v_attrib(&v[i], n);
			}
		}
	}
	num_enabled = j;

	cvec_reserve_glVertex(&c->glverts, count);

	// gl_InstanceID always starts at 0, base_instance is only added when grabbing attributes
	// https://www.khronos.org/opengl/wiki/Built-in_Variable_(GLSL)#Vertex_shader_inputs
	c->builtins.gl_InstanceID = instance_id;
	c->builtins.gl_BaseInstance = base_instance;
	GLsizeiptr first = (GLsizeiptr)indices;

	if (!use_elems_type) {
		for (vert=0, i=first; i<first+count; ++i, ++vert) {
			do_vertex(v, enabled, num_enabled, i, vert);
		}
	} else {
		GLuint* uint_array = (GLuint*)indices;
		GLushort* ushort_array = (GLushort*)indices;
		GLubyte* ubyte_array = (GLubyte*)indices;
		if (c->bound_buffers[GL_ELEMENT_ARRAY_BUFFER-GL_ARRAY_BUFFER]) {
			uint_array = (GLuint*)(c->buffers.a[elem_buffer].data + first);
			ushort_array = (GLushort*)(c->buffers.a[elem_buffer].data + first);
			ubyte_array = (GLubyte*)(c->buffers.a[elem_buffer].data + first);
		}
		if (use_elems_type == GL_UNSIGNED_BYTE) {
			for (i=0; i<count; ++i) {
				do_vertex(v, enabled, num_enabled, ubyte_array[i], i);
			}
		} else if (use_elems_type == GL_UNSIGNED_SHORT) {
			for (i=0; i<count; ++i) {
				do_vertex(v, enabled, num_enabled, ushort_array[i], i);
			}
		} else {
			for (i=0; i<count; ++i) {
				do_vertex(v, enabled, num_enabled, uint_array[i], i);
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
	point.z = rsw_mapf(point.z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	// TODO necessary for non-perspective?
	//if (c->depth_clamp)
	//	clamp(point.z, c->depth_range_near, c->depth_range_far);

	Shader_Builtins builtins;
	// 3.3 spec pg 110 says r,q are supposed to be replaced with 0 and 1...
	// but PointCoord is a vec2 and that is not in the 4.6 spec so it must be a typo

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

static void run_pipeline(GLenum mode, const GLvoid* indices, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements)
{
	GLsizei i;
	int provoke;

	PGL_ASSERT(count <= PGL_MAX_VERTICES);

	vertex_stage(indices, count, instance, base_instance, use_elements);

	//fragment portion
	if (mode == GL_POINTS) {
		for (i=0; i<count; ++i) {
			// clip only z and let partial points (size > 1)
			// show even if the center would have been clipped
			if (c->glverts.a[i].clip_code & CLIPZ_MASK)
				continue;

			c->glverts.a[i].screen_space = mult_mat4_vec4(c->vp_mat, c->glverts.a[i].clip_space);

			draw_point(&c->glverts.a[i], 0.0f);
		}
	} else if (mode == GL_LINES) {
		for (i=0; i<count-1; i+=2) {
			draw_line_clip(&c->glverts.a[i], &c->glverts.a[i+1]);
		}
	} else if (mode == GL_LINE_STRIP) {
		for (i=0; i<count-1; i++) {
			draw_line_clip(&c->glverts.a[i], &c->glverts.a[i+1]);
		}
	} else if (mode == GL_LINE_LOOP) {
		for (i=0; i<count-1; i++) {
			draw_line_clip(&c->glverts.a[i], &c->glverts.a[i+1]);
		}
		//draw ending line from last to first point
		draw_line_clip(&c->glverts.a[count-1], &c->glverts.a[0]);

	} else if (mode == GL_TRIANGLES) {
		provoke = (c->provoking_vert == GL_LAST_VERTEX_CONVENTION) ? 2 : 0;

		for (i=0; i<count-2; i+=3) {
			draw_triangle(&c->glverts.a[i], &c->glverts.a[i+1], &c->glverts.a[i+2], i+provoke);
		}

	} else if (mode == GL_TRIANGLE_STRIP) {
		unsigned int a=0, b=1, toggle = 0;
		provoke = (c->provoking_vert == GL_LAST_VERTEX_CONVENTION) ? 0 : -2;

		for (i=2; i<count; ++i) {
			draw_triangle(&c->glverts.a[a], &c->glverts.a[b], &c->glverts.a[i], i+provoke);

			if (!toggle)
				a = i;
			else
				b = i;

			toggle = !toggle;
		}
	} else if (mode == GL_TRIANGLE_FAN) {
		provoke = (c->provoking_vert == GL_LAST_VERTEX_CONVENTION) ? 0 : -1;

		for (i=2; i<count; ++i) {
			draw_triangle(&c->glverts.a[0], &c->glverts.a[i-1], &c->glverts.a[i], i+provoke);
		}
	}
}


static int depthtest(u32 zval, u32 zbufval)
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
	float* vs_output = &c->vs_output.output_buf[0];

	float inv_wa = 1.0/wa;
	float inv_wb = 1.0/wb;

	for (int i=0; i<c->vs_output.size; ++i) {
		if (c->vs_output.interpolation[i] == PGL_SMOOTH) {
			c->fs_input[i] = (v1_out[i]*inv_wa + t*(v2_out[i]*inv_wb - v1_out[i]*inv_wa)) / (inv_wa + t*(inv_wb - inv_wa));

		} else if (c->vs_output.interpolation[i] == PGL_NOPERSPECTIVE) {
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

	vec3 hp1, hp2;

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

		hp1 = vec4_to_vec3h(t1);
		hp2 = vec4_to_vec3h(t2);

		if (c->line_smooth) {
			draw_aa_line(hp1, hp2, t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
		} else {
			draw_thick_line(hp1, hp2, t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
		}
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

			hp1 = vec4_to_vec3h(t1);
			hp2 = vec4_to_vec3h(t2);

			if (c->line_smooth) {
				draw_aa_line(hp1, hp2, t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
			} else {
				draw_thick_line(hp1, hp2, t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
			}
		}
	}
}

#ifndef PGL_BETTER_THICK_LINES
static void draw_thick_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
{
	float tmp;
	float* tmp_ptr;

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
	z1 = rsw_mapf(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	z2 = rsw_mapf(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	float width = roundf(c->line_width);
	if (!width) {
		width = 1.0f;
	}
	//int wi = width;
	float half_w = width * 0.5f;

	// TODO solve off by one issues:
	//   See test outputs where there seems to occasionally be an extra pixel
	//   Also might be drawing lines one pixel lower on the minor axis
	//
	//   Also, I shouldn't have to clamp t, technically if it's outside [0,1]
	//   it's not part of the line so it should be skipped or blended if the
	//   pixel is partially covered and you're doing AA. Or mabye I do have to
	//   clamp but be more particular about starting and ending pixel which..
	//
	// TODO I need to do anyway, since GL specifically says two lines which
	// share an endpoint should *not* evaluate that pixel twice and which
	// gets it should be deterministic
	//
	// TODO maybe try simplifying into only 2 cases steep or not steep like
	// AA algorithm

	//4 cases based on slope
	if (m <= -1) {     //(-infinite, -1]
		//printf("slope <= -1\n");
		for (x = x_min, y = y_max; y>=y_min && x<=x_max; --y) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;
			t = clamp_01(t);

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			for (float j=x-half_w; j<x+half_w; ++j) {
				if (CLIPXY_TEST(j, y)) {
					if (fragdepth_or_discard || fragment_processing(j, y, z)) {
						SET_VEC4(c->builtins.gl_FragCoord, j, y, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard)
							draw_pixel(c->builtins.gl_FragColor, j, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}

			if (line_func(&line, x+0.5f, y-1) < 0) //A*(x+0.5f) + B*(y-1) + C < 0)
				++x;
		}
	} else if (m <= 0) {     //(-1, 0]
		//printf("slope = (-1, 0]\n");
		for (x = x_min, y = y_max; x<=x_max && y>=y_min; ++x) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;
			t = clamp_01(t);

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			for (float j=y-half_w; j<y+half_w; ++j) {
				if (CLIPXY_TEST(x, j)) {
					if (fragdepth_or_discard || fragment_processing(x, j, z)) {

						SET_VEC4(c->builtins.gl_FragCoord, x, j, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard)
							draw_pixel(c->builtins.gl_FragColor, x, j, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}
			if (line_func(&line, x+1, y-0.5f) > 0) //A*(x+1) + B*(y-0.5f) + C > 0)
				--y;
		}
	} else if (m <= 1) {     //(0, 1]
		//printf("slope = (0, 1]\n");
		for (x = x_min, y = y_min; x <= x_max && y <= y_max; ++x) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;
			t = clamp_01(t);

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			for (float j=y-half_w; j<y+half_w; ++j) {
				if (CLIPXY_TEST(x, j)) {
					if (fragdepth_or_discard || fragment_processing(x, j, z)) {

						SET_VEC4(c->builtins.gl_FragCoord, x, j, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard)
							draw_pixel(c->builtins.gl_FragColor, x, j, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}
			if (line_func(&line, x+1, y+0.5f) < 0) //A*(x+1) + B*(y+0.5f) + C < 0)
				++y;
		}

	} else {    //(1, +infinite)
		//printf("slope > 1\n");
		for (x = x_min, y = y_min; y<=y_max && x <= x_max; ++y) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;
			t = clamp_01(t);

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			for (float j=x-half_w; j<x+half_w; ++j) {
				if (CLIPXY_TEST(j, y)) {
					if (fragdepth_or_discard || fragment_processing(j, y, z)) {

						SET_VEC4(c->builtins.gl_FragCoord, j, y, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard)
							draw_pixel(c->builtins.gl_FragColor, j, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}
			if (line_func(&line, x+0.5f, y+1) > 0) //A*(x+0.5f) + B*(y+1) + C > 0)
				++x;
		}
	}
}
#else
static void draw_thick_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
{
	float tmp;
	float* tmp_ptr;

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

	// Need half for the rest
	float width = c->line_width * 0.5f;

	//calculate slope and implicit line parameters once
	float m = (y2-y1)/(x2-x1);
	Line line = make_Line(x1, y1, x2, y2);
	normalize_line(&line);

	vec2 p1 = { x1, y1 };
	vec2 p2 = { x2, y2 };
	vec2 v12 = sub_vec2s(p2, p1);
	vec2 v1r, pr; // v2r

	float dot_1212 = dot_vec2s(v12, v12);

	float x_min, x_max, y_min, y_max;

	x_min = p1.x - width;
	x_max = p2.x + width;
	if (m <= 0) {
		y_min = p2.y - width;
		y_max = p1.y + width;
	} else {
		y_min = p1.y - width;
		y_max = p2.y + width;
	}

	// clipping/scissoring against side planes here
	x_min = MAX(c->lx, x_min);
	x_max = MIN(c->ux, x_max);
	y_min = MAX(c->ly, y_min);
	y_max = MIN(c->uy, y_max);
	// end clipping
	
	y_min = floor(y_min) + 0.5f;
	x_min = floor(x_min) + 0.5f;
	float x_mino = x_min;
	float x_maxo = x_max;


	frag_func fragment_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniform = c->programs.a[c->cur_program].uniform;
	int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;

	float t, x, y, z, w, e, dist;
	//float width_squared = width*width;

	// calculate x_max or just use last logic?
	//int last = 0;

	//printf("%f %f %f %f   =\n", i_x1, i_y1, i_x2, i_y2);
	//printf("%f %f %f %f   x_min etc\n", x_min, x_max, y_min, y_max);

	// TODO should be done for each fragment, after poly_offset is added?
	z1 = rsw_mapf(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	z2 = rsw_mapf(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	for (y = y_min; y < y_max; ++y) {
		pr.y = y;
		//last = GL_FALSE;

		// could also check fabsf(line.A) > epsilon
		if (fabsf(m) > 0.0001f) {
			x_min = (-width - line.C - line.B*y)/line.A;
			x_max = (width - line.C - line.B*y)/line.A;
			if (x_min > x_max) {
				tmp = x_min;
				x_min = x_max;
				x_max = tmp;
			}
			x_min = MAX(c->lx, x_min);
			x_min = floorf(x_min) + 0.5f;
			x_max = MIN(c->ux, x_max);
			//printf("%f %f   x_min etc\n", x_min, x_max);
		} else {
			x_min = x_mino;
			x_max = x_maxo;
		}
		for (x = x_min; x < x_max; ++x) {
			pr.x = x;
			v1r = sub_vec2s(pr, p1);
			//v2r = sub_vec2s(pr, p2);
			e = dot_vec2s(v1r, v12);

			// c lies past the ends of the segment v12
			if (e <= 0.0f || e >= dot_1212) {
				continue;
			}

			// can do this because we normalized the line equation
			// TODO square or fabsf?
			dist = line_func(&line, pr.x, pr.y);
			//if (dist*dist < width_squared) {
			if (fabsf(dist) < width) {
				t = e / dot_1212;

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
			//	last = GL_TRUE;
			//} else if (last) {
			//	break; // we have passed the right edge of the line on this row
			}
		}
	}
}
#endif



// As an adaptation of Xialin Wu's AA line algorithm, unlike all other GL
// rasterization functions, this uses integer pixel centers and passes
// those in glFragCoord.

#define ipart_(X) ((int)(X))
#define round_(X) ((int)(((float)(X))+0.5f))
#define fpart_(X) (((float)(X))-(float)ipart_(X))
#define rfpart_(X) (1.0f-fpart_(X))

#define swap_(a, b) do{ __typeof__(a) tmp;  tmp = a; a = b; b = tmp; } while(0)
static void draw_aa_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
{
	float t, z, w;
	int x, y;

	frag_func fragment_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniform = c->programs.a[c->cur_program].uniform;
	int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;

	float x1 = hp1.x, x2 = hp2.x, y1 = hp1.y, y2 = hp2.y;
	float z1 = hp1.z, z2 = hp2.z;

	float dx = x2 - x1;
	float dy = y2 - y1;

	if (fabsf(dx) > fabsf(dy)) {
		if (x2 < x1) {
			swap_(x1, x2);
			swap_(y1, y2);
			swap_(z1, z2);
			swap_(w1, w2);
			swap_(v1_out, v2_out);
		}

		vec2 p1 = { x1, y1 }, p2 = { x2, y2 };
		vec2 pr, sub_p2p1 = sub_vec2s(p2, p1);
		float line_length_squared = length_vec2(sub_p2p1);
		line_length_squared *= line_length_squared;

		// TODO should be done for each fragment, after poly_offset is added?
		z1 = rsw_mapf(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
		z2 = rsw_mapf(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

		float gradient = dy / dx;
		float xend = round_(x1);
		float yend = y1 + gradient*(xend - x1);
		float xgap = rfpart_(x1 + 0.5);
		int xpxl1 = xend;
		int ypxl1 = ipart_(yend);

		t = 0.0f;
		z = z1 + poly_offset;
		w = w1;

		// TODO This is so ugly and repetitive...Should I bother with end points?
		// Or run the shader only once for each pair?
		x = xpxl1;
		y = ypxl1;
		if (CLIPXY_TEST(x, y)) {
			if (fragdepth_or_discard || fragment_processing(x, y, z)) {
				SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= rfpart_(yend)*xgap;
					draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}
		}
		if (CLIPXY_TEST(x, y+1)) {
			if (fragdepth_or_discard || fragment_processing(x, y+1, z)) {
				SET_VEC4(c->builtins.gl_FragCoord, x, y+1, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= fpart_(yend)*xgap;
					draw_pixel(c->builtins.gl_FragColor, x, y+1, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}
		}
		//printf("xgap = %f\n", xgap);
		//printf("%f %f\n", rfpart_(yend), fpart_(yend));
		//printf("%f %f\n", rfpart_(yend)*xgap, fpart_(yend)*xgap);
		float intery = yend + gradient;

		xend = round_(x2);
		yend = y2 + gradient*(xend - x2);
		xgap = fpart_(x2+0.5);
		int xpxl2 = xend;
		int ypxl2 = ipart_(yend);

		t = 1.0f;
		z = z2 + poly_offset;
		w = w2;

		x = xpxl2;
		y = ypxl2;
		if (CLIPXY_TEST(x, y)) {
			if (fragdepth_or_discard || fragment_processing(x, y, z)) {
				SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= rfpart_(yend)*xgap;
					draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}
		}
		if (CLIPXY_TEST(x, y+1)) {
			if (fragdepth_or_discard || fragment_processing(x, y+1, z)) {
				SET_VEC4(c->builtins.gl_FragCoord, x, y+1, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= fpart_(yend)*xgap;
					draw_pixel(c->builtins.gl_FragColor, x, y+1, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}
		}

		for(x=xpxl1+1; x < xpxl2; x++) {
			pr.x = x;
			pr.y = intery;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;
			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			y = ipart_(intery);
			if (CLIPXY_TEST(x, y)) {
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= rfpart_(intery);
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}
			if (CLIPXY_TEST(x, y+1)) {
				if (fragdepth_or_discard || fragment_processing(x, y+1, z)) {
					SET_VEC4(c->builtins.gl_FragCoord, x, y+1, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= fpart_(intery);
						draw_pixel(c->builtins.gl_FragColor, x, y+1, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}

			intery += gradient;
		}
	} else {
		if (y2 < y1) {
			swap_(x1, x2);
			swap_(y1, y2);
			swap_(z1, z2);
			swap_(w1, w2);
			swap_(v1_out, v2_out);
		}

		vec2 p1 = { x1, y1 }, p2 = { x2, y2 };
		vec2 pr, sub_p2p1 = sub_vec2s(p2, p1);
		float line_length_squared = length_vec2(sub_p2p1);
		line_length_squared *= line_length_squared;

		// TODO should be done for each fragment, after poly_offset is added?
		z1 = rsw_mapf(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
		z2 = rsw_mapf(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

		float gradient = dx / dy;
		float yend = round_(y1);
		float xend = x1 + gradient*(yend - y1);
		float ygap = rfpart_(y1 + 0.5);
		int ypxl1 = yend;
		int xpxl1 = ipart_(xend);

		t = 0.0f;
		z = z1 + poly_offset;
		w = w1;

		x = xpxl1;
		y = ypxl1;
		if (CLIPXY_TEST(x, y)) {
			if (fragdepth_or_discard || fragment_processing(x, y, z)) {
				SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= rfpart_(xend)*ygap;
					draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}
		}
		if (CLIPXY_TEST(x+1, y)) {
			if (fragdepth_or_discard || fragment_processing(x+1, y, z)) {
				SET_VEC4(c->builtins.gl_FragCoord, x+1, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= fpart_(xend)*ygap;
					draw_pixel(c->builtins.gl_FragColor, x+1, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}
		}

		float interx = xend + gradient;

		yend = round_(y2);
		xend = x2 + gradient*(yend - y2);
		ygap = fpart_(y2+0.5);
		int ypxl2 = yend;
		int xpxl2 = ipart_(xend);

		t = 1.0f;
		z = z2 + poly_offset;
		w = w2;

		x = xpxl2;
		y = ypxl2;
		if (CLIPXY_TEST(x, y)) {
			if (fragdepth_or_discard || fragment_processing(x, y, z)) {
				SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= rfpart_(xend)*ygap;
					draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}
		}
		if (CLIPXY_TEST(x+1, y)) {
			if (fragdepth_or_discard || fragment_processing(x+1, y, z)) {
				SET_VEC4(c->builtins.gl_FragCoord, x+1, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= fpart_(xend)*ygap;
					draw_pixel(c->builtins.gl_FragColor, x+1, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
				}
			}
		}

		for(y=ypxl1+1; y < ypxl2; y++) {
			pr.x = interx;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;
			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			x = ipart_(interx);
			if (CLIPXY_TEST(x, y)) {
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= rfpart_(interx);
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}
			if (CLIPXY_TEST(x+1, y)) {
				if (fragdepth_or_discard || fragment_processing(x+1, y, z)) {
					SET_VEC4(c->builtins.gl_FragCoord, x+1, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= fpart_(interx);
						draw_pixel(c->builtins.gl_FragColor, x+1, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}

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

	// TODO figure out how to remove XY clipping while still
	// handling weird edge cases like LearnPortableGL's skybox
	// case
	//v0->clip_code &= CLIPZ_MASK;
	//v1->clip_code &= CLIPZ_MASK;
	//v2->clip_code &= CLIPZ_MASK;
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

	// TODO when/if I get rid of glPolygonMode support for FRONT
	// and BACK, this becomes a single function pointer, no branch
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
		// this is correct for both smooth and noperspective because
		// it's in clip space, pre-perspective divide
		//
		// https://www.khronos.org/opengl/wiki/Vertex_Post-Processing#Clipping
		q->vs_out[i] = v0->vs_out[i] + (v1->vs_out[i] - v0->vs_out[i]) * t;

		//PGL_FLAT should be handled indirectly by the provoke index
		//nothing to do here unless I change that
	}
	
	q->clip_code = gl_clipcode(q->clip_space);
	//q->clip_code = gl_clipcode(q->clip_space) & CLIPZ_MASK;
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
		// TODO only clip z planes or only near
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
	PGL_UNUSED(provoke);

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

	if (c->line_smooth) {
		if (v0->edge_flag) {
			draw_aa_line(hp0, hp1, w0, w1, v0->vs_out, v1->vs_out, provoke, poly_offset);
		}
		if (v1->edge_flag) {
			draw_aa_line(hp1, hp2, w1, w2, v1->vs_out, v2->vs_out, provoke, poly_offset);
		}
		if (v2->edge_flag) {
			draw_aa_line(hp2, hp0, w2, w0, v2->vs_out, v0->vs_out, provoke, poly_offset);
		}
	} else {
		if (v0->edge_flag) {
			draw_thick_line(hp0, hp1, w0, w1, v0->vs_out, v1->vs_out, provoke, poly_offset);
		}
		if (v1->edge_flag) {
			draw_thick_line(hp1, hp2, w1, w2, v1->vs_out, v2->vs_out, provoke, poly_offset);
		}
		if (v2->edge_flag) {
			draw_thick_line(hp2, hp0, w2, w0, v2->vs_out, v0->vs_out, provoke, poly_offset);
		}
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
	float* vs_output = &c->vs_output.output_buf[0];

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
					//calculate interpolation here
					tmp2 = alpha*inv_w0 + beta*inv_w1 + gamma*inv_w2;

					z = alpha * hp0.z + beta * hp1.z + gamma * hp2.z;

					z += poly_offset;
					z = rsw_mapf(z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far); //TODO move out (ie can I map hp1.z etc.)?

					// early testing if shader doesn't use fragdepth or discard
					if (!fragdepth_or_discard && !fragment_processing(x, y, z)) {
						continue;
					}

					for (int i=0; i<c->vs_output.size; ++i) {
						if (c->vs_output.interpolation[i] == PGL_SMOOTH) {
							tmp = alpha*perspective[i] + beta*perspective[GL_MAX_VERTEX_OUTPUT_COMPONENTS + i] + gamma*perspective[2*GL_MAX_VERTEX_OUTPUT_COMPONENTS + i];

							fs_input[i] = tmp/tmp2;

						} else if (c->vs_output.interpolation[i] == PGL_NOPERSPECTIVE) {
							fs_input[i] = alpha * v0->vs_out[i] + beta * v1->vs_out[i] + gamma * v2->vs_out[i];
						} else { // == PGL_FLAT
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

	// TODO initialize to get rid of "possibly uninitialized warning?"
	vec4 Cs, Cd;

	switch (c->blend_sRGB) {
	case GL_ZERO:                     SET_VEC4(Cs, 0,0,0,0);                                 break;
	case GL_ONE:                      SET_VEC4(Cs, 1,1,1,1);                                 break;
	case GL_SRC_COLOR:                Cs = src;                                              break;
	case GL_ONE_MINUS_SRC_COLOR:      SET_VEC4(Cs, 1-src.x,1-src.y,1-src.z,1-src.w);         break;
	case GL_DST_COLOR:                Cs = dst;                                              break;
	case GL_ONE_MINUS_DST_COLOR:      SET_VEC4(Cs, 1-dst.x,1-dst.y,1-dst.z,1-dst.w);         break;
	case GL_SRC_ALPHA:    SET_VEC4(Cs, src.w, src.w, src.w, src.w);              break;
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
static pix_t logic_ops_pixel(pix_t s, pix_t d)
{
	switch (c->logic_func) {
	case GL_CLEAR:
		return 0;
	case GL_SET:
		return ~0;
	case GL_COPY:
		return s;
	case GL_COPY_INVERTED:
		return ~s;
	case GL_NOOP:
		return d;
	case GL_INVERT:
		return ~d;
	case GL_AND:
		return s & d;
	case GL_NAND:
		return ~(s & d);
	case GL_OR:
		return s | d;
	case GL_NOR:
		return ~(s | d);
	case GL_XOR:
		return s ^ d;
	case GL_EQUIV:
		return ~(s ^ d);
	case GL_AND_REVERSE:
		return s & ~d;
	case GL_AND_INVERTED:
		return ~s & d;
	case GL_OR_REVERSE:
		return s | ~d;
	case GL_OR_INVERTED:
		return ~s | d;
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
	// TODO make them proper arrays in gl_context?
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
	
	//Stencil Test
	//TODO have to handle when there is no stencil/depth buffer, comptime or runtime?
	u8* stencil_dest = &c->stencil_buf.lastrow[(-y*c->stencil_buf.w + x)*4+3];
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
		// TODO handle more than PGL_D24S8
		u32 orig = ((u32*)c->zbuf.lastrow)[-y*c->zbuf.w + x];
		u32 dest_depth = orig >> GL_STENCIL_BITS;
		u32 src_depth = z * PGL_MAX_Z;

		int depth_result = depthtest(src_depth, dest_depth);

		if (c->stencil_test) {
			stencil_op(1, depth_result, stencil_dest);
		}
		if (!depth_result) {
			return 0;
		}
		if (c->depth_mask) {
			((u32*)c->zbuf.lastrow)[-y*c->zbuf.w + x] = (orig & PGL_STENCIL_MASK) | (src_depth << GL_STENCIL_BITS);
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
	pix_t src, dst;
	pix_t* dest_loc = &((pix_t*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	dst = *dest_loc;
	
	// NOTE does not normalize, just extracts the channels
	dest_color = PIXEL_TO_COLOR(dst);

	if (c->blend) {
		//TODO clamp in blend_pixel?  return the vec4 and clamp?
		
		// TODO return pix_t directly?
		src_color = blend_pixel(cf, COLOR_TO_VEC4(dest_color));
	} else {
		cf.x = clamp_01(cf.x);
		cf.y = clamp_01(cf.y);
		cf.z = clamp_01(cf.z);
		cf.w = clamp_01(cf.w);
		//src_color = vec4_to_Color(cf);

		// have VEC4_TO_PIXEL()?
		src_color = VEC4_TO_COLOR(cf);
	}

	src = RGBA_TO_PIXEL(src_color.r, src_color.g, src_color.b, src_color.a);

	//Logic Ops
	if (c->logic_ops) {
		src = logic_ops_pixel(src, dst);
	}

	//Dithering

#ifndef PGL_DISABLE_COLOR_MASK
	src = (src & c->color_mask) | (dst & ~c->color_mask);
#endif

	*dest_loc = src;
}

