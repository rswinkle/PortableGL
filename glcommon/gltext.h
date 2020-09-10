#ifndef GLTEXT_H
#define GLTEXT_H

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "stb_truetype.h"

#include <stdint.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

unsigned char* make_color_bitmap(unsigned char* bitmap, int size, uint32_t background_rgba, uint32_t text_rgba);

unsigned char* stbtt_make_font_bitmap(stbtt_fontinfo* font, int pixel_size, int (*include_func)(int), int* height);


unsigned char* stbtt_make_text_bitmap(stbtt_fontinfo* font, int pixel_height, int line_gap, const char* text, int* w, int* h, uint32_t bg_color, uint32_t text_color);



typedef struct render_font
{
	unsigned char* image;
	size_t w, h;
	float pixel_height;
	size_t start;
	stbtt_bakedchar cdata[96];
} render_font;


typedef struct quad_data
{
	char* text;
	int len;
	int rlen;
	float* verts;
	float* texcoords;
	unsigned int* tris;
} quad_data;




typedef struct gltext_rect
{
	int x;
	int y;
	int w;
	int h;
} gltext_rect;

typedef struct raster_data
{
	char* text;
	int len;
	int rlen;
	gltext_rect* src_rects;
	gltext_rect* dst_rects;
} raster_data;


int stbtt_initface(render_font* rfont, stbtt_fontinfo* font_data, float pixel_height, uint32_t bg_color, uint32_t text_color);
int stbtt_print(render_font* font, float x, float y, float z, quad_data* qdata);

int stbtt_raster(render_font* font, float x, float y, raster_data* rdata);







#ifdef __cplusplus
}
#endif

#endif



#ifdef GLTEXT_IMPLEMENTATION


//pass in single channel 8 bit image background black, text white
unsigned char* make_color_bitmap(unsigned char* bitmap, int size, uint32_t background_rgba, uint32_t text_rgba)
{
	float rb, gb, bb, ab, rt, gt, bt, at;

	unsigned char* tmp = (unsigned char*)&background_rgba;
	rb = tmp[0];
	gb = tmp[1];
	bb = tmp[2];
	ab = tmp[3];

	tmp = (unsigned char*)&text_rgba;
	rt = tmp[0];
	gt = tmp[1];
	bt = tmp[2];
	at = tmp[3];

	printf("background = (%f, %f, %f, %f)\ntext = (%f, %f, %f, %f)\n", rb, gb, bb, ab, rt, gt, bt, at);

	unsigned char* color_image = (unsigned char*) malloc(size * 4); assert(color_image);
	float t;
	for (int i=0; i<size; ++i) {
		t = (bitmap[i] - 0) / 255.0;
		color_image[i*4] = rt * t + rb * (1 - t);
		color_image[i*4+1] = gt * t + gb * (1 - t);
		color_image[i*4+2] = bt * t + bb * (1 - t);
		color_image[i*4+3] = at * t + ab * (1 - t);
	}


	return color_image;
}


/* returns 8 bit white on black vertical bitmap of font */
unsigned char* stbtt_make_font_bitmap(stbtt_fontinfo* font, int pixel_size, int (*include_func)(int), int* height)
{
	int i, j;

	float scale = stbtt_ScaleForPixelHeight(font, pixel_size);

	for (i=0, j=0; i<256; ++i) {
		if (include_func(i))
			j++;
	}

	*height = j * pixel_size;

	//pixel_size*pixel_size per character, and j chars
	unsigned char* allchars = (unsigned char*) malloc(pixel_size*pixel_size*j);
	unsigned char* charbitmap = NULL;

	if (!allchars) {
		printf("could not allocate bitmap!\n\n");
		return NULL;
	}

//	int w, h, xoff, yoff;
	int x0, y0, x1, y1;
	int cur_char = 0;
	for (i=0; i<256; ++i) {
		if (include_func(i)) {
			stbtt_GetCodepointBitmapBox(font, i, scale, scale, &x0, &y0, &x1, &y1);
			charbitmap = &allchars[cur_char * pixel_size*pixel_size];
			cur_char++;

			stbtt_MakeCodepointBitmap(font, charbitmap, x1-x0, y1-y0, pixel_size, scale, scale, i);		
			/* for debugging purposes, output ascii bitmap 
			printf("%d: %d %d\n", i, x1-x0, y1-y0);

			for (j=0; j < pixel_size; ++j) {
				for (int k=0; k < pixel_size; ++k)
					putchar(" .:ioVM@"[*(charbitmap+j*(pixel_size)+k)>>5]);
				putchar('\n');
			}
			*/
		}
	}
	return allchars;
}




#define MAKE_VEC3(P, X, Y, Z) (P)[0] = (X); \
							  (P)[1] = (Y); \
							  (P)[2] = (Z); \
							  (P) += 3


#define MAKE_VEC2(P, X, Y) (P)[0] = (X); \
						   (P)[1] = (Y); \
						   (P) += 2

/*
char ttf_buffer[1<<20];
unsigned char temp_bitmap[512*512];

stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
GLstbtt_uint ftex;

void my_stbtt_initfont(void)
{
	fread(ttf_buffer, 1, 1<<20, fopen("c:/windows/fonts/times.ttf", "rb"));
	stbtt_BakeFontBitmap(data,0, 32.0, temp_bitmap,512,512, 32,96, cdata); // no guarantee this fits!
	// can free ttf_buffer at this point
	glGenTextures(1, &ftex);
	glBindTexture(GL_TEXTURE_2D, ftex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512,512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
	// can free temp_bitmap at this point
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void my_stbtt_print(float x, float y, char *text)
{
	// assume orthographic projection with units = screen pixels, origin at top left
	glBindTexture(GL_TEXTURE_2D, ftex);
	glBegin(GL_QUADS);
	while (*text) {
		if (*text >= 32 && *text < 128) {
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
			glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y0);
			glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y0);
			glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y1);
			glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y1);
		}
		++text;
	}
	glEnd();
}

	size_t w, h;
	size_t start;
	stbtt_bakedchar cdata[96];
*/

int stbtt_initface(render_font* rfont, stbtt_fontinfo* font_data, float pixel_height, uint32_t bg_color, uint32_t text_color)
{
	float min_dim;
	unsigned char* temp_bitmap, *color_bitmap;
	
	min_dim = sqrt(96 * pixel_height * pixel_height);
	min_dim = ceil(log(min_dim)/log(2));
	min_dim = pow(2, min_dim);

	if (!(temp_bitmap = (unsigned char*) malloc(min_dim * min_dim)))
		return 0;

	stbtt_BakeFontBitmap_font(font_data, pixel_height, temp_bitmap, min_dim, min_dim, 32, 96, rfont->cdata); // no guarantee this fits!
	// can free ttf_buffer at this point but I'll leave that to caller if they still want it
	
	if (!(!bg_color && !text_color)) {
		color_bitmap = make_color_bitmap(temp_bitmap, min_dim*min_dim, bg_color, text_color);
		free(temp_bitmap);
		temp_bitmap = color_bitmap;
	}
	
	rfont->start = 32;
	rfont->w = rfont->h = min_dim;
	rfont->pixel_height = pixel_height;
	rfont->image = temp_bitmap;

	return 1;
}


int stbtt_print(render_font* font, float x, float y, float z, quad_data* qdata)
{
	int len = strlen(qdata->text);
	if (!qdata->len) {
		qdata->len = len;

		qdata->verts = (float*) malloc(qdata->len*4*3*sizeof(float)); assert(qdata->verts);
		qdata->texcoords = (float*) malloc(qdata->len*4*2*sizeof(float)); assert(qdata->texcoords);
		qdata->tris = (unsigned int*) malloc(qdata->len*2*3*sizeof(unsigned int)); assert(qdata->tris);
	} else if (qdata->len != len) {
		qdata->len = len;

		qdata->verts = (float*) realloc(qdata->verts, len*4*3*sizeof(float)); assert(qdata->verts);
		qdata->texcoords = (float*) realloc(qdata->texcoords, len*4*2*sizeof(float)); assert(qdata->texcoords);
		qdata->tris = (unsigned int*) realloc(qdata->tris, len*2*3*sizeof(unsigned int)); assert(qdata->tris);
	}

	if (!qdata->verts || !qdata->texcoords || !qdata->tris)
		return 0;

	float* pverts = qdata->verts;
	float* ptex = qdata->texcoords;
	unsigned int* tris = qdata->tris;
	float orig_x = x, cur_y = y;
	
	stbtt_aligned_quad q;
	int i=0;
	char* text = qdata->text;
	while (*text) {
		if (*text >= font->start && *text < 128) {
			printf("%c %f %f\n", *text, x, y);
			stbtt_GetBakedQuad(font->cdata, font->w, font->h, *text-font->start, &x, &y, &q, 1);
			MAKE_VEC3(pverts, q.x0, -q.y0, z);
			MAKE_VEC3(pverts, q.x1, -q.y0, z);
			MAKE_VEC3(pverts, q.x1, -q.y1, z);
			MAKE_VEC3(pverts, q.x0, -q.y1, z);

			MAKE_VEC2(ptex, q.s0, q.t0);
			MAKE_VEC2(ptex, q.s1, q.t0);
			MAKE_VEC2(ptex, q.s1, q.t1);
			MAKE_VEC2(ptex, q.s0, q.t1);

			MAKE_VEC3(tris, i, i+3, i+2);
			MAKE_VEC3(tris, i, i+2, i+1);
			
			i += 4;
		} else if (*text == '\n') {
			x = orig_x;
			y = cur_y + font->pixel_height;
			cur_y = y;
		}
		++text;
	}

	qdata->rlen = i/4;

	/* not really necessary ...
	if (qdata->rlen != len) {
		len = qdata->rlen;
		qdata->verts = (float*) realloc(qdata->verts, len*4*3*sizeof(float)); assert(qdata->verts);
		qdata->texcoords = (float*) realloc(qdata->texcoords, len*4*2*sizeof(float)); assert(qdata->texcoords);
		qdata->tris = (unsigned int*) realloc(qdata->tris, len*2*3*sizeof(unsigned int)); assert(qdata->tris);
	}
	*/

	return 1;
}

int stbtt_raster(render_font* font, float x, float y, raster_data* rdata)
{
	int len = strlen(rdata->text);
	if (!rdata->len) {
		rdata->len = len;
		rdata->src_rects = (gltext_rect*) malloc(len*sizeof(gltext_rect)); assert(rdata->src_rects);
		rdata->dst_rects = (gltext_rect*) malloc(len*sizeof(gltext_rect)); assert(rdata->dst_rects);

	} else if (rdata->len != len) {
		rdata->len = len;
		rdata->src_rects = (gltext_rect*) realloc(rdata->src_rects, len*sizeof(gltext_rect)); assert(rdata->src_rects);
		rdata->dst_rects = (gltext_rect*) realloc(rdata->dst_rects, len*sizeof(gltext_rect)); assert(rdata->dst_rects);

	}

	if (!rdata->src_rects || !rdata->dst_rects)
		return 0;

	float orig_x = x, cur_y = y;
	
	stbtt_aligned_quad q;
	int i=0;
	char* text = rdata->text;
	while (*text) {
		if (*text >= font->start && *text < 128) {
			printf("%c %f %f\n", *text, x, y);
			stbtt_GetBakedQuad(font->cdata, font->w, font->h, *text-font->start, &x, &y, &q, 1);

			//TODO round floats to nearest integer/pixel?
			rdata->dst_rects[i].x = q.x0;
			rdata->dst_rects[i].y = q.y0;
			rdata->dst_rects[i].w = q.x1 - q.x0;
			rdata->dst_rects[i].h = q.y1 - q.y0;

			rdata->src_rects[i].x = q.s0 * font->w;
			rdata->src_rects[i].y = q.t0 * font->h;
			rdata->src_rects[i].w = (q.s1 - q.s0) * font->w;
			rdata->src_rects[i].h = (q.t1 - q.t0) * font->h;
			
			++i;
		} else if (*text == '\n') {
			x = orig_x;
			y = cur_y + font->pixel_height;
			cur_y = y;
		}
		++text;
	}

	rdata->rlen = i;
	/*
	if (i != len) {
		len = i;
		rdata->src_rects = (gltext_rect*) realloc(rdata->src_rects, len*sizeof(gltext_rect)); assert(rdata->src_rects);
		rdata->dst_rects = (gltext_rect*) realloc(rdata->dst_rects, len*sizeof(gltext_rect)); assert(rdata->dst_rects);
	}
	*/

	return 1;
}


unsigned char* stbtt_make_text_bitmap(stbtt_fontinfo* font, int pixel_height, int line_gap, const char* text, int* w, int* h, uint32_t bg_color, uint32_t text_color)
{
	int ascent, descent, linegap, baseline, ch=0;
	float scale, xpos=0;

	scale = stbtt_ScaleForPixelHeight(font, pixel_height);
	stbtt_GetFontVMetrics(font, &ascent, &descent, &linegap);
	baseline = (int) (ascent*scale);


	int advance,lsb,x0,y0,x1,y1;
	float x_shift;
	float max_xpos = 0;
	float max_vertical = 0;

	linegap = (line_gap >= 0) ? line_gap : linegap;	
	while (text[ch]) {
		if (text[ch] == '\n') {
			baseline += (int)((ascent - descent + linegap)*scale);
			xpos = 0;
			++ch;
			continue;
		}
		x_shift = xpos - (float) floor(xpos);
		stbtt_GetCodepointHMetrics(font, text[ch], &advance, &lsb);
		stbtt_GetCodepointBitmapBoxSubpixel(font, text[ch], scale, scale, x_shift, 0, &x0,&y0,&x1,&y1);

		printf("%d %d %d %d\n", x0, y0, x1, y1);
		xpos += (advance * scale);
		if (text[ch+1])
			xpos += scale*stbtt_GetCodepointKernAdvance(font, text[ch], text[ch+1]);
		++ch;
		
		max_xpos = (xpos > max_xpos) ? xpos : max_xpos;
	}


	printf("\n%d %d %d %f %f\n\n", baseline, descent, linegap, scale, floor(descent*scale));
	max_vertical = baseline - (int)floor(descent*scale) + (int)ceil(linegap*scale);


	int width = max_xpos; //(max_xpos > 600) ? max_xpos : 601;
	int height = max_vertical;
	
	*w = width;
	*h = height;
	printf("\n\n%d %d %f\n\n", width, height, max_vertical);
	unsigned char* color_bitmap = NULL;
	unsigned char* screen = (unsigned char*) calloc(1, width*height);
	if (!screen) {
		return NULL;
	}

	fprintf(stderr, "screen = %p %p\n\n", screen, screen + width*height); 

	baseline = (int) (ascent*scale);
	xpos = 0;
	ch = 0;
	while (text[ch]) {
		printf("%c\t", text[ch]);
		if (text[ch] == '\n') {
			baseline += (int)(ascent - descent + linegap)*scale;
			xpos = 0;
			++ch;
			continue;
		}
		x_shift = xpos - (float) floor(xpos);
		stbtt_GetCodepointHMetrics(font, text[ch], &advance, &lsb);
		stbtt_GetCodepointBitmapBoxSubpixel(font, text[ch], scale, scale, x_shift, 0, &x0,&y0,&x1,&y1);
		printf("%d %d %d %d\n", x0, y0, x1, y1);
		printf("%d %f %f\n", baseline, xpos, x_shift);
		stbtt_MakeCodepointBitmapSubpixel(font, &screen[(baseline + y0)*width + ((int) xpos + x0)], x1-x0, y1-y0, width, scale, scale, x_shift, 0, text[ch]);
		// note that this stomps the old data, so where character boxes overlap (e.g. 'lj') it's wrong
		// because this API is really for baking character bitmaps into textures. if you want to render
		// a sequence of characters, you really need to render each bitmap to a temp buffer, then
		// "alpha blend" that into the working buffer
		xpos += (advance * scale);
		if (text[ch+1])
			xpos += scale*stbtt_GetCodepointKernAdvance(font, text[ch], text[ch+1]);
		++ch;
	}

	/*
	for (j=0; j < 20; ++j) {
		for (i=0; i < 78; ++i)
			putchar(" .:ioVM@"[screen[j*text_len*pixel_height+i]>>5]);
		putchar('\n');
	}
	*/
	if (!(!bg_color && !text_color)) {
		color_bitmap = make_color_bitmap(screen, width*height, bg_color, text_color); assert(color_bitmap);
		free(screen);
		screen = color_bitmap;
	}

	return screen;
}






/*
unsigned char* ft_make_bitmap(FT_Face face, const char* text, FT_Int x, FT_Int y, uint32_t text_rgba, uint32_t background_rgba)
{
	FT_GlyphSlot  slot = face->glyph;  // a small shortcut
	FT_Bitmap*    bitmap = &slot->bitmap;
	FT_UInt       glyph_index;
	int           pen_x, pen_y, n, error, depth, ncolors;
	int num_chars = strlen(text);

	SDL_Surface* surface, *tmpsurface;
	SDL_Texture* tex;


	float rb, gb, bb, ab, rt, gt, bt, at, t;

	//TODO assumes LSB currently
	unsigned char* tmp = (unsigned char*)&background_rgba;
	rb = tmp[0];
	gb = tmp[1];
	bb = tmp[2];
	ab = tmp[3];

	tmp = (unsigned char*)&text_rgba;
	rt = tmp[0];
	gt = tmp[1];
	bt = tmp[2];
	at = tmp[3];


	pen_x = 0;
	pen_y = 0;

	error = FT_Load_Char(face, text[0], FT_LOAD_RENDER);
	if (error) {
		printf("%ld %ld %ld %ld\n", bitmap->width, bitmap->rows, bitmap->pitch, bitmap->pixel_mode);
		puts("Error loading char");
		return;
	}
	if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO) {
		depth = 1;
	} else if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY) {
		depth = 8;
	} else if (bitmap->pixel_mode == FT_PIXEL_MODE_NONE) {
		printf("==============\n%ld %ld %ld %ld\n============\n", bitmap->width, bitmap->rows, bitmap->pitch, bitmap->pixel_mode);
	} else {
		printf("draw_text: unsupported pixel type %d\n", bitmap->pixel_mode);
		exit(0);
	}

	//TODO maybe I should remove max_height and always just use pixel height
	//so that text drawn at the same y position will always line up along the bottom
	//and look right rather than lining up at the top and you could have "floating"
	//small letters if they're drawn without any tall ones to get max_height = pixel_height
	int max_height = 0, first_row=1;
	for (n = 0; n < num_chars; n++) {
		error = FT_Load_Char(face, text[n], FT_LOAD_RENDER);
		if (error)
			continue;

		if (first_row) {
			if (slot->bitmap_top > max_height)
				max_height = slot->bitmap_top;
		}

		pen_x += slot->advance.x >> 6;
		if (slot->advance.y) {
			pen_y += slot->advance.y >> 6;
			first_row = 0;
		}
		
	}

	int width = pen_x;
	int height = pen_y + max_height;
	printf("width = %d height = %d\n", width, height);

	SDL_Surface* orig_surface = SDL_CreateRGBSurface(0, width, height, depth, 0, 0, 0, 0);
	printf("width = %d height = %d, pitch = %d\n", width, height, orig_surface->pitch);
	if (orig_surface->format->palette) {
		ncolors = orig_surface->format->palette->ncolors;

		//In this case it'd depend on how freetype or the font did it, using a full byte of space but only going up to num_grays-1
		//or using num_gray different numbers that span 0-255
		if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY && ncolors != bitmap->num_grays) {
			printf("draw_text: unsupported pixel type, num_grays != ncolors\n");
			exit(0);
		}

		for (int i=0; i<ncolors; ++i) {
			t = (float)i / (float)(ncolors-1);
			orig_surface->format->palette->colors[i].r = rt * t + rb * (1 - t);
			orig_surface->format->palette->colors[i].g = gt * t + gb * (1 - t);
			orig_surface->format->palette->colors[i].b = bt * t + bb * (1 - t);
			orig_surface->format->palette->colors[i].a = at * t + ab * (1 - t);
		}
		SDL_SetSurfaceBlendMode(orig_surface, SDL_BLENDMODE_BLEND);
	}
	SDL_FillRect(orig_surface, NULL, SDL_MapRGBA(orig_surface->format, rb, gb, bb, ab)); //hopefully those floats convert right
	
	SDL_Surface* textimage, *rgba32surface;
	rgba32surface = SDL_CreateRGBSurface(0, 0, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000); assert(rgba32surface);
	textimage = SDL_ConvertSurface(orig_surface, rgba32surface->format, 0); assert(textimage);
	SDL_SetSurfaceBlendMode(textimage, SDL_BLENDMODE_BLEND);

//	puts("*******");
	//for (int i=0; i<textimage->h; ++i) {
	//	for (int j=0; j<textimage->pitch; ++j)
			//printf("%u ", ((unsigned char*)(textimage->pixels))[i]);
		//putchar('\n');
	//}

	SDL_Rect dstrect;
	pen_x = 0;
	pen_y = 0;
	for (n=0; n<num_chars; n++) {

		// load glyph image into the slot (erase previous one)
		error = FT_Load_Char(face, text[n], FT_LOAD_RENDER);
		if (error)
			continue;  // ignore errors


		//printf("%ld %ld %ld %ld\n", bitmap->width, bitmap->rows, bitmap->pitch, bitmap->pixel_mode);
		//for (int i=0; i<bitmap->width; ++i)
	//		printf("%u ", (bitmap->buffer[i/8] >> (7 - (i % 8))) & 1);
//		putchar('\n');


		// now, draw to our target surface
		tmpsurface = SDL_CreateRGBSurfaceFrom(bitmap->buffer, bitmap->width, bitmap->rows, depth, bitmap->pitch, 0, 0, 0, 0);
		if (orig_surface->format->palette) {
			SDL_SetSurfacePalette(tmpsurface, orig_surface->format->palette);
			//SDL_SetSurfaceBlendMode(tmpsurface, SDL_BLENDMODE_BLEND); //necessary? nope
		}
		surface = SDL_ConvertSurface(tmpsurface, rgba32surface->format, 0); assert(surface);
		//SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND); //necessary?  nope

		//TODO add scaling?
		dstrect.x = pen_x + slot->bitmap_left;
		dstrect.y = pen_y - slot->bitmap_top + max_height; //pen_x is as if on baseline but blit is top left corner
		dstrect.w = surface->w;
		dstrect.h = surface->h;
		printf("x y w h = %d %d %d %d\n", dstrect.x, dstrect.y, dstrect.w, dstrect.h);


		if (SDL_BlitSurface(surface, NULL, textimage, &dstrect)) {
			printf("Blit error: %s\n", SDL_GetError());
		}
		printf("x y w h = %d %d %d %d\n", dstrect.x, dstrect.y, dstrect.w, dstrect.h);
		SDL_FreeSurface(tmpsurface);
		SDL_FreeSurface(surface);
		
		// increment pen position
		pen_x += slot->advance.x >> 6;
		pen_y += slot->advance.y >> 6;
	}

//	puts("==================");
//	for (int i=0; i<textimage->h; ++i) {
//		for (int j=0; j<textimage->pitch; ++j)
//			printf("%u ", ((unsigned char*)(textimage->pixels))[i*textimage->pitch+j]);
//		putchar('\n');
//	}


	tex = SDL_CreateTextureFromSurface(ren, textimage);
	//SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND); //not necessary
	render_texture(tex, ren, x, y);
	SDL_FreeSurface(textimage);
	SDL_FreeSurface(orig_surface);
	SDL_FreeSurface(rgba32surface);

	SDL_DestroyTexture(tex); //return it instead?
}

*/


#endif
