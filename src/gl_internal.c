
static glContext* c;

static vec4 blend_pixel(vec4 src, vec4 dst, int buf);
static int fragment_processing(int x, int y, float z);
static void draw_pixel(vec4 cf, int x, int y, float z, int do_frag_processing);
// MRT-aware: depth/stencil once, then write gl_FragColor or gl_FragData[] to draw buffers
static void draw_fragment(Shader_Builtins* b, int x, int y, int do_frag_processing);
static void run_pipeline(GLenum mode, const GLvoid* indices, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements);

static float calc_poly_offset(vec3 hp0, vec3 hp1, vec3 hp2);

static void draw_triangle_clip(glVertex* v0, glVertex* v1, glVertex* v2,
                               int e0, int e1, int e2,
                               unsigned provoke, int clip_bit,
                               const int* cc_in,
                               pgl_tri* recs, int* n_out, int n_base);
static void draw_triangle_point(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_line(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_fill(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);

static void pgl_assemble_lines(GLenum mode, GLsizei count);
static void pgl_assemble_tris(GLenum mode, GLsizei count);
static void pgl_update_clip_rect(void);

// This is the prototype for either implementation; only one is defined based on
// whether PGL_BETTER_THICK_LINES is defined
static void draw_thick_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset);

// Only width 1 supported for now
static void draw_aa_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset);

/* this clip epsilon is needed to avoid some rounding errors after
   several clipping stages */

#define CLIP_EPSILON (1E-5f)
#define CLIP_Z_NEAR 0x1
#define CLIPZ_MASK 0x3
#define CLIP_GUARD_MASK 0x3C
#define CLIP_FRUSTUM_XY_MASK 0x3C0
#define CLIP_PLANES_MASK (CLIPZ_MASK | CLIP_GUARD_MASK)
#define CLIP_FRUSTUM_MASK (CLIPZ_MASK | CLIP_FRUSTUM_XY_MASK)
#define CLIPX_TEST(x) (x >= c->lx && x < c->ux)
#define CLIPY_TEST(y) (y >= c->ly && y < c->uy)

// Always-on raster clip to the current draw surface (not the viewport).
// If GL_SCISSOR_TEST is on, intersect with the scissor box.
static void pgl_update_clip_rect(void)
{
	GLsizei w = c->back_buffer.w;
	GLsizei h = c->back_buffer.h;
	if (c->scissor_test) {
		int ux = c->scissor_lx + c->scissor_w;
		int uy = c->scissor_ly + c->scissor_h;
		c->lx = MAX(c->scissor_lx, 0);
		c->ly = MAX(c->scissor_ly, 0);
		c->ux = MIN(ux, w);
		c->uy = MIN(uy, h);
	} else {
		c->lx = 0;
		c->ly = 0;
		c->ux = w;
		c->uy = h;
	}
}

// Viewport ∩ raster clip rect (lx/ux/ly/uy, which is the framebuffer ∩ scissor).
// lx/ly are >= 0, so a viewport that hangs off the buffer loses its negative edge.
// Empty intersection returns 0. Points do not use this; they test lx/uy directly.
// Lines use the same rect. Points test lx/uy directly.
static int pgl_viewport_raster_rect(int* left, int* bottom, int* right, int* top)
{
	int r_left = c->lx;
	int r_bottom = c->ly;
	int r_right = c->ux;
	int r_top = c->uy;

	if (c->xmin > r_left)
		r_left = c->xmin;
	if (c->ymin > r_bottom)
		r_bottom = c->ymin;

	int vr = c->xmin + c->width;
	int vt = c->ymin + c->height;
	if (vr < r_right)
		r_right = vr;
	if (vt < r_top)
		r_top = vt;

	if (r_left >= r_right || r_bottom >= r_top)
		return 0;

	*left = r_left;
	*bottom = r_bottom;
	*right = r_right;
	*top = r_top;
	return 1;
}

// Line rasterizers declare vp_l, vp_b, vp_r, vp_t from pgl_viewport_raster_rect.
#define LINE_XY(x, y) ((x) >= vp_l && (x) < vp_r && (y) >= vp_b && (y) < vp_t)

static inline int gl_clipcode(vec4 pt)
{
	float w = pt.w * (1.0f + CLIP_EPSILON);
	float xl = c->guard_ndc_left * w;
	float xr = c->guard_ndc_right * w;
	float yb = c->guard_ndc_bottom * w;
	float yt = c->guard_ndc_top * w;
	int zbits = ((pt.z < -w) | ((pt.z > w) << 1)) &
		((!c->depth_clamp) | ((!c->depth_clamp) << 1));
	int guard = ((pt.x < xl) << 2) |
		((pt.x > xr) << 3) |
		((pt.y < yb) << 4) |
		((pt.y > yt) << 5);
	int frustum = ((pt.x < -w) << 6) |
		((pt.x > w) << 7) |
		((pt.y < -w) << 8) |
		((pt.y > w) << 9);
	return zbits | guard | frustum;
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
	vec3 p0 = v4_to_v3h(v0->screen_space);
	vec3 p1 = v4_to_v3h(v1->screen_space);
	vec3 p2 = v4_to_v3h(v2->screen_space);

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
	u8* u8p = (u8*)(buf_data + v->offset + (GLsizeiptr)v->relativeoffset + v->stride * i);

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

	// Strips and fans share this vertex. Edge bits live on the
	// primitive record, not here.
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
	// No meaningful UV footprint for points
	c->mip_uv_per_px = 0.0f;

	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	vec3 point = v4_to_v3h(vert->screen_space);
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

			SET_V4(builtins.gl_FragCoord, j, i, point.z, 1/vert->screen_space.w);
			builtins.discard = GL_FALSE;
			builtins.gl_FragDepth = point.z;
			c->programs.a[c->cur_program].fragment_shader(fs_input, &builtins, c->programs.a[c->cur_program].uniform);
			if (!builtins.discard)
				draw_fragment(&builtins, j, i, fragdepth_or_discard);
		}
	}
}

static void run_pipeline(GLenum mode, const GLvoid* indices, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements)
{
	GLsizei i;

	PGL_ASSERT(count <= PGL_MAX_VERTICES);

	vertex_stage(indices, count, instance, base_instance, use_elements);

	//fragment portion
	if (mode == GL_POINTS) {
		// clip only z and let partial points (size > 1)
		// show even if the center would have been clipped
		u32* ids = (u32*)c->prim_buf;
		for (i = 0; i < count; ) {
			GLsizei chunk = i + PGL_CHUNK_PRIMS;
			GLsizei n_out = 0;
			GLsizei k;

			if (chunk > count)
				chunk = count;

			for (k = i; k < chunk; ++k) {
				if (c->glverts.a[k].clip_code & CLIPZ_MASK)
					continue;

				c->glverts.a[k].screen_space = mult_m4_v4(c->vp_mat, c->glverts.a[k].clip_space);
				PGL_ASSERT(k < count);
				PGL_ASSERT(((u32)k & PGL_VERT_ARENA) == 0);
				ids[n_out++] = (u32)k;
			}
			for (k = 0; k < n_out; ++k)
				draw_point(&c->glverts.a[ids[k]], 0.0f);
			i = chunk;
		}
	} else if (mode == GL_LINES || mode == GL_LINE_STRIP || mode == GL_LINE_LOOP) {
		pgl_assemble_lines(mode, count);
	} else if (mode == GL_TRIANGLES || mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN) {
		pgl_assemble_tris(mode, count);
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
	PGL_ASSERT(0 && "ERROR: unrecognized depth test!");
	return 0;
}


static void setup_fs_input(float t, float* v1_out, float* v2_out, float wa, float wb, unsigned int provoke)
{
	float* vs_output = &c->vs_output.output_buf[0];

	float inv_wa = 1.0f/wa;
	float inv_wb = 1.0f/wb;

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


static void pgl_arena_fix_vs_out(void)
{
	pgl_clip_arena* a = &c->clip_arena;
	int i;
	for (i = 0; i < a->count; ++i)
		a->verts[i].vs_out = a->varyings + (size_t)i * GL_MAX_VERTEX_OUTPUT_COMPONENTS;
}

static void pgl_arena_reserve(int n)
{
	pgl_clip_arena* a = &c->clip_arena;
	while (a->cap - a->count < n) {
		int ncap = a->cap * 2;
		glVertex* verts;
		float* varyings;
		PGL_ASSERT(ncap > a->cap);
		verts = (glVertex*)PGL_REALLOC(a->verts, (size_t)ncap * sizeof(glVertex));
		varyings = (float*)PGL_REALLOC(a->varyings, (size_t)ncap * GL_MAX_VERTEX_OUTPUT_COMPONENTS * sizeof(float));
		PGL_ASSERT(verts);
		PGL_ASSERT(varyings);
		a->verts = verts;
		a->varyings = varyings;
		a->cap = ncap;
		pgl_arena_fix_vs_out();
	}
}

static glVertex* pgl_arena_alloc(int n)
{
	pgl_clip_arena* a = &c->clip_arena;
	int i = a->count++;
	glVertex* v;
	PGL_ASSERT(a->count - a->prim_base <= n);
	PGL_ASSERT(a->count <= a->cap);
	v = &a->verts[i];
	v->vs_out = a->varyings + (size_t)i * GL_MAX_VERTEX_OUTPUT_COMPONENTS;
	return v;
}

static glVertex* pgl_vert(u32 id)
{
	u32 idx = id & PGL_INDEX_MASK;
	if (id & PGL_VERT_ARENA)
		return &c->clip_arena.verts[idx];
	return &c->glverts.a[idx];
}

static u32 pgl_vert_id(glVertex* v)
{
	glVertex* arena = c->clip_arena.verts;
	if (v >= arena && v < arena + c->clip_arena.count) {
		int slot = (int)(v - arena);
		PGL_ASSERT(slot >= 0 && slot <= PGL_INDEX_MASK);
		return PGL_VERT_ARENA | (u32)slot;
	}
	PGL_ASSERT(v >= c->glverts.a);
	PGL_ASSERT((u32)(v - c->glverts.a) <= PGL_INDEX_MASK);
	return (u32)(v - c->glverts.a);
}

static void pgl_screen_space_vert(GLsizei i)
{
	glVertex* v = &c->glverts.a[i];
	if (v->clip_space.w > 0.0f)
		v->screen_space = mult_m4_v4(c->vp_mat, v->clip_space);
}

static void pgl_screen_space_span(GLsizei first, GLsizei last)
{
	GLsizei i;
	for (i = first; i <= last; ++i)
		pgl_screen_space_vert(i);
}

// Vertices this chunk's primitives can reference. w <= 0 is left alone.
static void pgl_screen_space_range(GLenum mode, GLsizei count, GLsizei prim, GLsizei chunk)
{
	if (chunk <= prim)
		return;
	if (mode == GL_LINES) {
		pgl_screen_space_span(prim * 2, chunk * 2 - 1);
	} else if (mode == GL_LINE_STRIP) {
		pgl_screen_space_span(prim, chunk);
	} else if (mode == GL_LINE_LOOP) {
		GLsizei body_end = chunk;
		if (body_end > count - 1)
			body_end = count - 1;
		if (body_end > prim)
			pgl_screen_space_span(prim, body_end);
		if (chunk == count && count >= 1) {
			pgl_screen_space_vert(count - 1);
			pgl_screen_space_vert(0);
		}
	} else if (mode == GL_TRIANGLES) {
		pgl_screen_space_span(prim * 3, chunk * 3 - 1);
	} else if (mode == GL_TRIANGLE_STRIP) {
		pgl_screen_space_span(prim, chunk + 1);
	} else if (mode == GL_TRIANGLE_FAN) {
		pgl_screen_space_vert(0);
		pgl_screen_space_span(prim + 1, chunk + 1);
	}
}

static int pgl_line_prim_count(GLenum mode, GLsizei count)
{
	if (mode == GL_LINES)
		return count / 2;
	if (mode == GL_LINE_STRIP)
		return count < 2 ? 0 : count - 1;
	if (mode == GL_LINE_LOOP)
		return count >= 1 ? count : 0;
	return 0;
}

static void pgl_line_slots(GLenum mode, GLsizei count, GLsizei k, GLsizei* i0, GLsizei* i1, unsigned* provoke)
{
	int last = c->provoking_vert == GL_LAST_VERTEX_CONVENTION;
	if (mode == GL_LINES) {
		*i0 = k * 2;
		*i1 = k * 2 + 1;
	} else if (mode == GL_LINE_LOOP && k == count - 1) {
		*i0 = count - 1;
		*i1 = 0;
	} else {
		*i0 = k;
		*i1 = k + 1;
	}
	*provoke = last ? (unsigned)*i1 : (unsigned)*i0;
	PGL_ASSERT(*provoke <= PGL_PROVOKE_MASK);
}

// t == 0 is p1, the first endpoint. t == 1 is p1 + (p2 - p1), not p2.
static u32 pgl_line_endpoint(glVertex* v0, glVertex* v1, vec4 p1, vec4 d, float t, GLsizei i0)
{
	glVertex* q;
	int i, slot;
	if (t == 0.0f) {
		PGL_ASSERT(((u32)i0 & PGL_VERT_ARENA) == 0);
		return (u32)i0;
	}
	q = pgl_arena_alloc(2);
	q->clip_space = add_v4s(p1, scale_v4(d, t));
	for (i = 0; i < c->vs_output.size; ++i)
		q->vs_out[i] = v0->vs_out[i] + (v1->vs_out[i] - v0->vs_out[i]) * t;
	if (q->clip_space.w > 0.0f)
		q->screen_space = mult_m4_v4(c->vp_mat, q->clip_space);
	slot = (int)(q - c->clip_arena.verts);
	PGL_ASSERT(slot >= 0 && slot <= PGL_INDEX_MASK);
	return PGL_VERT_ARENA | (u32)slot;
}

static int pgl_emit_line(GLenum mode, GLsizei count, GLsizei k, pgl_line* rec)
{
	GLsizei i0, i1;
	unsigned provoke;
	glVertex* v0;
	glVertex* v1;
	int cc0, cc1;
	vec4 p1, p2, d;
	float tmin, tmax;
	u32 e0, e1;

	pgl_line_slots(mode, count, k, &i0, &i1, &provoke);
	v0 = &c->glverts.a[i0];
	v1 = &c->glverts.a[i1];
	cc0 = v0->clip_code;
	cc1 = v1->clip_code;
	p1 = v0->clip_space;
	p2 = v1->clip_space;

	if ((cc0 & cc1 & CLIP_FRUSTUM_MASK) != 0)
		return 0;

	if (((cc0 | cc1) & CLIP_PLANES_MASK) == 0 && p1.w > 0.0f && p2.w > 0.0f) {
		rec->v[0] = (u32)i0;
		rec->v[1] = (u32)i1;
		rec->meta = provoke;
		return 1;
	}

	d = sub_v4s(p2, p1);
	tmin = 0;
	tmax = 1;
	{
		float gl = c->guard_ndc_left;
		float gr = c->guard_ndc_right;
		float gb = c->guard_ndc_bottom;
		float gt = c->guard_ndc_top;
		if (!(clip_line( d.x - gl*d.w,  gl*p1.w - p1.x, &tmin, &tmax) &&
		      clip_line(-d.x + gr*d.w,  p1.x - gr*p1.w, &tmin, &tmax) &&
		      clip_line( d.y - gb*d.w,  gb*p1.w - p1.y, &tmin, &tmax) &&
		      clip_line(-d.y + gt*d.w,  p1.y - gt*p1.w, &tmin, &tmax) &&
		      clip_line( d.z+d.w, -p1.z-p1.w, &tmin, &tmax) &&
		      clip_line(-d.z+d.w,  p1.z-p1.w, &tmin, &tmax)))
			return 0;
	}

	c->clip_arena.prim_base = c->clip_arena.count;
	pgl_arena_reserve(2);
	e0 = pgl_line_endpoint(v0, v1, p1, d, tmin, i0);
	e1 = pgl_line_endpoint(v0, v1, p1, d, tmax, i0);
	if (pgl_vert(e0)->clip_space.w <= 0.0f || pgl_vert(e1)->clip_space.w <= 0.0f)
		return 0;

	rec->v[0] = e0;
	rec->v[1] = e1;
	rec->meta = provoke;
	return 1;
}

static void pgl_raster_lines(pgl_line* lines, int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		glVertex* a = pgl_vert(lines[i].v[0]);
		glVertex* b = pgl_vert(lines[i].v[1]);
		vec3 hp1 = v4_to_v3h(a->screen_space);
		vec3 hp2 = v4_to_v3h(b->screen_space);
		unsigned provoke = lines[i].meta & PGL_PROVOKE_MASK;
		if (c->line_smooth)
			draw_aa_line(hp1, hp2, a->screen_space.w, b->screen_space.w, a->vs_out, b->vs_out, provoke, 0.0f);
		else
			draw_thick_line(hp1, hp2, a->screen_space.w, b->screen_space.w, a->vs_out, b->vs_out, provoke, 0.0f);
	}
}

static void pgl_assemble_lines(GLenum mode, GLsizei count)
{
	int nprims = pgl_line_prim_count(mode, count);
	int prim = 0;
	int out_cap = (int)((PGL_CHUNK_PRIMS * sizeof(pgl_tri)) / sizeof(pgl_line));
	pgl_line* recs = (pgl_line*)c->prim_buf;

	while (prim < nprims) {
		int chunk = prim + PGL_CHUNK_PRIMS;
		int n_out = 0;
		if (chunk > nprims)
			chunk = nprims;

		pgl_screen_space_range(mode, count, prim, chunk);
		c->clip_arena.count = 0;

		for (; prim < chunk; ++prim) {
			if (n_out == out_cap) {
				pgl_raster_lines(recs, n_out);
				n_out = 0;
				c->clip_arena.count = 0;
			}
			n_out += pgl_emit_line(mode, count, prim, &recs[n_out]);
		}
		pgl_raster_lines(recs, n_out);
		c->clip_arena.count = 0;
	}
}

static int pgl_tri_prim_count(GLenum mode, GLsizei count)
{
	if (mode == GL_TRIANGLES)
		return count / 3;
	if (count < 3)
		return 0;
	return count - 2;
}

static void pgl_tri_slots(GLenum mode, GLsizei k, GLsizei* i0, GLsizei* i1, GLsizei* i2, unsigned* provoke)
{
	int last = c->provoking_vert == GL_LAST_VERTEX_CONVENTION;
	if (mode == GL_TRIANGLES) {
		*i0 = k * 3;
		*i1 = k * 3 + 1;
		*i2 = k * 3 + 2;
		*provoke = last ? (unsigned)(*i2) : (unsigned)(*i0);
	} else if (mode == GL_TRIANGLE_STRIP) {
		if ((k & 1) == 0) {
			*i0 = k;
			*i1 = k + 1;
		} else {
			*i0 = k + 1;
			*i1 = k;
		}
		*i2 = k + 2;
		*provoke = last ? (unsigned)(*i2) : (unsigned)k;
	} else {
		*i0 = 0;
		*i1 = k + 1;
		*i2 = k + 2;
		*provoke = last ? (unsigned)(*i2) : (unsigned)(*i1);
	}
	PGL_ASSERT(*provoke <= PGL_PROVOKE_MASK);
}

static int pgl_face_culled(int front)
{
	if (!c->cull_face)
		return 0;
	if (c->cull_mode == GL_FRONT_AND_BACK)
		return 1;
	if (c->cull_mode == GL_BACK && !front)
		return 1;
	if (c->cull_mode == GL_FRONT && front)
		return 1;
	return 0;
}

// -1 reject, 1 fast (guard, all w > 0), 0 needs the clipper.
static int pgl_tri_class(glVertex* v0, glVertex* v1, glVertex* v2)
{
	int c0 = v0->clip_code;
	int c1 = v1->clip_code;
	int c2 = v2->clip_code;
	if ((c0 & c1 & c2 & CLIP_FRUSTUM_MASK) != 0)
		return -1;
	if (((c0 | c1 | c2) & CLIP_PLANES_MASK) == 0 &&
	    v0->clip_space.w > 0.0f &&
	    v1->clip_space.w > 0.0f &&
	    v2->clip_space.w > 0.0f)
		return 1;
	return 0;
}

static int pgl_emit_tri_fast(glVertex* v0, glVertex* v1, glVertex* v2,
                             GLsizei i0, GLsizei i1, GLsizei i2,
                             unsigned provoke, pgl_tri* rec)
{
	int front = is_front_facing(v0, v1, v2);
	if (pgl_face_culled(front))
		return 0;
	PGL_ASSERT(((u32)i0 & PGL_VERT_ARENA) == 0);
	PGL_ASSERT(((u32)i1 & PGL_VERT_ARENA) == 0);
	PGL_ASSERT(((u32)i2 & PGL_VERT_ARENA) == 0);
	rec->v[0] = (u32)i0;
	rec->v[1] = (u32)i1;
	rec->v[2] = (u32)i2;
	rec->meta = provoke | PGL_EDGE_V0 | PGL_EDGE_V1 | PGL_EDGE_V2;
	if (front)
		rec->meta |= PGL_FRONT_BIT;
	return 1;
}

// Clip leaf. Drops a residual w <= 0 piece. Does not draw.
static void pgl_append_tri(glVertex* v0, glVertex* v1, glVertex* v2,
                           int e0, int e1, int e2, unsigned provoke,
                           pgl_tri* recs, int* n_out, int n_base)
{
	int front;
	pgl_tri* rec;
	if (v0->clip_space.w <= 0.0f || v1->clip_space.w <= 0.0f || v2->clip_space.w <= 0.0f)
		return;
	front = is_front_facing(v0, v1, v2);
	if (pgl_face_culled(front))
		return;
	PGL_ASSERT(*n_out < PGL_CHUNK_PRIMS);
	PGL_ASSERT(*n_out - n_base < PGL_MAX_CLIP_TRIS);
	rec = &recs[*n_out];
	rec->v[0] = pgl_vert_id(v0);
	rec->v[1] = pgl_vert_id(v1);
	rec->v[2] = pgl_vert_id(v2);
	rec->meta = provoke;
	if (e0) rec->meta |= PGL_EDGE_V0;
	if (e1) rec->meta |= PGL_EDGE_V1;
	if (e2) rec->meta |= PGL_EDGE_V2;
	if (front) rec->meta |= PGL_FRONT_BIT;
	(*n_out)++;
}

static void pgl_clip_tri(glVertex* v0, glVertex* v1, glVertex* v2, unsigned provoke,
                         pgl_tri* recs, int* n_out)
{
	int cc0[3];
	int n0 = *n_out;
	int vbase;
	cc0[0] = v0->clip_code;
	cc0[1] = v1->clip_code;
	cc0[2] = v2->clip_code;
	// depth_clamp clears Z bits, including for w <= 0. The root frame sees
	// the forced near bit. It is not stored on the shared glVertex.
	if (v0->clip_space.w <= 0.0f) cc0[0] |= CLIP_Z_NEAR;
	if (v1->clip_space.w <= 0.0f) cc0[1] |= CLIP_Z_NEAR;
	if (v2->clip_space.w <= 0.0f) cc0[2] |= CLIP_Z_NEAR;
	c->clip_arena.prim_base = c->clip_arena.count;
	pgl_arena_reserve(PGL_MAX_CLIP_VERTS);
	vbase = c->clip_arena.count;
	draw_triangle_clip(v0, v1, v2, 1, 1, 1, provoke, 0, cc0, recs, n_out, n0);
	PGL_ASSERT(*n_out - n0 <= PGL_MAX_CLIP_TRIS);
	PGL_ASSERT(c->clip_arena.count - vbase <= PGL_MAX_CLIP_VERTS);
	PGL_ASSERT(*n_out <= PGL_CHUNK_PRIMS);
}

static void pgl_raster_tris(pgl_tri* tris, int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		pgl_tri* t = &tris[i];
		glVertex* v0 = pgl_vert(t->v[0]);
		glVertex* v1 = pgl_vert(t->v[1]);
		glVertex* v2 = pgl_vert(t->v[2]);
		unsigned provoke = t->meta & PGL_PROVOKE_MASK;
		c->assemble_edges =
			((t->meta & PGL_EDGE_V0) ? 1u : 0u) |
			((t->meta & PGL_EDGE_V1) ? 2u : 0u) |
			((t->meta & PGL_EDGE_V2) ? 4u : 0u);
		c->builtins.gl_FrontFacing = (t->meta & PGL_FRONT_BIT) ? GL_TRUE : GL_FALSE;
		if (t->meta & PGL_FRONT_BIT)
			c->draw_triangle_front(v0, v1, v2, provoke);
		else
			c->draw_triangle_back(v0, v1, v2, provoke);
	}
}

static void pgl_assemble_tris(GLenum mode, GLsizei count)
{
	int nprims = pgl_tri_prim_count(mode, count);
	int prim = 0;
	pgl_tri* recs = (pgl_tri*)c->prim_buf;

	while (prim < nprims) {
		int chunk = prim + PGL_CHUNK_PRIMS;
		int n_out = 0;
		if (chunk > nprims)
			chunk = nprims;

		pgl_screen_space_range(mode, count, prim, chunk);
		c->clip_arena.count = 0;

		for (; prim < chunk; ++prim) {
			GLsizei i0, i1, i2;
			unsigned provoke;
			glVertex* v0;
			glVertex* v1;
			glVertex* v2;
			int kind;

			pgl_tri_slots(mode, (GLsizei)prim, &i0, &i1, &i2, &provoke);
			v0 = &c->glverts.a[i0];
			v1 = &c->glverts.a[i1];
			v2 = &c->glverts.a[i2];
			kind = pgl_tri_class(v0, v1, v2);
			if (kind < 0)
				continue;
			if (kind == 0) {
				// One clipped triangle needs 64 output slots. Raster first if
				// they are not free, then append into this same buffer.
				if (PGL_CHUNK_PRIMS - n_out < PGL_MAX_CLIP_TRIS) {
					pgl_raster_tris(recs, n_out);
					n_out = 0;
					c->clip_arena.count = 0;
				}
				pgl_clip_tri(v0, v1, v2, provoke, recs, &n_out);
				continue;
			}
			if (n_out == PGL_CHUNK_PRIMS) {
				pgl_raster_tris(recs, n_out);
				n_out = 0;
				c->clip_arena.count = 0;
			}
			n_out += pgl_emit_tri_fast(v0, v1, v2, i0, i1, i2, provoke, &recs[n_out]);
		}
		pgl_raster_tris(recs, n_out);
		c->clip_arena.count = 0;
	}
}

#ifndef PGL_BETTER_THICK_LINES
static void draw_thick_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
{
	int vp_l, vp_b, vp_r, vp_t;
	if (!pgl_viewport_raster_rect(&vp_l, &vp_b, &vp_r, &vp_t))
		return;

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
	vec2 pr, sub_p2p1 = sub_v2s(p2, p1);
	float line_length_squared = len_v2(sub_p2p1);
	line_length_squared *= line_length_squared;

	frag_func fragment_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniform = c->programs.a[c->cur_program].uniform;
	int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;

	float i_x1, i_y1, i_x2, i_y2;
	i_x1 = floorf(p1.x) + 0.5f;
	i_y1 = floorf(p1.y) + 0.5f;
	i_x2 = floorf(p2.x) + 0.5f;
	i_y2 = floorf(p2.y) + 0.5f;

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
			t = dot_v2s(sub_v2s(pr, p1), sub_p2p1) / line_length_squared;
			t = clamp_01(t);

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			for (float j=x-half_w; j<x+half_w; ++j) {
				if (LINE_XY(j, y)) {
					if (fragdepth_or_discard || fragment_processing(j, y, z)) {
						SET_V4(c->builtins.gl_FragCoord, j, y, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard)
							draw_fragment(&c->builtins, j, y, fragdepth_or_discard);
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
			t = dot_v2s(sub_v2s(pr, p1), sub_p2p1) / line_length_squared;
			t = clamp_01(t);

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			for (float j=y-half_w; j<y+half_w; ++j) {
				if (LINE_XY(x, j)) {
					if (fragdepth_or_discard || fragment_processing(x, j, z)) {

						SET_V4(c->builtins.gl_FragCoord, x, j, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard)
							draw_fragment(&c->builtins, x, j, fragdepth_or_discard);
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
			t = dot_v2s(sub_v2s(pr, p1), sub_p2p1) / line_length_squared;
			t = clamp_01(t);

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			for (float j=y-half_w; j<y+half_w; ++j) {
				if (LINE_XY(x, j)) {
					if (fragdepth_or_discard || fragment_processing(x, j, z)) {

						SET_V4(c->builtins.gl_FragCoord, x, j, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard)
							draw_fragment(&c->builtins, x, j, fragdepth_or_discard);
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
			t = dot_v2s(sub_v2s(pr, p1), sub_p2p1) / line_length_squared;
			t = clamp_01(t);

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			for (float j=x-half_w; j<x+half_w; ++j) {
				if (LINE_XY(j, y)) {
					if (fragdepth_or_discard || fragment_processing(j, y, z)) {

						SET_V4(c->builtins.gl_FragCoord, j, y, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard)
							draw_fragment(&c->builtins, j, y, fragdepth_or_discard);
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
	int vp_l, vp_b, vp_r, vp_t;
	if (!pgl_viewport_raster_rect(&vp_l, &vp_b, &vp_r, &vp_t))
		return;

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
	vec2 v12 = sub_v2s(p2, p1);
	vec2 v1r, pr; // v2r

	float dot_1212 = dot_v2s(v12, v12);

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

	x_min = MAX((float)vp_l, x_min);
	x_max = MIN((float)vp_r, x_max);
	y_min = MAX((float)vp_b, y_min);
	y_max = MIN((float)vp_t, y_max);
	
	y_min = floorf(y_min) + 0.5f;
	x_min = floorf(x_min) + 0.5f;
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
			x_min = MAX((float)vp_l, x_min);
			x_min = floorf(x_min) + 0.5f;
			x_max = MIN((float)vp_r, x_max);
			//printf("%f %f   x_min etc\n", x_min, x_max);
		} else {
			x_min = x_mino;
			x_max = x_maxo;
		}
		for (x = x_min; x < x_max; ++x) {
			pr.x = x;
			v1r = sub_v2s(pr, p1);
			//v2r = sub_v2s(pr, p2);
			e = dot_v2s(v1r, v12);

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

					SET_V4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);

					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard)
						draw_fragment(&c->builtins, x, y, fragdepth_or_discard);
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

#if defined(__GNUC__) || defined(__clang__)
#define swap_(a, b) do { __typeof__(a) tmp = (a); (a) = (b); (b) = tmp; } while (0)
#else
#define swap_(a, b) do { \
	char pgl_swap_tmp_[sizeof(a)]; \
	memcpy(pgl_swap_tmp_, &(a), sizeof(a)); \
	memcpy(&(a), &(b), sizeof(a)); \
	memcpy(&(b), pgl_swap_tmp_, sizeof(a)); \
} while (0)
#endif
static void draw_aa_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
{
	int vp_l, vp_b, vp_r, vp_t;
	if (!pgl_viewport_raster_rect(&vp_l, &vp_b, &vp_r, &vp_t))
		return;

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
		vec2 pr, sub_p2p1 = sub_v2s(p2, p1);
		float line_length_squared = len_v2(sub_p2p1);
		line_length_squared *= line_length_squared;

		// TODO should be done for each fragment, after poly_offset is added?
		z1 = rsw_mapf(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
		z2 = rsw_mapf(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

		float gradient = dy / dx;
		float xend = round_(x1);
		float yend = y1 + gradient*(xend - x1);
		float xgap = rfpart_(x1 + 0.5f);
		int xpxl1 = xend;
		int ypxl1 = ipart_(yend);

		t = 0.0f;
		z = z1 + poly_offset;
		w = w1;

		// TODO This is so ugly and repetitive...Should I bother with end points?
		// Or run the shader only once for each pair?
		x = xpxl1;
		y = ypxl1;
		if (LINE_XY(x, y)) {
			if (fragdepth_or_discard || fragment_processing(x, y, z)) {
				SET_V4(c->builtins.gl_FragCoord, x, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= rfpart_(yend)*xgap;
					draw_fragment(&c->builtins, x, y, fragdepth_or_discard);
				}
			}
		}
		if (LINE_XY(x, y+1)) {
			if (fragdepth_or_discard || fragment_processing(x, y+1, z)) {
				SET_V4(c->builtins.gl_FragCoord, x, y+1, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= fpart_(yend)*xgap;
					draw_fragment(&c->builtins, x, y+1, fragdepth_or_discard);
				}
			}
		}
		//printf("xgap = %f\n", xgap);
		//printf("%f %f\n", rfpart_(yend), fpart_(yend));
		//printf("%f %f\n", rfpart_(yend)*xgap, fpart_(yend)*xgap);
		float intery = yend + gradient;

		xend = round_(x2);
		yend = y2 + gradient*(xend - x2);
		xgap = fpart_(x2+0.5f);
		int xpxl2 = xend;
		int ypxl2 = ipart_(yend);

		t = 1.0f;
		z = z2 + poly_offset;
		w = w2;

		x = xpxl2;
		y = ypxl2;
		if (LINE_XY(x, y)) {
			if (fragdepth_or_discard || fragment_processing(x, y, z)) {
				SET_V4(c->builtins.gl_FragCoord, x, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= rfpart_(yend)*xgap;
					draw_fragment(&c->builtins, x, y, fragdepth_or_discard);
				}
			}
		}
		if (LINE_XY(x, y+1)) {
			if (fragdepth_or_discard || fragment_processing(x, y+1, z)) {
				SET_V4(c->builtins.gl_FragCoord, x, y+1, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= fpart_(yend)*xgap;
					draw_fragment(&c->builtins, x, y+1, fragdepth_or_discard);
				}
			}
		}

		for(x=xpxl1+1; x < xpxl2; x++) {
			pr.x = x;
			pr.y = intery;
			t = dot_v2s(sub_v2s(pr, p1), sub_p2p1) / line_length_squared;
			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			y = ipart_(intery);
			if (LINE_XY(x, y)) {
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					SET_V4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= rfpart_(intery);
						draw_fragment(&c->builtins, x, y, fragdepth_or_discard);
					}
				}
			}
			if (LINE_XY(x, y+1)) {
				if (fragdepth_or_discard || fragment_processing(x, y+1, z)) {
					SET_V4(c->builtins.gl_FragCoord, x, y+1, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= fpart_(intery);
						draw_fragment(&c->builtins, x, y+1, fragdepth_or_discard);
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
		vec2 pr, sub_p2p1 = sub_v2s(p2, p1);
		float line_length_squared = len_v2(sub_p2p1);
		line_length_squared *= line_length_squared;

		// TODO should be done for each fragment, after poly_offset is added?
		z1 = rsw_mapf(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
		z2 = rsw_mapf(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

		float gradient = dx / dy;
		float yend = round_(y1);
		float xend = x1 + gradient*(yend - y1);
		float ygap = rfpart_(y1 + 0.5f);
		int ypxl1 = yend;
		int xpxl1 = ipart_(xend);

		t = 0.0f;
		z = z1 + poly_offset;
		w = w1;

		x = xpxl1;
		y = ypxl1;
		if (LINE_XY(x, y)) {
			if (fragdepth_or_discard || fragment_processing(x, y, z)) {
				SET_V4(c->builtins.gl_FragCoord, x, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= rfpart_(xend)*ygap;
					draw_fragment(&c->builtins, x, y, fragdepth_or_discard);
				}
			}
		}
		if (LINE_XY(x+1, y)) {
			if (fragdepth_or_discard || fragment_processing(x+1, y, z)) {
				SET_V4(c->builtins.gl_FragCoord, x+1, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= fpart_(xend)*ygap;
					draw_fragment(&c->builtins, x+1, y, fragdepth_or_discard);
				}
			}
		}

		float interx = xend + gradient;

		yend = round_(y2);
		xend = x2 + gradient*(yend - y2);
		ygap = fpart_(y2+0.5f);
		int ypxl2 = yend;
		int xpxl2 = ipart_(xend);

		t = 1.0f;
		z = z2 + poly_offset;
		w = w2;

		x = xpxl2;
		y = ypxl2;
		if (LINE_XY(x, y)) {
			if (fragdepth_or_discard || fragment_processing(x, y, z)) {
				SET_V4(c->builtins.gl_FragCoord, x, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= rfpart_(xend)*ygap;
					draw_fragment(&c->builtins, x, y, fragdepth_or_discard);
				}
			}
		}
		if (LINE_XY(x+1, y)) {
			if (fragdepth_or_discard || fragment_processing(x+1, y, z)) {
				SET_V4(c->builtins.gl_FragCoord, x+1, y, z, 1/w);
				c->builtins.discard = GL_FALSE;
				c->builtins.gl_FragDepth = z;
				setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
				fragment_shader(c->fs_input, &c->builtins, uniform);
				if (!c->builtins.discard) {
					c->builtins.gl_FragColor.w *= fpart_(xend)*ygap;
					draw_fragment(&c->builtins, x+1, y, fragdepth_or_discard);
				}
			}
		}

		for(y=ypxl1+1; y < ypxl2; y++) {
			pr.x = interx;
			pr.y = y;
			t = dot_v2s(sub_v2s(pr, p1), sub_p2p1) / line_length_squared;
			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			x = ipart_(interx);
			if (LINE_XY(x, y)) {
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					SET_V4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= rfpart_(interx);
						draw_fragment(&c->builtins, x, y, fragdepth_or_discard);
					}
				}
			}
			if (LINE_XY(x+1, y)) {
				if (fragdepth_or_discard || fragment_processing(x+1, y, z)) {
					SET_V4(c->builtins.gl_FragCoord, x+1, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= fpart_(interx);
						draw_fragment(&c->builtins, x+1, y, fragdepth_or_discard);
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

/* We clip the segment [a,b] against the 6 planes of the normal volume.
 * We compute the point 'c' of intersection and the value of the parameter 't'
 * of the intersection if x=a+t(b-a).
 */

// Plane component = g * w. g is ±1 for Z. X/Y g is the guard NDC limit,
// which is ±1 when PGL_GUARD_BAND is 0.
static float clip_comp(vec4* dst, vec4* a, vec4* b, int axis, float g)
{
	float av, bv, dv, dw, den, t;
	if (axis == 0) {
		av = a->x;
		bv = b->x;
	} else if (axis == 1) {
		av = a->y;
		bv = b->y;
	} else {
		av = a->z;
		bv = b->z;
	}
	dv = bv - av;
	dw = b->w - a->w;
	den = dv - g * dw;
	if (den == 0.0f)
		t = 0.0f;
	else
		t = (g * a->w - av) / den;

	dst->x = a->x + t * (b->x - a->x);
	dst->y = a->y + t * (b->y - a->y);
	dst->z = a->z + t * (b->z - a->z);
	dst->w = a->w + t * dw;
	if (axis == 0)
		dst->x = g * dst->w;
	else if (axis == 1)
		dst->y = g * dst->w;
	else
		dst->z = g * dst->w;
	return t;
}

static float clip_xmin(vec4* dst, vec4* a, vec4* b)
{
	return clip_comp(dst, a, b, 0, c->guard_ndc_left);
}
static float clip_xmax(vec4* dst, vec4* a, vec4* b)
{
	return clip_comp(dst, a, b, 0, c->guard_ndc_right);
}
static float clip_ymin(vec4* dst, vec4* a, vec4* b)
{
	return clip_comp(dst, a, b, 1, c->guard_ndc_bottom);
}
static float clip_ymax(vec4* dst, vec4* a, vec4* b)
{
	return clip_comp(dst, a, b, 1, c->guard_ndc_top);
}
static float clip_zmin(vec4* dst, vec4* a, vec4* b)
{
	return clip_comp(dst, a, b, 2, -1.0f);
}
static float clip_zmax(vec4* dst, vec4* a, vec4* b)
{
	return clip_comp(dst, a, b, 2, 1.0f);
}


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
	if (q->clip_space.w > 0.0f)
		q->screen_space = mult_m4_v4(c->vp_mat, q->clip_space);
}




static glVertex* pgl_clip_new_vert(glVertex* a, glVertex* b, int clip_bit)
{
	glVertex* q = pgl_arena_alloc(PGL_MAX_CLIP_VERTS);
	float t = clip_proc[clip_bit](&q->clip_space, &a->clip_space, &b->clip_space);
	update_clip_pt(q, a, b, t);
	return q;
}

// cc_in is the root frame's codes, with CLIP_Z_NEAR forced where w <= 0.
// Deeper frames pass NULL and read clip_code off the vertex they were given.
// e0, e1, e2 are the edges v0-v1, v1-v2, v2-v0. Original glverts are not written.
static void draw_triangle_clip(glVertex* v0, glVertex* v1, glVertex* v2,
                               int e0, int e1, int e2,
                               unsigned provoke, int clip_bit,
                               const int* cc_in,
                               pgl_tri* recs, int* n_out, int n_base)
{
	int c_or, c_and, c_ex_or, cc[3], clip_mask;
	glVertex *q0, *q1, *q2, *n1, *n2;
	int eq0, eq1, eq2;

	if (cc_in) {
		cc[0] = cc_in[0];
		cc[1] = cc_in[1];
		cc[2] = cc_in[2];
	} else {
		cc[0] = v0->clip_code;
		cc[1] = v1->clip_code;
		cc[2] = v2->clip_code;
	}

	// Bits 6-9 are frustum X/Y, not planes. Walking them drops a guard-clipped
	// triangle at clip_bit == 6.
	c_or = (cc[0] | cc[1] | cc[2]) & CLIP_PLANES_MASK;
	c_and = (cc[0] & cc[1] & cc[2]) & CLIP_FRUSTUM_MASK;
	if (c_and != 0)
		return;
	if (c_or == 0) {
		pgl_append_tri(v0, v1, v2, e0, e1, e2, provoke, recs, n_out, n_base);
		return;
	}

	while (clip_bit < 6 && (c_or & (1 << clip_bit)) == 0)
		++clip_bit;

	// Rounding residual only. Same drop in every build, no log.
	if (clip_bit == 6)
		return;

	clip_mask = 1 << clip_bit;
	c_ex_or = (cc[0] ^ cc[1] ^ cc[2]) & clip_mask;

	if (c_ex_or) {
		/* one point outside */
		if (cc[0] & clip_mask) {
			q0 = v0; q1 = v1; q2 = v2;
			eq0 = e0; eq1 = e1; eq2 = e2;
		} else if (cc[1] & clip_mask) {
			q0 = v1; q1 = v2; q2 = v0;
			eq0 = e1; eq1 = e2; eq2 = e0;
		} else {
			q0 = v2; q1 = v0; q2 = v1;
			eq0 = e2; eq1 = e0; eq2 = e1;
		}

		n1 = pgl_clip_new_vert(q0, q1, clip_bit);
		n2 = pgl_clip_new_vert(q0, q2, clip_bit);

		// n1-q1 keeps q0-q1. q2-n1 is the new edge.
		draw_triangle_clip(n1, q1, q2, eq0, eq1, 0, provoke, clip_bit + 1,
		                   NULL, recs, n_out, n_base);
		// n2-n1 and n1-q2 are new. q2-n2 keeps q2-q0.
		// tmp1.edge_flag = 0 on this second triangle is the TinyGL fix.
		draw_triangle_clip(n2, n1, q2, 0, 0, eq2, provoke, clip_bit + 1,
		                   NULL, recs, n_out, n_base);
	} else {
		/* two points outside. q0 is the one inside. */
		if ((cc[0] & clip_mask) == 0) {
			q0 = v0; q1 = v1; q2 = v2;
			eq0 = e0; eq2 = e2;
		} else if ((cc[1] & clip_mask) == 0) {
			q0 = v1; q1 = v2; q2 = v0;
			eq0 = e1; eq2 = e0;
		} else {
			q0 = v2; q1 = v0; q2 = v1;
			eq0 = e2; eq2 = e1;
		}

		n1 = pgl_clip_new_vert(q0, q1, clip_bit);
		n2 = pgl_clip_new_vert(q0, q2, clip_bit);

		// n1-n2 is new. q0-n1 keeps q0-q1. n2-q0 keeps q2-q0.
		// tmp1.edge_flag = 0 is the TinyGL fix (was 1).
		draw_triangle_clip(q0, n1, n2, eq0, 0, eq2, provoke, clip_bit + 1,
		                   NULL, recs, n_out, n_base);
	}
}

static void draw_triangle_point(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke)
{
	//TODO use provoke?
	PGL_UNUSED(provoke);
	c->mip_uv_per_px = 0.0f;

	glVertex* vert[3] = { v0, v1, v2 };
	vec3 hp[3];
	hp[0] = v4_to_v3h(v0->screen_space);
	hp[1] = v4_to_v3h(v1->screen_space);
	hp[2] = v4_to_v3h(v2->screen_space);

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
	// Lines: no per-tri UV footprint (could add later from edge only)
	c->mip_uv_per_px = 0.0f;

	vec4 s0 = v0->screen_space;
	vec4 s1 = v1->screen_space;
	vec4 s2 = v2->screen_space;

	// TODO remove redundant calc in thick_line_shader
	vec3 hp0 = v4_to_v3h(s0);
	vec3 hp1 = v4_to_v3h(s1);
	vec3 hp2 = v4_to_v3h(s2);
	float w0 = v0->screen_space.w;
	float w1 = v1->screen_space.w;
	float w2 = v2->screen_space.w;

	float poly_offset = 0;
	if (c->poly_offset_line) {
		poly_offset = calc_poly_offset(hp0, hp1, hp2);
	}

	if (c->line_smooth) {
		if (c->assemble_edges & 1) {
			draw_aa_line(hp0, hp1, w0, w1, v0->vs_out, v1->vs_out, provoke, poly_offset);
		}
		if (c->assemble_edges & 2) {
			draw_aa_line(hp1, hp2, w1, w2, v1->vs_out, v2->vs_out, provoke, poly_offset);
		}
		if (c->assemble_edges & 4) {
			draw_aa_line(hp2, hp0, w2, w0, v2->vs_out, v0->vs_out, provoke, poly_offset);
		}
	} else {
		if (c->assemble_edges & 1) {
			draw_thick_line(hp0, hp1, w0, w1, v0->vs_out, v1->vs_out, provoke, poly_offset);
		}
		if (c->assemble_edges & 2) {
			draw_thick_line(hp1, hp2, w1, w2, v1->vs_out, v2->vs_out, provoke, poly_offset);
		}
		if (c->assemble_edges & 4) {
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

// Per-triangle constant LOD support (phase 2B): max |Δuv|/|Δxy| over edges.
//
// UV = the first two consecutive non-FLAT floats in vs_output (a vec2 pair,
// scanned at even slots to match PGL_SMOOTH2 packing).  Other smooth varyings
// (normals, eye-space positions, colors, …) must not contribute: taking max
// over all pairs massively inflates λ when those channels vary more than
// texcoords (e.g. webgl_lessons/lesson15 — UV + normal + position).
//
// Convention: put texcoords as that first non-FLAT float pair when using a
// *MIPMAP* MIN_FILTER with auto LOD.  texture*Lod is unaffected.
//
// Cost: once per filled triangle, a few edge sqrts — cheap vs FS work.  No
// program flag (unlike fragdepth_or_discard) because the savings for untextured
// draws are small and a false “off” would silently break mips.  n < 2 exits
// immediately after zeroing mip_uv_per_px.
static void pgl_setup_tri_mip_grad(glVertex* v0, glVertex* v1, glVertex* v2,
                                   vec3 hp0, vec3 hp1, vec3 hp2)
{
	c->mip_uv_per_px = 0.0f;

	int n = c->vs_output.size;
	if (n < 2)
		return;

	// First pair of consecutive non-FLAT floats (even-aligned) = UV
	int uv0 = -1;
	for (int i = 0; i + 1 < n; i += 2) {
		if (c->vs_output.interpolation[i] == PGL_FLAT ||
		    c->vs_output.interpolation[i + 1] == PGL_FLAT)
			continue;
		uv0 = i;
		break;
	}
	if (uv0 < 0)
		return;

	glVertex* verts[3] = { v0, v1, v2 };
	vec3 hps[3] = { hp0, hp1, hp2 };

	for (int e = 0; e < 3; ++e) {
		int a = e;
		int b = (e + 1) % 3;
		float dx = hps[b].x - hps[a].x;
		float dy = hps[b].y - hps[a].y;
		float pix = sqrtf(dx * dx + dy * dy);
		if (pix < 1e-6f)
			continue;
		float inv_pix = 1.0f / pix;

		float du = verts[b]->vs_out[uv0] - verts[a]->vs_out[uv0];
		float dv = verts[b]->vs_out[uv0 + 1] - verts[a]->vs_out[uv0 + 1];
		float uv_len = sqrtf(du * du + dv * dv);
		float scale = uv_len * inv_pix;
		if (scale > c->mip_uv_per_px)
			c->mip_uv_per_px = scale;
	}
}

// 8-bit subpixel grid (1/256 px), same as common GPU rasterizers.
#define PGL_SUBPIXEL_BITS 8
#define PGL_SUBPIXEL_SCALE (1 << PGL_SUBPIXEL_BITS)

static inline int pgl_snap_xy(float v)
{
	return (int)floorf(v * (float)PGL_SUBPIXEL_SCALE + 0.5f);
}

// Same implicit line as make_Line(ax,ay, bx,by) evaluated at (px,py).
static inline i64 pgl_edge_eq(int ax, int ay, int bx, int by, int px, int py)
{
	return (i64)(ay - by) * px
	     + (i64)(bx - ax) * py
	     + (i64)ax * by
	     - (i64)bx * ay;
}

static inline int pgl_same_sign(i64 a, i64 b)
{
	return (a > 0 && b > 0) || (a < 0 && b < 0);
}

static void draw_triangle_fill(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke)
{
	vec4 p0 = v0->screen_space;
	vec4 p1 = v1->screen_space;
	vec4 p2 = v2->screen_space;

	vec3 hp0 = v4_to_v3h(p0);
	vec3 hp1 = v4_to_v3h(p1);
	vec3 hp2 = v4_to_v3h(p2);

	pgl_setup_tri_mip_grad(v0, v1, v2, hp0, hp1, hp2);

	// TODO even worth calculating or just some constant?
	float poly_offset = 0;

	if (c->poly_offset_fill) {
		poly_offset = calc_poly_offset(hp0, hp1, hp2);
	}

	/*
	print_v4(hp0, "\n");
	print_v4(hp1, "\n");
	print_v4(hp2, "\n");

	printf("%f %f %f\n", p0.w, p1.w, p2.w);
	print_v3(hp0, "\n");
	print_v3(hp1, "\n");
	print_v3(hp2, "\n\n");
	*/

	int vx0 = pgl_snap_xy(hp0.x);
	int vy0 = pgl_snap_xy(hp0.y);
	int vx1 = pgl_snap_xy(hp1.x);
	int vy1 = pgl_snap_xy(hp1.y);
	int vx2 = pgl_snap_xy(hp2.x);
	int vy2 = pgl_snap_xy(hp2.y);

	i64 e01_v2 = pgl_edge_eq(vx0, vy0, vx1, vy1, vx2, vy2);
	i64 e20_v1 = pgl_edge_eq(vx2, vy2, vx0, vy0, vx1, vy1);
	i64 e12_v0 = pgl_edge_eq(vx1, vy1, vx2, vy2, vx0, vy0);
	if (!e01_v2 || !e20_v1 || !e12_v0)
		return;

	int svx = pgl_snap_xy(-1.0f);
	int svy = pgl_snap_xy(-2.5f);
	i64 e01_s = pgl_edge_eq(vx0, vy0, vx1, vy1, svx, svy);
	i64 e20_s = pgl_edge_eq(vx2, vy2, vx0, vy0, svx, svy);
	i64 e12_s = pgl_edge_eq(vx1, vy1, vx2, vy2, svx, svy);

	// Bbox from snapped XY so coverage and the loop agree.
	float inv_scale = 1.0f / (float)PGL_SUBPIXEL_SCALE;
	hp0.x = vx0 * inv_scale;
	hp0.y = vy0 * inv_scale;
	hp1.x = vx1 * inv_scale;
	hp1.y = vy1 * inv_scale;
	hp2.x = vx2 * inv_scale;
	hp2.y = vy2 * inv_scale;

	//can't think of a better/cleaner way to do this than these 8 lines
	float x_min = MIN(hp0.x, hp1.x);
	float x_max = MAX(hp0.x, hp1.x);
	float y_min = MIN(hp0.y, hp1.y);
	float y_max = MAX(hp0.y, hp1.y);

	x_min = MIN(hp2.x, x_min);
	x_max = MAX(hp2.x, x_max);
	y_min = MIN(hp2.y, y_min);
	y_max = MAX(hp2.y, y_max);

	// Viewport ∩ scissor ∩ framebuffer. While XY clipping is still ±w this
	// does not change covered pixels; it is what keeps fragments inside
	// glViewport once a triangle is allowed to extend past the frustum.
	int r_left, r_bottom, r_right, r_top;
	if (!pgl_viewport_raster_rect(&r_left, &r_bottom, &r_right, &r_top))
		return;

	x_min = MAX(x_min, (float)r_left);
	x_max = MIN(x_max, (float)r_right);
	y_min = MAX(y_min, (float)r_bottom);
	y_max = MIN(y_max, (float)r_top);
	if (!(x_min < x_max) || !(y_min < y_max))
		return;

	// TODO is there any point to having an int index?
	// I think I did it for OpenMP
	// Clipped bbox is >= 0; +0.5 then trunc is round-half-up (= roundf) without libm.
	int ix_max = x_max + 0.5f;
	int iy_max = y_max + 0.5f;

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

	int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;
	Shader_Builtins builtins;

	for (int iy = y_min; iy<iy_max; ++iy) {
		int py = (iy << PGL_SUBPIXEL_BITS) + (PGL_SUBPIXEL_SCALE / 2);

		for (int ix = x_min; ix<ix_max; ++ix) {
			int px = (ix << PGL_SUBPIXEL_BITS) + (PGL_SUBPIXEL_SCALE / 2);

			// Integer edges of snapped verts: a sample is in A, in B, or on the line.
			// page 117 of glspec / FoCG pg 34-5 and 167
			i64 e01 = pgl_edge_eq(vx0, vy0, vx1, vy1, px, py);
			i64 e20 = pgl_edge_eq(vx2, vy2, vx0, vy0, px, py);
			i64 e12 = pgl_edge_eq(vx1, vy1, vx2, vy2, px, py);

			if ((e01 == 0 || pgl_same_sign(e01, e01_v2)) &&
			    (e20 == 0 || pgl_same_sign(e20, e20_v1)) &&
			    (e12 == 0 || pgl_same_sign(e12, e12_v0))) {
				// On the edge, draw if the opposite vertex is on the same side as
				// (-1, -2.5). Deterministic owner for shared edges (e87e324).
				if ((e12 != 0 || pgl_same_sign(e12_v0, e12_s)) &&
				    (e20 != 0 || pgl_same_sign(e20_v1, e20_s)) &&
				    (e01 != 0 || pgl_same_sign(e01_v2, e01_s))) {
					gamma = (float)((double)e01 / (double)e01_v2);
					beta  = (float)((double)e20 / (double)e20_v1);
					alpha = (float)((double)e12 / (double)e12_v0);

					//calculate interpolation here
					tmp2 = alpha*inv_w0 + beta*inv_w1 + gamma*inv_w2;

					z = alpha * hp0.z + beta * hp1.z + gamma * hp2.z;

					z += poly_offset;
					z = rsw_mapf(z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far); //TODO move out (ie can I map hp1.z etc.)?

					// early testing if shader doesn't use fragdepth or discard
					if (!fragdepth_or_discard && !fragment_processing(ix, iy, z)) {
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
					// gl_FragCoord.xy is the pixel center (GL default; not pixel_center_integer)
					SET_V4(builtins.gl_FragCoord, ix + 0.5f, iy + 0.5f, z, tmp2);
					builtins.discard = GL_FALSE;
					builtins.gl_FragDepth = z;

					// have to do this here instead of outside the loop because somehow openmp messes it up
					// TODO probably some way to prevent that but it's just copying an int so no big deal
					builtins.gl_InstanceID = c->builtins.gl_InstanceID;

					c->programs.a[c->cur_program].fragment_shader(fs_input, &builtins, c->programs.a[c->cur_program].uniform);
					if (!builtins.discard) {

						draw_fragment(&builtins, ix, iy, fragdepth_or_discard);
					}
				}
			}
		}
	}
}


// TODO should this be done in colors/integers not vec4/floats?
// and if it's done in Colors/integers what's the performance difference?
static vec4 blend_pixel(vec4 src, vec4 dst, int buf)
{
	vec4 bc = c->blend_color;
	float i = MIN(src.w, 1-dst.w); // in colors this would be min(src.a, 255-dst.a)/255

	// only initializing to get rid of "possibly uninitialized warning"
	vec4 Cs = {0}, Cd = {0};

	switch (c->blend_sRGB[buf]) {
	case GL_ZERO:                     SET_V4(Cs, 0,0,0,0);                                 break;
	case GL_ONE:                      SET_V4(Cs, 1,1,1,1);                                 break;
	case GL_SRC_COLOR:                Cs = src;                                              break;
	case GL_ONE_MINUS_SRC_COLOR:      SET_V4(Cs, 1-src.x,1-src.y,1-src.z,1-src.w);         break;
	case GL_DST_COLOR:                Cs = dst;                                              break;
	case GL_ONE_MINUS_DST_COLOR:      SET_V4(Cs, 1-dst.x,1-dst.y,1-dst.z,1-dst.w);         break;
	case GL_SRC_ALPHA:                SET_V4(Cs, src.w, src.w, src.w, src.w);              break;
	case GL_ONE_MINUS_SRC_ALPHA:      SET_V4(Cs, 1-src.w,1-src.w,1-src.w,1-src.w);         break;
	case GL_DST_ALPHA:                SET_V4(Cs, dst.w, dst.w, dst.w, dst.w);              break;
	case GL_ONE_MINUS_DST_ALPHA:      SET_V4(Cs, 1-dst.w,1-dst.w,1-dst.w,1-dst.w);         break;
	case GL_CONSTANT_COLOR:           Cs = bc;                                               break;
	case GL_ONE_MINUS_CONSTANT_COLOR: SET_V4(Cs, 1-bc.x,1-bc.y,1-bc.z,1-bc.w);             break;
	case GL_CONSTANT_ALPHA:           SET_V4(Cs, bc.w, bc.w, bc.w, bc.w);                  break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: SET_V4(Cs, 1-bc.w,1-bc.w,1-bc.w,1-bc.w);             break;

	case GL_SRC_ALPHA_SATURATE:       SET_V4(Cs, i, i, i, 1);                              break;
	/*not implemented yet
	 * won't be until I implement dual source blending/dual output from frag shader
	 *https://www.opengl.org/wiki/Blending#Dual_Source_Blending
	case GL_SRC1_COLOR:               Cs =  break;
	case GL_ONE_MINUS_SRC1_COLOR:     Cs =  break;
	case GL_SRC1_ALPHA:               Cs =  break;
	case GL_ONE_MINUS_SRC1_ALPHA:     Cs =  break;
	*/
	default:
		PGL_ASSERT(0 && "ERROR: unrecognized blend_sRGB!");
		break;
	}

	switch (c->blend_dRGB[buf]) {
	case GL_ZERO:                     SET_V4(Cd, 0,0,0,0);                                 break;
	case GL_ONE:                      SET_V4(Cd, 1,1,1,1);                                 break;
	case GL_SRC_COLOR:                Cd = src;                                              break;
	case GL_ONE_MINUS_SRC_COLOR:      SET_V4(Cd, 1-src.x,1-src.y,1-src.z,1-src.w);         break;
	case GL_DST_COLOR:                Cd = dst;                                              break;
	case GL_ONE_MINUS_DST_COLOR:      SET_V4(Cd, 1-dst.x,1-dst.y,1-dst.z,1-dst.w);         break;
	case GL_SRC_ALPHA:                SET_V4(Cd, src.w, src.w, src.w, src.w);              break;
	case GL_ONE_MINUS_SRC_ALPHA:      SET_V4(Cd, 1-src.w,1-src.w,1-src.w,1-src.w);         break;
	case GL_DST_ALPHA:                SET_V4(Cd, dst.w, dst.w, dst.w, dst.w);              break;
	case GL_ONE_MINUS_DST_ALPHA:      SET_V4(Cd, 1-dst.w,1-dst.w,1-dst.w,1-dst.w);         break;
	case GL_CONSTANT_COLOR:           Cd = bc;                                               break;
	case GL_ONE_MINUS_CONSTANT_COLOR: SET_V4(Cd, 1-bc.x,1-bc.y,1-bc.z,1-bc.w);             break;
	case GL_CONSTANT_ALPHA:           SET_V4(Cd, bc.w, bc.w, bc.w, bc.w);                  break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: SET_V4(Cd, 1-bc.w,1-bc.w,1-bc.w,1-bc.w);             break;

	case GL_SRC_ALPHA_SATURATE:       SET_V4(Cd, i, i, i, 1);                              break;
	/*not implemented yet
	case GL_SRC_ALPHA_SATURATE:       Cd =  break;
	case GL_SRC1_COLOR:               Cd =  break;
	case GL_ONE_MINUS_SRC1_COLOR:     Cd =  break;
	case GL_SRC1_ALPHA:               Cd =  break;
	case GL_ONE_MINUS_SRC1_ALPHA:     Cd =  break;
	*/
	default:
		PGL_ASSERT(0 && "ERROR: unrecognized blend_dRGB!");
		break;
	}

	// TODO simplify combine redundancies
	switch (c->blend_sA[buf]) {
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
		PGL_ASSERT(0 && "ERROR: unrecognized blend_sA!");
		break;
	}

	switch (c->blend_dA[buf]) {
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
		PGL_ASSERT(0 && "ERROR: unrecognized blend_dA!");
		break;
	}

	vec4 result;

	// TODO eliminate function calls to avoid alpha component calculations?
	switch (c->blend_eqRGB[buf]) {
	case GL_FUNC_ADD:
		result = add_v4s(mult_v4s(Cs, src), mult_v4s(Cd, dst));
		break;
	case GL_FUNC_SUBTRACT:
		result = sub_v4s(mult_v4s(Cs, src), mult_v4s(Cd, dst));
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		result = sub_v4s(mult_v4s(Cd, dst), mult_v4s(Cs, src));
		break;
	case GL_MIN:
		SET_V4(result, MIN(src.x, dst.x), MIN(src.y, dst.y), MIN(src.z, dst.z), MIN(src.w, dst.w));
		break;
	case GL_MAX:
		SET_V4(result, MAX(src.x, dst.x), MAX(src.y, dst.y), MAX(src.z, dst.z), MAX(src.w, dst.w));
		break;
	default:
		PGL_ASSERT(0 && "ERROR: unrecognized blend_eqRGB!");
		break;
	}

	switch (c->blend_eqA[buf]) {
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
		PGL_ASSERT(0 && "ERROR: unrecognized blend_eqA!");
		break;
	}

	return result;
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
		PGL_ASSERT(0 && "ERROR: Unrecognized logic op!");
		return s; //defaults to GL_COPY
	}
}

#ifndef PGL_NO_STENCIL
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
		PGL_ASSERT(0 && "ERROR: unrecognized stencil function!");
		return 0;
	}

}

// TODO change ints to GLboolean? or just rename to indicate they're booleans?
// stencil_dest is pointer to stencil pixel which may be u32 for PGL_D24S8
static void stencil_op(int stencil, int depth, void* stencil_dest)
{
	GLuint op, ref, mask;
	// TODO make them proper arrays in gl_context?
	// ops is effectively set to an array of sfail, dpfail, dppass for either front
	// or back facing
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

	stencil_pix_t orig = *(stencil_pix_t*)stencil_dest;

	// TODO check C conversion guide...is there a point to masking the low byte?
#ifdef PGL_D16
	u8 val = orig;
#else
	u8 val = orig & PGL_STENCIL_MASK;
#endif

	switch (op) {
	case GL_KEEP: return;
	case GL_ZERO: val = 0; break;
	case GL_REPLACE: val = ref; break;
	case GL_INCR: if (val < 255) val++; break;
	case GL_INCR_WRAP: val++; break;
	case GL_DECR: if (val > 0) val--; break;
	case GL_DECR_WRAP: val--; break;
	case GL_INVERT: val = ~val; break;
	default: PGL_ASSERT(0 && "ERROR: unknown stencil op!");
	}

	// TODO is this sufficient? It doesn't really write protect
	// the bits not covered by the mask, it will just write 0 there
	//u8 result = val & mask;
	// TODO create a stencil test to verify correct behavior
	u8 result = (orig & ~mask) | (val & mask);

#ifdef PGL_D16
	*(u8*)stencil_dest = result;
#else
	*(u32*)stencil_dest = (orig & ~PGL_STENCIL_MASK) | result;
#endif
}

// end PGL_NO_STENCIL
#endif

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
#ifndef PGL_NO_DEPTH_NO_STENCIL
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

	int i = 0;
	if (c->has_depth_buf || c->has_stencil_buf)
		i = -y*c->zbuf.w + x;

	//MSAA
	
#ifndef PGL_NO_STENCIL
	stencil_pix_t* stencil_dest = NULL;
	if (c->has_stencil_buf) {
		stencil_dest = &GET_STENCIL_PIX(i);
		if (c->stencil_test) {
			if (!stencil_test(EXTRACT_STENCIL(*stencil_dest))) {
				stencil_op(GL_FALSE, GL_TRUE, stencil_dest);
				return 0;
			}
		}
	}
#endif

	// Spec: no depth buffer ⇒ depth test implicitly disabled (do not read/write Z).
	if (c->has_depth_buf && c->depth_test) {
		int depth_result;
		if (c->zbuf_float) {
			float* zrow = (float*)c->zbuf.lastrow;
			float dest_d = zrow[i];
			float src_d = z;
			// Reuse depth_func with float compares
			switch (c->depth_func) {
			case GL_LESS:     depth_result = src_d < dest_d; break;
			case GL_LEQUAL:   depth_result = src_d <= dest_d; break;
			case GL_GREATER:  depth_result = src_d > dest_d; break;
			case GL_GEQUAL:   depth_result = src_d >= dest_d; break;
			case GL_EQUAL:    depth_result = src_d == dest_d; break;
			case GL_NOTEQUAL: depth_result = src_d != dest_d; break;
			case GL_ALWAYS:   depth_result = 1; break;
			case GL_NEVER:    depth_result = 0; break;
			default:          PGL_ASSERT(0 && "ERROR: unrecognized depth test!"); depth_result = 0; break;
			}
#ifndef PGL_NO_STENCIL
			if (c->has_stencil_buf && c->stencil_test)
				stencil_op(GL_TRUE, depth_result, stencil_dest);
#endif
			if (!depth_result)
				return GL_FALSE;
			if (c->depth_mask)
				zrow[i] = src_d;
		} else {
			// I made gl_FragDepth read/write, ie same == to gl_FragCoord.z going into the shader
			// so I can just always use gl_FragDepth here
			u32 dest_depth = GET_Z(i);
			u32 src_depth = z * PGL_MAX_Z;

			depth_result = depthtest(src_depth, dest_depth);

#ifndef PGL_NO_STENCIL
			if (c->has_stencil_buf && c->stencil_test) {
				stencil_op(GL_TRUE, depth_result, stencil_dest);
			}
#endif
			if (!depth_result) {
				return GL_FALSE;
			}

			if (c->depth_mask) {
				SET_Z(i, src_depth);
			}
		}
#ifndef PGL_NO_STENCIL
	} else if (c->has_stencil_buf && c->stencil_test) {
		// Note depth test is treated as passed when depth testing is disabled
		stencil_op(GL_TRUE, GL_TRUE, stencil_dest);
#endif
	}
	return GL_TRUE;
#else
	// With no depth/stencil buffers this always returns true/pass
	return GL_TRUE;
#endif
}


// Write to default FB / pix_t surface (blend/logic/mask). No depth/stencil.
static void draw_pixel_fb(glFramebuffer* fb, vec4 cf, int x, int y)
{
	Color dest_color, src_color;
	pix_t src, dst;
	pix_t* dest_loc = &((pix_t*)fb->lastrow)[-y*fb->w + x];
	dst = *dest_loc;

	dest_color = PIXEL_TO_COLOR(dst);

	if (c->blend[0]) {
		src_color = v4_to_Color(clamp_01_v4(blend_pixel(cf, COLOR_TO_VEC4(dest_color), 0)));
	} else {
		cf = clamp_01_v4(cf);
		src_color = VEC4_TO_COLOR(cf);
	}

	src = RGBA_TO_PIXEL(src_color.r, src_color.g, src_color.b, src_color.a);

	if (c->logic_ops) {
		src = logic_ops_pixel(src, dst);
	}

#ifndef PGL_DISABLE_COLOR_MASK
	src = (src & c->color_mask_pix[0]) | (dst & ~c->color_mask_pix[0]);
#endif

	*dest_loc = src;
}

// Write to FBO color attachment using texture storage format (not window pix_t).
// U8 RGBA: Color* layout. Float R/RG/RGBA: raw floats (blend unclamped).
static void draw_pixel_color_rt(pglColorRT* rt, vec4 cf, int x, int y, int buf)
{
	int idx = -y * rt->w + x;
	if (rt->datatype == GL_FLOAT) {
		PGL_ASSERT(rt->components > 0);
		const int nc = rt->components;
		float* p = (float*)rt->lastrow + idx * nc;
		if (c->blend[buf]) {
			vec4 dst;
			SET_V4(dst, p[0],
			       nc > 1 ? p[1] : 0.f,
			       nc > 2 ? p[2] : 0.f,
			       nc > 3 ? p[3] : 1.f);
			cf = blend_pixel(cf, dst, buf);
		}
#ifndef PGL_DISABLE_COLOR_MASK
		GLboolean* wm = c->color_writemask[buf];
		if (wm[0]) p[0] = cf.x;
		if (nc > 1 && wm[1]) p[1] = cf.y;
		if (nc > 2 && wm[2]) p[2] = cf.z;
		if (nc > 3 && wm[3]) p[3] = cf.w;
#else
		p[0] = cf.x;
		if (nc > 1) p[1] = cf.y;
		if (nc > 2) p[2] = cf.z;
		if (nc > 3) p[3] = cf.w;
#endif
		return;
	}
	// U8 RGBA as Color
	Color* dest_loc = &((Color*)rt->lastrow)[idx];
	Color dest_color = *dest_loc;
	Color src_color;
	if (c->blend[buf]) {
		src_color = v4_to_Color(clamp_01_v4(blend_pixel(cf, COLOR_TO_VEC4(dest_color), buf)));
	} else {
		cf = clamp_01_v4(cf);
		src_color = VEC4_TO_COLOR(cf);
	}
#ifndef PGL_DISABLE_COLOR_MASK
	u32 m = c->color_mask_u8[buf];
	if (m != 0xFFFFFFFFu) {
		u32 d = *(u32*)dest_loc;
		u32 s = *(u32*)&src_color;
		*(u32*)dest_loc = (d & ~m) | (s & m);
		return;
	}
#endif
	*dest_loc = src_color;
}

// TODO not used anymore?
static void draw_pixel(vec4 cf, int x, int y, float z, int do_frag_processing)
{
	if (do_frag_processing && !fragment_processing(x, y, z)) {
		return;
	}
	if (!c->fbo_color_is_rt) {
		draw_pixel_fb(&c->back_buffer, cf, x, y);
		return;
	}
	// FBO: write gl_FragColor to first non-NONE draw buffer (lines / pgl helpers).
	// Desktop completeness guarantees that buffer has an attachment.
	for (GLsizei i = 0; i < c->num_draw_buffers; ++i) {
		if (c->draw_buffers[i] == GL_NONE) continue;
		int att = (int)(c->draw_buffers[i] - GL_COLOR_ATTACHMENT0);
		PGL_ASSERT(att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS);
		PGL_ASSERT(c->mrt_color[att].buf);
		draw_pixel_color_rt(&c->mrt_color[att], cf, x, y, (int)i);
		return;
	}
	// All draw buffers GL_NONE: no color write
}

// After FS: one depth/stencil test, then write all active draw buffers.
// Default FB: gl_FragColor → pix_t back_buffer.
// FBO: gl_FragColor / gl_FragData[i] → pglColorRT (U8 Color or float).
static void draw_fragment(Shader_Builtins* b, int x, int y, int do_frag_processing)
{
	if (do_frag_processing && !fragment_processing(x, y, b->gl_FragDepth)) {
		return;
	}

	if (!c->fbo_color_is_rt) {
		draw_pixel_fb(&c->back_buffer, b->gl_FragColor, x, y);
		return;
	}

	if (!c->mrt_active) {
		for (GLsizei i = 0; i < c->num_draw_buffers; ++i) {
			if (c->draw_buffers[i] == GL_NONE) continue;
			int att = (int)(c->draw_buffers[i] - GL_COLOR_ATTACHMENT0);
			PGL_ASSERT(att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS);
			PGL_ASSERT(c->mrt_color[att].buf);
			draw_pixel_color_rt(&c->mrt_color[att], b->gl_FragColor, x, y, (int)i);
			return;
		}
		return; // all GL_NONE
	}

	for (GLsizei i = 0; i < c->num_draw_buffers; ++i) {
		GLenum db = c->draw_buffers[i];
		if (db == GL_NONE)
			continue;
		int att = (int)(db - GL_COLOR_ATTACHMENT0);
		PGL_ASSERT(att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS);
		PGL_ASSERT(c->mrt_color[att].buf);
		draw_pixel_color_rt(&c->mrt_color[att], b->gl_FragData[i], x, y, (int)i);
	}
}

