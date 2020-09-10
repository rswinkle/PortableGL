
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
void clear_screen()
{
	memset(c->back_buffer.buf, 255, c->back_buffer.w * c->back_buffer.h * 4);
}

//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
void pglDrawFrame()
{
	frag_func frag_shader = c->programs.a[c->cur_program].fragment_shader;

	for (float y=0.5; y<c->back_buffer.h; ++y) {
		for (float x=0.5; x<c->back_buffer.w; ++x) {

			//ignore z and w components
			c->builtins.gl_FragCoord.x = x;
			c->builtins.gl_FragCoord.y = y;

			c->builtins.discard = GL_FALSE;
			frag_shader(NULL, &c->builtins, c->programs.a[c->cur_program].uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y);
		}
	}

}



void put_pixel(Color color, int x, int y)
{
	u32* dest = &((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	*dest = color.a << c->Ashift | color.r << c->Rshift | color.g << c->Gshift | color.b << c->Bshift;
}

//Should I have it take a glFramebuffer as paramater?
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
	float A = y1 - y2;
	float B = x2 - x1;
	float C = x1*y2 -x2*y1;

	int x, y;

	float x_min = MAX(0, MIN(x1, x2));
	float x_max = MIN(c->back_buffer.w-1, MAX(x1, x2));
	float y_min = MAX(0, MIN(y1, y2));
	float y_max = MIN(c->back_buffer.h-1, MAX(y1, y2));
	
	//4 cases based on slope
	if (m <= -1) {			//(-infinite, -1]
		x = x1;
		for (y=y_max; y>=y_min; --y) {
			put_pixel(the_color, x, y);
			if (A*(x+0.5f) + B*(y-1) + C < 0)
				x++;
		}
	} else if (m <= 0) {	//(-1, 0]
		y = y1;
		for (x=x_min; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (A*(x+1) + B*(y-0.5f) + C > 0)
				y--;
		}
	} else if (m <= 1) {	//(0, 1]
		y = y1;
		for (x=x_min; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (A*(x+1) + B*(y+0.5f) + C < 0)
				y++;
		}
		
	} else {				//(1, +infinite)
		x = x1;
		for (y=y_min; y<=y_max; ++y) {
			put_pixel(the_color, x, y);
			if (A*(x+0.5f) + B*(y+1) + C > 0)
				x++;
		}
	}
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


