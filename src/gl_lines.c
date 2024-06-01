
// NOTE/TODO: This file is just a scratch space of all line related functions.
// Doing thick lines and anti-aliased lines (GL_LINE_SMOOTH) correctly is difficult and
// costly.  I'll be using this file to save various implementations and tweaks
// and swapping them into gl_internal.c to test them as needed.

#define CLIPXY_TEST(x, y) (x >= c->lx && x < c->ux && y >= c->ly && y < c->uy)

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

static void draw_thick_line_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
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

	// TODO should be done for each fragment, after poly_offset is added?
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
			z += poly_offset;
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
			z += poly_offset;
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
			z += poly_offset;
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
			z += poly_offset;
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
			draw_line(vec4_to_vec3h(t1), vec4_to_vec3h(t2), t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
		else
			draw_thick_line_shader(t1, t2, v1->vs_out, v2->vs_out, provoke);
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

			if (c->line_width < 1.5f)
				draw_line(vec4_to_vec3h(t1), vec4_to_vec3h(t2), t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
			else
				draw_thick_line_shader(t1, t2, v1->vs_out, v2->vs_out, provoke);
		}
	}
}


// ~10% faster to use this for 1 width lines...Can I close that gap?
static void draw_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
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
	z1 = rsw_mapf(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	z2 = rsw_mapf(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

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

static void draw_thick_line_simple(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
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
	z1 = rsw_mapf(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	z2 = rsw_mapf(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	float width = roundf(c->line_width);
	if (!width) {
		width = 1.0f;
	}
	int wi = width;
	float width2 = width/2.0f;
	float j;

	//4 cases based on slope
	if (m <= -1) {     //(-infinite, -1]
		//printf("slope <= -1\n");
		for (x = x_min, y = y_max; y>=y_min && x<=x_max; --y) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;
			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			j = floorf(x - width2)+0.5f; // TODO always rounds down
			for (int i=0; i<wi; ++i, ++j) {
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

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			j = floorf(y - width2)+0.5f;
			for (int i=0; i<wi; ++i, ++j) {
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

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			j = floorf(y - width2)+0.5f;
			for (int i=0; i<wi; ++i, ++j) {
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

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			j = floorf(x - width2)+0.5f;
			for (int i=0; i<wi; ++i, ++j) {
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

// TODO try doing a line by rendering two triangles
static void draw_thick_line_rect(vec3 v1, vec3 v2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
{
	float tmp;
	float* tmp_ptr;

	float x1 = v1.x, x2 = v2.x, y1 = v1.y, y2 = v2.y;
	float z1 = v1.z, z2 = v2.z;


	vec3 tmp_v3;

	//always draw from left to right
	if (v2.x < v1.x) {
		tmp_v3 = p1;
		v1 = v2;
		v2 = tmp_v3;

		tmp = w1;
		w1 = w2;
		w2 = tmp;

		tmp_ptr = v1_out;
		v1_out = v2_out;
		v2_out = tmp_ptr;
	}

	float width = c->line_width / 2.0f;

	//calculate slope and implicit line parameters once
	float m = (y2-y1)/(x2-x1);
	Line line = make_Line(p1.x, p1.y, p2.x, p2.y);
	normalize_line(&line);

	vec2 p1 = { x1, y1 };
	vec2 p2 = { x2, y2 };
	vec2 v12 = sub_vec2s(p2, p1);
	vec2 v1r, pr; // v2r

	float dot_1212 = dot_vec2s(v12, v12);

	vec2 rect[4];

	vec2 normal = { line.A, line.B };
	rect[0] = add_vec2s(p1, normal);
	rect[1] = sub_vec2s(p1, normal);
	rect[2] = add_vec2s(p2, normal);
	rect[3] = sub_vec2s(p2, normal);



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
	//float width2 = width*width;

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
			//if (dist*dist < width2) {
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

// based on draw_aa_line and draw_thick_line_simple
static void draw_thick_aa_line_simple(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
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

	float width = roundf(c->line_width);
	if (width <= 0) {
		width = 1.0f;
	}
	float half_w = width * 0.5f;

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
		float yend = y1 + gradient*(xend - x1) - half_w;
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
		for (int i=0; i<width-1; i++) {
			y++;
			if (CLIPXY_TEST(x, y)) {
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= xgap;
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
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
		yend = y2 + gradient*(xend - x2) - half_w;
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
		for (int i=0; i<width-1; i++) {
			y++;
			if (CLIPXY_TEST(x, y)) {
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= xgap;
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
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
			t = clamp_01(t);
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
			for (int i=0; i<width-1; i++) {
				y++;
				if (CLIPXY_TEST(x, y)) {
					if (fragdepth_or_discard || fragment_processing(x, y, z)) {
						SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard) {
							draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
						}
					}
				}
				;
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
		float xend = x1 + gradient*(yend - y1) - half_w;
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
		for (int i=0; i<width-1; i++) {
			x++;
			if (CLIPXY_TEST(x, y)) {
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= ygap;
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
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
		xend = x2 + gradient*(yend - y2) - half_w;
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
		for (int i=0; i<width-1; i++) {
			x++;
			if (CLIPXY_TEST(x, y)) {
				if (fragdepth_or_discard || fragment_processing(x, y, z)) {
					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
					c->builtins.discard = GL_FALSE;
					c->builtins.gl_FragDepth = z;
					setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
					fragment_shader(c->fs_input, &c->builtins, uniform);
					if (!c->builtins.discard) {
						c->builtins.gl_FragColor.w *= ygap;
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
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
			t = clamp_01(t);
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
			for (int i=0; i<width-1; i++) {
				x++;
				if (CLIPXY_TEST(x, y)) {
					if (fragdepth_or_discard || fragment_processing(x, y, z)) {
						SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
						c->builtins.discard = GL_FALSE;
						c->builtins.gl_FragDepth = z;
						setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
						fragment_shader(c->fs_input, &c->builtins, uniform);
						if (!c->builtins.discard) {
							draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
						}
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


// based on draw_thick_line
static void draw_thick_aa_line(vec3 hp1, vec3 hp2, float w1, float w2, float* v1_out, float* v2_out, unsigned int provoke, float poly_offset)
{
	float tmp;
	float* tmp_ptr;

	float x1 = hp1.x, x2 = hp2.x, y1 = hp1.y, y2 = hp2.y;
	float z1 = hp1.z, z2 = hp2.z;

	float dx = x2 - x1;
	float dy = y2 - y1;

	float width = c->line_width;
	float half_w = width * 0.5f;

	frag_func fragment_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniform = c->programs.a[c->cur_program].uniform;
	int fragdepth_or_discard = c->programs.a[c->cur_program].fragdepth_or_discard;

	float t, x, y, z, w, e, dist;


	if (fabsf(dx) > fabsf(dy)) {
		//always draw from left to right
		if (x2 < x1) {
			swap_(x1, x2);
			swap_(y1, y2);
			swap_(z1, z2);
			swap_(w1, w2);
			swap_(v1_out, v2_out);
		}

		//calculate slope and implicit line parameters once
		// TODO division by 0
		float m = dy / dx;
		Line line = make_Line(x1, y1, x2, y2);
		normalize_line(&line);

		vec2 n = { line.A, line.B }
		if (line.B < 0) {
			n.x = -n.x, n.y = -n.y;
		}

		vec2 p1 = { x1, y1 };
		vec2 p2 = { x2, y2 };
		vec2 v12 = sub_vec2s(p2, p1);
		vec2 v1r, pr; // v2r

		float dot_1212 = dot_vec2s(v12, v12);

		float x_min, x_max, y_min, y_max;

		
		vec2 s1 = add_vec2s(p1, n);
		vec2 s2 = sub_vec2s(p1, n);


		if (line.A > 0) {
			x_min = p1.x - line.A*half_w;
			x_max = p2.x + line.A*half_w;
		} else {
			x_min = p1.x + line.A*half_w;
			x_max = p2.x - line.A*half_w;
		}
		if (m <= 0) {
			y_min = p2.y - line.B*half_w;
			y_max = p1.y + line.B*half_w;
		} else {
			y_min = p1.y - line.B*half_w;
			y_max = p2.y + line.B*half_w;
		}

		// clipping/scissoring against side planes here
		x_min = MAX(c->lx, x_min);
		x_max = MIN(c->ux, x_max);
		y_min = MAX(c->ly, y_min);
		y_max = MIN(c->uy, y_max);
		// end clipping
	
		float xend_lower = round_(x_min);
		float xend = round_(p1.x);
		float xend_upper = round_(x_max);
		//y_min = round_(y_min);

		float x_mino = x_min;
		float x_maxo = x_max;

		float intery = p1.y + m * (xend - p1.x);
		float abs_b = (line.B > 0) ? line.B : -line.B;
		float intery_lower = intery - half_w*abs_b;
		float intery_upper = intery + half_w*abs_b;

		// calculate x_max or just use last logic?
		//int last = 0;

		//printf("%f %f %f %f   x_min etc\n", x_min, x_max, y_min, y_max);

		// TODO should be done for each fragment, after poly_offset is added?
		z1 = rsw_mapf(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
		z2 = rsw_mapf(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

		// TODO figure out this entire function with pencil and paper
		for (x = x_min; x < x_max; ++x) {
			pr.x = x;

			for (y = intery_lower; y < y_max; ++y) {
				pr.y = y;

				v1r = sub_vec2s(pr, p1);
				//v2r = sub_vec2s(pr, p2);
				e = dot_vec2s(v1r, v12);

				// c lies past the ends of the segment v12
				if (e <= 0.0f || e >= dot_1212) {
					continue;
				}

				y = ipart_(intery);

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
					if (!c->builtins.discard) {
						// This treats any pixel with covered center as full coverage
						// would have to invert for partial coverage >= half
						if (dist > width) {
							c->builtins.gl_FragColor.w *= EXTRA-(dist-width);
						} else if (width - dist <= EXTRA) {
							c->builtins.gl_FragColor.w *= 1.0f - (width-dist);
						}
						draw_pixel(c->builtins.gl_FragColor, x, y, c->builtins.gl_FragDepth, fragdepth_or_discard);
					}
				}
			}
			intery += m;
		}
	} else {
		// TODO steep
	}
}


#undef swap_
#undef plot
#undef ipart_
#undef fpart_
#undef round_
#undef rfpart_


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

void put_line(Color the_color, float x1, float y1, float x2, float y2)
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

	//4 cases based on slope
	if (m <= -1) {           //(-infinite, -1]
		x = floorf(x1)+0.5f;
		for (y=y_max; y>=y_min; --y) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+0.5f, y-1) < 0)
				x++;
		}
	} else if (m <= 0) {     //(-1, 0]
		y = floorf(y1)+0.5f;
		for (x=x_min; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+1, y-0.5f) > 0)
				y--;
		}
	} else if (m <= 1) {     //(0, 1]
		y = floorf(y1)+0.5f;
		for (x=x_min1; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+1, y+0.5f) < 0)
				y++;
		}

	} else {                 //(1, +infinite)
		x = floorf(x1)+0.5f;
		for (y=y_min; y<=y_max; ++y) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+0.5f, y+1) > 0)
				x++;
		}
	}
}

#define fswap(a, b) do { float tmp = a; a = b; b = tmp; } while (0)
#define scale_color(col, t) 

void put_aa_line(Color col, float x1, float y1, float x2, float y2)
{
	float tmp;

	float dx = x2 - x1;
	float dy = y2 - y1;
	float gradient;
	Line line = make_Line(x1, y1, x2, y2);
	vec4 c = Color_to_vec4(col);

	if (fabsf(dx) > fabsf(dy)) {
		//always draw from left to right
		if (x2 < x1) {
			fswap(x1, x2);
			fswap(y1, y2);
		}

		gradient = dy / dx;
		float step = (gradient > 0) ? 1.0f : -1.0f;

		// not handling end points specially for now

		float x_min = floorf(x1) + 0.5f;
		float x_max = floorf(x2) + 0.5f;
		float x, y, t;
		float intery = y1;
		for (x = x_min; x < x_max; x++) {
			y = floorf(intery) + 0.5;
			// 0 distance from line = full weight
			t = 1.0f - line_func(&line, x, y);

			Color c1 = { col.r * t, col.g * t, col.b * t, col.a };
			put_pixel(c1, x, y);

			t = 1.0f - t;
			Color c2 = { col.r * t, col.g * t, col.b * t, col.a };
			put_pixel(c1, x, y+step);

			intery += gradient;
		}


	} else {
		if (y2 < y1) {
			fswap(x1, x2);
			fswap(y1, y2);
		}

		gradient = dx / dy;
		float step = (gradient > 0) ? 1.0f : -1.0f;

		// not handling end points specially for now

		float y_min = floorf(y1) + 0.5f;
		float y_max = floorf(y2) + 0.5f;
		float x, y, t;
		float interx = x1;
		for (y = y_min; y < y_max; y++) {
			x = floorf(interx) + 0.5;
			// 0 distance from line = full weight
			t = 1.0f - line_func(&line, x, y);

			Color c1 = { col.r * t, col.g * t, col.b * t, col.a };
			put_pixel(c1, x, y);

			t = 1.0f - t;
			Color c2 = { col.r * t, col.g * t, col.b * t, col.a };
			put_pixel(c1, x+step, y);

			interx += gradient;
		}
	}
}

void put_wide_line(Color color1, Color color2, float width, float x1, float y1, float x2, float y2)
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
	vec2 ac, bc;

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

	/*
	print_vec2(a, "\n");
	print_vec2(b, "\n");
	printf("%f %f %f %f\n", x_min, x_max, y_min, y_max);
	*/

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
			bc = sub_vec2s(c, b);
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

/*

// Trying to adapt Xiaolin Wu's AA line algorithm to work with 0.5 pixel centers (that OpenGL uses)
// Theoretically there should be a way to make it look the same but I just can't get
// there, plus it's just less elegant than the original

#define fswap(a, b) do { float tmp = a; a = b; b = tmp; } while (0)
//#define scale_color(col, t) 

#define myplot(x, y, c, d) \
	do { Color f; \
		f.r = (d) * (c).r; \
		f.g = (d) * (c).g; \
		f.b = (d) * (c).b; \
		f.a = 255; \
		put_pixel(f, x, y); \
	} while (0)


void put_aa_line(Color col, float x1, float y1, float x2, float y2)
{
	float tmp;

	float dx = x2 - x1;
	float dy = y2 - y1;
	float gradient;
	//printf("(%f, %f) to (%f, %f)\n", x1, y1, x2, y2);

	if (fabsf(dx) > fabsf(dy)) {
		//always draw from left to right
		if (x2 < x1) {
			fswap(x1, x2);
			fswap(y1, y2);
		}

		gradient = dy / dx;
		float xend = floorf(x1) + 0.5f;
		float yend = y1 + gradient*(xend - x1);
		float xgap = floorf(x1+0.99f) - x1;
		int xpxl1 = xend;
		int ypxl1 = floorf(yend);
		printf("%f %f\n", yend, (float)(int)yend);
		float t = fabsf(yend - (int)yend - 0.5f);
		myplot(xpxl1, ypxl1, col, (1.0f-t)*xgap);
		myplot(xpxl1, ypxl1+1, col, t*xgap);
    	printf("xgap = %f\n", xgap);
    	printf("%f %f\n", 1.0f-t, t);

		float intery = yend+gradient;

		xend = floorf(x2) + 0.5f;
		yend = y2 + gradient*(xend - x2);
		xgap = x2 - floorf(x2);
		int xpxl2 = xend;
		int ypxl2 = floorf(yend); // TODO?
		t = fabsf(yend - (int)yend - 0.5f);
		myplot(xpxl2, ypxl2, col, (1.0f-t)*xgap);
		myplot(xpxl2, ypxl2+1, col, t*xgap);

		for (int x = xpxl1+1; x < xpxl2; x++) {
			t = fabsf(intery - (int)intery - 0.5f);
			myplot(x, intery, col, (1.0f-t));
			myplot(x, intery+1, col, t);
			intery += gradient;
		}

	} else {
		if (y2 < y1) {
			fswap(x1, x2);
			fswap(y1, y2);
		}

		gradient = dx / dy;
		float yend = floorf(y1) + 0.5f;
		float xend = x1 + gradient*(yend - y1);
		float ygap = floorf(y1+0.99999f) - y1;
		int ypxl1 = yend;
		int xpxl1 = floor(xend);
		float t = fabsf(xend - (int)xend - 0.5f);
		myplot(xpxl1, ypxl1, col, (1.0f-t));
		myplot(xpxl1+1, ypxl1, col, t);

		float interx = xend+gradient;

		yend = floorf(y2) + 0.5f;
		xend = x2 + gradient*(yend - y2);
		ygap = y2 - floorf(y2);
		int ypxl2 = yend;
		int xpxl2 = floor(xend); // TODO?
		t = fabsf(xend - (int)xend - 0.5f);
		myplot(xpxl2, ypxl2, col, (1.0f-t)*ygap);
		myplot(xpxl2+1, ypxl2, col, t*ygap);

		for (int y = ypxl1+1; y < ypxl2; y++) {
			t = fabsf(interx - (int)interx - 0.5f);
			myplot(interx, y, col, (1.0f-t));
			myplot(interx+1, y, col, t);
			interx += gradient;
		}
	}
}
*/


