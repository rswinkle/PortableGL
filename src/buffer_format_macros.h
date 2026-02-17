


// Feel free to change these presets
// Do not use one of these combined with individual settings; you can cause problems
// if multiple selections within the same category are defined. I guard for
// depth/stencil settings but not framebuffer settings

#if !defined(PGL_D16) && !defined(PGL_D24S8) && !defined(PGL_NO_DEPTH_NO_STENCIL)
#  ifdef PGL_TINY_MEM
     // framebuffer mem use = 4*w*h
#    define PGL_RGB565
#    define PGL_D16
#    define PGL_NO_STENCIL
#  elif defined(PGL_SMALL_MEM)
     // 4*w*h
#    define PGL_RGB565
#    define PGL_D16
#    define PGL_NO_STENCIL
#  elif defined(PGL_MED_MEM)
     // 6*w*h
#    define PGL_RGB565
#    define PGL_D24S8
#  else
     // 8*w*h
     // defining this just for users convenience to detect different builds
     // though if you manually select smaller framebuffers this becomes confusing
#    define PGL_NORMAL_MEM

#    define PGL_D24S8
     // pixel format default to PGL_ABGR32 set below if no other defined
#  endif
#endif


// depth settings check
#ifdef PGL_NO_DEPTH_NO_STENCIL
#  if defined(PGL_D16) || defined(PGL_D24S8)
#    error "PGL_D16 and PGL_D24S8 are incompatible with PGL_NO_DEPTH_NO_STENCIL"
#  endif
#  ifdef PGL_NO_STENCIL
   //#warning is technically not standard till C23 and C++23 but supported by most compilers
#    warning "You don't need to define PGL_NO_STENCIL if you defined PGL_NO_DEPTH_NO_STENCIL"
#  else
#    define PGL_NO_STENCIL
#  endif
#endif


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
 #define PGL_PIX_STR "RGBA32"
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
 #define PGL_PIX_STR "ARGB32"
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
 #define PGL_PIX_STR "BGRA32"
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
 #define PGL_PIX_STR "RGB565"
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
 #define PGL_PIX_STR "BGR565"
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
 #define PGL_PIX_STR "RGBA5551"
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
 #define PGL_PIX_STR "ABGR1555"
#else
 // default to PGL_ABGR32 for RGBA memory order on LSB
 #define PGL_ABGR32
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
 #define PGL_PIX_STR "ABGR32"
#endif


// for now all 32 bit pixel types are 8888, no weird 10,10,10,2
#if PGL_BITDEPTH == 32
 #define RGBA_TO_PIXEL(r,g,b,a) ((u32)(a) << PGL_ASHIFT | (u32)(r) << PGL_RSHIFT | (u32)(g) << PGL_GSHIFT | (u32)(b) << PGL_BSHIFT)
 #define PIXEL_TO_COLOR(p) make_Color(((p) & PGL_RMASK) >> PGL_RSHIFT, ((p) & PGL_GMASK) >> PGL_GSHIFT, ((p) & PGL_BMASK) >> PGL_BSHIFT, ((p) & PGL_AMASK) >> PGL_ASHIFT)
 #define pix_t u32
 #define COLOR_TO_VEC4(c) Color_to_v4(c)
 #define VEC4_TO_COLOR(v) v4_to_Color(v)
#elif PGL_BITDEPTH == 16
 #if PGL_AMASK == 0
  #define RGBA_TO_PIXEL(r,g,b,a) ((int)(r) << PGL_RSHIFT | (int)(g) << PGL_GSHIFT | (int)(b) << PGL_BSHIFT)
  #define PIXEL_TO_COLOR(p) make_Color(((p) & PGL_RMASK) >> PGL_RSHIFT, ((p) & PGL_GMASK) >> PGL_GSHIFT, ((p) & PGL_BMASK) >> PGL_BSHIFT, 255)
 #else
  #define RGBA_TO_PIXEL(r,g,b,a) ((int)(a) << PGL_ASHIFT | (int)(r) << PGL_RSHIFT | (int)(g) << PGL_GSHIFT | (int)(b) << PGL_BSHIFT)
  #define PIXEL_TO_COLOR(p) make_Color(((p) & PGL_RMASK) >> PGL_RSHIFT, ((p) & PGL_GMASK) >> PGL_GSHIFT, ((p) & PGL_BMASK) >> PGL_BSHIFT, ((p) & PGL_AMASK) >> PGL_ASHIFT)
 #endif

 #define pix_t u16
 #define PIXEL_TO_VEC4(p) make_v4((((p) & PGL_RMASK) >> PGL_RSHIFT)/(float)PGL_RMAX, (((p) & PGL_GMASK) >> PGL_GSHIFT)/(float)PGL_GMAX, (((p) & PGL_BMASK) >> PGL_BSHIFT)/(float)PGL_BMAX, (((p) & PGL_AMASK) >> PGL_ASHIFT)/(float)PGL_AMAX)

 #define COLOR_TO_VEC4(c) make_v4((c).r/(float)PGL_RMAX, (c).g/(float)PGL_GMAX, (c).b/(float)PGL_BMAX, (c).a/(float)PGL_AMAX)
 #define VEC4_TO_COLOR(v) make_Color(v.x*PGL_RMAX, v.y*PGL_GMAX, v.z*PGL_BMAX, v.w*PGL_AMAX)
#endif


// TODO these are messy. It makes the code cleaner but I should try to simplify
// these some more. It's the different needs of glClear() vs fragment_processing()
// combined with the different formats that makes it a headache.
#ifdef PGL_D16
 #define PGL_MAX_Z 0xFFFF
 #define PGL_ZSHIFT 0
 #define GET_ZPIX(i) ((u16*)c->zbuf.lastrow)[(i)]
 #define GET_ZPIX_TOP(i) ((u16*)c->zbuf.buf)[(i)]
 #define GET_Z(i) GET_ZPIX(i)
 #define SET_Z_PRESHIFTED(i, v) GET_ZPIX(i) = (v)
 #define SET_Z_PRESHIFTED_TOP(i, v) GET_ZPIX_TOP(i) = (v)
 //#define SET_Z(i, orig_zpix, v) GET_ZPIX(i) = (v)
 #define SET_Z(i, v) GET_ZPIX(i) = (v)

 #define stencil_pix_t u8
 #define GET_STENCIL_PIX(i) c->stencil_buf.lastrow[(i)]
 #define GET_STENCIL_PIX_TOP(i) c->stencil_buf.buf[(i)]
 #define EXTRACT_STENCIL(stencil_pix) (stencil_pix)
 #define GET_STENCIL(i) GET_STENCIL_PIX(i)
 #define GET_STENCIL_TOP(i) GET_STENCIL_PIX_TOP(i)
 #define SET_STENCIL(i, v) GET_STENCIL_PIX(i) = (v)
 #define SET_STENCIL_TOP(i, v) GET_STENCIL_PIX_TOP(i) = (v)
#elif defined(PGL_D24S8)
 #ifdef PGL_NO_STENCIL
 #error "PGL_NO_STENCIL is incompatible with PGL_D24S8 format, use with PGL_D16"
 #endif

 #define PGL_MAX_Z 0xFFFFFF
 // could use GL_STENCIL_BITS..?
 #define PGL_ZSHIFT 8

 #define GET_ZPIX(i) ((u32*)c->zbuf.lastrow)[(i)]
 #define GET_ZPIX_TOP(i) ((u32*)c->zbuf.buf)[(i)]
 #define GET_Z(i) (GET_ZPIX(i) >> PGL_ZSHIFT)
 #define SET_Z_PRESHIFTED(i, v) \
     GET_ZPIX(i) &= PGL_STENCIL_MASK; \
     GET_ZPIX(i) |= (v)

 #define SET_Z_PRESHIFTED_TOP(i, v) \
     GET_ZPIX_TOP(i) &= PGL_STENCIL_MASK; \
     GET_ZPIX_TOP(i) |= (v)

// TO use this method I need to refactor to have the stencil val *after*
// the stencil test/op run, returned from stencil_op() perhaps.
// TODO compare perf eventually
/*
 #define SET_Z(i, stencil_val, v) \
     GET_ZPIX(i) = ((stencil_val) & PGL_STENCIL_MASK) | ((v) << PGL_ZSHIFT);
*/

 #define SET_Z(i, v) \
     GET_ZPIX(i) &= PGL_STENCIL_MASK; \
     GET_ZPIX(i) |= ((v) << PGL_ZSHIFT)

 #define stencil_pix_t u32
 #define GET_STENCIL_PIX(i) ((stencil_pix_t*)c->stencil_buf.lastrow)[(i)]
 #define GET_STENCIL_PIX_TOP(i) ((stencil_pix_t*)c->stencil_buf.buf)[(i)]
 #define EXTRACT_STENCIL(stencil_pix) ((stencil_pix) & PGL_STENCIL_MASK)
 #define GET_STENCIL(i) (GET_STENCIL_PIX(i) & PGL_STENCIL_MASK)
 #define GET_STENCIL_TOP(i) (GET_STENCIL_PIX_TOP(i) & PGL_STENCIL_MASK)
 #define SET_STENCIL(i, v) \
     GET_STENCIL_PIX(i) &= ~PGL_STENCIL_MASK; \
     GET_STENCIL_PIX(i) |= (v)

 #define SET_STENCIL_TOP(i, v) \
     GET_STENCIL_PIX_TOP(i) &= ~PGL_STENCIL_MASK; \
     GET_STENCIL_PIX_TOP(i) |= (v)
#elif defined(PGL_NO_DEPTH_NO_STENCIL)
/* ok */
#else
#error "Must define one of PGL_D16, PGL_D24S8, PGL_NO_DEPTH_NO_STENCIL"
#endif

