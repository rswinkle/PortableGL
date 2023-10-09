
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
	z1 = MAP(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	z2 = MAP(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	float width = c->line_width;

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
			for (float j=x-width/2; j<x+width/2; ++j) {
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
				draw_pixel(c->builtins.gl_FragColor, ypxl1, xpxl1, c->builtins.gl_FragDepth, GL_TRUE);

			SET_VEC4(c->builtins.gl_FragCoord, ypxl1+1, xpxl1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl1+1, xpxl1, c->builtins.gl_FragDepth, GL_TRUE);
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
				draw_pixel(c->builtins.gl_FragColor, xpxl1, ypxl1, c->builtins.gl_FragDepth, GL_TRUE);

			SET_VEC4(c->builtins.gl_FragCoord, xpxl1, ypxl1+1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl1, ypxl1+1, c->builtins.gl_FragDepth, GL_TRUE);
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
				draw_pixel(c->builtins.gl_FragColor, ypxl2, xpxl2, c->builtins.gl_FragDepth, GL_TRUE);

			SET_VEC4(c->builtins.gl_FragCoord, ypxl2+1, xpxl2, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl2+1, xpxl2, c->builtins.gl_FragDepth, GL_TRUE);
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
				draw_pixel(c->builtins.gl_FragColor, xpxl2, ypxl2, c->builtins.gl_FragDepth, GL_TRUE);

			SET_VEC4(c->builtins.gl_FragCoord, xpxl2, ypxl2+1, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl2, ypxl2+1, c->builtins.gl_FragDepth, GL_TRUE);
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
				draw_pixel(c->builtins.gl_FragColor, intery, x, c->builtins.gl_FragDepth, GL_TRUE);

			SET_VEC4(c->builtins.gl_FragCoord, intery+1, x, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, intery+1, x, c->builtins.gl_FragDepth, GL_TRUE);

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
				draw_pixel(c->builtins.gl_FragColor, x, intery, c->builtins.gl_FragDepth, GL_TRUE);

			SET_VEC4(c->builtins.gl_FragCoord, x, intery+1, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, intery+1, c->builtins.gl_FragDepth, GL_TRUE);

		}
	}
}
