
#if defined(PGL_AMASK) && defined(PGL_BMASK) && defined(PGL_GMASK) && defined(PGL_BMASK) && \
    defined(PGL_ASHIFT) && defined(PGL_BSHIFT) && defined(PGL_GSHIFT) && defined(PGL_BSHIFT) && \
    defined(PGL_RMAX) && defined(PGL_GMAX) && defined(PGL_BMAX) && defined(PGL_AMAX) && defined(PGL_BITDEPTH)
/* ok */
#elif !defined(PGL_AMASK) && !defined(PGL_BMASK) && !defined(PGL_GMASK) && !defined(PGL_BMASK) && \
    !defined(PGL_ASHIFT) && !defined(PGL_BSHIFT) && !defined(PGL_GSHIFT) && !defined(PGL_BSHIFT) && \
    !defined(PGL_RMAX) && !defined(PGL_GMAX) && !defined(PGL_BMAX) && !defined(PGL_AMAX) && !defined(PGL_BITDEPTH)
/* ok */
#else
#error "Must define all PGL_(RGBA)MASK, PGL_(RGBA)SHIFT, PGL_(RGBA)MAX, and PGL_BITDEPTH or none (which will give default PGL_AGBR32 format)"
#endif

// TODO more 32-bit formats
#ifdef PGL_RGBA32
#define PGL_RMASK 0xFF000000
#define PGL_GMASK 0x00FF0000
#define PGL_BMASK 0x0000FF00
#define PGL_AMASK 0x000000FF
#define PGL_RSHIFT 24
#define PGL_GSHIFT 16
#define PGL_BSHIFT 8
#define PGL_ASHIFT 0
#define PGL_RMAX 255
#define PGL_GMAX 255
#define PGL_BMAX 255
#define PGL_AMAX 255
#define PGL_BITDEPTH 32
#elif defined(PGL_ABGR32)
#define PGL_AMASK 0xFF000000
#define PGL_BMASK 0x00FF0000
#define PGL_GMASK 0x0000FF00
#define PGL_RMASK 0x000000FF
#define PGL_ASHIFT 24
#define PGL_BSHIFT 16
#define PGL_GSHIFT 8
#define PGL_RSHIFT 0
#define PGL_RMAX 255
#define PGL_GMAX 255
#define PGL_BMAX 255
#define PGL_AMAX 255
#define PGL_BITDEPTH 32
#elif defined(PGL_ARGB32)
#define PGL_AMASK 0xFF000000
#define PGL_RMASK 0x00FF0000
#define PGL_GMASK 0x0000FF00
#define PGL_BMASK 0x000000FF
#define PGL_ASHIFT 24
#define PGL_RSHIFT 16
#define PGL_GSHIFT 8
#define PGL_BSHIFT 0
#define PGL_RMAX 255
#define PGL_GMAX 255
#define PGL_BMAX 255
#define PGL_AMAX 255
#define PGL_BITDEPTH 32
#elif defined(PGL_BGRA32)
#define PGL_BMASK 0xFF000000
#define PGL_GMASK 0x00FF0000
#define PGL_RMASK 0x0000FF00
#define PGL_AMASK 0x000000FF
#define PGL_BSHIFT 24
#define PGL_GSHIFT 16
#define PGL_RSHIFT 8
#define PGL_ASHIFT 0
#define PGL_RMAX 255
#define PGL_GMAX 255
#define PGL_BMAX 255
#define PGL_AMAX 255
#define PGL_BITDEPTH 32
#elif defined(PGL_RGB565)
#define PGL_RMASK 0xF800
#define PGL_GMASK 0x07E0
#define PGL_BMASK 0x001F
#define PGL_AMASK 0x0000
#define PGL_RSHIFT 11
#define PGL_GSHIFT 5
#define PGL_BSHIFT 0
#define PGL_ASHIFT 0
#define PGL_RMAX 31
#define PGL_GMAX 63
#define PGL_BMAX 31
#define PGL_AMAX 0
#define PGL_BITDEPTH 16
#elif defined(PGL_BGR565)
#define PGL_BMASK 0xF800
#define PGL_GMASK 0x07E0
#define PGL_RMASK 0x001F
#define PGL_AMASK 0x0000
#define PGL_BSHIFT 11
#define PGL_GSHIFT 5
#define PGL_RSHIFT 0
#define PGL_ASHIFT 0
#define PGL_RMAX 31
#define PGL_GMAX 63
#define PGL_BMAX 31
#define PGL_AMAX 0
#define PGL_BITDEPTH 16
#elif defined(PGL_RGBA5551)
#define PGL_RMASK 0xF800
#define PGL_GMASK 0x07C0
#define PGL_BMASK 0x003E
#define PGL_AMASK 0x0001
#define PGL_RSHIFT 11
#define PGL_GSHIFT 6
#define PGL_BSHIFT 1
#define PGL_ASHIFT 0
#define PGL_RMAX 31
#define PGL_GMAX 31
#define PGL_BMAX 31
#define PGL_AMAX 1
#define PGL_BITDEPTH 16
#elif defined(PGL_ABGR1555)
#define PGL_AMASK 0x8000
#define PGL_BMASK 0x7C00
#define PGL_GMASK 0x03E0
#define PGL_RMASK 0x001F
#define PGL_ASHIFT 15
#define PGL_BSHIFT 10
#define PGL_GSHIFT 5
#define PGL_RSHIFT 0
#define PGL_RMAX 31
#define PGL_GMAX 31
#define PGL_BMAX 31
#define PGL_AMAX 1
#define PGL_BITDEPTH 16
#endif

// Default to RGBA memory order on a little endian architecture
// AKA PGL_ABGR32 above
#ifndef PGL_AMASK
#define PGL_AMASK 0xFF000000
#define PGL_BMASK 0x00FF0000
#define PGL_GMASK 0x0000FF00
#define PGL_RMASK 0x000000FF
#define PGL_ASHIFT 24
#define PGL_BSHIFT 16
#define PGL_GSHIFT 8
#define PGL_RSHIFT 0
#define PGL_RMAX 255
#define PGL_GMAX 255
#define PGL_BMAX 255
#define PGL_AMAX 255
#define PGL_BITDEPTH 32
#endif


// for now all 32 bit pixel types are 8888, no weird 10,10,10,2
#if PGL_BITDEPTH == 32
#define RGBA_TO_PIXEL(r,g,b,a) ((u32)(a) << PGL_ASHIFT | (u32)(r) << PGL_RSHIFT | (u32)(g) << PGL_GSHIFT | (u32)(b) << PGL_BSHIFT)
#define PIXEL_TO_COLOR(p) make_Color(((p) & PGL_RMASK) >> PGL_RSHIFT, ((p) & PGL_GMASK) >> PGL_GSHIFT, ((p) & PGL_BMASK) >> PGL_BSHIFT, ((p) & PGL_AMASK) >> PGL_ASHIFT)
#define pix_t u32

#define COLOR_TO_VEC4(c) Color_to_vec4(c)
#define VEC4_TO_COLOR(v) vec4_to_Color(v)

#elif PGL_BITDEPTH == 16

#if PGL_AMASK == 0
#define RGBA_TO_PIXEL(r,g,b,a) ((int)(r) << PGL_RSHIFT | (int)(g) << PGL_GSHIFT | (int)(b) << PGL_BSHIFT)
#define PIXEL_TO_COLOR(p) make_Color(((p) & PGL_RMASK) >> PGL_RSHIFT, ((p) & PGL_GMASK) >> PGL_GSHIFT, ((p) & PGL_BMASK) >> PGL_BSHIFT, 255)
#else
#define RGBA_TO_PIXEL(r,g,b,a) ((int)(a) << PGL_ASHIFT | (int)(r) << PGL_RSHIFT | (int)(g) << PGL_GSHIFT | (int)(b) << PGL_BSHIFT)
#define PIXEL_TO_COLOR(p) make_Color(((p) & PGL_RMASK) >> PGL_RSHIFT, ((p) & PGL_GMASK) >> PGL_GSHIFT, ((p) & PGL_BMASK) >> PGL_BSHIFT, ((p) & PGL_AMASK) >> PGL_ASHIFT)
#endif

#define pix_t u16
#define PIXEL_TO_VEC4(p) make_vec4((((p) & PGL_RMASK) >> PGL_RSHIFT)/(float)PGL_RMAX, (((p) & PGL_GMASK) >> PGL_GSHIFT)/(float)PGL_GMAX, (((p) & PGL_BMASK) >> PGL_BSHIFT)/(float)PGL_BMAX, (((p) & PGL_AMASK) >> PGL_ASHIFT)/(float)PGL_AMAX)

#define COLOR_TO_VEC4(c) make_vec4((c).r/(float)PGL_RMAX, (c).g/(float)PGL_GMAX, (c).b/(float)PGL_BMAX, (c).a/(float)PGL_AMAX)
#define VEC4_TO_COLOR(v) make_Color(v.x*PGL_RMAX, v.y*PGL_GMAX, v.z*PGL_BMAX, v.w*PGL_AMAX)
#endif


// TODO should PGL_D16 do separate stencil allocation or combine as 3 byte "pixel"? Or no stencil at all?
// For now it does a separate stencil allocation
#ifdef PGL_D16
#define PGL_MAX_Z 0xFFFF
#define PGL_ZSHIFT 0
#define PGL_STENCIL_STRIDE 1
#define GET_ZPIX(i) ((u16*)c->zbuf.lastrow)[(i)]
#define GET_STENCIL(i) c->stencil_buf.lastrow[(i)]
#else

// TODO not suported yet
#ifdef PGL_NO_STENCIL
#error "PGL_NO_STENCIL is incompatible with PGL_D24S8 format, use with PGL_D16"
#endif

#define PGL_D24S8 1
#define PGL_MAX_Z 0xFFFFFF
// could use GL_STENCIL_BITS..?
#define PGL_ZSHIFT 8
#define PGL_STENCIL_STRIDE 4
#define GET_ZPIX(i) ((u32*)c->zbuf.lastrow)[(i)]
#define GET_STENCIL(i) c->stencil_buf.lastrow[(i)*PGL_STENCIL_STRIDE+3]
#endif

