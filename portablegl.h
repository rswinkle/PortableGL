/*

PortableGL 0.101.0 MIT licensed software renderer that closely mirrors OpenGL 3.x
portablegl.com
robertwinkler.com

Do this:
    #define PORTABLEGL_IMPLEMENTATION
before you include this file in *one* C or C++ file to create the implementation.

If you plan on using your own 3D vector/matrix library rather than crsw_math
that is built into PortableGL and your names are the standard glsl vec[2-4],
mat[3-4] etc., define PGL_PREFIX_TYPES too before including portablegl to
prefix all those builtin types with pgl_ to avoid the clash. Note, if
you use PGL_PREFIX_TYPES and write your own shaders, the type for vertex_attribs
is also affected, changing from vec4* to pgl_vec4*.

You can check all the C++ examples and demos, I use my C++ rsw_math library.

// i.e. it should look like this:
#include ...
#include ...
#include ...
// if required
#define PGL_PREFIX_TYPES

#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

You can define PGL_ASSERT before the #include to avoid using assert.h.
You can define PGL_MALLOC, PGL_REALLOC, and PGL_FREE to avoid using malloc,
realloc, and free.
You can define PGL_MEMMOVE to avoid using memmove.

However, even if you define all of those before including portablegl, you
will still be using the standard library (math.h, string.h, stdlib.h, stdio.h
stdint.h, possibly others). It's not worth removing PortableGL's dependency on
the C standard library as it would make it far larger and more complicated
for no real benefit.


QUICK NOTES:
    Primarily of interest to game/graphics developers and other people who
    just want to play with the graphics pipeline and don't need peak
    performance or the the entirety of OpenGL or Vulkan features.

    For textures, color storage is GL_UNSIGNED_BYTE (RGBA8 after upload) or
    GL_FLOAT (R32F / RG32F / RGBA32F; no RGB32F). Both glTexImage* (PGL-owned
    copy) and pglTexImage* / pglTextureImage* (map user memory) accept that
    matrix for 1D/2D/3D; cubemaps remain U8 RGBA only. Float images are level 0
    only for now (mip-chain helpers are still RGBA8-centric). glTexImage* with
    GL_UNSIGNED_BYTE still converts many non-RGBA formats to packed RGBA8 unless
    PGL_DONT_CONVERT_TEXTURES is defined; the pgl map path does no conversion.
    Depth textures (GL_DEPTH_COMPONENT) are supported for FBO depth attach and
    sampling (.r). The argument internalformat is often ignored to ease porting.

    texture1D/2D: if MIN_FILTER is a *MIPMAP* mode and a mip chain exists,
    LOD is chosen once per triangle from screen-space UV scale.  λ = log2(ρ)
    with ρ ≈ (max |Δuv|/|Δxy| over edges) * max(base_w, base_h).  λ <= 0 uses
    level 0 + MAG_FILTER; otherwise an integer level from MIN_FILTER.
    The first two consecutive non-FLAT floats in vs_output (a vec2, scanned
    at even slots to match PGL_SMOOTH2 packing) are used as the UV pair for
    that scale.  Put texcoords there when other smooth varyings follow —
    treating normals/positions as UVs used to inflate λ and pick wrong mips.
    Points/lines force level 0.  Auto-LOD only runs on the normal glDraw* fill
    path (draw_triangle_fill sets c->mip_uv_per_px).  pgl_draw_geometry_raw /
    put_triangle_tex never set it, so they always sample level 0 via texture2D
    (SDL_RenderGeometryRaw-like).

    texture1DLod/2DLod/texture_cubemapLod take an explicit continuous LOD λ
    (not a mip index).  Same mag/min rule as texture1D/2D auto:
      λ ≤ 0 → base level + MAG_FILTER
      λ > 0 → MIN_FILTER: *MIPMAP_NEAREST picks one level (round λ);
              *MIPMAP_LINEAR blends floor(λ) and floor(λ)+1 (trilinear when
              within-level is LINEAR).  Within a chosen mip, filtering uses
              the within-level half of MIN (not MAG).  Trilinear only runs
              on the minify path.
    Non-mip MIN_FILTER: level 0 with within-level(MIN) on *Lod paths when
    no chain / incomplete-compat.

    texture1DGrad/2DGrad/texture_cubemapGrad take screen-space derivatives of
    the texture coordinate (GLSL textureGrad-style) and compute isotropic λ:
    ρx = length(dP/dx in texel units), ρy similarly, ρ = max(ρx,ρy),
    λ = log2(ρ).  Sampling then follows the same path as *Lod (including
    MAG when λ ≤ 0).  Cubemap grads project the direction onto the selected
    face and finite-difference the face UV (forced same face) before ρ.

    LOD helpers for apps that cannot use per-triangle auto-LOD (software
    full-frame shaders, custom raster paths, etc.):
      pgl_lod_screen(tex) / pgl_lod_screen_wh(tex, rt_w, rt_h)
        λ assuming UV = fragCoord/rt (0–1 across the render target).
        The no-_wh form uses c->back_buffer.w/h — the active color surface
        after glBindFramebuffer / pglSetBackBuffer / pglSetTexBackBuffer
        (viewport is ignored).
      pgl_lod_uv_scale(tex, s) / pgl_lod_uv_scale_wh(...)
        λ_screen + log2(|s|) for UV' = s * UV_screen.
      pgl_lod_grad / pgl_lod_grad1D
        λ from explicit derivatives (same math as texture*Grad).
    textureSize and pgl_lod_* require a non-zero texture object.  Default
    texture name 0 is not accepted (ambiguous across targets; typed samplers
    like texture2D still accept 0).  Passing 0 sets GL_INVALID_VALUE and is
    logged in debug builds; under PGL_UNSAFE the check is compiled out.

    Incomplete textures (MIN_FILTER is a *MIPMAP* mode but no chain /
    num_levels <= 1): by default PGL samples level 0 with MAG_FILTER
    (Compatibility-friendly).  Define PGL_CORE_PROFILE before including PGL
    to return black (0,0,0,1) instead, closer to Core incomplete-texture
    sampling.  Other Core-only checks can grow under the same macro over time
    (e.g. RECTANGLE wrap limited to CLAMP_TO_EDGE / CLAMP_TO_BORDER).

    MIN_FILTER stores full enums including *MIPMAP*.  MAG_FILTER is only
    NEAREST or LINEAR.

    Mipmap storage: glTexImage1D/2D honor the level argument for
    GL_TEXTURE_1D and GL_TEXTURE_2D (level 0 replaces the whole image block).
    All levels live in one contiguous allocation pointed to by tex->data
    (~4/3 the base image for a full chain); levels[] are fixed views into it.
    glGenerateMipmap(GL_TEXTURE_1D/2D/CUBE_MAP) builds an RGBA8 box-filtered
    chain.  If level 0 was user-owned (pglTexImage* / pglTextureImage*
    mapped pointer), GenerateMipmap copies L0 into a new PGL-owned block and
    appends the filtered levels — the caller's memory is left alone.
    Cubemap levels pack 6 faces each; faces are box-filtered independently
    (no edge seam filtering).  Cubemap faces via glTexImage2D/glTexSubImage2D
    remain level 0 only — use GenerateMipmap for the rest of the chain.
    texture_cubemap uses the same per-triangle auto LOD as texture2D when
    MIN_FILTER is a *MIPMAP* mode and a chain exists; otherwise level 0 +
    MAG_FILTER (or black under PGL_CORE_PROFILE if incomplete).
    texelFetch* honor lod for 1D/2D; textureSize honors lod for non-zero
    texture names (see above for name 0).  3D/rectangle mips are not
    implemented.  Automatic per-fragment derivatives are not implemented;
    use texture*Grad or the pgl_lod_* helpers instead.

    pglTexImage* / pglTextureImage* map user memory as level 0 only
    (level != 0 is INVALID_VALUE).  That sets num_levels = 1 and discards any
    previous mip chain descriptors.  Same format/type matrix as glTexImage*
    for 1D/2D/3D (U8 RGBA or float R/RG/RGBA/depth); no conversion on map.
    Higher U8 mip levels must use glTexImage* or glGenerateMipmap (which
    copies out of user memory as above).

    GL_TEXTURE_BASE_LEVEL / MAX_LEVEL and MIN_LOD / MAX_LOD enums exist but are
    not implemented.  PGL behaves as if BASE_LEVEL = 0 and the full defined
    chain is active (lod 0 is always the highest-res level present).  Note
    BASE_LEVEL/MAX_LEVEL (integer mip indices in the pyramid) are not the same
    as MIN_LOD/MAX_LOD (float clamps on λ); with base 0, lod is still not
    identical to MAX_LOD.

    The framebuffer format is a compile time setting which defaults
    to 32-bit RGBA memory order, though PGL supports any 32 or 16 bit pixel format
    as long as the appropriate macros are defined.
    Several variants are predefined for easy use, named for the integer order
    not the memory order:

        PGL_RGBA32: RGBA memory order on MSB architecture
        PGL_ABGR32: RGBA memory order on LSB architecture, default
        PGL_ARGB32/PGL_BGRA32: Two other common 32-bit formats
        PGL_RGB565/PGL_BGR565: Very common 16 bit formats
        PGL_RGBA5551/PGL_ABGR1555: Less common 16 bit formats

    Search PGL for those macros to see how to set up the controlling macros
    for other formats; it's pretty self explanatory though I may try to improve
    it in the future.

    The depth and stencil buffer defaults to a combined buffer with the high
    24 bits used for the depth value and the low 8 bits used for the stencil.
    This format is called PGL_D24S8 internally. The only other format supported
    is a 16 bit depth buffer and a separate 8-bit buffer for the stencil. This
    is selected by defining PGL_D16 before including PGL.

    If you define PGL_D16, you may also define PGL_NO_STENCIL to disable the
    stencil buffer entirely to save a bit more memory. However if you define
    PGL_NO_STENCIL you must define PGL_D16 as it makes no sense with
    the default PGL_D24S8.

    Lastly, you can define PGL_NO_DEPTH_NO_STENCIL which will of course
    disable both the depth and stencil buffers entirely.

    There are several predefined configuration depending on how much memory
    you want to/can use that select settings for the framebuffer formats and
    the vertex scratch space size:

    PGL_TINY_MEM: RGB565, D16, NO_STENCIL, 4 vertex attribs, 80 KB scratch space
    PGL_SMALL_MEM: Same as TINY but 800 KB scratch space
    PGL_MED_MEM: RGB565, 4 vertex attribs, 1.6 MB scratch space
    default: ABGR32, D24S8, 8 vertex attribs, 16 MB scratch space

    Obviously most of the time the default is fine, and if none of the
    presets match what you want you can mix and match and adjust any of
    the finer grained options individually, but don't define a preset *and*
    define individual settings as that will cause problems.


DOCUMENTATION
=============

Any PortableGL program has roughly this structure, with some things
possibly declared globally or passed around in function parameters
as needed:

    #define WIDTH 640
    #define HEIGHT 480

    // shaders are functions matching these prototypes
    void smooth_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
    void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

    typedef struct My_Uniforms {
        mat4 mvp_mat;
        vec4 v_color;
    } My_Uniforms;

    pix_t* backbuf = NULL;
    glContext the_context;

    if (!init_glContext(&the_context, &backbuf, WIDTH, HEIGHT)) {
        puts("Failed to initialize glContext");
        exit(0);
    }

    // interpolation is an array with an entry of PGL_SMOOTH, PGL_FLAT or
    // PGL_NOPERSPECTIVE for each float being interpolated between the
    // vertex and fragment shaders.  Convenience macros are available
    // for 2, 3, and 4 components, ie
    // PGL_FLAT3 expands to PGL_FLAT, PGL_FLAT, PGL_FLAT

    // the last parameter is whether the fragment shader writes to
    // gl_FragDepth or discard. When it is false, PGL may do early
    // fragment processing (scissor, depth, stencil etc) for a minor
    // performance boost but canonicaly these happen after the frag
    // shader
    GLenum interpolation[4] = { PGL_SMOOTH4 };
    GLuint myshader = pglCreateProgram(smooth_vs, smooth_fs, 4, interpolation, GL_FALSE);
    glUseProgram(myshader);

    // v_color is not actually used since we're using per vert color
    My_Uniform the_uniforms = { IDENTITY_MAT4() };
    pglSetUniform(&the_uniforms);

    // Your standard OpenGL buffer setup etc. here
    // Like the compatibility profile, we allow/enable a default
    // VAO.  We also have a default shader program for the same reason,
    // something to fill index 0.
    // see implementation of init_glContext for details

    while (1) {

        // standard glDraw calls, switching shaders etc.

        // use backbuf however you want, whether that's blitting
        // it to some framebuffer in your GUI system, or even writing
        // it out to disk with something like stb_image_write.
    }

    free_glContext(&the_context);

    // compare with equivalent glsl below
    void smooth_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
    {
        ((vec4*)vs_output)[0] = vertex_attribs[1]; //color

        builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), vertex_attribs[0]);
    }

    void smooth_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
    {
        builtins->gl_FragColor = ((vec4*)fs_input)[0];
    }

    // note smooth is the default so this is the same as smooth out vec4 vary_color
    // https://www.khronos.org/opengl/wiki/Type_Qualifier_(GLSL)#Interpolation_qualifiers
    uniform mvp_mat
    layout (location = 0) in vec4 in_vertex;
    layout (location = 1) in vec4 in_color;
    out vec4 vary_color;
    void main(void)
    {
        vary_color = in_color;
        gl_Position = mvp_mat * in_vertex;
    }

    in vec4 vary_color;
    out vec4 frag_color;
    void main(void)
    {
        frag_color = vary_color;
    }

    // You might also want to resize the framebuffer if your window is resizable
    // instead of just letting your GUI system scale the output in which case when
    // you handle a resize event you would do something like this:

    pglResizeFramebuffer(new_width, new_height);
    backbuf = (pix_t*)pglGetBackBuffer();
    glViewport(0, 0, new_width, new_height);
    // anything else you need for your particular GUI/windowing system here

    // alternatively, if your default color buffer pointer is changing (e.g.
    // switching back after a shortcut texture path), you can call:

    pglSetTexBackBuffer(tex_handle);   // shortcut: draw into a 2D texture
    pglSetBackBuffer(backbuf, width, height);

    // Prefer real FBOs for new code (see RENDER TARGETS / FBOs below). Notes on
    // these pgl helpers:
    //
    // 1. Default FB color is compile-time pix_t (ABGR32, RGB565, ...). Offscreen
    //    color attachments use texture storage (RGBA8 or float R/RG/RGBA), not
    //    pix_t. pglSetTexBackBuffer is safest with 32-bit pix_t and RGBA8 textures;
    //    with 16-bit pix_t, prefer glFramebufferTexture2D so writes use texture
    //    layout instead of packing pix_t into the texture.
    //
    // 2. Neither function changes depth/stencil. Keep texture size matching the
    //    depth/stencil buffers or resize them (e.g. pglResizeFramebuffer) so
    //    dimensions stay consistent.
    //
    // 3. pglSetBackBuffer() does not free the existing framebuffer or change
    //    ownership of the pointer you pass. If PGL owned the previous buffer and
    //    you drop your only pointer without freeing, you can leak. Holding a
    //    pointer from pglGetBackBuffer() and switching back/forth is the usual case.
    //
    // 4. pglSetTexBackBuffer() marks the texture as a render target (invert_y) and
    //    points the color draw surface at it. Ownership follows the texture.
    //
    //  See the lesson16 example for older pglSet*BackBuffer usage.

RENDER TARGETS / FBOs
=====================

    PortableGL supports render-to-texture via a practical FBO subset (plus the
    pgl shortcuts above).

    Window vs offscreen
    -------------------
    - Default framebuffer 0 color = pix_t (for present / SDL / etc.).
    - FBO color attachments = texture memory: tightly packed RGBA8 (Color) or
      float R32F / RG32F / RGBA32F (no RGB32F), created with glTexImage* or
      mapped with pglTexImage* / pglTextureImage*. Draw and sample use that layout.
    - RGB565 (or other 16-bit pix_t) as the *window* format does not make offscreen
      attachments 16-bit; composite/sample into the window as a separate step.

    Typical FBO path
    ----------------
        GLuint fbo, color_tex; // + optional depth tex or renderbuffer
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, color_tex, 0);
        // optional:
        // glFramebufferTexture2D(..., GL_DEPTH_ATTACHMENT, ..., depth_tex, 0);
        // glFramebufferRenderbuffer(..., GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);
        glCheckFramebufferStatus(GL_FRAMEBUFFER); // GL_FRAMEBUFFER_COMPLETE
        // draw...
        glBindFramebuffer(GL_FRAMEBUFFER, 0);     // present default FB

    MRT: attach COLOR_ATTACHMENT1..N, glDrawBuffers(), write gl_FragData[i] in the
    fragment shader. Single-target shaders still use gl_FragColor.

    Completeness follows desktop rules for draw/read buffers: any non-GL_NONE
    DRAW_BUFFERi or READ_BUFFER must name a color attachment that has an image,
    else GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER / _READ_BUFFER. Drawing or reading
    an incomplete FBO yields GL_INVALID_FRAMEBUFFER_OPERATION.

    Texture origin (invert_y)
    -------------------------
    Uploaded assets sample with linear indexing (y=0 = first row of data).
    Textures used as color/depth render targets are marked invert_y so fragCoord
    y=0 (bottom) matches lastrow-style writes and later texture()/texelFetch.
    Attach and pglSetTexBackBuffer / pglTextureAsRenderTarget set this for you.

    Depth / stencil / renderbuffers
    -------------------------------
    Depth: texture (GL_DEPTH_COMPONENT) or renderbuffer; sample depth textures in
    .r (float store as-is; integer pack normalized). Float depth attachments use
    float depth compares. Stencil: packed with D24S8 depth or a stencil
    renderbuffer where enabled at compile time.

    Readback
    --------
    Thin glReadBuffer / glReadPixels: GL_RGBA or GL_RED, GL_UNSIGNED_BYTE or
    GL_FLOAT, from the current read color buffer (default FB or FBO attachment).

That's basically it.  There are some other non-standard features like
pglSetInterp that lets you change the interpolation of a shader
whenever you want.  In real OpenGL you'd have to have 2 (or more) separate
but almost identical shaders to do that.


ADDITIONAL CONFIGURATION
========================

We've already mentioned several configuration macros above but here are
all of the non-framebuffer/memory related ones:

PGL_UNSAFE
    This replaces the old portablegl_unsafe.h
    It turns off all error checking and debug message/logging the same way
    NDEBUG turns off assert(). By default PGL is a GL_DEBUG_CONTEXT with
    GL_DEBUG_OUTPUT on and a default callback function printing to stdout.
    You can use Enable/Disable and DebugMessageCallback to turn it on/off
    or use your own callback function like normal. However with PGL_UNSAFE
    defined, there's nothing compiled in at all so I would only use it
    when you're pushing for every ounce of perf.

PGL_PREFIX_TYPES
    This prefixes the standard glsl types (and a couple other internal types)
    with pgl_ (ie vec2 becomes pgl_vec2)

PGL_ASSERT
PGL_MALLOC/PGL_REALLOC/PGL_FREE
PGL_MEMMOVE
    These overrride the standard functions of the same names

PGL_DONT_CONVERT_TEXTURES
    This makes passing PGL a texture with a format other than GL_RGBA an error.
    By default other types are automatically converted. You can perform the
    conversion manually using the function convert_format_to_packed_rgba().
    The included function convert_grayscale_to_rgba() is also useful,
    especially for font textures.

PGL_PREFIX_GLSL or PGL_SUFFIX_GLSL
    These replace PGL_EXCLUDE_GLSL. Since PGL depends on at least a few
    glsl functions and potentially more in the future it doesn't make
    sense to exclude GLSL entirely, especially since they're all inline so
    it really doesn't save you anything in the final executable.
    Instead, using one of these two macros you can change the handful of
    functions that are likely to cause a conflict with an external
    math library like glm (with a using declaration/directive of course).
    So smoothstep() would become either pgl_smoothstep() or smoothstepf(). So far it is less than
    10 functions that are modified but feel free to add more.

PGL_HERMITE_SMOOTHING
    Turn on hermite smoothing when doing linear interpolation of textures.
    It is not required by the spec and it does slow it down but it does
    look smoother so it's worth trying if you're curious. Note, most
    implementations do not use it.

PGL_DOUBLE_TEX_FILTER
    Use double for texture UV scaling, filter weights, and the LINEAR color
    mix.  Default is float-only (better for soft-float / no-double targets).
    Float filtering can change some channels by 1 after the truncating
    [0,1]<->[0,255] conversion — usually invisible, and was accepted when
    doubles were removed from the texture path (f66741f5).  Define this if
    you want fewer of those off-by-ones and can afford double.

PGL_BETTER_THICK_LINES
    If defined, use a more mathematically correct thick line drawing algorithm
    than the one in the official OpenGL spec.  It is about 15-17% slower but
    has the correct width. The default draws exactly width pixels in the
    minor axis, which results in only horizontal and vertical lines being
    correct. It also means the ends are not perpendicular to the line which
    looks worse the thicker the line.  The better algorithm is about what is
    specified for GL_LINE_SMOOTH/AA lines except without the actual
    anti-aliasing (ie no changes to the alpha channel).

PGL_DISABLE_COLOR_MASK
    If defined, color masking (which is set using glColorMask()) is ignored
    which provides some performance benefit though it varies depending on
    what you're doing.

PGL_EXCLUDE_STUBS
    If defined, PGL will exclude stubs for dozens of OpenGL functions that
    make porting existing OpenGL projects and reusing existing OpenGL
    helper/library code with PortableGL much easier.  This might make
    sense to define if you're starting a PGL project from scratch.

PGL_ENABLE_CLAMP_TO_BORDER
    By default it's ignored and treated the same as CLAMP_TO_EDGE because
    I can only think of two ways to implement it. The first way was to
    manually add a 1 pixel border around textures which was far more
    painful and ugly that it sounds to make work and means I can't have
    mapped texture data (ie pglTexImage2D that uses the pointer passed in)
    as well as making render-to-texture / mapped textures more complicated.
    Th second way is with a bunch of extra if statements in the texture sampling
    code which slows down all accesses regardless of if they're using a border
    or not. So it's off by default and you can turn it on with this macro.

PGL_CORE_PROFILE
    Opt into stricter Core-like behavior over time.  PGL remains
    Compatibility-friendly by default.  Currently this affects:
      - Incomplete textures: if MIN_FILTER is a *MIPMAP* mode but no mip
        chain exists (num_levels <= 1), texture1D/2D/Lod and texture_cubemap
        return black (0,0,0,1) instead of falling back to level 0.
      - GL_TEXTURE_RECTANGLE wrap modes: only GL_CLAMP_TO_EDGE and
        GL_CLAMP_TO_BORDER are accepted (GL_INVALID_ENUM otherwise).
    More Core vs Compatibility differences may be gated on this later.

There are also several predefined maximums which you can change.
However, considering the performance limitations of PortableGL, the defaults
are probably more than enough, and in fact you might want to decrease PGL_MAX_VERTICES
and GL_MAX_VERTEX_ATTRIBS to save memory, see PGL memory presets in QUICK_NOTES above.

GL_MAX_DRAW_BUFFERS / GL_MAX_COLOR_ATTACHMENTS cap MRT (glDrawBuffers + gl_FragData).
PGL_MAX_VERTICES refers to the number of output vertices of a single draw call.

#define GL_MAX_VERTEX_ATTRIBS 8
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS (4*GL_MAX_VERTEX_ATTRIBS)
#define GL_MAX_DRAW_BUFFERS 4
#define GL_MAX_COLOR_ATTACHMENTS 4

#define PGL_MAX_VERTICES 500000
#define PGL_MAX_ALIASED_WIDTH 2048.0f
#define PGL_MAX_MIPMAP_LEVELS 15
#define PGL_MAX_TEXTURE_SIZE (1 << (PGL_MAX_MIPMAP_LEVELS - 1))  // 16384
#define PGL_MAX_3D_TEXTURE_SIZE 8192
#define PGL_MAX_ARRAY_TEXTURE_LAYERS 8192

MIT License
Copyright (c) 2011-2026 Robert Winkler
Copyright (c) 1997-2022 Fabrice Bellard (clipping code from TinyGL)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.


*/

#ifdef PGL_PREFIX_TYPES
#define vec2 pgl_vec2
#define vec3 pgl_vec3
#define vec4 pgl_vec4
#define ivec2 pgl_ivec2
#define ivec3 pgl_ivec3
#define ivec4 pgl_ivec4
#define uvec2 pgl_uvec2
#define uvec3 pgl_uvec3
#define uvec4 pgl_uvec4
#define bvec2 pgl_bvec2
#define bvec3 pgl_bvec3
#define bvec4 pgl_bvec4
#define mat2 pgl_mat2
#define mat3 pgl_mat3
#define mat4 pgl_mat4
#define Color pgl_Color
#define Line pgl_Line
#define Plane pgl_Plane
#endif


// I really need to think about these
// Maybe suffixes should just be the default since I already give many glsl
// functions suffixes but then we still have the problem if I ever want
// to support doubles with no suffix like C math funcs..
//
// For now it's just functions that are used inside PortableGL itself
// as that is what will definitely break without them if these macros
// are used

// Add/remove as needed as long as you also modify
// matching undef section in close_pgl.h

#ifdef PGL_PREFIX_GLSL
#define smoothstep pgl_smoothstep
#define clamp_01 pgl_clamp_01
#define clamp_01_v4 pgl_clamp_01_v4
#define clamp pgl_clamp
#define clampi pgl_clampi

#elif defined(PGL_SUFFIX_GLSL)

#define smoothstep smoothstepf
#define clamp_01 clampf_01
#define clamp_01_v4 clampf_01_v4
#define clamp clampf
#define clampi clampi
#endif


#ifndef GL_H
#define GL_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef PGLDEF
    #ifdef PGL_STATIC
        #define PGLDEF static
    #else
        #define PGLDEF extern
    #endif
#endif

#ifndef PGL_ASSERT
#include <assert.h>
#define PGL_ASSERT(x) assert(x)
#endif

#ifndef CVEC_ASSERT
#define CVEC_ASSERT(x) PGL_ASSERT(x)
#endif

#if defined(PGL_MALLOC) && defined(PGL_FREE) && defined(PGL_REALLOC)
/* ok */
#elif !defined(PGL_MALLOC) && !defined(PGL_FREE) && !defined(PGL_REALLOC)
/* ok */
#else
#error "Must define all or none of PGL_MALLOC, PGL_FREE, and PGL_REALLOC."
#endif

#ifndef PGL_MALLOC
#define PGL_MALLOC(sz)      malloc(sz)
#define PGL_REALLOC(p, sz)  realloc(p, sz)
#define PGL_FREE(p)         free(p)
#endif

#define CVEC_MALLOC(sz) PGL_MALLOC(sz)
#define CVEC_REALLOC(p, sz) PGL_REALLOC(p, sz)
#define CVEC_FREE(p) PGL_FREE(p)

#ifndef PGL_MEMMOVE
#include <string.h>
#define PGL_MEMMOVE(dst, src, sz)   memmove(dst, src, sz)
#else
#define CVEC_MEMMOVE(dst, src, sz) PGL_MEMMOVE(dst, src, sz)
#endif

// Get rid of signed/unsigned comparison warnings when looping through vectors
#ifndef CVEC_SIZE_T
#define CVEC_SIZE_T i64
#endif




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


#if defined(PGL_AMASK) && defined(PGL_RMASK) && defined(PGL_GMASK) && defined(PGL_BMASK) && \
    defined(PGL_ASHIFT) && defined(PGL_RSHIFT) && defined(PGL_GSHIFT) && defined(PGL_BSHIFT) && \
    defined(PGL_RMAX) && defined(PGL_GMAX) && defined(PGL_BMAX) && defined(PGL_AMAX) && defined(PGL_BITDEPTH)
/* ok */
#elif !defined(PGL_AMASK) && !defined(PGL_RMASK) && !defined(PGL_GMASK) && !defined(PGL_BMASK) && \
    !defined(PGL_ASHIFT) && !defined(PGL_RSHIFT) && !defined(PGL_GSHIFT) && !defined(PGL_BSHIFT) && \
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

#ifndef CRSW_MATH_H
#define CRSW_MATH_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Unfortunately this is not supported in gcc even though
// it's in the C99+ spec.  Have to use compiler option
// -ffp-contract=off for gcc (which defaults to =fast)
// unlike clang
//
//  https://stackoverflow.com/questions/43352510/difference-in-gcc-ffp-contract-options
// MSVC does not implement #pragma STDC (C4068 unknown pragma).
#ifndef _MSC_VER
#pragma STDC FP_CONTRACT OFF
#endif

// Key off the *compiler*, not the OS: MinGW is _WIN32 + GCC and wants
// __attribute__; MSVC is _WIN32 without GCC and rejects it.
#ifndef RSW_INLINE
#if defined(__GNUC__) || defined(__clang__)
	#define RSW_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
	#define RSW_INLINE __forceinline
#else
	#define RSW_INLINE inline
#endif
#endif

#define RM_PI (3.14159265358979323846)
#define RM_2PI (2.0 * RM_PI)
#define PI_DIV_180 (0.017453292519943296)
#define INV_PI_DIV_180 (57.2957795130823229)

#define DEG_TO_RAD(x)   ((x)*PI_DIV_180)
#define RAD_TO_DEG(x)   ((x)*INV_PI_DIV_180)

/* Hour angles */
#define HR_TO_DEG(x)    ((x) * (1.0 / 15.0))
#define HR_TO_RAD(x)    DEG_TO_RAD(HR_TO_DEG(x))

#define DEG_TO_HR(x)    ((x) * 15.0)
#define RAD_TO_HR(x)    DEG_TO_HR(RAD_TO_DEG(x))

// TODO rename RM_MAX/RSW_MAX?  make proper inline functions?
#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;


typedef struct vec2
{
	float x;
	float y;
} vec2;

#define SET_V2(v, _x, _y) \
	do {\
	(v).x = _x;\
	(v).y = _y;\
	} while (0)

RSW_INLINE vec2 make_v2(float x, float y)
{
	vec2 v = { x, y };
	return v;
}

RSW_INLINE vec2 neg_v2(vec2 v)
{
	vec2 r = { -v.x, -v.y };
	return r;
}

RSW_INLINE void fprint_v2(FILE* f, vec2 v, const char* append)
{
	fprintf(f, "(%f, %f)%s", v.x, v.y, append);
}

RSW_INLINE void print_v2(vec2 v, const char* append)
{
	printf("(%f, %f)%s", v.x, v.y, append);
}

RSW_INLINE int fread_v2(FILE* f, vec2* v)
{
	int tmp = fscanf(f, " (%f, %f)", &v->x, &v->y);
	return (tmp == 2);
}

RSW_INLINE float len_v2(vec2 a)
{
	return sqrt(a.x * a.x + a.y * a.y);
}

RSW_INLINE vec2 norm_v2(vec2 a)
{
	float l = len_v2(a);
	vec2 c = { a.x/l, a.y/l };
	return c;
}

RSW_INLINE void normalize_v2(vec2* a)
{
	float l = len_v2(*a);
	a->x /= l;
	a->y /= l;
}

RSW_INLINE vec2 add_v2s(vec2 a, vec2 b)
{
	vec2 c = { a.x + b.x, a.y + b.y };
	return c;
}

RSW_INLINE vec2 sub_v2s(vec2 a, vec2 b)
{
	vec2 c = { a.x - b.x, a.y - b.y };
	return c;
}

RSW_INLINE vec2 mult_v2s(vec2 a, vec2 b)
{
	vec2 c = { a.x * b.x, a.y * b.y };
	return c;
}

RSW_INLINE vec2 div_v2s(vec2 a, vec2 b)
{
	vec2 c = { a.x / b.x, a.y / b.y };
	return c;
}

RSW_INLINE float dot_v2s(vec2 a, vec2 b)
{
	return a.x*b.x + a.y*b.y;
}

RSW_INLINE vec2 add_v2(vec2 a, float s)
{
	vec2 b = { a.x + s, a.y + s };
	return b;
}

RSW_INLINE vec2 scale_v2(vec2 a, float s)
{
	vec2 b = { a.x * s, a.y * s };
	return b;
}

RSW_INLINE int equal_v2s(vec2 a, vec2 b)
{
	return (a.x == b.x && a.y == b.y);
}

RSW_INLINE int equal_epsilon_v2s(vec2 a, vec2 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon);
}

RSW_INLINE float cross_v2s(vec2 a, vec2 b)
{
	return a.x * b.y - a.y * b.x;
}

RSW_INLINE float angle_v2s(vec2 a, vec2 b)
{
	return acos(dot_v2s(a, b) / (len_v2(a) * len_v2(b)));
}


typedef struct vec3
{
	float x;
	float y;
	float z;
} vec3;

#define SET_V3(v, _x, _y, _z) \
	do {\
	(v).x = _x;\
	(v).y = _y;\
	(v).z = _z;\
	} while (0)

RSW_INLINE vec3 make_v3(float x, float y, float z)
{
	vec3 v = { x, y, z };
	return v;
}

RSW_INLINE vec3 neg_v3(vec3 v)
{
	vec3 r = { -v.x, -v.y, -v.z };
	return r;
}

RSW_INLINE void fprint_v3(FILE* f, vec3 v, const char* append)
{
	fprintf(f, "(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

RSW_INLINE void print_v3(vec3 v, const char* append)
{
	printf("(%f, %f, %f)%s", v.x, v.y, v.z, append);
}

RSW_INLINE int fread_v3(FILE* f, vec3* v)
{
	int tmp = fscanf(f, " (%f, %f, %f)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}

RSW_INLINE float len_v3(vec3 a)
{
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

RSW_INLINE vec3 norm_v3(vec3 a)
{
	float l = len_v3(a);
	vec3 c = { a.x/l, a.y/l, a.z/l };
	return c;
}

RSW_INLINE void normalize_v3(vec3* a)
{
	float l = len_v3(*a);
	a->x /= l;
	a->y /= l;
	a->z /= l;
}

RSW_INLINE vec3 add_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x + b.x, a.y + b.y, a.z + b.z };
	return c;
}

RSW_INLINE vec3 sub_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x - b.x, a.y - b.y, a.z - b.z };
	return c;
}

RSW_INLINE vec3 mult_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x * b.x, a.y * b.y, a.z * b.z };
	return c;
}

RSW_INLINE vec3 div_v3s(vec3 a, vec3 b)
{
	vec3 c = { a.x / b.x, a.y / b.y, a.z / b.z };
	return c;
}

RSW_INLINE float dot_v3s(vec3 a, vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

RSW_INLINE vec3 add_v3(vec3 a, float s)
{
	vec3 b = { a.x + s, a.y + s, a.z + s };
	return b;
}

RSW_INLINE vec3 scale_v3(vec3 a, float s)
{
	vec3 b = { a.x * s, a.y * s, a.z * s };
	return b;
}

RSW_INLINE int equal_v3s(vec3 a, vec3 b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}

RSW_INLINE int equal_epsilon_v3s(vec3 a, vec3 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
			fabs(a.z - b.z) < epsilon);
}

RSW_INLINE vec3 cross_v3s(const vec3 u, const vec3 v)
{
	vec3 result;
	result.x = u.y*v.z - v.y*u.z;
	result.y = -u.x*v.z + v.x*u.z;
	result.z = u.x*v.y - v.x*u.y;
	return result;
}

RSW_INLINE float angle_v3s(const vec3 u, const vec3 v)
{
	return acos(dot_v3s(u, v));
}


typedef struct vec4
{
	float x;
	float y;
	float z;
	float w;
} vec4;

#define SET_V4(v, _x, _y, _z, _w) \
	do {\
	(v).x = _x;\
	(v).y = _y;\
	(v).z = _z;\
	(v).w = _w;\
	} while (0)

RSW_INLINE vec4 make_v4(float x, float y, float z, float w)
{
	vec4 v = { x, y, z, w };
	return v;
}

RSW_INLINE vec4 neg_v4(vec4 v)
{
	vec4 r = { -v.x, -v.y, -v.z, -v.w };
	return r;
}

RSW_INLINE void fprint_v4(FILE* f, vec4 v, const char* append)
{
	fprintf(f, "(%f, %f, %f, %f)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE void print_v4(vec4 v, const char* append)
{
	printf("(%f, %f, %f, %f)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE int fread_v4(FILE* f, vec4* v)
{
	int tmp = fscanf(f, " (%f, %f, %f, %f)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}

RSW_INLINE float len_v4(vec4 a)
{
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

RSW_INLINE vec4 norm_v4(vec4 a)
{
	float l = len_v4(a);
	vec4 c = { a.x/l, a.y/l, a.z/l, a.w/l };
	return c;
}

RSW_INLINE void normalize_v4(vec4* a)
{
	float l = len_v4(*a);
	a->x /= l;
	a->y /= l;
	a->z /= l;
	a->w /= l;
}

RSW_INLINE vec4 add_v4s(vec4 a, vec4 b)
{
	vec4 c = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
	return c;
}

RSW_INLINE vec4 sub_v4s(vec4 a, vec4 b)
{
	vec4 c = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
	return c;
}

RSW_INLINE vec4 mult_v4s(vec4 a, vec4 b)
{
	vec4 c = { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
	return c;
}

RSW_INLINE vec4 div_v4s(vec4 a, vec4 b)
{
	vec4 c = { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
	return c;
}

RSW_INLINE float dot_v4s(vec4 a, vec4 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

RSW_INLINE vec4 add_v4(vec4 a, float s)
{
	vec4 b = { a.x + s, a.y + s, a.z + s, a.w + s };
	return b;
}

RSW_INLINE vec4 scale_v4(vec4 a, float s)
{
	vec4 b = { a.x * s, a.y * s, a.z * s, a.w * s };
	return b;
}

RSW_INLINE int equal_v4s(vec4 a, vec4 b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
}

RSW_INLINE int equal_epsilon_v4s(vec4 a, vec4 b, float epsilon)
{
	return (fabs(a.x-b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
	        fabs(a.z - b.z) < epsilon && fabs(a.w - b.w) < epsilon);
}


typedef struct ivec2
{
	int x;
	int y;
} ivec2;

RSW_INLINE ivec2 make_iv2(int x, int y)
{
	ivec2 v = { x, y };
	return v;
}

RSW_INLINE void fprint_iv2(FILE* f, ivec2 v, const char* append)
{
	fprintf(f, "(%d, %d)%s", v.x, v.y, append);
}

RSW_INLINE int fread_iv2(FILE* f, ivec2* v)
{
	int tmp = fscanf(f, " (%d, %d)", &v->x, &v->y);
	return (tmp == 2);
}


typedef struct ivec3
{
	int x;
	int y;
	int z;
} ivec3;

RSW_INLINE ivec3 make_iv3(int x, int y, int z)
{
	ivec3 v = { x, y, z };
	return v;
}

RSW_INLINE void fprint_iv3(FILE* f, ivec3 v, const char* append)
{
	fprintf(f, "(%d, %d, %d)%s", v.x, v.y, v.z, append);
}

RSW_INLINE int fread_iv3(FILE* f, ivec3* v)
{
	int tmp = fscanf(f, " (%d, %d, %d)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}



typedef struct ivec4
{
	int x;
	int y;
	int z;
	int w;
} ivec4;

RSW_INLINE ivec4 make_iv4(int x, int y, int z, int w)
{
	ivec4 v = { x, y, z, w };
	return v;
}

RSW_INLINE void fprint_iv4(FILE* f, ivec4 v, const char* append)
{
	fprintf(f, "(%d, %d, %d, %d)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE int fread_iv4(FILE* f, ivec4* v)
{
	int tmp = fscanf(f, " (%d, %d, %d, %d)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}



typedef struct uvec2
{
	unsigned int x;
	unsigned int y;
} uvec2;

RSW_INLINE uvec2 make_uv2(unsigned int x, unsigned int y)
{
	uvec2 v = { x, y };
	return v;
}

RSW_INLINE void fprint_uv2(FILE* f, uvec2 v, const char* append)
{
	fprintf(f, "(%u, %u)%s", v.x, v.y, append);
}

RSW_INLINE int fread_uv2(FILE* f, uvec2* v)
{
	int tmp = fscanf(f, " (%u, %u)", &v->x, &v->y);
	return (tmp == 2);
}


typedef struct uvec3
{
	unsigned int x;
	unsigned int y;
	unsigned int z;
} uvec3;

RSW_INLINE uvec3 make_uv3(unsigned int x, unsigned int y, unsigned int z)
{
	uvec3 v = { x, y, z };
	return v;
}

RSW_INLINE void fprint_uv3(FILE* f, uvec3 v, const char* append)
{
	fprintf(f, "(%u, %u, %u)%s", v.x, v.y, v.z, append);
}

RSW_INLINE int fread_uv3(FILE* f, uvec3* v)
{
	int tmp = fscanf(f, " (%u, %u, %u)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}


typedef struct uvec4
{
	unsigned int x;
	unsigned int y;
	unsigned int z;
	unsigned int w;
} uvec4;

RSW_INLINE uvec4 make_uv4(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
{
	uvec4 v = { x, y, z, w };
	return v;
}

RSW_INLINE void fprint_uv4(FILE* f, uvec4 v, const char* append)
{
	fprintf(f, "(%u, %u, %u, %u)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE int fread_uv4(FILE* f, uvec4* v)
{
	int tmp = fscanf(f, " (%u, %u, %u, %u)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}


typedef struct bvec2
{
	u8 x;
	u8 y;
} bvec2;

// TODO What to do here? param type?  enforce 0 or 1?
RSW_INLINE bvec2 make_bv2(int x, int y)
{
	bvec2 v = { !!x, !!y };
	return v;
}

RSW_INLINE void fprint_bv2(FILE* f, bvec2 v, const char* append)
{
	fprintf(f, "(%u, %u)%s", v.x, v.y, append);
}

// Should technically use SCNu8 macro not hhu
RSW_INLINE int fread_bv2(FILE* f, bvec2* v)
{
	int tmp = fscanf(f, " (%hhu, %hhu)", &v->x, &v->y);
	return (tmp == 2);
}


typedef struct bvec3
{
	u8 x;
	u8 y;
	u8 z;
} bvec3;

RSW_INLINE bvec3 make_bv3(int x, int y, int z)
{
	bvec3 v = { !!x, !!y, !!z };
	return v;
}

RSW_INLINE void fprint_bv3(FILE* f, bvec3 v, const char* append)
{
	fprintf(f, "(%u, %u, %u)%s", v.x, v.y, v.z, append);
}

RSW_INLINE int fread_bv3(FILE* f, bvec3* v)
{
	int tmp = fscanf(f, " (%hhu, %hhu, %hhu)", &v->x, &v->y, &v->z);
	return (tmp == 3);
}


typedef struct bvec4
{
	u8 x;
	u8 y;
	u8 z;
	u8 w;
} bvec4;

RSW_INLINE bvec4 make_bv4(int x, int y, int z, int w)
{
	bvec4 v = { !!x, !!y, !!z, !!w };
	return v;
}

RSW_INLINE void fprint_bv4(FILE* f, bvec4 v, const char* append)
{
	fprintf(f, "(%u, %u, %u, %u)%s", v.x, v.y, v.z, v.w, append);
}

RSW_INLINE int fread_bv4(FILE* f, bvec4* v)
{
	int tmp = fscanf(f, " (%hhu, %hhu, %hhu, %hhu)", &v->x, &v->y, &v->z, &v->w);
	return (tmp == 4);
}


RSW_INLINE vec2 v3_to_v2(vec3 a)
{
	vec2 v = { a.x, a.y };
	return v;
}

RSW_INLINE vec2 v4_to_v2(vec4 a)
{
	vec2 v = { a.x, a.y };
	return v;
}

RSW_INLINE vec3 v4_to_v3(vec4 a)
{
	vec3 v = { a.x, a.y, a.z };
	return v;
}

RSW_INLINE vec2 v4_to_v2h(vec4 a)
{
	vec2 v = { a.x/a.w, a.y/a.w };
	return v;
}

RSW_INLINE vec3 v4_to_v3h(vec4 a)
{
	vec3 v = { a.x/a.w, a.y/a.w, a.z/a.w };
	return v;
}


/* matrices **************/

typedef float mat2[4];
typedef float mat3[9];
typedef float mat4[16];

#define IDENTITY_M2() { 1, 0, 0, 1 }
#define IDENTITY_M3() { 1, 0, 0, 0, 1, 0, 0, 0, 1 }
#define IDENTITY_M4() { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }
#define SET_IDENTITY_M2(m) \
	do { \
	m[1] = m[2] = 0; \
	m[0] = m[3] = 1; \
	} while (0)

#define SET_IDENTITY_M3(m) \
	do { \
	m[1] = m[2] = m[3] = 0; \
	m[5] = m[6] = m[7] = 0; \
	m[0] = m[4] = m[8] = 1; \
	} while (0)

#define SET_IDENTITY_M4(m) \
	do { \
	m[1] = m[2] = m[3] = m[4] = 0; \
	m[6] = m[7] = m[8] = m[9] = 0; \
	m[11] = m[12] = m[13] = m[14] = 0; \
	m[0] = m[5] = m[10] = m[15] = 1; \
	} while (0)

#ifndef ROW_MAJOR
RSW_INLINE vec2 x_m2(mat2 m) {  return make_v2(m[0], m[2]); }
RSW_INLINE vec2 y_m2(mat2 m) {  return make_v2(m[1], m[3]); }
RSW_INLINE vec2 c1_m2(mat2 m) { return make_v2(m[0], m[1]); }
RSW_INLINE vec2 c2_m2(mat2 m) { return make_v2(m[2], m[3]); }

RSW_INLINE void setc1_m2(mat2 m, vec2 v) { m[0]=v.x, m[1]=v.y; }
RSW_INLINE void setc2_m2(mat2 m, vec2 v) { m[2]=v.x, m[3]=v.y; }

RSW_INLINE void setx_m2(mat2 m, vec2 v) { m[0]=v.x, m[2]=v.y; }
RSW_INLINE void sety_m2(mat2 m, vec2 v) { m[1]=v.x, m[3]=v.y; }
#else
RSW_INLINE vec2 x_m2(mat2 m) {  return make_v2(m[0], m[1]); }
RSW_INLINE vec2 y_m2(mat2 m) {  return make_v2(m[2], m[3]); }
RSW_INLINE vec2 c1_m2(mat2 m) { return make_v2(m[0], m[2]); }
RSW_INLINE vec2 c2_m2(mat2 m) { return make_v2(m[1], m[3]); }

RSW_INLINE void setc1_m2(mat2 m, vec2 v) { m[0]=v.x, m[2]=v.y; }
RSW_INLINE void setc2_m2(mat2 m, vec2 v) { m[1]=v.x, m[3]=v.y; }

RSW_INLINE void setx_m2(mat2 m, vec2 v) { m[0]=v.x, m[1]=v.y; }
RSW_INLINE void sety_m2(mat2 m, vec2 v) { m[2]=v.x, m[3]=v.y; }
#endif


#ifndef ROW_MAJOR
RSW_INLINE vec3 x_m3(mat3 m) {  return make_v3(m[0], m[3], m[6]); }
RSW_INLINE vec3 y_m3(mat3 m) {  return make_v3(m[1], m[4], m[7]); }
RSW_INLINE vec3 z_m3(mat3 m) {  return make_v3(m[2], m[5], m[8]); }
RSW_INLINE vec3 c1_m3(mat3 m) { return make_v3(m[0], m[1], m[2]); }
RSW_INLINE vec3 c2_m3(mat3 m) { return make_v3(m[3], m[4], m[5]); }
RSW_INLINE vec3 c3_m3(mat3 m) { return make_v3(m[6], m[7], m[8]); }

RSW_INLINE void setc1_m3(mat3 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z; }
RSW_INLINE void setc2_m3(mat3 m, vec3 v) { m[3]=v.x, m[4]=v.y, m[5]=v.z; }
RSW_INLINE void setc3_m3(mat3 m, vec3 v) { m[6]=v.x, m[7]=v.y, m[8]=v.z; }

RSW_INLINE void setx_m3(mat3 m, vec3 v) { m[0]=v.x, m[3]=v.y, m[6]=v.z; }
RSW_INLINE void sety_m3(mat3 m, vec3 v) { m[1]=v.x, m[4]=v.y, m[7]=v.z; }
RSW_INLINE void setz_m3(mat3 m, vec3 v) { m[2]=v.x, m[5]=v.y, m[8]=v.z; }
#else
RSW_INLINE vec3 x_m3(mat3 m) {  return make_v3(m[0], m[1], m[2]); }
RSW_INLINE vec3 y_m3(mat3 m) {  return make_v3(m[3], m[4], m[5]); }
RSW_INLINE vec3 z_m3(mat3 m) {  return make_v3(m[6], m[7], m[8]); }
RSW_INLINE vec3 c1_m3(mat3 m) { return make_v3(m[0], m[3], m[6]); }
RSW_INLINE vec3 c2_m3(mat3 m) { return make_v3(m[1], m[4], m[7]); }
RSW_INLINE vec3 c3_m3(mat3 m) { return make_v3(m[2], m[5], m[8]); }

RSW_INLINE void setc1_m3(mat3 m, vec3 v) { m[0]=v.x, m[3]=v.y, m[6]=v.z; }
RSW_INLINE void setc2_m3(mat3 m, vec3 v) { m[1]=v.x, m[4]=v.y, m[7]=v.z; }
RSW_INLINE void setc3_m3(mat3 m, vec3 v) { m[2]=v.x, m[5]=v.y, m[8]=v.z; }

RSW_INLINE void setx_m3(mat3 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z; }
RSW_INLINE void sety_m3(mat3 m, vec3 v) { m[3]=v.x, m[4]=v.y, m[5]=v.z; }
RSW_INLINE void setz_m3(mat3 m, vec3 v) { m[6]=v.x, m[7]=v.y, m[8]=v.z; }
#endif


#ifndef ROW_MAJOR
RSW_INLINE vec4 c1_m4(mat4 m) { return make_v4(m[ 0], m[ 1], m[ 2], m[ 3]); }
RSW_INLINE vec4 c2_m4(mat4 m) { return make_v4(m[ 4], m[ 5], m[ 6], m[ 7]); }
RSW_INLINE vec4 c3_m4(mat4 m) { return make_v4(m[ 8], m[ 9], m[10], m[11]); }
RSW_INLINE vec4 c4_m4(mat4 m) { return make_v4(m[12], m[13], m[14], m[15]); }

RSW_INLINE vec4 x_m4(mat4 m) { return make_v4(m[0], m[4], m[8], m[12]); }
RSW_INLINE vec4 y_m4(mat4 m) { return make_v4(m[1], m[5], m[9], m[13]); }
RSW_INLINE vec4 z_m4(mat4 m) { return make_v4(m[2], m[6], m[10], m[14]); }
RSW_INLINE vec4 w_m4(mat4 m) { return make_v4(m[3], m[7], m[11], m[15]); }

//sets 4th row to 0 0 0 1
RSW_INLINE void setc1_m4v3(mat4 m, vec3 v) { m[ 0]=v.x, m[ 1]=v.y, m[ 2]=v.z, m[ 3]=0; }
RSW_INLINE void setc2_m4v3(mat4 m, vec3 v) { m[ 4]=v.x, m[ 5]=v.y, m[ 6]=v.z, m[ 7]=0; }
RSW_INLINE void setc3_m4v3(mat4 m, vec3 v) { m[ 8]=v.x, m[ 9]=v.y, m[10]=v.z, m[11]=0; }
RSW_INLINE void setc4_m4v3(mat4 m, vec3 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=1; }

RSW_INLINE void setc1_m4v4(mat4 m, vec4 v) { m[ 0]=v.x, m[ 1]=v.y, m[ 2]=v.z, m[ 3]=v.w; }
RSW_INLINE void setc2_m4v4(mat4 m, vec4 v) { m[ 4]=v.x, m[ 5]=v.y, m[ 6]=v.z, m[ 7]=v.w; }
RSW_INLINE void setc3_m4v4(mat4 m, vec4 v) { m[ 8]=v.x, m[ 9]=v.y, m[10]=v.z, m[11]=v.w; }
RSW_INLINE void setc4_m4v4(mat4 m, vec4 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=v.w; }

//sets 4th column to 0 0 0 1
RSW_INLINE void setx_m4v3(mat4 m, vec3 v) { m[0]=v.x, m[4]=v.y, m[ 8]=v.z, m[12]=0; }
RSW_INLINE void sety_m4v3(mat4 m, vec3 v) { m[1]=v.x, m[5]=v.y, m[ 9]=v.z, m[13]=0; }
RSW_INLINE void setz_m4v3(mat4 m, vec3 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=0; }
RSW_INLINE void setw_m4v3(mat4 m, vec3 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=1; }

RSW_INLINE void setx_m4v4(mat4 m, vec4 v) { m[0]=v.x, m[4]=v.y, m[ 8]=v.z, m[12]=v.w; }
RSW_INLINE void sety_m4v4(mat4 m, vec4 v) { m[1]=v.x, m[5]=v.y, m[ 9]=v.z, m[13]=v.w; }
RSW_INLINE void setz_m4v4(mat4 m, vec4 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=v.w; }
RSW_INLINE void setw_m4v4(mat4 m, vec4 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=v.w; }
#else
RSW_INLINE vec4 c1_m4(mat4 m) { return make_v4(m[0], m[4], m[8], m[12]); }
RSW_INLINE vec4 c2_m4(mat4 m) { return make_v4(m[1], m[5], m[9], m[13]); }
RSW_INLINE vec4 c3_m4(mat4 m) { return make_v4(m[2], m[6], m[10], m[14]); }
RSW_INLINE vec4 c4_m4(mat4 m) { return make_v4(m[3], m[7], m[11], m[15]); }

RSW_INLINE vec4 x_m4(mat4 m) { return make_v4(m[0], m[1], m[2], m[3]); }
RSW_INLINE vec4 y_m4(mat4 m) { return make_v4(m[4], m[5], m[6], m[7]); }
RSW_INLINE vec4 z_m4(mat4 m) { return make_v4(m[8], m[9], m[10], m[11]); }
RSW_INLINE vec4 w_m4(mat4 m) { return make_v4(m[12], m[13], m[14], m[15]); }

//sets 4th row to 0 0 0 1
RSW_INLINE void setc1_m4v3(mat4 m, vec3 v) { m[0]=v.x, m[4]=v.y, m[8]=v.z, m[12]=0; }
RSW_INLINE void setc2_m4v3(mat4 m, vec3 v) { m[1]=v.x, m[5]=v.y, m[9]=v.z, m[13]=0; }
RSW_INLINE void setc3_m4v3(mat4 m, vec3 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=0; }
RSW_INLINE void setc4_m4v3(mat4 m, vec3 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=1; }

RSW_INLINE void setc1_m4v4(mat4 m, vec4 v) { m[0]=v.x, m[4]=v.y, m[8]=v.z, m[12]=v.w; }
RSW_INLINE void setc2_m4v4(mat4 m, vec4 v) { m[1]=v.x, m[5]=v.y, m[9]=v.z, m[13]=v.w; }
RSW_INLINE void setc3_m4v4(mat4 m, vec4 v) { m[2]=v.x, m[6]=v.y, m[10]=v.z, m[14]=v.w; }
RSW_INLINE void setc4_m4v4(mat4 m, vec4 v) { m[3]=v.x, m[7]=v.y, m[11]=v.z, m[15]=v.w; }

//sets 4th column to 0 0 0 1
RSW_INLINE void setx_m4v3(mat4 m, vec3 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z, m[3]=0; }
RSW_INLINE void sety_m4v3(mat4 m, vec3 v) { m[4]=v.x, m[5]=v.y, m[6]=v.z, m[7]=0; }
RSW_INLINE void setz_m4v3(mat4 m, vec3 v) { m[8]=v.x, m[9]=v.y, m[10]=v.z, m[11]=0; }
RSW_INLINE void setw_m4v3(mat4 m, vec3 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=1; }

RSW_INLINE void setx_m4v4(mat4 m, vec4 v) { m[0]=v.x, m[1]=v.y, m[2]=v.z, m[3]=v.w; }
RSW_INLINE void sety_m4v4(mat4 m, vec4 v) { m[4]=v.x, m[5]=v.y, m[6]=v.z, m[7]=v.w; }
RSW_INLINE void setz_m4v4(mat4 m, vec4 v) { m[8]=v.x, m[9]=v.y, m[10]=v.z, m[11]=v.w; }
RSW_INLINE void setw_m4v4(mat4 m, vec4 v) { m[12]=v.x, m[13]=v.y, m[14]=v.z, m[15]=v.w; }
#endif


RSW_INLINE void fprint_m2(FILE* f, mat2 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f)\n (%f, %f)]%s",
	        m[0], m[2], m[1], m[3], append);
#else
	fprintf(f, "[(%f, %f)\n (%f, %f)]%s",
	        m[0], m[1], m[2], m[3], append);
#endif
}


RSW_INLINE void fprint_m3(FILE* f, mat3 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f, %f)\n (%f, %f, %f)\n (%f, %f, %f)]%s",
	        m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8], append);
#else
	fprintf(f, "[(%f, %f, %f)\n (%f, %f, %f)\n (%f, %f, %f)]%s",
	        m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], append);
#endif
}

RSW_INLINE void fprint_m4(FILE* f, mat4 m, const char* append)
{
#ifndef ROW_MAJOR
	fprintf(f, "[(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)]%s",
	        m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14],
	        m[3], m[7], m[11], m[15], append);
#else
	fprintf(f, "[(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)\n(%f, %f, %f, %f)]%s",
	        m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11],
	        m[12], m[13], m[14], m[15], append);
#endif
}

// macros?
RSW_INLINE void print_m2(mat2 m, const char* append)
{
	fprint_m2(stdout, m, append);
}

RSW_INLINE void print_m3(mat3 m, const char* append)
{
	fprint_m3(stdout, m, append);
}

RSW_INLINE void print_m4(mat4 m, const char* append)
{
	fprint_m4(stdout, m, append);
}

//TODO define macros for doing array version
RSW_INLINE vec2 mult_m2_v2(mat2 m, vec2 v)
{
	vec2 r;
#ifndef ROW_MAJOR
	r.x = m[0]*v.x + m[2]*v.y;
	r.y = m[1]*v.x + m[3]*v.y;
#else
	r.x = m[0]*v.x + m[1]*v.y;
	r.y = m[3]*v.x + m[3]*v.y;
#endif
	return r;
}


RSW_INLINE vec3 mult_m3_v3(mat3 m, vec3 v)
{
	vec3 r;
#ifndef ROW_MAJOR
	r.x = m[0]*v.x + m[3]*v.y + m[6]*v.z;
	r.y = m[1]*v.x + m[4]*v.y + m[7]*v.z;
	r.z = m[2]*v.x + m[5]*v.y + m[8]*v.z;
#else
	r.x = m[0]*v.x + m[1]*v.y + m[2]*v.z;
	r.y = m[3]*v.x + m[4]*v.y + m[5]*v.z;
	r.z = m[6]*v.x + m[7]*v.y + m[8]*v.z;
#endif
	return r;
}

RSW_INLINE vec4 mult_m4_v4(mat4 m, vec4 v)
{
	vec4 r;
#ifndef ROW_MAJOR
	r.x = m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12]*v.w;
	r.y = m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13]*v.w;
	r.z = m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w;
	r.w = m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w;
#else
	r.x = m[0]*v.x + m[1]*v.y + m[2]*v.z + m[3]*v.w;
	r.y = m[4]*v.x + m[5]*v.y + m[6]*v.z + m[7]*v.w;
	r.z = m[8]*v.x + m[9]*v.y + m[10]*v.z + m[11]*v.w;
	r.w = m[12]*v.x + m[13]*v.y + m[14]*v.z + m[15]*v.w;
#endif
	return r;
}

void mult_m2_m2(mat2 c, mat2 a, mat2 b);

void mult_m3_m3(mat3 c, mat3 a, mat3 b);

void mult_m4_m4(mat4 c, mat4 a, mat4 b);

RSW_INLINE void load_rotation_m2(mat2 mat, float angle)
{
#ifndef ROW_MAJOR
	mat[0] = cos(angle);
	mat[2] = -sin(angle);

	mat[1] = sin(angle);
	mat[3] = cos(angle);
#else
	mat[0] = cos(angle);
	mat[1] = -sin(angle);

	mat[2] = sin(angle);
	mat[3] = cos(angle);
#endif
}

void load_rotation_m3(mat3 mat, vec3 v, float angle);

void load_rotation_m4(mat4 mat, vec3 vec, float angle);

//void invert_m4(mat4 mInverse, const mat4 m);

void make_perspective_m4(mat4 mat, float fFov, float aspect, float near, float far);
void make_pers_m4(mat4 mat, float z_near, float z_far);

void make_perspective_proj_m4(mat4 mat, float left, float right, float bottom, float top, float near, float far);

void make_orthographic_m4(mat4 mat, float left, float right, float bottom, float top, float near, float far);

void make_viewport_m4(mat4 mat, int x, int y, unsigned int width, unsigned int height, int opengl);

void lookAt(mat4 mat, vec3 eye, vec3 center, vec3 up);


///////////Matrix transformation functions
RSW_INLINE void scale_m3(mat3 m, float x, float y, float z)
{
#ifndef ROW_MAJOR
	m[0] = x; m[3] = 0; m[6] = 0;
	m[1] = 0; m[4] = y; m[7] = 0;
	m[2] = 0; m[5] = 0; m[8] = z;
#else
	m[0] = x; m[1] = 0; m[2] = 0;
	m[3] = 0; m[4] = y; m[5] = 0;
	m[6] = 0; m[7] = 0; m[8] = z;
#endif
}

RSW_INLINE void scale_m4(mat4 m, float x, float y, float z)
{
#ifndef ROW_MAJOR
	m[ 0] = x; m[ 4] = 0; m[ 8] = 0; m[12] = 0;
	m[ 1] = 0; m[ 5] = y; m[ 9] = 0; m[13] = 0;
	m[ 2] = 0; m[ 6] = 0; m[10] = z; m[14] = 0;
	m[ 3] = 0; m[ 7] = 0; m[11] = 0; m[15] = 1;
#else
	m[ 0] = x; m[ 1] = 0; m[ 2] = 0; m[ 3] = 0;
	m[ 4] = 0; m[ 5] = y; m[ 6] = 0; m[ 7] = 0;
	m[ 8] = 0; m[ 9] = 0; m[10] = z; m[11] = 0;
	m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
#endif
}

// Create a Translation matrix. Only 4x4 matrices have translation components
RSW_INLINE void translation_m4(mat4 m, float x, float y, float z)
{
#ifndef ROW_MAJOR
	m[ 0] = 1; m[ 4] = 0; m[ 8] = 0; m[12] = x;
	m[ 1] = 0; m[ 5] = 1; m[ 9] = 0; m[13] = y;
	m[ 2] = 0; m[ 6] = 0; m[10] = 1; m[14] = z;
	m[ 3] = 0; m[ 7] = 0; m[11] = 0; m[15] = 1;
#else
	m[ 0] = 1; m[ 1] = 0; m[ 2] = 0; m[ 3] = x;
	m[ 4] = 0; m[ 5] = 1; m[ 6] = 0; m[ 7] = y;
	m[ 8] = 0; m[ 9] = 0; m[10] = 1; m[11] = z;
	m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
#endif
}


// Extract a rotation matrix from a 4x4 matrix
// Extracts the rotation matrix (3x3) from a 4x4 matrix
//
#ifndef ROW_MAJOR
#define M44(m, row, col) m[col*4 + row]
#define M33(m, row, col) m[col*3 + row]
#else
#define M44(m, row, col) m[row*4 + col]
#define M33(m, row, col) m[row*3 + col]
#endif
RSW_INLINE void extract_rotation_m4(mat3 dst, mat4 src, int normalize)
{
	vec3 tmp;
	if (normalize) {
		tmp.x = M44(src, 0, 0);
		tmp.y = M44(src, 1, 0);
		tmp.z = M44(src, 2, 0);
		normalize_v3(&tmp);

		M33(dst, 0, 0) = tmp.x;
		M33(dst, 1, 0) = tmp.y;
		M33(dst, 2, 0) = tmp.z;

		tmp.x = M44(src, 0, 1);
		tmp.y = M44(src, 1, 1);
		tmp.z = M44(src, 2, 1);
		normalize_v3(&tmp);

		M33(dst, 0, 1) = tmp.x;
		M33(dst, 1, 1) = tmp.y;
		M33(dst, 2, 1) = tmp.z;

		tmp.x = M44(src, 0, 2);
		tmp.y = M44(src, 1, 2);
		tmp.z = M44(src, 2, 2);
		normalize_v3(&tmp);

		M33(dst, 0, 2) = tmp.x;
		M33(dst, 1, 2) = tmp.y;
		M33(dst, 2, 2) = tmp.z;
	} else {
		M33(dst, 0, 0) = M44(src, 0, 0);
		M33(dst, 1, 0) = M44(src, 1, 0);
		M33(dst, 2, 0) = M44(src, 2, 0);

		M33(dst, 0, 1) = M44(src, 0, 1);
		M33(dst, 1, 1) = M44(src, 1, 1);
		M33(dst, 2, 1) = M44(src, 2, 1);

		M33(dst, 0, 2) = M44(src, 0, 2);
		M33(dst, 1, 2) = M44(src, 1, 2);
		M33(dst, 2, 2) = M44(src, 2, 2);
	}
}
#undef M33
#undef M44


// returns float [0,1)
RSW_INLINE float rsw_randf(void)
{
	return rand() / ((float)RAND_MAX + 1.0f);
}

RSW_INLINE float rsw_randf_range(float min, float max)
{
	return min + (max-min) * rsw_randf();
}

RSW_INLINE double rsw_map(double x, double a, double b, double c, double d)
{
	return (x-a)/(b-a) * (d-c) + c;
}

RSW_INLINE float rsw_mapf(float x, float a, float b, float c, float d)
{
	return (x-a)/(b-a) * (d-c) + c;
}


typedef struct Color
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
} Color;

/*
Color make_Color(void)
{
	r = g = b = 0;
	a = 255;
}
*/

RSW_INLINE Color make_Color(u8 red, u8 green, u8 blue, u8 alpha)
{
	Color c = { red, green, blue, alpha };
	return c;
}

RSW_INLINE void print_Color(Color c, const char* append)
{
	printf("(%d, %d, %d, %d)%s", c.r, c.g, c.b, c.a, append);
}

RSW_INLINE Color v4_to_Color(vec4 v)
{
	//assume all in the range of [0, 1]
	//NOTE(rswinkle): There are other ways of doing the conversion:
	//
	// round like HH: (u8)(v.x * 255.0f + 0.5f)
	// so 0 and 255 get half sized buckets, the rest get [(n-1).5, n.5)
	//
	// allocate equal sized buckets: (u8)(v.x * 256.0f - EPSILON) (where epsilon is eg 0.000001f)
	//
	// But as far as I can tell the spec does it this way
	Color c;
	c.r = v.x * 255.0f;
	c.g = v.y * 255.0f;
	c.b = v.z * 255.0f;
	c.a = v.w * 255.0f;
	return c;
}

RSW_INLINE vec4 Color_to_v4(Color c)
{
	vec4 v = { (float)c.r/255.0f, (float)c.g/255.0f, (float)c.b/255.0f, (float)c.a/255.0f };
	return v;
}

typedef struct Line
{
	float A, B, C;
} Line;

RSW_INLINE Line make_Line(float x1, float y1, float x2, float y2)
{
	Line l;
	l.A = y1 - y2;
	l.B = x2 - x1;
	l.C = x1*y2 - x2*y1;
	return l;
}

RSW_INLINE void normalize_line(Line* line)
{
	// TODO could enforce that n always points toward +y or +x...should I?
	vec2 n = { line->A, line->B };
	float len = len_v2(n);
	line->A /= len;
	line->B /= len;
	line->C /= len;
}

RSW_INLINE float line_func(Line* line, float x, float y)
{
	return line->A*x + line->B*y + line->C;
}
RSW_INLINE float line_findy(Line* line, float x)
{
	return -(line->A*x + line->C)/line->B;
}

RSW_INLINE float line_findx(Line* line, float y)
{
	return -(line->B*y + line->C)/line->A;
}

// return squared distance from c to line segment between a and b
RSW_INLINE float sq_dist_pt_segment2d(vec2 a, vec2 b, vec2 c)
{
	vec2 ab = sub_v2s(b, a);
	vec2 ac = sub_v2s(c, a);
	vec2 bc = sub_v2s(c, b);
	float e = dot_v2s(ac, ab);

	// cases where c projects outside ab
	if (e <= 0.0f) return dot_v2s(ac, ac);
	float f = dot_v2s(ab, ab);
	if (e >= f) return dot_v2s(bc, bc);

	// handle cases where c projects onto ab
	return dot_v2s(ac, ac) - e * e / f;
}

// return t and closest pt on segment ab to c
RSW_INLINE void closest_pt_pt_segment(vec2 c, vec2 a, vec2 b, float* t, vec2* d)
{
	vec2 ab = sub_v2s(b, a);

	// project c onto ab, compute t
	float t_ = dot_v2s(sub_v2s(c, a), ab) / dot_v2s(ab, ab);

	// clamp if outside segment
	if (t_ < 0.0f) t_ = 0.0f;
	if (t_ > 1.0f) t_ = 1.0f;

	// compute projected position
	*d = add_v2s(a, scale_v2(ab, t_));
	*t = t_;
}

RSW_INLINE float closest_pt_pt_segment_t(vec2 c, vec2 a, vec2 b)
{
	vec2 ab = sub_v2s(b, a);

	// project c onto ab, compute t
	float t = dot_v2s(sub_v2s(c, a), ab) / dot_v2s(ab, ab);
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	return t;
}

typedef struct Plane
{
	vec3 n;	//normal points x on plane satisfy n dot x = d
	float d; //d = n dot p

} Plane;

/*
Plane(void) {}
Plane(vec3 a, vec3 b, vec3 c)	//ccw winding
{
	n = cross_product(b-a, c-a).norm();
	d = n * a;
}
*/

//int intersect_segment_plane(vec3 a, vec3 b, Plane p, float* t, vec3* q);


// TODO hmm would have to change mat3 and mat4 to proper
// structures to have operators return them since our
// current mat*mat functions take the output mat as a parameter


// For some reason g++ chokes on these operator overloads but they work just
// fine with clang++.  Commented till I figure out what's going on.
/*
#ifdef __cplusplus
RSW_INLINE vec2 operator*(vec2 v, float a) { return scale_v2(v, a); }
RSW_INLINE vec2 operator*(float a, vec2 v) { return scale_v2(v, a); }
RSW_INLINE vec3 operator*(vec3 v, float a) { return scale_v3(v, a); }
RSW_INLINE vec3 operator*(float a, vec3 v) { return scale_v3(v, a); }
RSW_INLINE vec4 operator*(vec4 v, float a) { return scale_v4(v, a); }
RSW_INLINE vec4 operator*(float a, vec4 v) { return scale_v4(v, a); }

RSW_INLINE vec2 operator+(vec2 v1, vec2 v2) { return add_v2s(v1, v2); }
RSW_INLINE vec3 operator+(vec3 v1, vec3 v2) { return add_v3s(v1, v2); }
RSW_INLINE vec4 operator+(vec4 v1, vec4 v2) { return add_v4s(v1, v2); }

RSW_INLINE vec2 operator-(vec2 v1, vec2 v2) { return sub_v2s(v1, v2); }
RSW_INLINE vec3 operator-(vec3 v1, vec3 v2) { return sub_v3s(v1, v2); }
RSW_INLINE vec4 operator-(vec4 v1, vec4 v2) { return sub_v4s(v1, v2); }

RSW_INLINE int operator==(vec2 v1, vec2 v2) { return equal_v2s(v1, v2); }
RSW_INLINE int operator==(vec3 v1, vec3 v2) { return equal_v3s(v1, v2); }
RSW_INLINE int operator==(vec4 v1, vec4 v2) { return equal_v4s(v1, v2); }

RSW_INLINE vec2 operator-(vec2 v) { return neg_v2(v); }
RSW_INLINE vec3 operator-(vec3 v) { return neg_v3(v); }
RSW_INLINE vec4 operator-(vec4 v) { return neg_v4(v); }

RSW_INLINE vec2 operator*(mat2 m, vec2 v) { return mult_m2_v2(m, v); }
RSW_INLINE vec3 operator*(mat3 m, vec3 v) { return mult_m3_v3(m, v); }
RSW_INLINE vec4 operator*(mat4 m, vec4 v) { return mult_m4_v4(m, v); }

#include <iostream>
static inline std::ostream& operator<<(std::ostream& stream, const vec2& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<")";
}
static inline std::ostream& operator<<(std::ostream& stream, const vec3& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<")";
}

static inline std::ostream& operator<<(std::ostream& stream, const vec4& a)
{
	return stream <<"("<<a.x<<", "<<a.y<<", "<<a.z<<", "<<a.w<<")";
}

#endif
*/


// Built-in GLSL functions from Chapter 8 of the GLSLangSpec.3.30.pdf
// Some functionality is included elsewhere in crsw_math (especially
// the geometric functions) and texture lookup functions are in
// gl_glsl.c but this is for the rest of them.  May be moved eventually

// For functions that take 1 float input
#define PGL_VECTORIZE_VEC2(func) \
RSW_INLINE vec2 func##_v2(vec2 v) \
{ \
	return make_v2(func(v.x), func(v.y)); \
}
#define PGL_VECTORIZE_VEC3(func) \
RSW_INLINE vec3 func##_v3(vec3 v) \
{ \
	return make_v3(func(v.x), func(v.y), func(v.z)); \
}
#define PGL_VECTORIZE_VEC4(func) \
RSW_INLINE vec4 func##_v4(vec4 v) \
{ \
	return make_v4(func(v.x), func(v.y), func(v.z), func(v.w)); \
}

#define PGL_VECTORIZE_VEC(func) \
	PGL_VECTORIZE_VEC2(func) \
	PGL_VECTORIZE_VEC3(func) \
	PGL_VECTORIZE_VEC4(func)

#define PGL_STATIC_VECTORIZE_VEC(func) \
static PGL_VECTORIZE_VEC2(func) \
static PGL_VECTORIZE_VEC3(func) \
static PGL_VECTORIZE_VEC4(func)

// for functions that take 2 float inputs and return a float
#define PGL_VECTORIZE2_VEC2(func) \
RSW_INLINE vec2 func##_v2(vec2 a, vec2 b) \
{ \
	return make_v2(func(a.x, b.x), func(a.y, b.y)); \
}
#define PGL_VECTORIZE2_VEC3(func) \
RSW_INLINE vec3 func##_v3(vec3 a, vec3 b) \
{ \
	return make_v3(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z)); \
}
#define PGL_VECTORIZE2_VEC4(func) \
RSW_INLINE vec4 func##_v4(vec4 a, vec4 b) \
{ \
	return make_v4(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z), func(a.w, b.w)); \
}

#define PGL_VECTORIZE2_VEC(func) \
	PGL_VECTORIZE2_VEC2(func) \
	PGL_VECTORIZE2_VEC3(func) \
	PGL_VECTORIZE2_VEC4(func)

#define PGL_STATIC_VECTORIZE2_VEC(func) \
static PGL_VECTORIZE2_VEC2(func) \
static PGL_VECTORIZE2_VEC3(func) \
static PGL_VECTORIZE2_VEC4(func)

// For functions that take 2 float inputs and 1 float control
//  and return a float like mix
#define PGL_VECTORIZE2_1_VEC2(func) \
RSW_INLINE vec2 func##_v2(vec2 a, vec2 b, float c) \
{ \
	return make_v2(func(a.x, b.x, c), func(a.y, b.y, c)); \
}
#define PGL_VECTORIZE2_1_VEC3(func) \
RSW_INLINE vec3 func##_v3(vec3 a, vec3 b, float c) \
{ \
	return make_v3(func(a.x, b.x, c), func(a.y, b.y, c), func(a.z, b.z, c)); \
}
#define PGL_VECTORIZE2_1_VEC4(func) \
RSW_INLINE vec4 func##_v4(vec4 a, vec4 b, float c) \
{ \
	return make_v4(func(a.x, b.x, c), func(a.y, b.y, c), func(a.z, b.z, c), func(a.w, b.w, c)); \
}

#define PGL_VECTORIZE2_1_VEC(func) \
	PGL_VECTORIZE2_1_VEC2(func) \
	PGL_VECTORIZE2_1_VEC3(func) \
	PGL_VECTORIZE2_1_VEC4(func)

#define PGL_STATIC_VECTORIZE2_1_VEC(func) \
static PGL_VECTORIZE2_1_VEC2(func) \
static PGL_VECTORIZE2_1_VEC3(func) \
static PGL_VECTORIZE2_1_VEC4(func)

// for functions that take 1 input and 2 control floats
// and return a float like clamp
#define PGL_VECTORIZE_2_VEC2(func) \
RSW_INLINE vec2 func##_v2(vec2 v, float a, float b) \
{ \
	return make_v2(func(v.x, a, b), func(v.y, a, b)); \
}
#define PGL_VECTORIZE_2_VEC3(func) \
RSW_INLINE vec3 func##_v3(vec3 v, float a, float b) \
{ \
	return make_v3(func(v.x, a, b), func(v.y, a, b), func(v.z, a, b)); \
}
#define PGL_VECTORIZE_2_VEC4(func) \
RSW_INLINE vec4 func##_v4(vec4 v, float a, float b) \
{ \
	return make_v4(func(v.x, a, b), func(v.y, a, b), func(v.z, a, b), func(v.w, a, b)); \
}

#define PGL_VECTORIZE_2_VEC(func) \
	PGL_VECTORIZE_2_VEC2(func) \
	PGL_VECTORIZE_2_VEC3(func) \
	PGL_VECTORIZE_2_VEC4(func)

#define PGL_STATIC_VECTORIZE_2_VEC(func) \
static PGL_VECTORIZE_2_VEC2(func) \
static PGL_VECTORIZE_2_VEC3(func) \
static PGL_VECTORIZE_2_VEC4(func)

// hmm name VECTORIZEI_IVEC2?  suffix is return type?
#define PGL_VECTORIZE_IVEC2(func) \
RSW_INLINE ivec2 func##_iv2(ivec2 v) \
{ \
	return make_iv2(func(v.x), func(v.y)); \
}
#define PGL_VECTORIZE_IVEC3(func) \
RSW_INLINE ivec3 func##_iv3(ivec3 v) \
{ \
	return make_iv3(func(v.x), func(v.y), func(v.z)); \
}
#define PGL_VECTORIZE_IVEC4(func) \
RSW_INLINE ivec4 func##_iv4(ivec4 v) \
{ \
	return make_iv4(func(v.x), func(v.y), func(v.z), func(v.w)); \
}

#define PGL_VECTORIZE_IVEC(func) \
	PGL_VECTORIZE_IVEC2(func) \
	PGL_VECTORIZE_IVEC3(func) \
	PGL_VECTORIZE_IVEC4(func)

#define PGL_VECTORIZE_BVEC2(func) \
RSW_INLINE bvec2 func##_bv2(bvec2 v) \
{ \
	return make_bv2(func(v.x), func(v.y)); \
}
#define PGL_VECTORIZE_BVEC3(func) \
RSW_INLINE bvec3 func##_bv3(bvec3 v) \
{ \
	return make_bv3(func(v.x), func(v.y), func(v.z)); \
}
#define PGL_VECTORIZE_BVEC4(func) \
RSW_INLINE bvec4 func##_bv4(bvec4 v) \
{ \
	return make_bv4(func(v.x), func(v.y), func(v.z), func(v.w)); \
}

#define PGL_VECTORIZE_BVEC(func) \
	PGL_VECTORIZE_BVEC2(func) \
	PGL_VECTORIZE_BVEC3(func) \
	PGL_VECTORIZE_BVEC4(func)

#define PGL_STATIC_VECTORIZE_BVEC(func) \
static PGL_VECTORIZE_BVEC2(func) \
static PGL_VECTORIZE_BVEC3(func) \
static PGL_VECTORIZE_BVEC4(func)

// for functions that take 2 float inputs and return a bool
#define PGL_VECTORIZE2_BVEC2(func) \
RSW_INLINE bvec2 func##_v2(vec2 a, vec2 b) \
{ \
	return make_bv2(func(a.x, b.x), func(a.y, b.y)); \
}
#define PGL_VECTORIZE2_BVEC3(func) \
RSW_INLINE bvec3 func##_v3(vec3 a, vec3 b) \
{ \
	return make_bv3(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z)); \
}
#define PGL_VECTORIZE2_BVEC4(func) \
RSW_INLINE bvec4 func##_v4(vec4 a, vec4 b) \
{ \
	return make_bv4(func(a.x, b.x), func(a.y, b.y), func(a.z, b.z), func(a.w, b.w)); \
}

#define PGL_VECTORIZE2_BVEC(func) \
	PGL_VECTORIZE2_BVEC2(func) \
	PGL_VECTORIZE2_BVEC3(func) \
	PGL_VECTORIZE2_BVEC4(func)

#define PGL_STATIC_VECTORIZE2_BVEC(func) \
static PGL_VECTORIZE2_BVEC2(func) \
static PGL_VECTORIZE2_BVEC3(func) \
static PGL_VECTORIZE2_BVEC4(func)



// 8.1 Angle and Trig Functions
static inline float radiansf(float degrees) { return DEG_TO_RAD(degrees); }
static inline float degreesf(float radians) { return RAD_TO_DEG(radians); }

static inline double radians(double degrees) { return DEG_TO_RAD(degrees); }
static inline double degrees(double radians) { return RAD_TO_DEG(radians); }

PGL_STATIC_VECTORIZE_VEC(radiansf)
PGL_STATIC_VECTORIZE_VEC(degreesf)
PGL_VECTORIZE_VEC(sinf)
PGL_VECTORIZE_VEC(cosf)
PGL_VECTORIZE_VEC(tanf)
PGL_VECTORIZE_VEC(asinf)
PGL_VECTORIZE_VEC(acosf)
PGL_VECTORIZE_VEC(atanf)
PGL_VECTORIZE2_VEC(atan2f)
PGL_VECTORIZE_VEC(sinhf)
PGL_VECTORIZE_VEC(coshf)
PGL_VECTORIZE_VEC(tanhf)
PGL_VECTORIZE_VEC(asinhf)
PGL_VECTORIZE_VEC(acoshf)
PGL_VECTORIZE_VEC(atanhf)

// 8.2 Exponential Functions

static inline float inversesqrtf(float x)
{
	return 1/sqrtf(x);
}

PGL_VECTORIZE2_VEC(powf)
PGL_VECTORIZE_VEC(expf)
PGL_VECTORIZE_VEC(exp2f)
PGL_VECTORIZE_VEC(logf)
PGL_VECTORIZE_VEC(log2f)
PGL_VECTORIZE_VEC(sqrtf)
PGL_STATIC_VECTORIZE_VEC(inversesqrtf)

// 8.3 Common Functions
//
static inline float signf(float x)
{
	if (x > 0.0f) return 1.0f;
	if (x < 0.0f) return -1.0f;
	return 0.0f;
}

static inline float fractf(float x) { return x - floorf(x); }

// GLSL mod() function, can't do modf for float because
// modf is a different standard C function for doubles
// TODO final name?
static inline float modulusf(float x, float y)
{
	return x - y * floorf(x/y);
}

static inline float minf(float x, float y)
{
	return (x < y) ? x : y;
}
static inline float maxf(float x, float y)
{
	return (x > y) ? x : y;
}

static inline float clamp_01(float f)
{
	if (f < 0.0f) return 0.0f;
	if (f > 1.0f) return 1.0f;
	return f;
}

static inline float clamp(float x, float minVal, float maxVal)
{
	if (x < minVal) return minVal;
	if (x > maxVal) return maxVal;
	return x;
}

static inline int clampi(int i, int min, int max)
{
	if (i < min) return min;
	if (i > max) return max;
	return i;
}

static inline float mixf(float x, float y, float a)
{
	return x*(1-a) + y*a;
}

PGL_VECTORIZE_IVEC(abs)
PGL_VECTORIZE_VEC(fabsf)
PGL_STATIC_VECTORIZE_VEC(signf)
PGL_VECTORIZE_VEC(floorf)
PGL_VECTORIZE_VEC(truncf)
PGL_VECTORIZE_VEC(roundf)

// assumes current rounding direction (fegetround/fesetround)
// is nearest in which case nearbyintf rounds to nearest even
#define roundEvenf nearbyintf
PGL_VECTORIZE_VEC(nearbyintf)

PGL_VECTORIZE_VEC(ceilf)
PGL_STATIC_VECTORIZE_VEC(fractf)

PGL_STATIC_VECTORIZE2_VEC(modulusf)
PGL_STATIC_VECTORIZE2_VEC(minf)
PGL_STATIC_VECTORIZE2_VEC(maxf)

PGL_STATIC_VECTORIZE_VEC(clamp_01)
PGL_STATIC_VECTORIZE_2_VEC(clamp)
PGL_STATIC_VECTORIZE2_1_VEC(mixf)

PGL_VECTORIZE_VEC(isnan)
PGL_VECTORIZE_VEC(isinf)


// 8.4 Geometric Functions
// Most of these are elsewhere in the the file
// TODO Where should these go?

static inline float distance_v2(vec2 a, vec2 b)
{
	return len_v2(sub_v2s(a, b));
}
static inline float distance_v3(vec3 a, vec3 b)
{
	return len_v3(sub_v3s(a, b));
}

static inline vec3 reflect_v3(vec3 i, vec3 n)
{
	return sub_v3s(i, scale_v3(n, 2 * dot_v3s(i, n)));
}

static inline float smoothstep(float edge0, float edge1, float x)
{
	float t = clamp_01((x-edge0)/(edge1-edge0));
	return t*t*(3 - 2*t);
}

// 8.5 Matrix Functions
// Again the ones that exist are currently elsewhere

// 8.6 Vector Relational functions

static inline u8 lessThan(float x, float y) { return x < y; }
static inline u8 lessThanEqual(float x, float y) { return x <= y; }
static inline u8 greaterThan(float x, float y) { return x > y; }
static inline u8 greaterThanEqual(float x, float y) { return x >= y; }
static inline u8 equal(float x, float y) { return x == y; }
static inline u8 notEqual(float x, float y) { return x != y; }

//TODO any, all, not

PGL_STATIC_VECTORIZE2_BVEC(lessThan)
PGL_STATIC_VECTORIZE2_BVEC(lessThanEqual)
PGL_STATIC_VECTORIZE2_BVEC(greaterThan)
PGL_STATIC_VECTORIZE2_BVEC(greaterThanEqual)
PGL_STATIC_VECTORIZE2_BVEC(equal)
PGL_STATIC_VECTORIZE2_BVEC(notEqual)

// 8.7 Texture Lookup Functions
// currently in gl_glsl.h/c

#endif



#include <stdint.h>

// References
// https://www.khronos.org/opengl/wiki/OpenGL_Type
// https://registry.khronos.org/EGL/api/KHR/khrplatform.h
// https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/xml/gl.xml
//
// NOTES:
// Non-negative is not the same as unsigned
// They use plain int for GLsizei not unsigned like you'd think hence all
// the GL_INVALID_VALUE errors when a GLsizei param is < 0
// Similarly, according to these links, GLsizeiptr is signed
//
// Also, there are some minor/rare contradictions in the links above. They use
// plain int for GLint and GLsizei and unsigned int for GLbitfield but the first
// link above insists all 3 must be 32-bits while the C standard only guarantees
// an int (signed or unsigned) is *at least* 16-bits.  Obviously 16 bit
// architectures are rare and it's probably impossible to run OpenGL on one for
// other reasons, but still, why not use an int32_t/khronos_int32_t in the
// official registry?

typedef uint8_t   GLboolean;
typedef char      GLchar;
typedef int8_t    GLbyte;
typedef uint8_t   GLubyte;
typedef int16_t   GLshort;
typedef uint16_t  GLushort;
typedef int32_t   GLint;
typedef uint32_t  GLuint;
typedef int64_t   GLint64;
typedef uint64_t  GLuint64;

typedef int32_t   GLsizei;

typedef uint32_t  GLenum;
typedef uint32_t  GLbitfield;

typedef intptr_t  GLintptr;
typedef intptr_t  GLsizeiptr;
typedef void      GLvoid;

typedef float     GLfloat;
typedef float     GLclampf;

// not used
typedef double    GLdouble;
typedef double    GLclampd;

#define PGL_UNUSED(var) (void)(var)

enum
{
	//gl error codes
	GL_NO_ERROR = 0,
	GL_INVALID_ENUM,
	GL_INVALID_VALUE,
	GL_INVALID_OPERATION,
	GL_INVALID_FRAMEBUFFER_OPERATION,
	GL_OUT_OF_MEMORY,

	//buffer types (only ARRAY_BUFFER and ELEMENT_ARRAY_BUFFER are currently used)
	GL_ARRAY_BUFFER,
	GL_COPY_READ_BUFFER,
	GL_COPY_WRITE_BUFFER,
	GL_ELEMENT_ARRAY_BUFFER,
	GL_PIXEL_PACK_BUFFER,
	GL_PIXEL_UNPACK_BUFFER,
	GL_TEXTURE_BUFFER,
	GL_TRANSFORM_FEEDBACK_BUFFER,
	GL_UNIFORM_BUFFER,
	GL_NUM_BUFFER_TYPES,

	// Framebuffer stuff (unused/supported yet)
	GL_FRAMEBUFFER,
	GL_DRAW_FRAMEBUFFER,
	GL_READ_FRAMEBUFFER,

	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2,
	GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4,
	GL_COLOR_ATTACHMENT5,
	GL_COLOR_ATTACHMENT6,
	GL_COLOR_ATTACHMENT7,

	GL_DEPTH_ATTACHMENT,
	GL_STENCIL_ATTACHMENT,
	GL_DEPTH_STENCIL_ATTACHMENT,

	// Framebuffer completeness
	GL_FRAMEBUFFER_COMPLETE,
	GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,
	GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,
	GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS,
	GL_FRAMEBUFFER_UNSUPPORTED,
	// Desktop: non-GL_NONE DRAW_BUFFERi / READ_BUFFER must name an attached image
	GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER,
	GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER,

	GL_NONE,

	GL_RENDERBUFFER,

	//buffer use hints (not used currently)
	GL_STREAM_DRAW,
	GL_STREAM_READ,
	GL_STREAM_COPY,
	GL_STATIC_DRAW,
	GL_STATIC_READ,
	GL_STATIC_COPY,
	GL_DYNAMIC_DRAW,
	GL_DYNAMIC_READ,
	GL_DYNAMIC_COPY,

	// mapped buffer access
	GL_READ_ONLY,
	GL_WRITE_ONLY,
	GL_READ_WRITE,

	//polygon modes
	GL_POINT,
	GL_LINE,
	GL_FILL,

	//primitive types
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,

	// unsupported primitives because I don't support the geometry shader
	GL_LINE_STRIP_ADJACENCY,
	GL_LINES_ADJACENCY,
	GL_TRIANGLES_ADJACENCY,
	GL_TRIANGLE_STRIP_ADJACENCY,

	//depth functions (and stencil funcs)
	GL_LESS,
	GL_LEQUAL,
	GL_GREATER,
	GL_GEQUAL,
	GL_EQUAL,
	GL_NOTEQUAL,
	GL_ALWAYS,
	GL_NEVER,

	//blend functions
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_CONSTANT_COLOR,
	GL_ONE_MINUS_CONSTANT_COLOR,
	GL_CONSTANT_ALPHA,
	GL_ONE_MINUS_CONSTANT_ALPHA,
	GL_SRC_ALPHA_SATURATE,
	NUM_BLEND_FUNCS,
	GL_SRC1_COLOR,
	GL_ONE_MINUS_SRC1_COLOR,
	GL_SRC1_ALPHA,
	GL_ONE_MINUS_SRC1_ALPHA,
	//NUM_BLEND_FUNCS

	//blend equations
	GL_FUNC_ADD,
	GL_FUNC_SUBTRACT,
	GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN,
	GL_MAX,
	NUM_BLEND_EQUATIONS,

	//texture types
	GL_TEXTURE_UNBOUND,
	GL_TEXTURE_1D,
	GL_TEXTURE_2D,
	GL_TEXTURE_3D,
	GL_TEXTURE_1D_ARRAY,
	GL_TEXTURE_2D_ARRAY,
	GL_TEXTURE_RECTANGLE,
	GL_TEXTURE_CUBE_MAP,

	// not needed (just use uniforms (or globals), that's the beauty of
	// software rendering, everything is normal/unified RAM. Also the fact
	// that this is used for both textures and buffers breaks my convenient
	// enum -> bound array index scheme so it would be a pain anyway
	//GL_TEXTURE_BUFFER,

	GL_NUM_TEXTURE_TYPES,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,

	//texture parameters i
	GL_TEXTURE_BASE_LEVEL,
	GL_TEXTURE_BORDER_COLOR, // doesn't actually do anything
	GL_TEXTURE_COMPARE_FUNC,
	GL_TEXTURE_COMPARE_MODE,
	GL_TEXTURE_LOD_BIAS,
	GL_TEXTURE_MIN_FILTER,
	GL_TEXTURE_MAG_FILTER,
	GL_TEXTURE_MIN_LOD,
	GL_TEXTURE_MAX_LOD,
	GL_TEXTURE_MAX_LEVEL,
	GL_TEXTURE_SWIZZLE_R,
	GL_TEXTURE_SWIZZLE_G,
	GL_TEXTURE_SWIZZLE_B,
	GL_TEXTURE_SWIZZLE_A,
	GL_TEXTURE_SWIZZLE_RGBA,
	GL_TEXTURE_WRAP_S,
	GL_TEXTURE_WRAP_T,
	GL_TEXTURE_WRAP_R,

	//texture parameter values
	// CLAMP_TO_BORDER is an alias to CLAMP_TO_EDGE by default
	// enable it by defining PGL_ENABLE_CLAMP_TO_BORDER
	GL_REPEAT,
	GL_CLAMP_TO_EDGE,
	GL_CLAMP_TO_BORDER,
	GL_MIRRORED_REPEAT,
	GL_NEAREST,
	GL_LINEAR,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR,

	//texture/depth/stencil formats including some from GLES and custom
	PGL_ONE_ALPHA, // Like GL_ALPHA except uses 1's for rgb not 0's

	// From OpenGL ES
	GL_ALPHA, // Fills 0's in for rgb
	GL_LUMINANCE, // used for rgb, fills 1 for alpha
	GL_LUMINANCE_ALPHA, // lum used for rgb

	GL_RED,
	GL_RG,
	GL_RGB,
	GL_BGR,
	GL_RGBA,
	GL_BGRA,
	GL_COMPRESSED_RED,
	GL_COMPRESSED_RG,
	GL_COMPRESSED_RGB,
	GL_COMPRESSED_RGBA,
	//lots more go here but not important

	// None of these are used currently just to help porting
	GL_DEPTH_COMPONENT, // generic depth (texture format / RB internalformat)
	GL_DEPTH_COMPONENT16,
	GL_DEPTH_COMPONENT24,
	GL_DEPTH_COMPONENT32,
	GL_DEPTH_COMPONENT32F, // PGL uses a float depth buffer

	GL_DEPTH24_STENCIL8,
	GL_DEPTH32F_STENCIL8,  // <- we do this

	GL_STENCIL_INDEX1,
	GL_STENCIL_INDEX4,
	GL_STENCIL_INDEX8,   // this
	GL_STENCIL_INDEX16,

	
	//PixelStore parameters
	GL_UNPACK_ALIGNMENT,
	GL_PACK_ALIGNMENT,

	// Texture units (not used but eases porting)
	// but I'm not doing 80 or bothering with GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
	GL_TEXTURE0,
	GL_TEXTURE1,
	GL_TEXTURE2,
	GL_TEXTURE3,
	GL_TEXTURE4,
	GL_TEXTURE5,
	GL_TEXTURE6,
	GL_TEXTURE7,
	
	//implemented glEnable options
	GL_CULL_FACE,
	GL_DEPTH_TEST,
	GL_DEPTH_CLAMP,
	GL_LINE_SMOOTH,  // TODO correctly
	GL_BLEND,
	GL_COLOR_LOGIC_OP,
	GL_POLYGON_OFFSET_POINT,
	GL_POLYGON_OFFSET_LINE,
	GL_POLYGON_OFFSET_FILL,
	GL_SCISSOR_TEST,
	GL_STENCIL_TEST,

	//provoking vertex
	GL_FIRST_VERTEX_CONVENTION,
	GL_LAST_VERTEX_CONVENTION,

	//point sprite stuff
	GL_POINT_SPRITE_COORD_ORIGIN,
	GL_UPPER_LEFT,
	GL_LOWER_LEFT,

	//front face determination/culling
	GL_FRONT,
	GL_BACK,
	GL_FRONT_AND_BACK,
	GL_CCW,
	GL_CW,

	// glLogicOp logic ops
	GL_CLEAR,
	GL_SET,
	GL_COPY,
	GL_COPY_INVERTED,
	GL_NOOP,
	GL_AND,
	GL_NAND,
	GL_OR,
	GL_NOR,
	GL_XOR,
	GL_EQUIV,
	GL_AND_REVERSE,
	GL_AND_INVERTED,
	GL_OR_REVERSE,
	GL_OR_INVERTED,
	GL_INVERT,

	// glStencilOp
	GL_KEEP,
	//GL_ZERO, already defined in blend functions aggh
	GL_REPLACE,
	GL_INCR,
	GL_INCR_WRAP,
	GL_DECR,
	GL_DECR_WRAP,
	//GL_INVERT,   // already defined in LogicOps

	//data types
	GL_UNSIGNED_BYTE,
	GL_BYTE,
	GL_UNSIGNED_SHORT,
	GL_SHORT,
	GL_UNSIGNED_INT,
	GL_INT,
	GL_FLOAT,
	GL_DOUBLE,

	GL_BITMAP,  // TODO what is this for?

	//glGetString info
	GL_VENDOR,
	GL_RENDERER,
	GL_VERSION,
	GL_SHADING_LANGUAGE_VERSION,

	// glGet enums
	GL_POLYGON_OFFSET_FACTOR,
	GL_POLYGON_OFFSET_UNITS,
	GL_POINT_SIZE,

	GL_LINE_WIDTH,
	GL_ALIASED_LINE_WIDTH_RANGE,
	GL_SMOOTH_LINE_WIDTH_RANGE,
	GL_SMOOTH_LINE_WIDTH_GRANULARITY,

	GL_DEPTH_CLEAR_VALUE,
	GL_DEPTH_RANGE,
	GL_STENCIL_WRITE_MASK,
	GL_STENCIL_REF,
	GL_STENCIL_VALUE_MASK,
	GL_STENCIL_FUNC,
	GL_STENCIL_FAIL,
	GL_STENCIL_PASS_DEPTH_FAIL,
	GL_STENCIL_PASS_DEPTH_PASS,

	GL_STENCIL_BACK_WRITE_MASK,
	GL_STENCIL_BACK_REF,
	GL_STENCIL_BACK_VALUE_MASK,
	GL_STENCIL_BACK_FUNC,
	GL_STENCIL_BACK_FAIL,
	GL_STENCIL_BACK_PASS_DEPTH_FAIL,
	GL_STENCIL_BACK_PASS_DEPTH_PASS,

	GL_LOGIC_OP_MODE,
	GL_BLEND_SRC_RGB,
	GL_BLEND_SRC_ALPHA,
	GL_BLEND_DST_RGB,
	GL_BLEND_DST_ALPHA,

	GL_BLEND_EQUATION_RGB,
	GL_BLEND_EQUATION_ALPHA,

	GL_CULL_FACE_MODE,
	GL_FRONT_FACE,
	GL_DEPTH_FUNC,
	//GL_POINT_SPRITE_COORD_ORIGIN,
	GL_PROVOKING_VERTEX,

	GL_POLYGON_MODE,

	GL_MAJOR_VERSION,
	GL_MINOR_VERSION,

	GL_TEXTURE_BINDING_1D,
	GL_TEXTURE_BINDING_1D_ARRAY,
	GL_TEXTURE_BINDING_2D,
	GL_TEXTURE_BINDING_2D_ARRAY,

	// Not supported
	GL_TEXTURE_BINDING_2D_MULTISAMPLE,
	GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY,

	GL_TEXTURE_BINDING_3D,
	GL_TEXTURE_BINDING_BUFFER,
	GL_TEXTURE_BINDING_CUBE_MAP,
	GL_TEXTURE_BINDING_RECTANGLE,

	GL_ARRAY_BUFFER_BINDING,
	GL_ELEMENT_ARRAY_BUFFER_BINDING,
	GL_VERTEX_ARRAY_BINDING,
	GL_CURRENT_PROGRAM,

	GL_VIEWPORT,
	GL_SCISSOR_BOX,

	GL_MAX_TEXTURE_BUFFER_SIZE,
	GL_MAX_TEXTURE_IMAGE_UNITS,
	GL_MAX_TEXTURE_LOD_BIAS,
	GL_MAX_TEXTURE_SIZE,
	GL_MAX_3D_TEXTURE_SIZE,
	GL_MAX_ARRAY_TEXTURE_LAYERS,

	// glDebugOutput
	GL_DEBUG_OUTPUT,

	GL_DEBUG_SOURCE_API,
	GL_DEBUG_SOURCE_SHADER_COMPILER,
	GL_DEBUG_SOURCE_WINDOW_SYSTEM,
	GL_DEBUG_SOURCE_THIRD_PARTY,
	GL_DEBUG_SOURCE_APPLICATION,
	GL_DEBUG_SOURCE_OTHER,

	GL_DEBUG_TYPE_ERROR,
	GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR,
	GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR,
	GL_DEBUG_TYPE_PERFORMANCE,
	GL_DEBUG_TYPE_PORTABILITY,
	GL_DEBUG_TYPE_MARKER,
	GL_DEBUG_TYPE_PUSH_GROUP,
	GL_DEBUG_TYPE_POP_GROUP,
	GL_DEBUG_TYPE_OTHER,

	GL_DEBUG_SEVERITY_HIGH,
	GL_DEBUG_SEVERITY_MEDIUM,
	GL_DEBUG_SEVERITY_LOW,
	GL_DEBUG_SEVERITY_NOTIFICATION,

	GL_MAX_DEBUG_MESSAGE_LENGTH,

	//shader types etc. not used, just here for compatibility add what you
	//need so you can use your OpenGL code with PortableGL with minimal changes
	GL_COMPUTE_SHADER,
	GL_VERTEX_SHADER,
	GL_TESS_CONTROL_SHADER,
	GL_TESS_EVALUATION_SHADER,
	GL_GEOMETRY_SHADER,
	GL_FRAGMENT_SHADER,

	GL_INFO_LOG_LENGTH,
	GL_COMPILE_STATUS,
	GL_LINK_STATUS,

	// buffer clearing selections are a mask so can't have overlap
	// choosing arbitrary bits higher than all other constants in enum
	GL_COLOR_BUFFER_BIT = 1 << 10,
	GL_DEPTH_BUFFER_BIT = 1 << 11,
	GL_STENCIL_BUFFER_BIT = 1 << 12
};

#define GL_FALSE 0
#define GL_TRUE 1

#define GL_STENCIL_BITS 8

// Just GL_STENCIL_BITS of 1's, not an official GL enum/value
//#define PGL_STENCIL_MASK ((1 << GL_STENCIL_BITS)-1)
#define PGL_STENCIL_MASK 0xFF


// Feel free to change these
#ifdef PGL_TINY_MEM
// 80 KB
#define GL_MAX_VERTEX_ATTRIBS 4
#define PGL_MAX_VERTICES 5000
#elif defined(PGL_SMALL_MEM)
// 800 KB
#define GL_MAX_VERTEX_ATTRIBS 4
#define PGL_MAX_VERTICES 50000
#elif defined(PGL_MED_MEM)
//1.6 MB
#define GL_MAX_VERTEX_ATTRIBS 4
#define PGL_MAX_VERTICES 100000
#else
// 16 MB
#define GL_MAX_VERTEX_ATTRIBS 8
#define PGL_MAX_VERTICES 500000
#endif


#define GL_MAX_VERTEX_OUTPUT_COMPONENTS (4*GL_MAX_VERTEX_ATTRIBS)

// Mostly arbitrarily chosen, some match my AMD/Mesa output, some not really used
#define GL_MAX_DRAW_BUFFERS 4
#define GL_MAX_COLOR_ATTACHMENTS 4

#define PGL_MAX_ALIASED_WIDTH 2048.0f
// Primary knob: full chain L0..L(n-1) down to 1px. Max edge is 2^(n-1).
// 15 => 16384 (current default); lower this if you want smaller limits / less stack in glTexture.
#define PGL_MAX_MIPMAP_LEVELS 15
#define PGL_MAX_TEXTURE_SIZE (1 << (PGL_MAX_MIPMAP_LEVELS - 1))
#define PGL_MAX_3D_TEXTURE_SIZE 8192
#define PGL_MAX_ARRAY_TEXTURE_LAYERS 8192
#define PGL_MAX_DEBUG_MESSAGE_LENGTH 256

// TODO for now I only support smooth AA lines width 1, so granularity is meaningless
#define PGL_MAX_SMOOTH_WIDTH 1.0f
#define PGL_SMOOTH_GRANULARITY 1.0f

enum { PGL_SMOOTH, PGL_FLAT, PGL_NOPERSPECTIVE };

#define PGL_SMOOTH2 PGL_SMOOTH, PGL_SMOOTH
#define PGL_SMOOTH3 PGL_SMOOTH2, PGL_SMOOTH
#define PGL_SMOOTH4 PGL_SMOOTH3, PGL_SMOOTH

#define PGL_FLAT2 PGL_FLAT, PGL_FLAT
#define PGL_FLAT3 PGL_FLAT2, PGL_FLAT
#define PGL_FLAT4 PGL_FLAT3, PGL_FLAT

#define PGL_NOPERSPECTIVE2 PGL_NOPERSPECTIVE, PGL_NOPERSPECTIVE
#define PGL_NOPERSPECTIVE3 PGL_NOPERSPECTIVE2, PGL_NOPERSPECTIVE
#define PGL_NOPERSPECTIVE4 PGL_NOPERSPECTIVE3, PGL_NOPERSPECTIVE

//TODO NOT USED YET
typedef struct PerVertex {
	vec4 gl_Position;
	float gl_PointSize;
	float gl_ClipDistance[6];
} PerVertex;

// TODO separate structs for vertex and fragment shader builtins?
// input vs output?
typedef struct Shader_Builtins
{
	// vertex inputs
	GLint gl_InstanceID;
	GLint gl_BaseInstance; // 4.6 feature

	// vertex outputs
	vec4 gl_Position;
	//float gl_PointSize;
	//float gl_ClipDistance[6]

	// fragment inputs
	vec4 gl_FragCoord;
	vec2 gl_PointCoord;
	GLboolean gl_FrontFacing;  // struct packing fail I know

	// fragment outputs
	// Single-target shaders: write gl_FragColor (draw buffer 0 / back_buffer).
	// MRT (glDrawBuffers n>1): write gl_FragData[i] for each draw buffer i.
	vec4 gl_FragColor;
	vec4 gl_FragData[GL_MAX_DRAW_BUFFERS];
	float gl_FragDepth;
	GLboolean discard;

} Shader_Builtins;

// TODO GLfloat* and GLvoid*?
typedef void (*vert_func)(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
typedef void (*frag_func)(float* fs_input, Shader_Builtins* builtins, void* uniforms);

typedef void (*GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

typedef struct glProgram
{
	vert_func vertex_shader;
	frag_func fragment_shader;
	void* uniform;
	GLsizei vs_output_size;
	GLenum interpolation[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	GLboolean fragdepth_or_discard;

	GLboolean deleted;

} glProgram;

typedef struct glBuffer
{
	/*
	GLenum usage;
	GLenum access;
	GLint access_flags;
	void* map_pointer;
	GLsizei map_offset;
	GLsizei map_length;
	*/

	GLsizei size;
	GLenum type;
	u8* data;

	GLboolean deleted;

	// true if the user uses one of the pgl data extension functions that
	// doesn't copy the data.
	// If true, PGL does not free it when deleting the buffer
	GLboolean user_owned;
} glBuffer;

typedef struct glVertex_Attrib
{
	GLint size;      // number of components 1-4
	GLenum type;     // GL_FLOAT, default
	GLsizei stride;  //
	GLsizeiptr offset;  //
	GLboolean normalized;
	GLuint buf;
	GLboolean enabled;
	GLuint divisor;
} glVertex_Attrib;

typedef struct glVertex_Array
{
	glVertex_Attrib vertex_attribs[GL_MAX_VERTEX_ATTRIBS];

	GLuint element_buffer;
	GLboolean deleted;

} glVertex_Array;

// Descriptor for one mip level.  data points into the single tex->data allocation
// (never freed individually).  levels[0].data == tex->data when the texture has image data.
typedef struct glMipLevel
{
	GLsizei w;
	GLsizei h;
	u8* data;
} glMipLevel;

typedef struct glTexture
{
	GLsizei w;
	GLsizei h;
	GLsizei d;

	// Single allocation holds the full packed chain [L0][L1]...[Ln-1] (~4/3 base size).
	// Freeing tex->data frees every level.  levels[] is a fixed table of views into it.
	// num_levels: 0 empty, 1 base only, >1 mip chain present.
	GLint num_levels;
	glMipLevel levels[PGL_MAX_MIPMAP_LEVELS];
	// Bytes allocated for data when !user_owned (0 if user_owned or empty)
	size_t data_alloc;

	//GLint base_level;  // Not used yet
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	vec4 border_color;
#endif

	GLenum mag_filter;
	GLenum min_filter;
	GLenum wrap_s;
	GLenum wrap_t;
	GLenum wrap_r;

	// Pixel layout for tex->data (Phase D):
	// datatype: GL_UNSIGNED_BYTE or GL_FLOAT
	// format: GL_RED / GL_RG / GL_RGBA (color), or GL_DEPTH_COMPONENT (depth)
	// components: 1, 2, or 4 (derived from format; RGB32F not supported)
	// Depth textures: components=1; sample returns depth in .r (normalized if integer Z pack)
	GLenum datatype;
	GLenum format;
	GLint components;
	GLboolean is_depth;
	
	GLenum type; // GL_TEXTURE_UNBOUND, GL_TEXTURE_2D etc.

	GLboolean deleted;

	// TODO same meaning as in glBuffer
	GLboolean user_owned;

	// Row origin for sampling (and documenting RT write layout).
	// GL_FALSE (default): linear index y*w+x — uploaded assets (optional vflip on load).
	// GL_TRUE: lastrow-style — logical/fragCoord y=0 is memory row h-1, matching
	// default FB writes and pglSetTexBackBuffer / pglTextureAsRenderTarget RTs.
	// See scratch/ai_notes/render_to_texture.md Phase A.
	GLboolean invert_y;
	// data + (h-1)*w*bpp when invert_y and L0 is set; else NULL. Sample paths use
	// index math (not this pointer); kept for symmetry with glFramebuffer and apps.
	u8* lastrow;

	// Start of the single image allocation (level 0 / full chain)
	u8* data;
} glTexture;

typedef struct glVertex
{
	vec4 clip_space;
	vec4 screen_space;
	int clip_code;
	int edge_flag;
	float* vs_out;
} glVertex;

typedef struct glFramebuffer
{
	u8* buf;
	u8* lastrow; //better or worse than + h-1 every pixel draw?
	GLsizei w;
	GLsizei h;
} glFramebuffer;

// One texture or renderbuffer attachment on a framebuffer object
// (not the pixel glFramebuffer surface).
typedef struct glFBO_Attachment
{
	GLuint tex;   // 0 = none
	GLuint rb;    // 0 = none; mutually exclusive with tex
	GLint level;  // textures: level 0 only for now
} glFBO_Attachment;

// GL framebuffer *object* (name handle). Name 0 is the default window FB (not stored here).
typedef struct glFBO
{
	glFBO_Attachment color[GL_MAX_COLOR_ATTACHMENTS];
	glFBO_Attachment depth;
	glFBO_Attachment stencil; // RB or packed with depth (D24S8)

	// Draw/read buffer state is per-framebuffer (GL 3+).
	GLenum draw_buffers[GL_MAX_DRAW_BUFFERS];
	GLsizei num_draw_buffers;
	GLenum read_buffer; // glReadBuffer; default COLOR_ATTACHMENT0

	GLboolean deleted;
	GLboolean status_dirty;
	GLenum status; // last completeness result
} glFBO;

// Renderbuffer: non-sampleable storage (depth/stencil).
typedef struct glRenderbuffer
{
	GLsizei w;
	GLsizei h;
	GLenum internalformat;
	GLboolean deleted;
	GLboolean user_owned;
	size_t data_alloc;
	u8* data;
	u8* lastrow;
} glRenderbuffer;

// Resolved color RT for FBO draws (not default FB pix_t). Writes use texture
// storage format (Color U8 or float), never window pix_t.
typedef struct pglColorRT
{
	u8* buf;
	u8* lastrow;
	GLsizei w;
	GLsizei h;
	GLenum datatype; // GL_UNSIGNED_BYTE or GL_FLOAT
	GLint components; // 1, 2, 4
} pglColorRT;

typedef struct Vertex_Shader_output
{
	GLsizei size;
	GLenum* interpolation;

	// TODO Should this be a vector?  or just a pointer?
	// All I currently use is the constructor, reserve and free...
	// I could remove the rest of the cvector_float functions to save on bloat
	// but still easily add back functions as needed...
	//
	// or like comment in init_glContext says just allocate to the max size and be done
	float* output_buf;
} Vertex_Shader_output;


typedef void (*draw_triangle_func)(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);



#ifndef CVEC_SIZE_T
#include <stdlib.h>
#define CVEC_SIZE_T size_t
#endif

#ifndef CVEC_SZ
#define CVEC_SZ
typedef CVEC_SIZE_T cvec_sz;
#endif


/** Data structure for glVertex_Array vector. */
typedef struct cvector_glVertex_Array
{
	glVertex_Array* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glVertex_Array;



extern cvec_sz CVEC_glVertex_Array_SZ;

int cvec_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array* vals, cvec_sz num);

cvector_glVertex_Array* cvec_glVertex_Array_heap(cvec_sz size, cvec_sz capacity);
cvector_glVertex_Array* cvec_init_glVertex_Array_heap(glVertex_Array* vals, cvec_sz num);
int cvec_copyc_glVertex_Array(void* dest, void* src);
int cvec_copy_glVertex_Array(cvector_glVertex_Array* dest, cvector_glVertex_Array* src);

int cvec_push_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array a);
glVertex_Array cvec_pop_glVertex_Array(cvector_glVertex_Array* vec);

int cvec_extend_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz num);
int cvec_insert_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz i, glVertex_Array a);
int cvec_insert_array_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz i, glVertex_Array* a, cvec_sz num);
glVertex_Array cvec_replace_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz i, glVertex_Array a);
void cvec_erase_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz size);
#define cvec_shrink_to_fit_glVertex_Array(vec) cvec_set_cap_glVertex_Array((vec), (vec)->size)
int cvec_set_cap_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz size);
void cvec_set_val_sz_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val);
void cvec_set_val_cap_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val);

glVertex_Array* cvec_back_glVertex_Array(cvector_glVertex_Array* vec);

void cvec_clear_glVertex_Array(cvector_glVertex_Array* vec);
void cvec_free_glVertex_Array_heap(void* vec);
void cvec_free_glVertex_Array(void* vec);



/** Data structure for glBuffer vector. */
typedef struct cvector_glBuffer
{
	glBuffer* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glBuffer;



extern cvec_sz CVEC_glBuffer_SZ;

int cvec_glBuffer(cvector_glBuffer* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glBuffer(cvector_glBuffer* vec, glBuffer* vals, cvec_sz num);

cvector_glBuffer* cvec_glBuffer_heap(cvec_sz size, cvec_sz capacity);
cvector_glBuffer* cvec_init_glBuffer_heap(glBuffer* vals, cvec_sz num);
int cvec_copyc_glBuffer(void* dest, void* src);
int cvec_copy_glBuffer(cvector_glBuffer* dest, cvector_glBuffer* src);

int cvec_push_glBuffer(cvector_glBuffer* vec, glBuffer a);
glBuffer cvec_pop_glBuffer(cvector_glBuffer* vec);

int cvec_extend_glBuffer(cvector_glBuffer* vec, cvec_sz num);
int cvec_insert_glBuffer(cvector_glBuffer* vec, cvec_sz i, glBuffer a);
int cvec_insert_array_glBuffer(cvector_glBuffer* vec, cvec_sz i, glBuffer* a, cvec_sz num);
glBuffer cvec_replace_glBuffer(cvector_glBuffer* vec, cvec_sz i, glBuffer a);
void cvec_erase_glBuffer(cvector_glBuffer* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glBuffer(cvector_glBuffer* vec, cvec_sz size);
#define cvec_shrink_to_fit_glBuffer(vec) cvec_set_cap_glBuffer((vec), (vec)->size)
int cvec_set_cap_glBuffer(cvector_glBuffer* vec, cvec_sz size);
void cvec_set_val_sz_glBuffer(cvector_glBuffer* vec, glBuffer val);
void cvec_set_val_cap_glBuffer(cvector_glBuffer* vec, glBuffer val);

glBuffer* cvec_back_glBuffer(cvector_glBuffer* vec);

void cvec_clear_glBuffer(cvector_glBuffer* vec);
void cvec_free_glBuffer_heap(void* vec);
void cvec_free_glBuffer(void* vec);



/** Data structure for glTexture vector. */
typedef struct cvector_glTexture
{
	glTexture* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glTexture;



extern cvec_sz CVEC_glTexture_SZ;

int cvec_glTexture(cvector_glTexture* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glTexture(cvector_glTexture* vec, glTexture* vals, cvec_sz num);

cvector_glTexture* cvec_glTexture_heap(cvec_sz size, cvec_sz capacity);
cvector_glTexture* cvec_init_glTexture_heap(glTexture* vals, cvec_sz num);
int cvec_copyc_glTexture(void* dest, void* src);
int cvec_copy_glTexture(cvector_glTexture* dest, cvector_glTexture* src);

int cvec_push_glTexture(cvector_glTexture* vec, glTexture a);
glTexture cvec_pop_glTexture(cvector_glTexture* vec);

int cvec_extend_glTexture(cvector_glTexture* vec, cvec_sz num);
int cvec_insert_glTexture(cvector_glTexture* vec, cvec_sz i, glTexture a);
int cvec_insert_array_glTexture(cvector_glTexture* vec, cvec_sz i, glTexture* a, cvec_sz num);
glTexture cvec_replace_glTexture(cvector_glTexture* vec, cvec_sz i, glTexture a);
void cvec_erase_glTexture(cvector_glTexture* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glTexture(cvector_glTexture* vec, cvec_sz size);
#define cvec_shrink_to_fit_glTexture(vec) cvec_set_cap_glTexture((vec), (vec)->size)
int cvec_set_cap_glTexture(cvector_glTexture* vec, cvec_sz size);
void cvec_set_val_sz_glTexture(cvector_glTexture* vec, glTexture val);
void cvec_set_val_cap_glTexture(cvector_glTexture* vec, glTexture val);

glTexture* cvec_back_glTexture(cvector_glTexture* vec);

void cvec_clear_glTexture(cvector_glTexture* vec);
void cvec_free_glTexture_heap(void* vec);
void cvec_free_glTexture(void* vec);



/** Data structure for glProgram vector. */
typedef struct cvector_glProgram
{
	glProgram* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glProgram;



extern cvec_sz CVEC_glProgram_SZ;

int cvec_glProgram(cvector_glProgram* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glProgram(cvector_glProgram* vec, glProgram* vals, cvec_sz num);

cvector_glProgram* cvec_glProgram_heap(cvec_sz size, cvec_sz capacity);
cvector_glProgram* cvec_init_glProgram_heap(glProgram* vals, cvec_sz num);
int cvec_copyc_glProgram(void* dest, void* src);
int cvec_copy_glProgram(cvector_glProgram* dest, cvector_glProgram* src);

int cvec_push_glProgram(cvector_glProgram* vec, glProgram a);
glProgram cvec_pop_glProgram(cvector_glProgram* vec);

int cvec_extend_glProgram(cvector_glProgram* vec, cvec_sz num);
int cvec_insert_glProgram(cvector_glProgram* vec, cvec_sz i, glProgram a);
int cvec_insert_array_glProgram(cvector_glProgram* vec, cvec_sz i, glProgram* a, cvec_sz num);
glProgram cvec_replace_glProgram(cvector_glProgram* vec, cvec_sz i, glProgram a);
void cvec_erase_glProgram(cvector_glProgram* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glProgram(cvector_glProgram* vec, cvec_sz size);
#define cvec_shrink_to_fit_glProgram(vec) cvec_set_cap_glProgram((vec), (vec)->size)
int cvec_set_cap_glProgram(cvector_glProgram* vec, cvec_sz size);
void cvec_set_val_sz_glProgram(cvector_glProgram* vec, glProgram val);
void cvec_set_val_cap_glProgram(cvector_glProgram* vec, glProgram val);

glProgram* cvec_back_glProgram(cvector_glProgram* vec);

void cvec_clear_glProgram(cvector_glProgram* vec);
void cvec_free_glProgram_heap(void* vec);
void cvec_free_glProgram(void* vec);



/** Data structure for glVertex vector. */
typedef struct cvector_glVertex
{
	glVertex* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glVertex;



extern cvec_sz CVEC_glVertex_SZ;

int cvec_glVertex(cvector_glVertex* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glVertex(cvector_glVertex* vec, glVertex* vals, cvec_sz num);

cvector_glVertex* cvec_glVertex_heap(cvec_sz size, cvec_sz capacity);
cvector_glVertex* cvec_init_glVertex_heap(glVertex* vals, cvec_sz num);
int cvec_copyc_glVertex(void* dest, void* src);
int cvec_copy_glVertex(cvector_glVertex* dest, cvector_glVertex* src);

int cvec_push_glVertex(cvector_glVertex* vec, glVertex a);
glVertex cvec_pop_glVertex(cvector_glVertex* vec);

int cvec_extend_glVertex(cvector_glVertex* vec, cvec_sz num);
int cvec_insert_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex a);
int cvec_insert_array_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex* a, cvec_sz num);
glVertex cvec_replace_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex a);
void cvec_erase_glVertex(cvector_glVertex* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glVertex(cvector_glVertex* vec, cvec_sz size);
#define cvec_shrink_to_fit_glVertex(vec) cvec_set_cap_glVertex((vec), (vec)->size)
int cvec_set_cap_glVertex(cvector_glVertex* vec, cvec_sz size);
void cvec_set_val_sz_glVertex(cvector_glVertex* vec, glVertex val);
void cvec_set_val_cap_glVertex(cvector_glVertex* vec, glVertex val);

glVertex* cvec_back_glVertex(cvector_glVertex* vec);

void cvec_clear_glVertex(cvector_glVertex* vec);
void cvec_free_glVertex_heap(void* vec);
void cvec_free_glVertex(void* vec);



/** Data structure for glFBO vector. */
typedef struct cvector_glFBO
{
	glFBO* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glFBO;



extern cvec_sz CVEC_glFBO_SZ;

int cvec_glFBO(cvector_glFBO* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glFBO(cvector_glFBO* vec, glFBO* vals, cvec_sz num);

cvector_glFBO* cvec_glFBO_heap(cvec_sz size, cvec_sz capacity);
cvector_glFBO* cvec_init_glFBO_heap(glFBO* vals, cvec_sz num);
int cvec_copyc_glFBO(void* dest, void* src);
int cvec_copy_glFBO(cvector_glFBO* dest, cvector_glFBO* src);

int cvec_push_glFBO(cvector_glFBO* vec, glFBO a);
glFBO cvec_pop_glFBO(cvector_glFBO* vec);

int cvec_extend_glFBO(cvector_glFBO* vec, cvec_sz num);
int cvec_insert_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO a);
int cvec_insert_array_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO* a, cvec_sz num);
glFBO cvec_replace_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO a);
void cvec_erase_glFBO(cvector_glFBO* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glFBO(cvector_glFBO* vec, cvec_sz size);
#define cvec_shrink_to_fit_glFBO(vec) cvec_set_cap_glFBO((vec), (vec)->size)
int cvec_set_cap_glFBO(cvector_glFBO* vec, cvec_sz size);
void cvec_set_val_sz_glFBO(cvector_glFBO* vec, glFBO val);
void cvec_set_val_cap_glFBO(cvector_glFBO* vec, glFBO val);

glFBO* cvec_back_glFBO(cvector_glFBO* vec);

void cvec_clear_glFBO(cvector_glFBO* vec);
void cvec_free_glFBO_heap(void* vec);
void cvec_free_glFBO(void* vec);



/** Data structure for glRenderbuffer vector. */
typedef struct cvector_glRenderbuffer
{
	glRenderbuffer* a;           /**< Array. */
	cvec_sz size;       /**< Current size (amount you use when manipulating array directly). */
	cvec_sz capacity;   /**< Allocated size of array; always >= size. */
} cvector_glRenderbuffer;



extern cvec_sz CVEC_glRenderbuffer_SZ;

int cvec_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size, cvec_sz capacity);
int cvec_init_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer* vals, cvec_sz num);

cvector_glRenderbuffer* cvec_glRenderbuffer_heap(cvec_sz size, cvec_sz capacity);
cvector_glRenderbuffer* cvec_init_glRenderbuffer_heap(glRenderbuffer* vals, cvec_sz num);
int cvec_copyc_glRenderbuffer(void* dest, void* src);
int cvec_copy_glRenderbuffer(cvector_glRenderbuffer* dest, cvector_glRenderbuffer* src);

int cvec_push_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer a);
glRenderbuffer cvec_pop_glRenderbuffer(cvector_glRenderbuffer* vec);

int cvec_extend_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz num);
int cvec_insert_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer a);
int cvec_insert_array_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer* a, cvec_sz num);
glRenderbuffer cvec_replace_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer a);
void cvec_erase_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz start, cvec_sz end);
int cvec_reserve_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size);
#define cvec_shrink_to_fit_glRenderbuffer(vec) cvec_set_cap_glRenderbuffer((vec), (vec)->size)
int cvec_set_cap_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size);
void cvec_set_val_sz_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer val);
void cvec_set_val_cap_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer val);

glRenderbuffer* cvec_back_glRenderbuffer(cvector_glRenderbuffer* vec);

void cvec_clear_glRenderbuffer(cvector_glRenderbuffer* vec);
void cvec_free_glRenderbuffer_heap(void* vec);
void cvec_free_glRenderbuffer(void* vec);


typedef struct glContext
{
	mat4 vp_mat;

	// viewport control TODO not currently used internally
	GLint xmin, ymin;
	GLsizei width, height;

	// Always on scissoring (ie screenspace/guardband clipping)
	GLint lx, ly, ux, uy;

	cvector_glVertex_Array vertex_arrays;
	cvector_glBuffer buffers;
	cvector_glTexture textures;
	cvector_glProgram programs;

	// default 0 textures, have to exist per target
	glTexture default_textures[GL_NUM_TEXTURE_TYPES-GL_TEXTURE_UNBOUND-1];

	GLuint cur_vertex_array;
	GLuint bound_buffers[GL_NUM_BUFFER_TYPES-GL_ARRAY_BUFFER];
	GLuint bound_textures[GL_NUM_TEXTURE_TYPES-GL_TEXTURE_UNBOUND-1];
	GLuint cur_texture2D;
	GLuint cur_program;

	GLenum error;
	GLDEBUGPROC dbg_callback;
	GLchar dbg_msg_buf[PGL_MAX_DEBUG_MESSAGE_LENGTH];
	void* dbg_userparam;
	GLboolean dbg_output;

	// TODO make some or all of these locals, measure performance
	// impact. Would be necessary in the long term if I ever
	// parallelize more
	vec4 vertex_attribs_vs[GL_MAX_VERTEX_ATTRIBS];
	Shader_Builtins builtins;
	Vertex_Shader_output vs_output;
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	// Phase 2B: max |ΔUV|/|Δscreen| over triangle edges (UV units per pixel).
	// texture*D multiplies by texture size to get ρ / λ.  0 => treat as mag (level 0).
	float mip_uv_per_px;

	GLboolean depth_test;
	GLboolean line_smooth;
	GLboolean cull_face;
	GLboolean fragdepth_or_discard;
	GLboolean depth_clamp;
	GLboolean depth_mask;
	GLboolean blend;
	GLboolean logic_ops;
	GLboolean poly_offset_pt;
	GLboolean poly_offset_line;
	GLboolean poly_offset_fill;
	GLboolean scissor_test;

	pix_t color_mask;

#ifndef PGL_NO_STENCIL
	GLboolean stencil_test;
	GLuint stencil_writemask;
	GLuint stencil_writemask_back;
	GLint stencil_ref;
	GLint stencil_ref_back;
	GLuint stencil_valuemask;
	GLuint stencil_valuemask_back;
	GLenum stencil_func;
	GLenum stencil_func_back;
	GLenum stencil_sfail;
	GLenum stencil_dpfail;
	GLenum stencil_dppass;
	GLenum stencil_sfail_back;
	GLenum stencil_dpfail_back;
	GLenum stencil_dppass_back;

	GLint clear_stencil;
	glFramebuffer stencil_buf;
#endif

	GLenum logic_func;
	GLenum blend_sRGB;
	GLenum blend_sA;
	GLenum blend_dRGB;
	GLenum blend_dA;
	GLenum blend_eqRGB;
	GLenum blend_eqA;
	GLenum cull_mode;
	GLenum front_face;
	GLenum poly_mode_front;
	GLenum poly_mode_back;
	GLenum depth_func;
	GLenum point_spr_origin;
	GLenum provoking_vert;

	GLfloat poly_factor;
	GLfloat poly_units;

	GLint scissor_lx;
	GLint scissor_ly;
	GLsizei scissor_w;
	GLsizei scissor_h;

	GLint unpack_alignment;
	GLint pack_alignment;

	pix_t clear_color;
	vec4 blend_color;
	GLfloat point_size;
	GLfloat line_width;
	GLfloat clear_depth;
	//GLuint clear_depth;
	GLfloat depth_range_near;
	GLfloat depth_range_far;

	draw_triangle_func draw_triangle_front;
	draw_triangle_func draw_triangle_back;

	// I don't think it's actualy worth ifdef'ing all the depth buffer
	// stuff for PGL_NO_DEPTH_NO_STENCIL. Arguably it wasn't worth it
	// for PGL_NO_STENCIL either but I can always add it later
	glFramebuffer zbuf;
	glFramebuffer back_buffer;

	int user_alloced_backbuf;

	// Framebuffer objects. Name 0 = default window FB (not in vector).
	// When bound_framebuffer != 0, back_buffer/zbuf may point at attachments;
	// window_* hold the default surfaces to restore on bind 0.
	cvector_glFBO framebuffers;
	GLuint bound_framebuffer; // draw+read for v1 (no separate DRAW/READ bind)
	GLboolean fbo_redirected;
	glFramebuffer window_back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
	glFramebuffer window_zbuf;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
	glFramebuffer window_stencil_buf;
#  endif
	// Scratch Z when FBO has color but no depth attachment (size matches color).
	glFramebuffer fbo_scratch_z;
	// When bound FBO depth is float depth texture, depth test uses float compares.
	GLboolean zbuf_float;
#endif

	// FBO color RTs: format-correct surfaces (not window pix_t).
	// Default FB draws still use back_buffer as pix_t.
	pglColorRT mrt_color[GL_MAX_COLOR_ATTACHMENTS];
	GLboolean mrt_active; // true when bound FBO has num_draw_buffers > 1
	GLboolean fbo_color_is_rt; // true when drawing to FBO color (use mrt_color / float path)
	// Default FB draw-buffer state (user FBOs store their own on glFBO)
	GLenum default_draw_buffers[GL_MAX_DRAW_BUFFERS];
	GLsizei default_num_draw_buffers;
	// Active draw buffer list (copied from bound FBO or default on bind/DrawBuffers)
	GLenum draw_buffers[GL_MAX_DRAW_BUFFERS];
	GLsizei num_draw_buffers;
	// Read buffer for glReadPixels (default FB: GL_BACK; FBO: COLOR_ATTACHMENTi)
	GLenum read_buffer;
	GLenum default_read_buffer;

	cvector_glRenderbuffer renderbuffers;
	GLuint bound_renderbuffer;

	cvector_glVertex glverts;
} glContext;




/*************************************
 *  GLSL(ish) functions
 *************************************/

// Some duplication with crsw_math.h because
// we use these internally and the user can exclude
// those functions (with the official glsl names) to
// avoid clashes
//float clampf_01(float f);
//float clampf(float f, float min, float max);
//int clampi(int i, int min, int max);

//shader texture functions
PGLDEF vec4 texture1D(GLuint tex, float x);
PGLDEF vec4 texture2D(GLuint tex, float x, float y);
PGLDEF vec4 texture3D(GLuint tex, float x, float y, float z);
PGLDEF vec4 texture2DArray(GLuint tex, float x, float y, int z);
PGLDEF vec4 texture_rect(GLuint tex, float x, float y);
PGLDEF vec4 texture_cubemap(GLuint texture, float x, float y, float z);

// Explicit LOD (no automatic derivatives).  Within-level filter from MIN_FILTER.
// *MIPMAP_NEAREST: one level (round).  *MIPMAP_LINEAR: blend floor(lod) and +1
// (trilinear when within-level is LINEAR).
PGLDEF vec4 texture1DLod(GLuint tex, float x, float lod);
PGLDEF vec4 texture2DLod(GLuint tex, float x, float y, float lod);
PGLDEF vec4 texture_cubemapLod(GLuint texture, float x, float y, float z, float lod);

// Explicit screen-space derivatives → λ (isotropic ρ = max(length(dPdx), length(dPdy))
// in texel units).  Same sample path as *Lod once λ is known.
// 1D: dPdx/dPdy are ∂u/∂x, ∂u/∂y (scalar coord).
// 2D: dUdx,dVdx = ∂(u,v)/∂x; dUdy,dVdy = ∂(u,v)/∂y.
// Cubemap: d* are derivatives of the direction vector; face-UV Jacobian via
// same-face finite difference (see gl_glsl.c).
PGLDEF vec4 texture1DGrad(GLuint tex, float x, float dPdx, float dPdy);
PGLDEF vec4 texture2DGrad(GLuint tex, float x, float y,
                          float dUdx, float dVdx, float dUdy, float dVdy);
PGLDEF vec4 texture_cubemapGrad(GLuint texture, float x, float y, float z,
                                float dPdx_x, float dPdx_y, float dPdx_z,
                                float dPdy_x, float dPdy_y, float dPdy_z);

// --- LOD helpers (for texture*Lod / manual control when auto-LOD is unavailable) ---
//
// tex must be a non-zero texture object (not default name 0).  tex==0 →
// GL_INVALID_VALUE in debug; check removed under PGL_UNSAFE.
//
// "Screen" = current color write surface: c->back_buffer.w/h.  That is updated by
// glBindFramebuffer / pgl_apply_draw_framebuffer and pglSetBackBuffer /
// pglSetTexBackBuffer.  Viewport is not used.  If you rasterize offline into a
// buffer without redirecting the back buffer (e.g. some full-frame callbacks),
// pass the real RT size with the *_wh variants.
//
// pgl_lod_screen: λ for UV = fragCoord/res (0–1 across the RT).
//   ρ = max(tex_w/rt_w, tex_h/rt_h), λ = log2(ρ)
// pgl_lod_uv_scale: same with UV' = s * UV_screen  →  λ_screen + log2(|s|)
// pgl_lod_grad / pgl_lod_grad1D: λ from explicit derivatives (texture*Grad core).

PGLDEF float pgl_lod_screen_wh(GLuint tex, float rt_w, float rt_h);
PGLDEF float pgl_lod_uv_scale_wh(GLuint tex, float scale, float rt_w, float rt_h);
PGLDEF float pgl_lod_screen(GLuint tex);
PGLDEF float pgl_lod_uv_scale(GLuint tex, float scale);

PGLDEF float pgl_lod_grad1D(GLuint tex, float dUdx, float dUdy);
PGLDEF float pgl_lod_grad(GLuint tex, float dUdx, float dVdx, float dUdy, float dVdy);

PGLDEF vec4 texelFetch1D(GLuint tex, int x, int lod);
PGLDEF vec4 texelFetch2D(GLuint tex, int x, int y, int lod);
PGLDEF vec4 texelFetch3D(GLuint tex, int x, int y, int z, int lod);

// tex must be non-zero (default 0 is ambiguous across targets).  tex==0 →
// GL_INVALID_VALUE in debug; (0,0,0) returned.  Check removed under PGL_UNSAFE.
PGLDEF ivec3 textureSize(GLuint tex, GLint lod);

typedef struct pgl_uniforms
{
	mat4 mvp_mat;
	mat4 mv_mat;
	mat4 p_mat;
	mat3 normal_mat;
	vec4 color;

	GLuint tex0;
	vec3 light_pos;
} pgl_uniforms;

typedef struct pgl_prog_info
{
	vert_func vs;
	frag_func fs;
	int vs_out_sz;
	GLenum interp[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	GLboolean uses_fragdepth_or_discard;
} pgl_prog_info;

enum {
	PGL_ATTR_VERT,
	PGL_ATTR_COLOR,
	PGL_ATTR_NORMAL,
	PGL_ATTR_TEXCOORD0,
	PGL_ATTR_TEXCOORD1
};

enum {
	PGL_SHADER_IDENTITY,
	PGL_SHADER_FLAT,
	PGL_SHADER_SHADED,
	PGL_SHADER_DFLT_LIGHT,
	PGL_SHADER_POINT_LIGHT_DIFF,
	PGL_SHADER_TEX_REPLACE,
	PGL_SHADER_TEX_MODULATE,
	PGL_SHADER_TEX_POINT_LIGHT_DIFF,
	PGL_SHADER_TEX_RECT_REPLACE,
	PGL_NUM_SHADERS
};

PGLDEF void pgl_init_std_shaders(GLuint programs[PGL_NUM_SHADERS]);


// TODO leave these non gl* functions here?  prefix with pgl?
PGLDEF GLboolean init_glContext(glContext* c, pix_t** back_buffer, GLsizei width, GLsizei height);
PGLDEF void free_glContext(glContext* context);
PGLDEF void set_glContext(glContext* context);

PGLDEF GLboolean pglResizeFramebuffer(GLsizei width, GLsizei height);

PGLDEF void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

PGLDEF void glDebugMessageCallback(GLDEBUGPROC callback, void* userParam);

PGLDEF GLubyte* glGetString(GLenum name);
PGLDEF GLenum glGetError(void);
PGLDEF void glGetBooleanv(GLenum pname, GLboolean* data);
PGLDEF void glGetFloatv(GLenum pname, GLfloat* data);
PGLDEF void glGetIntegerv(GLenum pname, GLint* data);
PGLDEF GLboolean glIsEnabled(GLenum cap);
PGLDEF GLboolean glIsProgram(GLuint program);

PGLDEF void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
PGLDEF void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
PGLDEF void glClearDepthf(GLfloat depth);
PGLDEF void glClearDepth(GLdouble depth);
PGLDEF void glDepthFunc(GLenum func);
PGLDEF void glDepthRangef(GLfloat nearVal, GLfloat farVal);
PGLDEF void glDepthRange(GLdouble nearVal, GLdouble farVal);
PGLDEF void glDepthMask(GLboolean flag);
PGLDEF void glBlendFunc(GLenum sfactor, GLenum dfactor);
PGLDEF void glBlendEquation(GLenum mode);
PGLDEF void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
PGLDEF void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
PGLDEF void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
PGLDEF void glClear(GLbitfield mask);
PGLDEF void glProvokingVertex(GLenum provokeMode);
PGLDEF void glEnable(GLenum cap);
PGLDEF void glDisable(GLenum cap);
PGLDEF void glCullFace(GLenum mode);
PGLDEF void glFrontFace(GLenum mode);
PGLDEF void glPolygonMode(GLenum face, GLenum mode);
PGLDEF void glPointSize(GLfloat size);
PGLDEF void glPointParameteri(GLenum pname, GLint param);
PGLDEF void glLineWidth(GLfloat width);
PGLDEF void glLogicOp(GLenum opcode);
PGLDEF void glPolygonOffset(GLfloat factor, GLfloat units);
PGLDEF void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
#ifndef PGL_NO_STENCIL
PGLDEF void glStencilFunc(GLenum func, GLint ref, GLuint mask);
PGLDEF void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
PGLDEF void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
PGLDEF void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
PGLDEF void glClearStencil(GLint s);
PGLDEF void glStencilMask(GLuint mask);
PGLDEF void glStencilMaskSeparate(GLenum face, GLuint mask);
#endif

// Framebuffer objects (color/depth/stencil, MRT, renderbuffers, readback)
PGLDEF void glGenFramebuffers(GLsizei n, GLuint* ids);
PGLDEF void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
PGLDEF void glBindFramebuffer(GLenum target, GLuint framebuffer);
PGLDEF GLboolean glIsFramebuffer(GLuint framebuffer);
PGLDEF void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
PGLDEF void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
PGLDEF void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
PGLDEF GLenum glCheckFramebufferStatus(GLenum target);
PGLDEF void glDrawBuffers(GLsizei n, const GLenum* bufs);
PGLDEF void glReadBuffer(GLenum mode);
PGLDEF void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data);
PGLDEF void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
PGLDEF void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
PGLDEF void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
PGLDEF GLboolean glIsRenderbuffer(GLuint renderbuffer);
PGLDEF void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

// textures
PGLDEF void glGenTextures(GLsizei n, GLuint* textures);
PGLDEF void glDeleteTextures(GLsizei n, const GLuint* textures);
PGLDEF void glBindTexture(GLenum target, GLuint texture);

PGLDEF void glTexParameteri(GLenum target, GLenum pname, GLint param);
PGLDEF void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params);
PGLDEF void glTexParameteriv(GLenum target, GLenum pname, const GLint* params);
PGLDEF void glTextureParameteri(GLuint texture, GLenum pname, GLint param);
PGLDEF void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* params);
PGLDEF void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* params);

PGLDEF void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params);
PGLDEF void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params);
PGLDEF void glGetTexParameterIiv(GLenum target, GLenum pname, GLint* params);
PGLDEF void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint* params);
PGLDEF void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params);
PGLDEF void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params);
PGLDEF void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params);
PGLDEF void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params);

PGLDEF void glPixelStorei(GLenum pname, GLint param);
PGLDEF void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data);

PGLDEF void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data);

// 1D/2D only for now; builds RGBA8 box-filtered chain from level 0
PGLDEF void glGenerateMipmap(GLenum target);
PGLDEF void glGenerateTextureMipmap(GLuint texture);

PGLDEF void glGenVertexArrays(GLsizei n, GLuint* arrays);
PGLDEF void glDeleteVertexArrays(GLsizei n, const GLuint* arrays);
PGLDEF void glBindVertexArray(GLuint array);
PGLDEF void glGenBuffers(GLsizei n, GLuint* buffers);
PGLDEF void glDeleteBuffers(GLsizei n, const GLuint* buffers);
PGLDEF void glBindBuffer(GLenum target, GLuint buffer);
PGLDEF void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
PGLDEF void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
PGLDEF void* glMapBuffer(GLenum target, GLenum access);
PGLDEF void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
PGLDEF void glVertexAttribDivisor(GLuint index, GLuint divisor);
PGLDEF void glEnableVertexAttribArray(GLuint index);
PGLDEF void glDisableVertexAttribArray(GLuint index);
PGLDEF void glDrawArrays(GLenum mode, GLint first, GLsizei count);
PGLDEF void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount);
PGLDEF void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
PGLDEF void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const GLvoid* const* indices, GLsizei drawcount);
PGLDEF void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
PGLDEF void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance);
PGLDEF void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount);
PGLDEF void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount, GLuint baseinstance);

//DSA functions (from OpenGL 4.5+)
#define glCreateBuffers(n, buffers) glGenBuffers(n, buffers)
PGLDEF void glNamedBufferData(GLuint buffer, GLsizeiptr size, const GLvoid* data, GLenum usage);
PGLDEF void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const GLvoid* data);
PGLDEF void* glMapNamedBuffer(GLuint buffer, GLenum access);
PGLDEF void glCreateTextures(GLenum target, GLsizei n, GLuint* textures);

PGLDEF void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index);
PGLDEF void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index);


//shaders
PGLDEF GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean fragdepth_or_discard);
PGLDEF void glDeleteProgram(GLuint program);
PGLDEF void glUseProgram(GLuint program);

// These are here, not in pgl_ext.h/c because they take the place of standard OpenGL
// functions glUniform*() and glProgramUniform*()
PGLDEF void pglSetUniform(void* uniform);
PGLDEF void pglSetProgramUniform(GLuint program, void* uniform);



#ifndef PGL_EXCLUDE_STUBS

// Stubs to let real OpenGL libs compile with minimal modifications/ifdefs
// add what you need
//
PGLDEF const GLubyte* glGetStringi(GLenum name, GLuint index);

PGLDEF void glColorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);

// glGenerateMipmap is a real implementation (see gl_prototypes.h / gl_impl.c)
PGLDEF void glActiveTexture(GLenum texture);

PGLDEF void glTexParameterf(GLenum target, GLenum pname, GLfloat param);

PGLDEF void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param);

// TODO what the heck are these?
PGLDEF void glTexParameterliv(GLenum target, GLenum pname, const GLint* params);
PGLDEF void glTexParameterluiv(GLenum target, GLenum pname, const GLuint* params);

PGLDEF void glTextureParameterliv(GLuint texture, GLenum pname, const GLint* params);
PGLDEF void glTextureParameterluiv(GLuint texture, GLenum pname, const GLuint* params);

PGLDEF void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data);
PGLDEF void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
PGLDEF void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);

PGLDEF void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer);
PGLDEF void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer);

PGLDEF void glGetDoublev(GLenum pname, GLdouble* params);
PGLDEF void glGetInteger64v(GLenum pname, GLint64* params);

// Draw buffers (glDrawBuffers implemented in gl_fbo.c)
PGLDEF void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum* bufs);

// Framebuffers/Renderbuffers (gen/bind/delete/texture2D/check implemented in gl_fbo.c)
PGLDEF void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
PGLDEF void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);

PGLDEF void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
PGLDEF void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);

PGLDEF void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode);

PGLDEF void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
PGLDEF void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

// Core renderbuffer/read APIs implemented in gl_fbo.c

PGLDEF void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
PGLDEF void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

PGLDEF void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value);
PGLDEF void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value);
PGLDEF void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value);
PGLDEF void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
PGLDEF void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value);
PGLDEF void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value);
PGLDEF void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value);
PGLDEF void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);


PGLDEF void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
PGLDEF void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
PGLDEF void glAttachShader(GLuint program, GLuint shader);
PGLDEF void glCompileShader(GLuint shader);
PGLDEF void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);

// use pglCreateProgram()
PGLDEF GLuint glCreateProgram(void);

PGLDEF void glLinkProgram(GLuint program);
PGLDEF void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
PGLDEF void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
PGLDEF GLuint glCreateShader(GLenum shaderType);
PGLDEF void glDeleteShader(GLuint shader);
PGLDEF void glDetachShader(GLuint program, GLuint shader);

PGLDEF GLint glGetUniformLocation(GLuint program, const GLchar* name);
PGLDEF GLint glGetAttribLocation(GLuint program, const GLchar* name);

PGLDEF GLboolean glUnmapBuffer(GLenum target);
PGLDEF GLboolean glUnmapNamedBuffer(GLuint buffer);

PGLDEF void glUniform1f(GLint location, GLfloat v0);
PGLDEF void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
PGLDEF void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
PGLDEF void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

PGLDEF void glUniform1i(GLint location, GLint v0);
PGLDEF void glUniform2i(GLint location, GLint v0, GLint v1);
PGLDEF void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
PGLDEF void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

PGLDEF void glUniform1ui(GLint location, GLuint v0);
PGLDEF void glUniform2ui(GLint location, GLuint v0, GLuint v1);
PGLDEF void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);
PGLDEF void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

PGLDEF void glUniform1fv(GLint location, GLsizei count, const GLfloat* value);
PGLDEF void glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
PGLDEF void glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
PGLDEF void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);

PGLDEF void glUniform1iv(GLint location, GLsizei count, const GLint* value);
PGLDEF void glUniform2iv(GLint location, GLsizei count, const GLint* value);
PGLDEF void glUniform3iv(GLint location, GLsizei count, const GLint* value);
PGLDEF void glUniform4iv(GLint location, GLsizei count, const GLint* value);

PGLDEF void glUniform1uiv(GLint location, GLsizei count, const GLuint* value);
PGLDEF void glUniform2uiv(GLint location, GLsizei count, const GLuint* value);
PGLDEF void glUniform3uiv(GLint location, GLsizei count, const GLuint* value);
PGLDEF void glUniform4uiv(GLint location, GLsizei count, const GLuint* value);

PGLDEF void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PGLDEF void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

#endif


// Modeled after SDL for RenderGeometry
// Color c like SDL or vec4 c?
typedef struct pgl_vertex
{
	vec2 pos;
	Color color;
	vec2 tex_coord;
} pgl_vertex;


// TODO use ints like SDL or keep floats?
typedef struct pgl_fill_data
{
	vec2 dst;
	Color c;
} pgl_fill_data;

typedef struct pgl_copy_data
{
	vec2 src;
	vec2 dst;
	Color c;
} pgl_copy_data;

PGLDEF void pglClearScreen(void);

//This isn't possible in regular OpenGL, changing the interpolation of vs output of
//an existing shader.  You'd have to switch between 2 almost identical shaders.
PGLDEF void pglSetInterp(GLsizei n, GLenum* interpolation);

#define pglVertexAttribPointer(index, size, type, normalized, stride, offset) \
glVertexAttribPointer(index, size, type, normalized, stride, (void*)(offset))


PGLDEF GLuint pglCreateFragProgram(frag_func fragment_shader, GLboolean fragdepth_or_discard);

//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
PGLDEF void pglDrawFrame(void);
PGLDEF void pglDrawFrame2(frag_func frag_shader, void* uniforms);

// TODO should these be called pglMapped* since that's what they do?  I don't think so, since it's too different from actual spec for mapped buffers
PGLDEF void pglBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage);

// NOTE: All 3 of these functions are "named only", meaning no default texure 0 allowed, unlike their non-pgl-extension counter parts. They call
// the DSA functions below internally where texure == 0 is an error.
PGLDEF void pglTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void pglTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void pglTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data);

PGLDEF void pglTextureImage1D(GLuint texture, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void pglTextureImage2D(GLuint texture, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data);
PGLDEF void pglTextureImage3D(GLuint texture, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data);

// I could make these return the data?
PGLDEF void pglGetBufferData(GLuint buffer, GLvoid** data);
PGLDEF void pglGetTextureData(GLuint texture, GLvoid** data);

PGLDEF const glTexture* pglGetTexture(GLuint texture);

GLvoid* pglGetBackBuffer(void);
PGLDEF void pglSetBackBuffer(GLvoid* backbuf, GLsizei w, GLsizei h, GLboolean user_owned);
PGLDEF void pglSetTexBackBuffer(GLuint texture);

// Mark texture as RT: sample/fetch use lastrow Y (fragCoord y=0 = bottom of image).
// Does not change the current back buffer (unlike pglSetTexBackBuffer).
PGLDEF void pglTextureAsRenderTarget(GLuint texture);


PGLDEF u8* convert_format_to_packed_rgba(u8* output, u8* input, int w, int h, int pitch, GLenum format);
PGLDEF u8* convert_grayscale_to_rgba(u8* input, int size, u32 bg_rgba, u32 text_rgba);

PGLDEF int setup_default_textures(void);

PGLDEF void put_pixel(Color color, int x, int y);
PGLDEF void put_pixel_blend(vec4 src, int x, int y);

//Should I have it take a glFramebuffer as paramater?
PGLDEF void put_line(Color the_color, float x1, float y1, float x2, float y2);
PGLDEF void put_wide_line_simple(Color the_color, float width, float x1, float y1, float x2, float y2);
PGLDEF void put_wide_line(Color color1, Color color2, float width, float x1, float y1, float x2, float y2);

PGLDEF void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3);
PGLDEF void put_triangle_tex(int tex, vec2 uv1, vec2 uv2, vec2 uv3, vec2 p1, vec2 p2, vec2 p3);

// Immediate-mode textured triangles (SDL_RenderGeometryRaw-style).
// Samples with texture2D() but does NOT set per-triangle mip LOD (c->mip_uv_per_px),
// so *MIPMAP* min filters still read level 0.  Automatic LOD only runs on the
// normal glDraw* / fragment-shader path.  Use texture2DLod in a real FS if you
// need an explicit level; this helper always calls texture2D.
PGLDEF void pgl_draw_geometry_raw(int tex, const float* xy, int xy_stride, const Color* color, int color_stride, const float* uv, int uv_stride, int n_verts, const void* indices, int n_indices, int sz_indices);

PGLDEF void put_aa_line(vec4 c, float x1, float y1, float x2, float y2);
PGLDEF void put_aa_line_interp(vec4 c1, vec4 c2, float x1, float y1, float x2, float y2);


#ifdef __cplusplus
}
#endif

// end GL_H
#endif

#ifdef PORTABLEGL_IMPLEMENTATION


extern inline vec2 make_v2(float x, float y);
extern inline vec2 neg_v2(vec2 v);
extern inline void fprint_v2(FILE* f, vec2 v, const char* append);
extern inline void print_v2(vec2 v, const char* append);
extern inline int fread_v2(FILE* f, vec2* v);
extern inline float len_v2(vec2 a);
extern inline vec2 norm_v2(vec2 a);
extern inline void normalize_v2(vec2* a);
extern inline vec2 add_v2s(vec2 a, vec2 b);
extern inline vec2 sub_v2s(vec2 a, vec2 b);
extern inline vec2 mult_v2s(vec2 a, vec2 b);
extern inline vec2 div_v2s(vec2 a, vec2 b);
extern inline float dot_v2s(vec2 a, vec2 b);
extern inline vec2 add_v2(vec2 a, float s);
extern inline vec2 scale_v2(vec2 a, float s);
extern inline int equal_v2s(vec2 a, vec2 b);
extern inline int equal_epsilon_v2s(vec2 a, vec2 b, float epsilon);
extern inline float cross_v2s(vec2 a, vec2 b);
extern inline float angle_v2s(vec2 a, vec2 b);


extern inline vec3 make_v3(float x, float y, float z);
extern inline vec3 neg_v3(vec3 v);
extern inline void fprint_v3(FILE* f, vec3 v, const char* append);
extern inline void print_v3(vec3 v, const char* append);
extern inline int fread_v3(FILE* f, vec3* v);
extern inline float len_v3(vec3 a);
extern inline vec3 norm_v3(vec3 a);
extern inline void normalize_v3(vec3* a);
extern inline vec3 add_v3s(vec3 a, vec3 b);
extern inline vec3 sub_v3s(vec3 a, vec3 b);
extern inline vec3 mult_v3s(vec3 a, vec3 b);
extern inline vec3 div_v3s(vec3 a, vec3 b);
extern inline float dot_v3s(vec3 a, vec3 b);
extern inline vec3 add_v3(vec3 a, float s);
extern inline vec3 scale_v3(vec3 a, float s);
extern inline int equal_v3s(vec3 a, vec3 b);
extern inline int equal_epsilon_v3s(vec3 a, vec3 b, float epsilon);
extern inline vec3 cross_v3s(const vec3 u, const vec3 v);
extern inline float angle_v3s(const vec3 u, const vec3 v);


extern inline vec4 make_v4(float x, float y, float z, float w);
extern inline vec4 neg_v4(vec4 v);
extern inline void fprint_v4(FILE* f, vec4 v, const char* append);
extern inline void print_v4(vec4 v, const char* append);
extern inline int fread_v4(FILE* f, vec4* v);
extern inline float len_v4(vec4 a);
extern inline vec4 norm_v4(vec4 a);
extern inline void normalize_v4(vec4* a);
extern inline vec4 add_v4s(vec4 a, vec4 b);
extern inline vec4 sub_v4s(vec4 a, vec4 b);
extern inline vec4 mult_v4s(vec4 a, vec4 b);
extern inline vec4 div_v4s(vec4 a, vec4 b);
extern inline float dot_v4s(vec4 a, vec4 b);
extern inline vec4 add_v4(vec4 a, float s);
extern inline vec4 scale_v4(vec4 a, float s);
extern inline int equal_v4s(vec4 a, vec4 b);
extern inline int equal_epsilon_v4s(vec4 a, vec4 b, float epsilon);


extern inline ivec2 make_iv2(int x, int y);
extern inline void fprint_iv2(FILE* f, ivec2 v, const char* append);
extern inline int fread_iv2(FILE* f, ivec2* v);

extern inline ivec3 make_iv3(int x, int y, int z);
extern inline void fprint_iv3(FILE* f, ivec3 v, const char* append);
extern inline int fread_iv3(FILE* f, ivec3* v);

extern inline ivec4 make_iv4(int x, int y, int z, int w);
extern inline void fprint_iv4(FILE* f, ivec4 v, const char* append);
extern inline int fread_iv4(FILE* f, ivec4* v);

extern inline uvec2 make_uv2(unsigned int x, unsigned int y);
extern inline void fprint_uv2(FILE* f, uvec2 v, const char* append);
extern inline int fread_uv2(FILE* f, uvec2* v);

extern inline uvec3 make_uv3(unsigned int x, unsigned int y, unsigned int z);
extern inline void fprint_uv3(FILE* f, uvec3 v, const char* append);
extern inline int fread_uv3(FILE* f, uvec3* v);

extern inline uvec4 make_uv4(unsigned int x, unsigned int y, unsigned int z, unsigned int w);
extern inline void fprint_uv4(FILE* f, uvec4 v, const char* append);
extern inline int fread_uv4(FILE* f, uvec4* v);

extern inline bvec2 make_bv2(int x, int y);
extern inline void fprint_bv2(FILE* f, bvec2 v, const char* append);
extern inline int fread_bv2(FILE* f, bvec2* v);

extern inline bvec3 make_bv3(int x, int y, int z);
extern inline void fprint_bv3(FILE* f, bvec3 v, const char* append);
extern inline int fread_bv3(FILE* f, bvec3* v);

extern inline bvec4 make_bv4(int x, int y, int z, int w);
extern inline void fprint_bv4(FILE* f, bvec4 v, const char* append);
extern inline int fread_bv4(FILE* f, bvec4* v);

extern inline vec2 v3_to_v2(vec3 a);
extern inline vec2 v4_to_v2(vec4 a);
extern inline vec3 v4_to_v3(vec4 a);
extern inline vec2 v4_to_v2h(vec4 a);
extern inline vec3 v4_to_v3h(vec4 a);

extern inline void fprint_m2(FILE* f, mat2 m, const char* append);
extern inline void fprint_m3(FILE* f, mat3 m, const char* append);
extern inline void fprint_m4(FILE* f, mat4 m, const char* append);
extern inline void print_m2(mat2 m, const char* append);
extern inline void print_m3(mat3 m, const char* append);
extern inline void print_m4(mat4 m, const char* append);
extern inline vec2 mult_m2_v2(mat2 m, vec2 v);
extern inline vec3 mult_m3_v3(mat3 m, vec3 v);
extern inline vec4 mult_m4_v4(mat4 m, vec4 v);
extern inline void scale_m3(mat3 m, float x, float y, float z);
extern inline void scale_m4(mat4 m, float x, float y, float z);
extern inline void translation_m4(mat4 m, float x, float y, float z);
extern inline void extract_rotation_m4(mat3 dst, mat4 src, int normalize);

extern inline vec2 x_m2(mat2 m);
extern inline vec2 y_m2(mat2 m);
extern inline vec2 c1_m2(mat2 m);
extern inline vec2 c2_m2(mat2 m);

extern inline void setc1_m2(mat2 m, vec2 v);
extern inline void setc2_m2(mat2 m, vec2 v);
extern inline void setx_m2(mat2 m, vec2 v);
extern inline void sety_m2(mat2 m, vec2 v);

extern inline vec3 x_m3(mat3 m);
extern inline vec3 y_m3(mat3 m);
extern inline vec3 z_m3(mat3 m);
extern inline vec3 c1_m3(mat3 m);
extern inline vec3 c2_m3(mat3 m);
extern inline vec3 c3_m3(mat3 m);

extern inline void setc1_m3(mat3 m, vec3 v);
extern inline void setc2_m3(mat3 m, vec3 v);
extern inline void setc3_m3(mat3 m, vec3 v);

extern inline void setx_m3(mat3 m, vec3 v);
extern inline void sety_m3(mat3 m, vec3 v);
extern inline void setz_m3(mat3 m, vec3 v);

extern inline vec4 c1_m4(mat4 m);
extern inline vec4 c2_m4(mat4 m);
extern inline vec4 c3_m4(mat4 m);
extern inline vec4 c4_m4(mat4 m);

extern inline vec4 x_m4(mat4 m);
extern inline vec4 y_m4(mat4 m);
extern inline vec4 z_m4(mat4 m);
extern inline vec4 w_m4(mat4 m);

extern inline void setc1_m4v3(mat4 m, vec3 v);
extern inline void setc2_m4v3(mat4 m, vec3 v);
extern inline void setc3_m4v3(mat4 m, vec3 v);
extern inline void setc4_m4v3(mat4 m, vec3 v);

extern inline void setc1_m4v4(mat4 m, vec4 v);
extern inline void setc2_m4v4(mat4 m, vec4 v);
extern inline void setc3_m4v4(mat4 m, vec4 v);
extern inline void setc4_m4v4(mat4 m, vec4 v);

extern inline void setx_m4v3(mat4 m, vec3 v);
extern inline void sety_m4v3(mat4 m, vec3 v);
extern inline void setz_m4v3(mat4 m, vec3 v);
extern inline void setw_m4v3(mat4 m, vec3 v);

extern inline void setx_m4v4(mat4 m, vec4 v);
extern inline void sety_m4v4(mat4 m, vec4 v);
extern inline void setz_m4v4(mat4 m, vec4 v);
extern inline void setw_m4v4(mat4 m, vec4 v);


void mult_m2_m2(mat2 c, mat2 a, mat2 b)
{
#ifndef ROW_MAJOR
	c[0] = a[0]*b[0] + a[2]*b[1];
	c[2] = a[0]*b[2] + a[2]*b[3];

	c[1] = a[1]*b[0] + a[3]*b[1];
	c[3] = a[1]*b[2] + a[3]*b[3];
#else
	c[0] = a[0]*b[0] + a[1]*b[2];
	c[1] = a[0]*b[1] + a[1]*b[3];

	c[2] = a[2]*b[0] + a[3]*b[2];
	c[3] = a[2]*b[1] + a[3]*b[3];
#endif
}

extern inline void load_rotation_m2(mat2 mat, float angle);

void mult_m3_m3(mat3 c, mat3 a, mat3 b)
{
#ifndef ROW_MAJOR
	c[0] = a[0]*b[0] + a[3]*b[1] + a[6]*b[2];
	c[3] = a[0]*b[3] + a[3]*b[4] + a[6]*b[5];
	c[6] = a[0]*b[6] + a[3]*b[7] + a[6]*b[8];

	c[1] = a[1]*b[0] + a[4]*b[1] + a[7]*b[2];
	c[4] = a[1]*b[3] + a[4]*b[4] + a[7]*b[5];
	c[7] = a[1]*b[6] + a[4]*b[7] + a[7]*b[8];

	c[2] = a[2]*b[0] + a[5]*b[1] + a[8]*b[2];
	c[5] = a[2]*b[3] + a[5]*b[4] + a[8]*b[5];
	c[8] = a[2]*b[6] + a[5]*b[7] + a[8]*b[8];
#else
	c[0] = a[0]*b[0] + a[1]*b[3] + a[2]*b[6];
	c[1] = a[0]*b[1] + a[1]*b[4] + a[2]*b[7];
	c[2] = a[0]*b[2] + a[1]*b[5] + a[2]*b[8];

	c[3] = a[3]*b[0] + a[4]*b[3] + a[5]*b[6];
	c[4] = a[3]*b[1] + a[4]*b[4] + a[5]*b[7];
	c[5] = a[3]*b[2] + a[4]*b[5] + a[5]*b[8];

	c[6] = a[6]*b[0] + a[7]*b[3] + a[8]*b[6];
	c[7] = a[6]*b[1] + a[7]*b[4] + a[8]*b[7];
	c[8] = a[6]*b[2] + a[7]*b[5] + a[8]*b[8];
#endif
}

void load_rotation_m3(mat3 mat, vec3 v, float angle)
{
	float s, c;
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	s = sin(angle);
	c = cos(angle);

	// Rotation matrix is normalized
	normalize_v3(&v);

	xx = v.x * v.x;
	yy = v.y * v.y;
	zz = v.z * v.z;
	xy = v.x * v.y;
	yz = v.y * v.z;
	zx = v.z * v.x;
	xs = v.x * s;
	ys = v.y * s;
	zs = v.z * s;
	one_c = 1.0f - c;

#ifndef ROW_MAJOR
	mat[0] = (one_c * xx) + c;
	mat[3] = (one_c * xy) - zs;
	mat[6] = (one_c * zx) + ys;

	mat[1] = (one_c * xy) + zs;
	mat[4] = (one_c * yy) + c;
	mat[7] = (one_c * yz) - xs;

	mat[2] = (one_c * zx) - ys;
	mat[5] = (one_c * yz) + xs;
	mat[8] = (one_c * zz) + c;
#else
	mat[0] = (one_c * xx) + c;
	mat[1] = (one_c * xy) - zs;
	mat[2] = (one_c * zx) + ys;

	mat[3] = (one_c * xy) + zs;
	mat[4] = (one_c * yy) + c;
	mat[5] = (one_c * yz) - xs;

	mat[6] = (one_c * zx) - ys;
	mat[7] = (one_c * yz) + xs;
	mat[8] = (one_c * zz) + c;
#endif
}



/*
 * mat4
 */

//TODO use restrict?
void mult_m4_m4(mat4 c, mat4 a, mat4 b)
{
#ifndef ROW_MAJOR
	c[ 0] = a[0]*b[ 0] + a[4]*b[ 1] + a[8]*b[ 2] + a[12]*b[ 3];
	c[ 4] = a[0]*b[ 4] + a[4]*b[ 5] + a[8]*b[ 6] + a[12]*b[ 7];
	c[ 8] = a[0]*b[ 8] + a[4]*b[ 9] + a[8]*b[10] + a[12]*b[11];
	c[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14] + a[12]*b[15];

	c[ 1] = a[1]*b[ 0] + a[5]*b[ 1] + a[9]*b[ 2] + a[13]*b[ 3];
	c[ 5] = a[1]*b[ 4] + a[5]*b[ 5] + a[9]*b[ 6] + a[13]*b[ 7];
	c[ 9] = a[1]*b[ 8] + a[5]*b[ 9] + a[9]*b[10] + a[13]*b[11];
	c[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14] + a[13]*b[15];

	c[ 2] = a[2]*b[ 0] + a[6]*b[ 1] + a[10]*b[ 2] + a[14]*b[ 3];
	c[ 6] = a[2]*b[ 4] + a[6]*b[ 5] + a[10]*b[ 6] + a[14]*b[ 7];
	c[10] = a[2]*b[ 8] + a[6]*b[ 9] + a[10]*b[10] + a[14]*b[11];
	c[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];

	c[ 3] = a[3]*b[ 0] + a[7]*b[ 1] + a[11]*b[ 2] + a[15]*b[ 3];
	c[ 7] = a[3]*b[ 4] + a[7]*b[ 5] + a[11]*b[ 6] + a[15]*b[ 7];
	c[11] = a[3]*b[ 8] + a[7]*b[ 9] + a[11]*b[10] + a[15]*b[11];
	c[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];

#else
	c[0] = a[0]*b[0] + a[1]*b[4] + a[2]*b[8] + a[3]*b[12];
	c[1] = a[0]*b[1] + a[1]*b[5] + a[2]*b[9] + a[3]*b[13];
	c[2] = a[0]*b[2] + a[1]*b[6] + a[2]*b[10] + a[3]*b[14];
	c[3] = a[0]*b[3] + a[1]*b[7] + a[2]*b[11] + a[3]*b[15];

	c[4] = a[4]*b[0] + a[5]*b[4] + a[6]*b[8] + a[7]*b[12];
	c[5] = a[4]*b[1] + a[5]*b[5] + a[6]*b[9] + a[7]*b[13];
	c[6] = a[4]*b[2] + a[5]*b[6] + a[6]*b[10] + a[7]*b[14];
	c[7] = a[4]*b[3] + a[5]*b[7] + a[6]*b[11] + a[7]*b[15];

	c[ 8] = a[8]*b[0] + a[9]*b[4] + a[10]*b[8] + a[11]*b[12];
	c[ 9] = a[8]*b[1] + a[9]*b[5] + a[10]*b[9] + a[11]*b[13];
	c[10] = a[8]*b[2] + a[9]*b[6] + a[10]*b[10] + a[11]*b[14];
	c[11] = a[8]*b[3] + a[9]*b[7] + a[10]*b[11] + a[11]*b[15];

	c[12] = a[12]*b[0] + a[13]*b[4] + a[14]*b[8] + a[15]*b[12];
	c[13] = a[12]*b[1] + a[13]*b[5] + a[14]*b[9] + a[15]*b[13];
	c[14] = a[12]*b[2] + a[13]*b[6] + a[14]*b[10] + a[15]*b[14];
	c[15] = a[12]*b[3] + a[13]*b[7] + a[14]*b[11] + a[15]*b[15];
#endif
}

void load_rotation_m4(mat4 mat, vec3 v, float angle)
{
	float s, c;
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	s = sin(angle);
	c = cos(angle);

	// Rotation matrix is normalized
	normalize_v3(&v);

	xx = v.x * v.x;
	yy = v.y * v.y;
	zz = v.z * v.z;
	xy = v.x * v.y;
	yz = v.y * v.z;
	zx = v.z * v.x;
	xs = v.x * s;
	ys = v.y * s;
	zs = v.z * s;
	one_c = 1.0f - c;

#ifndef ROW_MAJOR
	mat[ 0] = (one_c * xx) + c;
	mat[ 4] = (one_c * xy) - zs;
	mat[ 8] = (one_c * zx) + ys;
	mat[12] = 0.0f;

	mat[ 1] = (one_c * xy) + zs;
	mat[ 5] = (one_c * yy) + c;
	mat[ 9] = (one_c * yz) - xs;
	mat[13] = 0.0f;

	mat[ 2] = (one_c * zx) - ys;
	mat[ 6] = (one_c * yz) + xs;
	mat[10] = (one_c * zz) + c;
	mat[14] = 0.0f;

	mat[ 3] = 0.0f;
	mat[ 7] = 0.0f;
	mat[11] = 0.0f;
	mat[15] = 1.0f;
#else
	mat[0] = (one_c * xx) + c;
	mat[1] = (one_c * xy) - zs;
	mat[2] = (one_c * zx) + ys;
	mat[3] = 0.0f;

	mat[4] = (one_c * xy) + zs;
	mat[5] = (one_c * yy) + c;
	mat[6] = (one_c * yz) - xs;
	mat[7] = 0.0f;

	mat[8] = (one_c * zx) - ys;
	mat[9] = (one_c * yz) + xs;
	mat[10] = (one_c * zz) + c;
	mat[11] = 0.0f;

	mat[12] = 0.0f;
	mat[13] = 0.0f;
	mat[14] = 0.0f;
	mat[15] = 1.0f;
#endif
}


/* TODO
static float det_ij(const mat4 m, const int i, const int j)
{
	float ret, mat[3][3];
	int x = 0, y = 0;

	for (int ii=0; ii<4; ++ii) {
		y = 0;
		if (ii == i) continue;
		for (int jj=0; jj<4; ++jj) {
			if (jj == j) continue;
			mat[x][y] = m[ii*4+jj];
			y++;
		}
		x++;
	}



	ret =  mat[0][0]*(mat[1][1]*mat[2][2]-mat[2][1]*mat[1][2]);
	ret -= mat[0][1]*(mat[1][0]*mat[2][2]-mat[2][0]*mat[1][2]);
	ret += mat[0][2]*(mat[1][0]*mat[2][1]-mat[2][0]*mat[1][1]);

	return ret;
}


void invert_m4(mat4 mInverse, const mat4& m)
{
	int i, j;
	float det, detij;
	mat4 inverse_mat;

	// calculate 4x4 determinant
	det = 0.0f;
	for (i = 0; i < 4; i++) {
		det += (i & 0x1) ? (-m.matrix[i] * det_ij(m, 0, i)) : (m.matrix[i] * det_ij(m, 0, i));
	}
	det = 1.0f / det;

	// calculate inverse
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			detij = det_ij(m, j, i);
			inverse_mat[(i*4)+j] = ((i+j) & 0x1) ? (-detij * det) : (detij *det);
		}
	}

}


*/


////////////////////////////////////////////////////////////////////////////////////////////

//assumes converting from canonical view volume [-1,1]^3
//works just like glViewport, x and y are lower left corner.  opengl should be 1.
void make_viewport_m4(mat4 mat, int x, int y, unsigned int width, unsigned int height, int opengl)
{
	float w, h, l, t, b, r;

	if (opengl) {
		//See glspec page 104, integer grid is lower left pixel corners
		w = width, h = height;
		l = x, b = y;
		//range is [l, l+w) x [b , b+h)
		//TODO pick best epsilon?
		r = l + w - 0.01; //epsilon larger than float precision
		t = b + h - 0.01;

#ifndef ROW_MAJOR
		mat[ 0] = (r - l) / 2;
		mat[ 4] = 0;
		mat[ 8] = 0;
		mat[12] = (l + r) / 2;

		mat[ 1] = 0;
		//see below
		mat[ 5] = (t - b) / 2;
		mat[ 9] = 0;
		mat[13] = (b + t) / 2;

		mat[ 2] = 0;
		mat[ 6] = 0;
		mat[10] = 1;
		mat[14] = 0;

		mat[ 3] = 0;
		mat[ 7] = 0;
		mat[11] = 0;
		mat[15] = 1;
#else
		mat[0] = (r - l) / 2;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = (l + r) / 2;

		mat[4] = 0;
		//this used to be negative to flip y till I changed glFramebuffer and draw_pixel to accomplish the same thing
		mat[5] = (t - b) / 2;
		mat[6] = 0;
		mat[7] = (b + t) / 2;

		mat[8] = 0;
		mat[9] = 0;
		mat[10] = 1;
		mat[11] = 0;

		mat[12] = 0;
		mat[13] = 0;
		mat[14] = 0;
		mat[15] = 1;
#endif

	} else {
		//old way with pixel centers at integer coordinates
		//see pages 133/4 and 144 of FoCG
		//necessary for fast integer only bresenham line drawing

		w = width, h = height;
		l = x - 0.5f;
		b = y - 0.5f;
		r = l + w;
		t = b + h;

#ifndef ROW_MAJOR
		mat[ 0] = (r - l) / 2;
		mat[ 4] = 0;
		mat[ 8] = 0;
		mat[12] = (l + r) / 2;

		mat[ 1] = 0;
		//see below
		mat[ 5] = (t - b) / 2;
		mat[ 9] = 0;
		mat[13] = (b + t) / 2;

		mat[ 2] = 0;
		mat[ 6] = 0;
		mat[10] = 1;
		mat[14] = 0;

		mat[ 3] = 0;
		mat[ 7] = 0;
		mat[11] = 0;
		mat[15] = 1;
#else
		mat[0] = (r - l) / 2;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = (l + r) / 2;

		mat[4] = 0;
		//make this negative to reflect y otherwise positive y maps to lower half of the screen
		//this is mapping the unit square [-1,1]^2 to the window size. x is fine because it increases left to right
		//but the screen coordinates (ie framebuffer memory) increase top to bottom opposite of the canonical square
		//negating this is the easiest way to fix it without any side effects.
		mat[5] = (t - b) / 2;
		mat[6] = 0;
		mat[7] = (b + t) / 2;

		mat[8] = 0;
		mat[9] = 0;
		mat[10] = 1;
		mat[11] = 0;

		mat[12] = 0;
		mat[13] = 0;
		mat[14] = 0;
		mat[15] = 1;
#endif
	}
}

//I can't really think of any reason to ever use this matrix alone.
//You'd always do ortho * pers and really if you're doing perspective projection
//just use make_perspective_matrix (or less likely make perspective_proj_matrix)
//
//This function is really just for completeness sake based off of FoCG 3rd edition pg 152
//changed slightly.  z_near and z_far are always positive and z_near < z_far
//
//Inconsistently, to generate an ortho matrix to multiply with that will get the equivalent
//of the other 2 functions you'd use -z_near and -z_far and near > far.
void make_pers_m4(mat4 mat, float z_near, float z_far)
{
#ifndef ROW_MAJOR
	mat[ 0] = z_near;
	mat[ 4] = 0;
	mat[ 8] = 0;
	mat[12] = 0;

	mat[ 1] = 0;
	mat[ 5] = z_near;
	mat[ 9] = 0;
	mat[13] = 0;

	mat[ 2] = 0;
	mat[ 6] = 0;
	mat[10] = z_near + z_far;
	mat[14] = (z_far * z_near);

	mat[ 3] = 0;
	mat[ 7] = 0;
	mat[11] = -1;
	mat[15] = 0;
#else
	mat[0] = z_near;
	mat[1] = 0;
	mat[2] = 0;
	mat[3] = 0;

	mat[4] = 0;
	mat[5] = z_near;
	mat[6] = 0;
	mat[7] = 0;

	mat[ 8] = 0;
	mat[ 9] = 0;
	mat[10] = z_near + z_far;
	mat[11] = (z_far * z_near);

	mat[12] = 0;
	mat[13] = 0;
	mat[14] = -1;
	mat[15] = 0;
#endif
}

// Create a projection matrix
// Similiar to the old gluPerspective... fov is in radians btw...
void make_perspective_m4(mat4 mat, float fov, float aspect, float n, float f)
{
	float t = n * tanf(fov * 0.5f);
	float b = -t;
	float l = b * aspect;
	float r = -l;

	make_perspective_proj_m4(mat, l, r, b, t, n, f);

}

void make_perspective_proj_m4(mat4 mat, float l, float r, float b, float t, float n, float f)
{
#ifndef ROW_MAJOR
	mat[ 0] = (2.0f * n) / (r - l);
	mat[ 4] = 0.0f;
	mat[ 8] = (r + l) / (r - l);
	mat[12] = 0.0f;

	mat[ 1] = 0.0f;
	mat[ 5] = (2.0f * n) / (t - b);
	mat[ 9] = (t + b) / (t - b);
	mat[13] = 0.0f;

	mat[ 2] = 0.0f;
	mat[ 6] = 0.0f;
	mat[10] = -((f + n) / (f - n));
	mat[14] = -((2.0f * (f*n))/(f - n));

	mat[ 3] = 0.0f;
	mat[ 7] = 0.0f;
	mat[11] = -1.0f;
	mat[15] = 0.0f;
#else
	mat[0] = (2.0f * n) / (r - l);
	mat[1] = 0.0f;
	mat[2] = (r + l) / (r - l);
	mat[3] = 0.0f;

	mat[4] = 0.0f;
	mat[5] = (2.0f * n) / (t - b);
	mat[6] = (t + b) / (t - b);
	mat[7] = 0.0f;

	mat[8] = 0.0f;
	mat[9] = 0.0f;
	mat[10] = -((f + n) / (f - n));
	mat[11] = -((2.0f * (f*n))/(f - n));

	mat[12] = 0.0f;
	mat[13] = 0.0f;
	mat[14] = -1.0f;
	mat[15] = 0.0f;
#endif
}

//n and f really are near and far not min and max so if you want the standard looking down the -z axis
// then n > f otherwise n < f
void make_orthographic_m4(mat4 mat, float l, float r, float b, float t, float n, float f)
{
#ifndef ROW_MAJOR
	mat[ 0] = 2.0f / (r - l);
	mat[ 4] = 0;
	mat[ 8] = 0;
	mat[12] = -((r + l)/(r - l));

	mat[ 1] = 0;
	mat[ 5] = 2.0f / (t - b);
	mat[ 9] = 0;
	mat[13] = -((t + b)/(t - b));

	mat[ 2] = 0;
	mat[ 6] = 0;
	mat[10] = 2.0f / (f - n);  //removed - in front of 2 . . . book doesn't have it but superbible did
	mat[14] = -((n + f)/(f - n));

	mat[ 3] = 0;
	mat[ 7] = 0;
	mat[11] = 0;
	mat[15] = 1;
#else
	mat[0] = 2.0f / (r - l);
	mat[1] = 0;
	mat[2] = 0;
	mat[3] = -((r + l)/(r - l));
	mat[4] = 0;
	mat[5] = 2.0f / (t - b);
	mat[6] = 0;
	mat[7] = -((t + b)/(t - b));
	mat[8] = 0;
	mat[9] = 0;
	mat[10] = 2.0f / (f - n);  //removed - in front of 2 . . . book doesn't have it but superbible did
	mat[11] = -((n + f)/(f - n));
	mat[12] = 0;
	mat[13] = 0;
	mat[14] = 0;
	mat[15] = 1;
#endif


	//now I know why the superbible had the -
	//OpenGL uses a left handed canonical view volume [-1,1]^3 when passed the identity matrix
	//ie in Normalized Device Coordinates.  The math/matrix presented in Fundamentals of Computer
	//Graphics assumes a right handed version of the same volume.  The negative isn't necessary
	//if you set n and f correctly as near and far not low and high
}

//per https://www.opengl.org/sdk/docs/man2/xhtml/gluLookAt.xml
//and glm.g-truc.net (glm/gtc/matrix_transform.inl)
void lookAt(mat4 mat, vec3 eye, vec3 center, vec3 up)
{
	SET_IDENTITY_M4(mat);

	vec3 f = norm_v3(sub_v3s(center, eye));
	vec3 s = norm_v3(cross_v3s(f, up));
	vec3 u = cross_v3s(s, f);

	setx_m4v3(mat, s);
	sety_m4v3(mat, u);
	setz_m4v3(mat, neg_v3(f));
	setc4_m4v3(mat, make_v3(-dot_v3s(s, eye), -dot_v3s(u, eye), dot_v3s(f, eye)));
}

extern inline float rsw_randf(void);
extern inline float rsw_randf_range(float min, float max);
extern inline double rsw_map(double x, double a, double b, double c, double d);
extern inline float rsw_mapf(float x, float a, float b, float c, float d);

extern inline Color make_Color(u8 red, u8 green, u8 blue, u8 alpha);
extern inline Color v4_to_Color(vec4 v);
extern inline void print_Color(Color c, const char* append);
extern inline vec4 Color_to_v4(Color c);
extern inline Line make_Line(float x1, float y1, float x2, float y2);
extern inline void normalize_line(Line* line);
extern inline float line_func(Line* line, float x, float y);
extern inline float line_findy(Line* line, float x);
extern inline float line_findx(Line* line, float y);
extern inline float sq_dist_pt_segment2d(vec2 a, vec2 b, vec2 c);
extern inline void closest_pt_pt_segment(vec2 c, vec2 a, vec2 b, float* t, vec2* d);
extern inline float closest_pt_pt_segment_t(vec2 c, vec2 a, vec2 b);


#if defined(CVEC_MALLOC) && defined(CVEC_FREE) && defined(CVEC_REALLOC)
/* ok */
#elif !defined(CVEC_MALLOC) && !defined(CVEC_FREE) && !defined(CVEC_REALLOC)
/* ok */
#else
#error "Must define all or none of CVEC_MALLOC, CVEC_FREE, and CVEC_REALLOC."
#endif

#ifndef CVEC_MALLOC
#include <stdlib.h>
#define CVEC_MALLOC(sz)      malloc(sz)
#define CVEC_REALLOC(p, sz)  realloc(p, sz)
#define CVEC_FREE(p)         free(p)
#endif

#ifndef CVEC_MEMMOVE
#include <string.h>
#define CVEC_MEMMOVE(dst, src, sz)  memmove(dst, src, sz)
#endif

#ifndef CVEC_ASSERT
#include <assert.h>
#define CVEC_ASSERT(x)       assert(x)
#endif

cvec_sz CVEC_glVertex_Array_SZ = 50;

#define CVEC_glVertex_Array_ALLOCATOR(x) ((x+1) * 2)

cvector_glVertex_Array* cvec_glVertex_Array_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glVertex_Array* vec;
	if (!(vec = (cvector_glVertex_Array*)CVEC_MALLOC(sizeof(cvector_glVertex_Array)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glVertex_Array_SZ;

	if (!(vec->a = (glVertex_Array*)CVEC_MALLOC(vec->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glVertex_Array* cvec_init_glVertex_Array_heap(glVertex_Array* vals, cvec_sz num)
{
	cvector_glVertex_Array* vec;
	
	if (!(vec = (cvector_glVertex_Array*)CVEC_MALLOC(sizeof(cvector_glVertex_Array)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glVertex_Array_SZ;
	vec->size = num;
	if (!(vec->a = (glVertex_Array*)CVEC_MALLOC(vec->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glVertex_Array)*num);

	return vec;
}

int cvec_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glVertex_Array_SZ;

	if (!(vec->a = (glVertex_Array*)CVEC_MALLOC(vec->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glVertex_Array_SZ;
	vec->size = num;
	if (!(vec->a = (glVertex_Array*)CVEC_MALLOC(vec->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glVertex_Array)*num);

	return 1;
}

int cvec_copyc_glVertex_Array(void* dest, void* src)
{
	cvector_glVertex_Array* vec1 = (cvector_glVertex_Array*)dest;
	cvector_glVertex_Array* vec2 = (cvector_glVertex_Array*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glVertex_Array(vec1, vec2);
}

int cvec_copy_glVertex_Array(cvector_glVertex_Array* dest, cvector_glVertex_Array* src)
{
	glVertex_Array* tmp = NULL;
	if (!(tmp = (glVertex_Array*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glVertex_Array)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glVertex_Array));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array a)
{
	glVertex_Array* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glVertex_Array_ALLOCATOR(vec->capacity);
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glVertex_Array cvec_pop_glVertex_Array(cvector_glVertex_Array* vec)
{
	return vec->a[--vec->size];
}

glVertex_Array* cvec_back_glVertex_Array(cvector_glVertex_Array* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz num)
{
	glVertex_Array* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glVertex_Array_SZ;
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz i, glVertex_Array a)
{
	glVertex_Array* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glVertex_Array));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glVertex_Array_ALLOCATOR(vec->capacity);
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glVertex_Array));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz i, glVertex_Array* a, cvec_sz num)
{
	glVertex_Array* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glVertex_Array_SZ;
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glVertex_Array));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glVertex_Array));
	vec->size += num;
	return 1;
}

glVertex_Array cvec_replace_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz i, glVertex_Array a)
{
	glVertex_Array tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glVertex_Array));
	vec->size -= d;
}


int cvec_reserve_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz size)
{
	glVertex_Array* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*(size+CVEC_glVertex_Array_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glVertex_Array_SZ;
	}
	return 1;
}

int cvec_set_cap_glVertex_Array(cvector_glVertex_Array* vec, cvec_sz size)
{
	glVertex_Array* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glVertex_Array*)CVEC_REALLOC(vec->a, sizeof(glVertex_Array)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glVertex_Array(cvector_glVertex_Array* vec, glVertex_Array val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glVertex_Array(cvector_glVertex_Array* vec) { vec->size = 0; }

void cvec_free_glVertex_Array_heap(void* vec)
{
	cvector_glVertex_Array* tmp = (cvector_glVertex_Array*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glVertex_Array(void* vec)
{
	cvector_glVertex_Array* tmp = (cvector_glVertex_Array*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}


cvec_sz CVEC_glBuffer_SZ = 50;

#define CVEC_glBuffer_ALLOCATOR(x) ((x+1) * 2)

cvector_glBuffer* cvec_glBuffer_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glBuffer* vec;
	if (!(vec = (cvector_glBuffer*)CVEC_MALLOC(sizeof(cvector_glBuffer)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glBuffer_SZ;

	if (!(vec->a = (glBuffer*)CVEC_MALLOC(vec->capacity*sizeof(glBuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glBuffer* cvec_init_glBuffer_heap(glBuffer* vals, cvec_sz num)
{
	cvector_glBuffer* vec;
	
	if (!(vec = (cvector_glBuffer*)CVEC_MALLOC(sizeof(cvector_glBuffer)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glBuffer_SZ;
	vec->size = num;
	if (!(vec->a = (glBuffer*)CVEC_MALLOC(vec->capacity*sizeof(glBuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glBuffer)*num);

	return vec;
}

int cvec_glBuffer(cvector_glBuffer* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glBuffer_SZ;

	if (!(vec->a = (glBuffer*)CVEC_MALLOC(vec->capacity*sizeof(glBuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glBuffer(cvector_glBuffer* vec, glBuffer* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glBuffer_SZ;
	vec->size = num;
	if (!(vec->a = (glBuffer*)CVEC_MALLOC(vec->capacity*sizeof(glBuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glBuffer)*num);

	return 1;
}

int cvec_copyc_glBuffer(void* dest, void* src)
{
	cvector_glBuffer* vec1 = (cvector_glBuffer*)dest;
	cvector_glBuffer* vec2 = (cvector_glBuffer*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glBuffer(vec1, vec2);
}

int cvec_copy_glBuffer(cvector_glBuffer* dest, cvector_glBuffer* src)
{
	glBuffer* tmp = NULL;
	if (!(tmp = (glBuffer*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glBuffer)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glBuffer));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glBuffer(cvector_glBuffer* vec, glBuffer a)
{
	glBuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glBuffer_ALLOCATOR(vec->capacity);
		if (!(tmp = (glBuffer*)CVEC_REALLOC(vec->a, sizeof(glBuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glBuffer cvec_pop_glBuffer(cvector_glBuffer* vec)
{
	return vec->a[--vec->size];
}

glBuffer* cvec_back_glBuffer(cvector_glBuffer* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glBuffer(cvector_glBuffer* vec, cvec_sz num)
{
	glBuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glBuffer_SZ;
		if (!(tmp = (glBuffer*)CVEC_REALLOC(vec->a, sizeof(glBuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glBuffer(cvector_glBuffer* vec, cvec_sz i, glBuffer a)
{
	glBuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glBuffer));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glBuffer_ALLOCATOR(vec->capacity);
		if (!(tmp = (glBuffer*)CVEC_REALLOC(vec->a, sizeof(glBuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glBuffer));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glBuffer(cvector_glBuffer* vec, cvec_sz i, glBuffer* a, cvec_sz num)
{
	glBuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glBuffer_SZ;
		if (!(tmp = (glBuffer*)CVEC_REALLOC(vec->a, sizeof(glBuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glBuffer));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glBuffer));
	vec->size += num;
	return 1;
}

glBuffer cvec_replace_glBuffer(cvector_glBuffer* vec, cvec_sz i, glBuffer a)
{
	glBuffer tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glBuffer(cvector_glBuffer* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glBuffer));
	vec->size -= d;
}


int cvec_reserve_glBuffer(cvector_glBuffer* vec, cvec_sz size)
{
	glBuffer* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glBuffer*)CVEC_REALLOC(vec->a, sizeof(glBuffer)*(size+CVEC_glBuffer_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glBuffer_SZ;
	}
	return 1;
}

int cvec_set_cap_glBuffer(cvector_glBuffer* vec, cvec_sz size)
{
	glBuffer* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glBuffer*)CVEC_REALLOC(vec->a, sizeof(glBuffer)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glBuffer(cvector_glBuffer* vec, glBuffer val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glBuffer(cvector_glBuffer* vec, glBuffer val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glBuffer(cvector_glBuffer* vec) { vec->size = 0; }

void cvec_free_glBuffer_heap(void* vec)
{
	cvector_glBuffer* tmp = (cvector_glBuffer*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glBuffer(void* vec)
{
	cvector_glBuffer* tmp = (cvector_glBuffer*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}


cvec_sz CVEC_glTexture_SZ = 50;

#define CVEC_glTexture_ALLOCATOR(x) ((x+1) * 2)

cvector_glTexture* cvec_glTexture_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glTexture* vec;
	if (!(vec = (cvector_glTexture*)CVEC_MALLOC(sizeof(cvector_glTexture)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glTexture_SZ;

	if (!(vec->a = (glTexture*)CVEC_MALLOC(vec->capacity*sizeof(glTexture)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glTexture* cvec_init_glTexture_heap(glTexture* vals, cvec_sz num)
{
	cvector_glTexture* vec;
	
	if (!(vec = (cvector_glTexture*)CVEC_MALLOC(sizeof(cvector_glTexture)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glTexture_SZ;
	vec->size = num;
	if (!(vec->a = (glTexture*)CVEC_MALLOC(vec->capacity*sizeof(glTexture)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glTexture)*num);

	return vec;
}

int cvec_glTexture(cvector_glTexture* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glTexture_SZ;

	if (!(vec->a = (glTexture*)CVEC_MALLOC(vec->capacity*sizeof(glTexture)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glTexture(cvector_glTexture* vec, glTexture* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glTexture_SZ;
	vec->size = num;
	if (!(vec->a = (glTexture*)CVEC_MALLOC(vec->capacity*sizeof(glTexture)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glTexture)*num);

	return 1;
}

int cvec_copyc_glTexture(void* dest, void* src)
{
	cvector_glTexture* vec1 = (cvector_glTexture*)dest;
	cvector_glTexture* vec2 = (cvector_glTexture*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glTexture(vec1, vec2);
}

int cvec_copy_glTexture(cvector_glTexture* dest, cvector_glTexture* src)
{
	glTexture* tmp = NULL;
	if (!(tmp = (glTexture*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glTexture)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glTexture));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glTexture(cvector_glTexture* vec, glTexture a)
{
	glTexture* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glTexture_ALLOCATOR(vec->capacity);
		if (!(tmp = (glTexture*)CVEC_REALLOC(vec->a, sizeof(glTexture)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glTexture cvec_pop_glTexture(cvector_glTexture* vec)
{
	return vec->a[--vec->size];
}

glTexture* cvec_back_glTexture(cvector_glTexture* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glTexture(cvector_glTexture* vec, cvec_sz num)
{
	glTexture* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glTexture_SZ;
		if (!(tmp = (glTexture*)CVEC_REALLOC(vec->a, sizeof(glTexture)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glTexture(cvector_glTexture* vec, cvec_sz i, glTexture a)
{
	glTexture* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glTexture));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glTexture_ALLOCATOR(vec->capacity);
		if (!(tmp = (glTexture*)CVEC_REALLOC(vec->a, sizeof(glTexture)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glTexture));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glTexture(cvector_glTexture* vec, cvec_sz i, glTexture* a, cvec_sz num)
{
	glTexture* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glTexture_SZ;
		if (!(tmp = (glTexture*)CVEC_REALLOC(vec->a, sizeof(glTexture)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glTexture));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glTexture));
	vec->size += num;
	return 1;
}

glTexture cvec_replace_glTexture(cvector_glTexture* vec, cvec_sz i, glTexture a)
{
	glTexture tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glTexture(cvector_glTexture* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glTexture));
	vec->size -= d;
}


int cvec_reserve_glTexture(cvector_glTexture* vec, cvec_sz size)
{
	glTexture* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glTexture*)CVEC_REALLOC(vec->a, sizeof(glTexture)*(size+CVEC_glTexture_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glTexture_SZ;
	}
	return 1;
}

int cvec_set_cap_glTexture(cvector_glTexture* vec, cvec_sz size)
{
	glTexture* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glTexture*)CVEC_REALLOC(vec->a, sizeof(glTexture)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glTexture(cvector_glTexture* vec, glTexture val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glTexture(cvector_glTexture* vec, glTexture val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glTexture(cvector_glTexture* vec) { vec->size = 0; }

void cvec_free_glTexture_heap(void* vec)
{
	cvector_glTexture* tmp = (cvector_glTexture*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glTexture(void* vec)
{
	cvector_glTexture* tmp = (cvector_glTexture*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}


cvec_sz CVEC_glProgram_SZ = 50;

#define CVEC_glProgram_ALLOCATOR(x) ((x+1) * 2)

cvector_glProgram* cvec_glProgram_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glProgram* vec;
	if (!(vec = (cvector_glProgram*)CVEC_MALLOC(sizeof(cvector_glProgram)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glProgram_SZ;

	if (!(vec->a = (glProgram*)CVEC_MALLOC(vec->capacity*sizeof(glProgram)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glProgram* cvec_init_glProgram_heap(glProgram* vals, cvec_sz num)
{
	cvector_glProgram* vec;
	
	if (!(vec = (cvector_glProgram*)CVEC_MALLOC(sizeof(cvector_glProgram)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glProgram_SZ;
	vec->size = num;
	if (!(vec->a = (glProgram*)CVEC_MALLOC(vec->capacity*sizeof(glProgram)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glProgram)*num);

	return vec;
}

int cvec_glProgram(cvector_glProgram* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glProgram_SZ;

	if (!(vec->a = (glProgram*)CVEC_MALLOC(vec->capacity*sizeof(glProgram)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glProgram(cvector_glProgram* vec, glProgram* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glProgram_SZ;
	vec->size = num;
	if (!(vec->a = (glProgram*)CVEC_MALLOC(vec->capacity*sizeof(glProgram)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glProgram)*num);

	return 1;
}

int cvec_copyc_glProgram(void* dest, void* src)
{
	cvector_glProgram* vec1 = (cvector_glProgram*)dest;
	cvector_glProgram* vec2 = (cvector_glProgram*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glProgram(vec1, vec2);
}

int cvec_copy_glProgram(cvector_glProgram* dest, cvector_glProgram* src)
{
	glProgram* tmp = NULL;
	if (!(tmp = (glProgram*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glProgram)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glProgram));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glProgram(cvector_glProgram* vec, glProgram a)
{
	glProgram* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glProgram_ALLOCATOR(vec->capacity);
		if (!(tmp = (glProgram*)CVEC_REALLOC(vec->a, sizeof(glProgram)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glProgram cvec_pop_glProgram(cvector_glProgram* vec)
{
	return vec->a[--vec->size];
}

glProgram* cvec_back_glProgram(cvector_glProgram* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glProgram(cvector_glProgram* vec, cvec_sz num)
{
	glProgram* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glProgram_SZ;
		if (!(tmp = (glProgram*)CVEC_REALLOC(vec->a, sizeof(glProgram)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glProgram(cvector_glProgram* vec, cvec_sz i, glProgram a)
{
	glProgram* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glProgram));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glProgram_ALLOCATOR(vec->capacity);
		if (!(tmp = (glProgram*)CVEC_REALLOC(vec->a, sizeof(glProgram)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glProgram));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glProgram(cvector_glProgram* vec, cvec_sz i, glProgram* a, cvec_sz num)
{
	glProgram* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glProgram_SZ;
		if (!(tmp = (glProgram*)CVEC_REALLOC(vec->a, sizeof(glProgram)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glProgram));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glProgram));
	vec->size += num;
	return 1;
}

glProgram cvec_replace_glProgram(cvector_glProgram* vec, cvec_sz i, glProgram a)
{
	glProgram tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glProgram(cvector_glProgram* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glProgram));
	vec->size -= d;
}


int cvec_reserve_glProgram(cvector_glProgram* vec, cvec_sz size)
{
	glProgram* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glProgram*)CVEC_REALLOC(vec->a, sizeof(glProgram)*(size+CVEC_glProgram_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glProgram_SZ;
	}
	return 1;
}

int cvec_set_cap_glProgram(cvector_glProgram* vec, cvec_sz size)
{
	glProgram* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glProgram*)CVEC_REALLOC(vec->a, sizeof(glProgram)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glProgram(cvector_glProgram* vec, glProgram val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glProgram(cvector_glProgram* vec, glProgram val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glProgram(cvector_glProgram* vec) { vec->size = 0; }

void cvec_free_glProgram_heap(void* vec)
{
	cvector_glProgram* tmp = (cvector_glProgram*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glProgram(void* vec)
{
	cvector_glProgram* tmp = (cvector_glProgram*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}


cvec_sz CVEC_glVertex_SZ = 50;

#define CVEC_glVertex_ALLOCATOR(x) ((x+1) * 2)

cvector_glVertex* cvec_glVertex_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glVertex* vec;
	if (!(vec = (cvector_glVertex*)CVEC_MALLOC(sizeof(cvector_glVertex)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glVertex_SZ;

	if (!(vec->a = (glVertex*)CVEC_MALLOC(vec->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glVertex* cvec_init_glVertex_heap(glVertex* vals, cvec_sz num)
{
	cvector_glVertex* vec;
	
	if (!(vec = (cvector_glVertex*)CVEC_MALLOC(sizeof(cvector_glVertex)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glVertex_SZ;
	vec->size = num;
	if (!(vec->a = (glVertex*)CVEC_MALLOC(vec->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glVertex)*num);

	return vec;
}

int cvec_glVertex(cvector_glVertex* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glVertex_SZ;

	if (!(vec->a = (glVertex*)CVEC_MALLOC(vec->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glVertex(cvector_glVertex* vec, glVertex* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glVertex_SZ;
	vec->size = num;
	if (!(vec->a = (glVertex*)CVEC_MALLOC(vec->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glVertex)*num);

	return 1;
}

int cvec_copyc_glVertex(void* dest, void* src)
{
	cvector_glVertex* vec1 = (cvector_glVertex*)dest;
	cvector_glVertex* vec2 = (cvector_glVertex*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glVertex(vec1, vec2);
}

int cvec_copy_glVertex(cvector_glVertex* dest, cvector_glVertex* src)
{
	glVertex* tmp = NULL;
	if (!(tmp = (glVertex*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glVertex)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glVertex));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glVertex(cvector_glVertex* vec, glVertex a)
{
	glVertex* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glVertex_ALLOCATOR(vec->capacity);
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glVertex cvec_pop_glVertex(cvector_glVertex* vec)
{
	return vec->a[--vec->size];
}

glVertex* cvec_back_glVertex(cvector_glVertex* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glVertex(cvector_glVertex* vec, cvec_sz num)
{
	glVertex* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glVertex_SZ;
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex a)
{
	glVertex* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glVertex));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glVertex_ALLOCATOR(vec->capacity);
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glVertex));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex* a, cvec_sz num)
{
	glVertex* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glVertex_SZ;
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glVertex));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glVertex));
	vec->size += num;
	return 1;
}

glVertex cvec_replace_glVertex(cvector_glVertex* vec, cvec_sz i, glVertex a)
{
	glVertex tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glVertex(cvector_glVertex* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glVertex));
	vec->size -= d;
}


int cvec_reserve_glVertex(cvector_glVertex* vec, cvec_sz size)
{
	glVertex* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*(size+CVEC_glVertex_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glVertex_SZ;
	}
	return 1;
}

int cvec_set_cap_glVertex(cvector_glVertex* vec, cvec_sz size)
{
	glVertex* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glVertex*)CVEC_REALLOC(vec->a, sizeof(glVertex)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glVertex(cvector_glVertex* vec, glVertex val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glVertex(cvector_glVertex* vec, glVertex val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glVertex(cvector_glVertex* vec) { vec->size = 0; }

void cvec_free_glVertex_heap(void* vec)
{
	cvector_glVertex* tmp = (cvector_glVertex*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glVertex(void* vec)
{
	cvector_glVertex* tmp = (cvector_glVertex*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}


cvec_sz CVEC_glFBO_SZ = 50;

#define CVEC_glFBO_ALLOCATOR(x) ((x+1) * 2)

cvector_glFBO* cvec_glFBO_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glFBO* vec;
	if (!(vec = (cvector_glFBO*)CVEC_MALLOC(sizeof(cvector_glFBO)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glFBO_SZ;

	if (!(vec->a = (glFBO*)CVEC_MALLOC(vec->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glFBO* cvec_init_glFBO_heap(glFBO* vals, cvec_sz num)
{
	cvector_glFBO* vec;
	
	if (!(vec = (cvector_glFBO*)CVEC_MALLOC(sizeof(cvector_glFBO)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glFBO_SZ;
	vec->size = num;
	if (!(vec->a = (glFBO*)CVEC_MALLOC(vec->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glFBO)*num);

	return vec;
}

int cvec_glFBO(cvector_glFBO* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glFBO_SZ;

	if (!(vec->a = (glFBO*)CVEC_MALLOC(vec->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glFBO(cvector_glFBO* vec, glFBO* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glFBO_SZ;
	vec->size = num;
	if (!(vec->a = (glFBO*)CVEC_MALLOC(vec->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glFBO)*num);

	return 1;
}

int cvec_copyc_glFBO(void* dest, void* src)
{
	cvector_glFBO* vec1 = (cvector_glFBO*)dest;
	cvector_glFBO* vec2 = (cvector_glFBO*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glFBO(vec1, vec2);
}

int cvec_copy_glFBO(cvector_glFBO* dest, cvector_glFBO* src)
{
	glFBO* tmp = NULL;
	if (!(tmp = (glFBO*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glFBO)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glFBO));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glFBO(cvector_glFBO* vec, glFBO a)
{
	glFBO* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glFBO_ALLOCATOR(vec->capacity);
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glFBO cvec_pop_glFBO(cvector_glFBO* vec)
{
	return vec->a[--vec->size];
}

glFBO* cvec_back_glFBO(cvector_glFBO* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glFBO(cvector_glFBO* vec, cvec_sz num)
{
	glFBO* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glFBO_SZ;
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO a)
{
	glFBO* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glFBO));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glFBO_ALLOCATOR(vec->capacity);
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glFBO));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO* a, cvec_sz num)
{
	glFBO* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glFBO_SZ;
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glFBO));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glFBO));
	vec->size += num;
	return 1;
}

glFBO cvec_replace_glFBO(cvector_glFBO* vec, cvec_sz i, glFBO a)
{
	glFBO tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glFBO(cvector_glFBO* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glFBO));
	vec->size -= d;
}


int cvec_reserve_glFBO(cvector_glFBO* vec, cvec_sz size)
{
	glFBO* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*(size+CVEC_glFBO_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glFBO_SZ;
	}
	return 1;
}

int cvec_set_cap_glFBO(cvector_glFBO* vec, cvec_sz size)
{
	glFBO* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glFBO*)CVEC_REALLOC(vec->a, sizeof(glFBO)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glFBO(cvector_glFBO* vec, glFBO val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glFBO(cvector_glFBO* vec, glFBO val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glFBO(cvector_glFBO* vec) { vec->size = 0; }

void cvec_free_glFBO_heap(void* vec)
{
	cvector_glFBO* tmp = (cvector_glFBO*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glFBO(void* vec)
{
	cvector_glFBO* tmp = (cvector_glFBO*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}


cvec_sz CVEC_glRenderbuffer_SZ = 50;

#define CVEC_glRenderbuffer_ALLOCATOR(x) ((x+1) * 2)

cvector_glRenderbuffer* cvec_glRenderbuffer_heap(cvec_sz size, cvec_sz capacity)
{
	cvector_glRenderbuffer* vec;
	if (!(vec = (cvector_glRenderbuffer*)CVEC_MALLOC(sizeof(cvector_glRenderbuffer)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glRenderbuffer_SZ;

	if (!(vec->a = (glRenderbuffer*)CVEC_MALLOC(vec->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	return vec;
}

cvector_glRenderbuffer* cvec_init_glRenderbuffer_heap(glRenderbuffer* vals, cvec_sz num)
{
	cvector_glRenderbuffer* vec;
	
	if (!(vec = (cvector_glRenderbuffer*)CVEC_MALLOC(sizeof(cvector_glRenderbuffer)))) {
		CVEC_ASSERT(vec != NULL);
		return NULL;
	}

	vec->capacity = num + CVEC_glRenderbuffer_SZ;
	vec->size = num;
	if (!(vec->a = (glRenderbuffer*)CVEC_MALLOC(vec->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		CVEC_FREE(vec);
		return NULL;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glRenderbuffer)*num);

	return vec;
}

int cvec_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size, cvec_sz capacity)
{
	vec->size = size;
	vec->capacity = (capacity > vec->size || (vec->size && capacity == vec->size)) ? capacity : vec->size + CVEC_glRenderbuffer_SZ;

	if (!(vec->a = (glRenderbuffer*)CVEC_MALLOC(vec->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	return 1;
}

int cvec_init_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer* vals, cvec_sz num)
{
	vec->capacity = num + CVEC_glRenderbuffer_SZ;
	vec->size = num;
	if (!(vec->a = (glRenderbuffer*)CVEC_MALLOC(vec->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(vec->a != NULL);
		vec->size = vec->capacity = 0;
		return 0;
	}

	CVEC_MEMMOVE(vec->a, vals, sizeof(glRenderbuffer)*num);

	return 1;
}

int cvec_copyc_glRenderbuffer(void* dest, void* src)
{
	cvector_glRenderbuffer* vec1 = (cvector_glRenderbuffer*)dest;
	cvector_glRenderbuffer* vec2 = (cvector_glRenderbuffer*)src;

	vec1->a = NULL;
	vec1->size = 0;
	vec1->capacity = 0;

	return cvec_copy_glRenderbuffer(vec1, vec2);
}

int cvec_copy_glRenderbuffer(cvector_glRenderbuffer* dest, cvector_glRenderbuffer* src)
{
	glRenderbuffer* tmp = NULL;
	if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(dest->a, src->capacity*sizeof(glRenderbuffer)))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	dest->a = tmp;

	CVEC_MEMMOVE(dest->a, src->a, src->size*sizeof(glRenderbuffer));
	dest->size = src->size;
	dest->capacity = src->capacity;
	return 1;
}


int cvec_push_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer a)
{
	glRenderbuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		vec->a[vec->size++] = a;
	} else {
		tmp_sz = CVEC_glRenderbuffer_ALLOCATOR(vec->capacity);
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->a[vec->size++] = a;
		vec->capacity = tmp_sz;
	}
	return 1;
}

glRenderbuffer cvec_pop_glRenderbuffer(cvector_glRenderbuffer* vec)
{
	return vec->a[--vec->size];
}

glRenderbuffer* cvec_back_glRenderbuffer(cvector_glRenderbuffer* vec)
{
	return &vec->a[vec->size-1];
}

int cvec_extend_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz num)
{
	glRenderbuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glRenderbuffer_SZ;
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	vec->size += num;
	return 1;
}

int cvec_insert_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer a)
{
	glRenderbuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity > vec->size) {
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glRenderbuffer));
		vec->a[i] = a;
	} else {
		tmp_sz = CVEC_glRenderbuffer_ALLOCATOR(vec->capacity);
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		CVEC_MEMMOVE(&vec->a[i+1], &vec->a[i], (vec->size-i)*sizeof(glRenderbuffer));
		vec->a[i] = a;
		vec->capacity = tmp_sz;
	}

	vec->size++;
	return 1;
}

int cvec_insert_array_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer* a, cvec_sz num)
{
	glRenderbuffer* tmp;
	cvec_sz tmp_sz;
	if (vec->capacity < vec->size + num) {
		tmp_sz = vec->capacity + num + CVEC_glRenderbuffer_SZ;
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*tmp_sz))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = tmp_sz;
	}

	CVEC_MEMMOVE(&vec->a[i+num], &vec->a[i], (vec->size-i)*sizeof(glRenderbuffer));
	CVEC_MEMMOVE(&vec->a[i], a, num*sizeof(glRenderbuffer));
	vec->size += num;
	return 1;
}

glRenderbuffer cvec_replace_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz i, glRenderbuffer a)
{
	glRenderbuffer tmp = vec->a[i];
	vec->a[i] = a;
	return tmp;
}

void cvec_erase_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz start, cvec_sz end)
{
	cvec_sz d = end - start + 1;
	CVEC_MEMMOVE(&vec->a[start], &vec->a[end+1], (vec->size-1-end)*sizeof(glRenderbuffer));
	vec->size -= d;
}


int cvec_reserve_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size)
{
	glRenderbuffer* tmp;
	if (vec->capacity < size) {
		if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*(size+CVEC_glRenderbuffer_SZ)))) {
			CVEC_ASSERT(tmp != NULL);
			return 0;
		}
		vec->a = tmp;
		vec->capacity = size + CVEC_glRenderbuffer_SZ;
	}
	return 1;
}

int cvec_set_cap_glRenderbuffer(cvector_glRenderbuffer* vec, cvec_sz size)
{
	glRenderbuffer* tmp;
	if (size < vec->size) {
		vec->size = size;
	}

	if (!(tmp = (glRenderbuffer*)CVEC_REALLOC(vec->a, sizeof(glRenderbuffer)*size))) {
		CVEC_ASSERT(tmp != NULL);
		return 0;
	}
	vec->a = tmp;
	vec->capacity = size;
	return 1;
}

void cvec_set_val_sz_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer val)
{
	cvec_sz i;
	for (i=0; i<vec->size; i++) {
		vec->a[i] = val;
	}
}

void cvec_set_val_cap_glRenderbuffer(cvector_glRenderbuffer* vec, glRenderbuffer val)
{
	cvec_sz i;
	for (i=0; i<vec->capacity; i++) {
		vec->a[i] = val;
	}
}

void cvec_clear_glRenderbuffer(cvector_glRenderbuffer* vec) { vec->size = 0; }

void cvec_free_glRenderbuffer_heap(void* vec)
{
	cvector_glRenderbuffer* tmp = (cvector_glRenderbuffer*)vec;
	if (!tmp) return;
	CVEC_FREE(tmp->a);
	CVEC_FREE(tmp);
}

void cvec_free_glRenderbuffer(void* vec)
{
	cvector_glRenderbuffer* tmp = (cvector_glRenderbuffer*)vec;
	CVEC_FREE(tmp->a);
	tmp->size = 0;
	tmp->capacity = 0;
}


static glContext* c;

static Color blend_pixel(vec4 src, vec4 dst);
static int fragment_processing(int x, int y, float z);
static void draw_pixel(vec4 cf, int x, int y, float z, int do_frag_processing);
// MRT-aware: depth/stencil once, then write gl_FragColor or gl_FragData[] to draw buffers
static void draw_fragment(Shader_Builtins* b, int x, int y, int do_frag_processing);
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

			c->glverts.a[i].screen_space = mult_m4_v4(c->vp_mat, c->glverts.a[i].clip_space);

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
	PGL_ASSERT(0 && "ERROR: unrecognized depth test!");
	return 0;
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
		t1 = mult_m4_v4(c->vp_mat, p1);
		t2 = mult_m4_v4(c->vp_mat, p2);

		hp1 = v4_to_v3h(t1);
		hp2 = v4_to_v3h(t2);

		if (c->line_smooth) {
			draw_aa_line(hp1, hp2, t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
		} else {
			draw_thick_line(hp1, hp2, t1.w, t2.w, v1->vs_out, v2->vs_out, provoke, 0.0f);
		}
	} else {

		d = sub_v4s(p2, p1);

		tmin = 0;
		tmax = 1;
		if (clip_line( d.x+d.w, -p1.x-p1.w, &tmin, &tmax) &&
		    clip_line(-d.x+d.w,  p1.x-p1.w, &tmin, &tmax) &&
		    clip_line( d.y+d.w, -p1.y-p1.w, &tmin, &tmax) &&
		    clip_line(-d.y+d.w,  p1.y-p1.w, &tmin, &tmax) &&
		    clip_line( d.z+d.w, -p1.z-p1.w, &tmin, &tmax) &&
		    clip_line(-d.z+d.w,  p1.z-p1.w, &tmin, &tmax)) {

			//printf("%f %f\n", tmin, tmax);

			t1 = add_v4s(p1, scale_v4(d, tmin));
			t2 = add_v4s(p1, scale_v4(d, tmax));

			t1 = mult_m4_v4(c->vp_mat, t1);
			t2 = mult_m4_v4(c->vp_mat, t2);
			//print_v4(t1, "\n");
			//print_v4(t2, "\n");

			interpolate_clipped_line(v1, v2, v1_out, v2_out, tmin, tmax);

			hp1 = v4_to_v3h(t1);
			hp2 = v4_to_v3h(t2);

			if (c->line_smooth) {
				draw_aa_line(hp1, hp2, t1.w, t2.w, v1_out, v2_out, provoke, 0.0f);
			} else {
				draw_thick_line(hp1, hp2, t1.w, t2.w, v1_out, v2_out, provoke, 0.0f);
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
	vec2 pr, sub_p2p1 = sub_v2s(p2, p1);
	float line_length_squared = len_v2(sub_p2p1);
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
			t = dot_v2s(sub_v2s(pr, p1), sub_p2p1) / line_length_squared;
			t = clamp_01(t);

			z = (1 - t) * z1 + t * z2;
			z += poly_offset;
			w = (1 - t) * w1 + t * w2;

			for (float j=x-half_w; j<x+half_w; ++j) {
				if (CLIPXY_TEST(j, y)) {
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
				if (CLIPXY_TEST(x, j)) {
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
				if (CLIPXY_TEST(x, j)) {
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
				if (CLIPXY_TEST(j, y)) {
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
		if (CLIPXY_TEST(x, y+1)) {
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
		if (CLIPXY_TEST(x, y+1)) {
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
			if (CLIPXY_TEST(x, y)) {
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
			if (CLIPXY_TEST(x, y+1)) {
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
		if (CLIPXY_TEST(x+1, y)) {
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
		if (CLIPXY_TEST(x+1, y)) {
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
			if (CLIPXY_TEST(x, y)) {
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
			if (CLIPXY_TEST(x+1, y)) {
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
	v0->screen_space = mult_m4_v4(c->vp_mat, v0->clip_space);
	v1->screen_space = mult_m4_v4(c->vp_mat, v1->clip_space);
	v2->screen_space = mult_m4_v4(c->vp_mat, v2->clip_space);

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
	print_v4(v0->clip_space, "\n");
	print_v4(v1->clip_space, "\n");
	print_v4(v2->clip_space, "\n");
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
			print_v4(v0->clip_space, "\n");
			print_v4(v1->clip_space, "\n");
			print_v4(v2->clip_space, "\n");
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
					SET_V4(builtins.gl_FragCoord, x, y, z, tmp2);
					builtins.discard = GL_FALSE;
					builtins.gl_FragDepth = z;

					// have to do this here instead of outside the loop because somehow openmp messes it up
					// TODO probably some way to prevent that but it's just copying an int so no big deal
					builtins.gl_InstanceID = c->builtins.gl_InstanceID;

					c->programs.a[c->cur_program].fragment_shader(fs_input, &builtins, c->programs.a[c->cur_program].uniform);
					if (!builtins.discard) {

						draw_fragment(&builtins, x, y, fragdepth_or_discard);
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

	// only initializing to get rid of "possibly uninitialized warning"
	vec4 Cs = {0}, Cd = {0};

	switch (c->blend_sRGB) {
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

	switch (c->blend_dRGB) {
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
		PGL_ASSERT(0 && "ERROR: unrecognized blend_sA!");
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
		PGL_ASSERT(0 && "ERROR: unrecognized blend_dA!");
		break;
	}

	vec4 result;

	// TODO eliminate function calls to avoid alpha component calculations?
	switch (c->blend_eqRGB) {
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
		PGL_ASSERT(0 && "ERROR: unrecognized blend_eqA!");
		break;
	}

	// TODO should I clamp in v4_to_Color() instead
	result = clamp_01_v4(result);
	return v4_to_Color(result);
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

	// NOTE/TODO assumes all 3 buffers have the same dimensions
	int i = -y*c->zbuf.w + x;

	//MSAA
	
#ifndef PGL_NO_STENCIL
	//Stencil Test
	stencil_pix_t* stencil_dest = &GET_STENCIL_PIX(i);
	if (c->stencil_test) {
		if (!stencil_test(EXTRACT_STENCIL(*stencil_dest))) {
			stencil_op(GL_FALSE, GL_TRUE, stencil_dest);
			return 0;
		}
	}
#endif

	//Depth test if necessary
	if (c->depth_test) {
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
			if (c->stencil_test)
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
			if (c->stencil_test) {
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
	} else if (c->stencil_test) {
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

	if (c->blend) {
		src_color = blend_pixel(cf, COLOR_TO_VEC4(dest_color));
	} else {
		cf = clamp_01_v4(cf);
		src_color = VEC4_TO_COLOR(cf);
	}

	src = RGBA_TO_PIXEL(src_color.r, src_color.g, src_color.b, src_color.a);

	if (c->logic_ops) {
		src = logic_ops_pixel(src, dst);
	}

#ifndef PGL_DISABLE_COLOR_MASK
	src = (src & c->color_mask) | (dst & ~c->color_mask);
#endif

	*dest_loc = src;
}

// Write to FBO color attachment using texture storage format (not window pix_t).
// U8 RGBA: Color* layout. Float R/RG/RGBA: raw floats; blend is replace-only (no float blend).
static void draw_pixel_color_rt(pglColorRT* rt, vec4 cf, int x, int y)
{
	int idx = -y * rt->w + x;
	if (rt->datatype == GL_FLOAT) {
		PGL_ASSERT(rt->components > 0);
		const int nc = rt->components;
		float* p = (float*)rt->lastrow + idx * nc;
		// replace write (no float blend in Phase D)
		p[0] = cf.x;
		if (nc > 1) p[1] = cf.y;
		if (nc > 2) p[2] = cf.z;
		if (nc > 3) p[3] = cf.w;
		return;
	}
	// U8 RGBA as Color
	Color* dest_loc = &((Color*)rt->lastrow)[idx];
	Color dest_color = *dest_loc;
	Color src_color;
	if (c->blend) {
		src_color = blend_pixel(cf, COLOR_TO_VEC4(dest_color));
	} else {
		cf = clamp_01_v4(cf);
		src_color = VEC4_TO_COLOR(cf);
	}
	// logic ops / color mask: only defined for pix_t window path; skip on RT
	*dest_loc = src_color;
}

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
		draw_pixel_color_rt(&c->mrt_color[att], cf, x, y);
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
			draw_pixel_color_rt(&c->mrt_color[att], b->gl_FragColor, x, y);
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
		draw_pixel_color_rt(&c->mrt_color[att], b->gl_FragData[i], x, y);
	}
}



#include <stdarg.h>


/******************************************
 * PORTABLEGL_IMPLEMENTATION
 ******************************************/

#include <stdio.h>
#include <float.h>

// for CHAR_BIT
#include <limits.h>

// TODO different name? NO_ERROR_CHECKING? LOOK_MA_NO_HANDS?
#ifdef PGL_UNSAFE
#define PGL_SET_ERR(err)
#define PGL_ERR(check, err)
#define PGL_SET_ERR_RET(err) return
#define PGL_ERR_RET_VAL(check, err, ret)
#define PGL_LOG(err)
#else
#define PGL_LOG(err) \
	do { \
		if (c->dbg_output && c->dbg_callback) { \
			int len = snprintf(c->dbg_msg_buf, PGL_MAX_DEBUG_MESSAGE_LENGTH, "%s in %s() at %s:%d", pgl_err_strs[err-GL_NO_ERROR], __func__, __FILE__, __LINE__); \
			c->dbg_callback(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, len, c->dbg_msg_buf, c->dbg_userparam); \
		} \
	} while (0)

#define PGL_SET_ERR(err) \
	do { \
		if (!c->error) c->error = err; \
		PGL_LOG(err); \
	} while (0)

#define PGL_ERR(check, err) \
	do { \
		if (check) {  \
			if (!c->error) c->error = err; \
			PGL_LOG(err); \
			return; \
		} \
	} while (0)

#define PGL_SET_ERR_RET(err) \
	do { \
		PGL_SET_ERR(err); \
		return; \
	} while (0)

#define PGL_ERR_RET_VAL(check, err, ret) \
	do { \
		if (check) {  \
			if (!c->error) c->error = err; \
			PGL_LOG(err); \
			return ret; \
		} \
	} while (0)
#endif

// Defined in gl_fbo.c (amalgamation order: after this file)
static GLboolean pgl_draw_framebuffer_ok(void);

// I just set everything even if not everything applies to the type
// see section 3.8.15 pg 181 of spec for what it's supposed to be
// TODO better name and inline?
static void INIT_TEX(glTexture* tex, GLenum target)
{
	tex->type = target;
	tex->mag_filter = GL_LINEAR;
	if (target != GL_TEXTURE_RECTANGLE) {
		//tex->min_filter = GL_NEAREST_MIPMAP_LINEAR;
		tex->min_filter = GL_NEAREST;
		tex->wrap_s = GL_REPEAT;
		tex->wrap_t = GL_REPEAT;
		tex->wrap_r = GL_REPEAT;
	} else {
		tex->min_filter = GL_LINEAR;
		tex->wrap_s = GL_CLAMP_TO_EDGE;
		tex->wrap_t = GL_CLAMP_TO_EDGE;
		tex->wrap_r = GL_CLAMP_TO_EDGE;
	}
	tex->data = NULL;
	tex->data_alloc = 0;
	tex->num_levels = 0;
	memset(tex->levels, 0, sizeof(tex->levels));
	tex->deleted = GL_FALSE;
	tex->user_owned = GL_TRUE;
	tex->datatype = GL_UNSIGNED_BYTE;
	tex->format = GL_RGBA;
	tex->components = 4;
	tex->is_depth = GL_FALSE;
	tex->invert_y = GL_FALSE;
	tex->lastrow = NULL;
	tex->w = 0;
	tex->h = 0;
	tex->d = 0;

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	tex->border_color = make_v4(0,0,0,0);
#endif
}

// Dimension of mip level `level` given base size (at least 1)
static GLsizei pgl_mip_dim(GLsizei base, GLint level)
{
	GLsizei d = base >> level;
	return d > 0 ? d : 1;
}

static size_t pgl_rgba_bytes_2d(GLsizei w, GLsizei h)
{
	return (size_t)w * (size_t)h * 4u;
}

static size_t pgl_rgba_bytes_1d(GLsizei w)
{
	return (size_t)w * 4u;
}

static size_t pgl_chain_bytes_2d(GLsizei bw, GLsizei bh, int nlevels)
{
	size_t total = 0;
	for (int i = 0; i < nlevels; ++i)
		total += pgl_rgba_bytes_2d(pgl_mip_dim(bw, i), pgl_mip_dim(bh, i));
	return total;
}

static size_t pgl_chain_bytes_1d(GLsizei bw, int nlevels)
{
	size_t total = 0;
	for (int i = 0; i < nlevels; ++i)
		total += pgl_rgba_bytes_1d(pgl_mip_dim(bw, i));
	return total;
}

// One cubemap mip level = 6 square (or rectangular) faces packed contiguously
static size_t pgl_rgba_bytes_cube_level(GLsizei face_w, GLsizei face_h)
{
	return (size_t)face_w * (size_t)face_h * 6u * 4u;
}

static size_t pgl_chain_bytes_cube(GLsizei bw, GLsizei bh, int nlevels)
{
	size_t total = 0;
	for (int i = 0; i < nlevels; ++i)
		total += pgl_rgba_bytes_cube_level(pgl_mip_dim(bw, i), pgl_mip_dim(bh, i));
	return total;
}

// Forward decl: used after realloc/bind so RT lastrow tracks tex->data
static void pgl_tex_refresh_lastrow(glTexture* tex);

// Point levels[0..nlevels) into the packed tex->data block (2D)
static void pgl_bind_level_ptrs_2d(glTexture* tex, int nlevels)
{
	u8* p = tex->data;
	for (int i = 0; i < nlevels; ++i) {
		GLsizei lw = pgl_mip_dim(tex->w, i);
		GLsizei lh = pgl_mip_dim(tex->h, i);
		tex->levels[i].w = lw;
		tex->levels[i].h = lh;
		tex->levels[i].data = p;
		p += pgl_rgba_bytes_2d(lw, lh);
	}
	for (int i = nlevels; i < PGL_MAX_MIPMAP_LEVELS; ++i) {
		tex->levels[i].w = 0;
		tex->levels[i].h = 0;
		tex->levels[i].data = NULL;
	}
	tex->num_levels = nlevels;
	pgl_tex_refresh_lastrow(tex);
}

static void pgl_bind_level_ptrs_1d(glTexture* tex, int nlevels)
{
	u8* p = tex->data;
	for (int i = 0; i < nlevels; ++i) {
		GLsizei lw = pgl_mip_dim(tex->w, i);
		tex->levels[i].w = lw;
		tex->levels[i].h = 1;
		tex->levels[i].data = p;
		p += pgl_rgba_bytes_1d(lw);
	}
	for (int i = nlevels; i < PGL_MAX_MIPMAP_LEVELS; ++i) {
		tex->levels[i].w = 0;
		tex->levels[i].h = 0;
		tex->levels[i].data = NULL;
	}
	tex->num_levels = nlevels;
}

// Cubemap: each level is [face0][face1]...[face5] at that face size
static void pgl_bind_level_ptrs_cube(glTexture* tex, int nlevels)
{
	u8* p = tex->data;
	for (int i = 0; i < nlevels; ++i) {
		GLsizei lw = pgl_mip_dim(tex->w, i);
		GLsizei lh = pgl_mip_dim(tex->h, i);
		tex->levels[i].w = lw;
		tex->levels[i].h = lh;
		tex->levels[i].data = p;
		p += pgl_rgba_bytes_cube_level(lw, lh);
	}
	for (int i = nlevels; i < PGL_MAX_MIPMAP_LEVELS; ++i) {
		tex->levels[i].w = 0;
		tex->levels[i].h = 0;
		tex->levels[i].data = NULL;
	}
	tex->num_levels = nlevels;
}

// Bytes per texel for tightly packed L0 (color or depth).
static int pgl_tex_bytes_per_pixel(const glTexture* tex)
{
	if (tex->is_depth) {
		if (tex->datatype == GL_FLOAT)
			return (int)sizeof(float);
#ifdef PGL_D16
		return (int)sizeof(u16);
#else
		return (int)sizeof(u32); // D24S8-style pack or depth24/32
#endif
	}
	PGL_ASSERT(tex->components > 0);
	const int nc = tex->components;
	if (tex->datatype == GL_FLOAT)
		return nc * (int)sizeof(float); // R32F / RG32F / RGBA32F
	return nc; // U8 channels (RGBA8 etc.)
}

static GLint pgl_format_components(GLenum format)
{
	if (format == GL_RED || format == GL_DEPTH_COMPONENT ||
	    format == GL_DEPTH_COMPONENT16 || format == GL_DEPTH_COMPONENT24 ||
	    format == GL_DEPTH_COMPONENT32 || format == GL_DEPTH_COMPONENT32F)
		return 1;
	if (format == GL_RG)
		return 2;
	if (format == GL_RGB || format == GL_BGR)
		return 3; // not fully supported for float RT
	return 4; // RGBA / BGRA
}

static void pgl_tex_set_format(glTexture* tex, GLenum format, GLenum datatype)
{
	tex->format = format;
	tex->datatype = datatype;
	tex->is_depth = (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_COMPONENT16 ||
	                 format == GL_DEPTH_COMPONENT24 || format == GL_DEPTH_COMPONENT32 ||
	                 format == GL_DEPTH_COMPONENT32F) ? GL_TRUE : GL_FALSE;
	tex->components = pgl_format_components(format);
	if (tex->is_depth)
		tex->components = 1;
}

// Recompute tex->lastrow from L0 data/w/h/datatype when invert_y; else NULL.
static void pgl_tex_refresh_lastrow(glTexture* tex)
{
	if (!tex->invert_y || !tex->data || tex->w <= 0 || tex->h <= 0) {
		tex->lastrow = NULL;
		return;
	}
	tex->lastrow = tex->data +
	               (size_t)(tex->h - 1) * (size_t)tex->w * (size_t)pgl_tex_bytes_per_pixel(tex);
}

// Mark texture as a render target: sample with lastrow indexing (fragCoord y=0 = bottom).
static void pgl_tex_mark_render_target(glTexture* tex)
{
	tex->invert_y = GL_TRUE;
	pgl_tex_refresh_lastrow(tex);
}

// levels[0] only; clear higher descriptors (does not free memory)
static void pgl_set_level0_desc(glTexture* tex)
{
	tex->levels[0].w = tex->w;
	tex->levels[0].h = tex->h;
	tex->levels[0].data = tex->data;
	for (int i = 1; i < PGL_MAX_MIPMAP_LEVELS; ++i) {
		tex->levels[i].w = 0;
		tex->levels[i].h = 0;
		tex->levels[i].data = NULL;
	}
	// Keep lastrow in sync if this is already an RT (remap/resize).
	pgl_tex_refresh_lastrow(tex);
}

// Free the one image allocation (all levels).  Honors user_owned.
static void pgl_free_texture_images(glTexture* tex)
{
	if (!tex->user_owned) {
		PGL_FREE(tex->data);
	}
	tex->data = NULL;
	tex->data_alloc = 0;
	tex->w = 0;
	tex->h = 0;
	tex->d = 0;
	tex->num_levels = 0;
	tex->user_owned = GL_FALSE;
	tex->invert_y = GL_FALSE;
	tex->lastrow = NULL;
	memset(tex->levels, 0, sizeof(tex->levels));
}

// Ensure a packed 2D chain of nlevels fits in one block; preserves existing prefix.
// PGL-owned storage is grown with realloc; user-owned L0 is copied into a new block.
// Returns 0 on OOM.
static int pgl_alloc_mip_chain_2d(glTexture* tex, int nlevels)
{
	if (nlevels < 1 || nlevels > PGL_MAX_MIPMAP_LEVELS)
		return 0;

	size_t need = pgl_chain_bytes_2d(tex->w, tex->h, nlevels);

	// Already large enough (may just need more level descriptors bound)
	if (!tex->user_owned && tex->data && tex->data_alloc >= need) {
		pgl_bind_level_ptrs_2d(tex, nlevels);
		return 1;
	}

	if (!tex->user_owned && tex->data) {
		// Grow our own block in place when the allocator allows
		size_t old = tex->data_alloc;
		u8* neu = (u8*)PGL_REALLOC(tex->data, need);
		if (!neu)
			return 0;
		if (need > old)
			memset(neu + old, 0, need - old);
		tex->data = neu;
		tex->data_alloc = need;
		pgl_bind_level_ptrs_2d(tex, nlevels);
		return 1;
	}

	// user_owned (or empty): allocate a PGL-owned block; copy L0 if present
	u8* neu = (u8*)PGL_MALLOC(need);
	if (!neu)
		return 0;

	if (tex->data) {
		size_t keep = pgl_rgba_bytes_2d(tex->w, tex->h);
		if (keep > need) keep = need;
		memcpy(neu, tex->data, keep);
		if (need > keep)
			memset(neu + keep, 0, need - keep);
		// leave user memory alone
	} else {
		memset(neu, 0, need);
	}

	tex->data = neu;
	tex->data_alloc = need;
	tex->user_owned = GL_FALSE;
	pgl_bind_level_ptrs_2d(tex, nlevels);
	return 1;
}

static int pgl_alloc_mip_chain_1d(glTexture* tex, int nlevels)
{
	if (nlevels < 1 || nlevels > PGL_MAX_MIPMAP_LEVELS)
		return 0;

	size_t need = pgl_chain_bytes_1d(tex->w, nlevels);

	if (!tex->user_owned && tex->data && tex->data_alloc >= need) {
		pgl_bind_level_ptrs_1d(tex, nlevels);
		return 1;
	}

	if (!tex->user_owned && tex->data) {
		size_t old = tex->data_alloc;
		u8* neu = (u8*)PGL_REALLOC(tex->data, need);
		if (!neu)
			return 0;
		if (need > old)
			memset(neu + old, 0, need - old);
		tex->data = neu;
		tex->data_alloc = need;
		pgl_bind_level_ptrs_1d(tex, nlevels);
		return 1;
	}

	u8* neu = (u8*)PGL_MALLOC(need);
	if (!neu)
		return 0;

	if (tex->data) {
		size_t keep = pgl_rgba_bytes_1d(tex->w);
		if (keep > need) keep = need;
		memcpy(neu, tex->data, keep);
		if (need > keep)
			memset(neu + keep, 0, need - keep);
	} else {
		memset(neu, 0, need);
	}

	tex->data = neu;
	tex->data_alloc = need;
	tex->user_owned = GL_FALSE;
	pgl_bind_level_ptrs_1d(tex, nlevels);
	return 1;
}

// Packed cubemap chain: L0 is 6 faces (~same layout as today), then L1..Ln-1 each 6 faces.
// Preserves the full L0 pack (6 * face_w * face_h * 4), not a single face.
static int pgl_alloc_mip_chain_cube(glTexture* tex, int nlevels)
{
	if (nlevels < 1 || nlevels > PGL_MAX_MIPMAP_LEVELS)
		return 0;

	size_t need = pgl_chain_bytes_cube(tex->w, tex->h, nlevels);

	if (!tex->user_owned && tex->data && tex->data_alloc >= need) {
		pgl_bind_level_ptrs_cube(tex, nlevels);
		return 1;
	}

	if (!tex->user_owned && tex->data) {
		size_t old = tex->data_alloc;
		u8* neu = (u8*)PGL_REALLOC(tex->data, need);
		if (!neu)
			return 0;
		if (need > old)
			memset(neu + old, 0, need - old);
		tex->data = neu;
		tex->data_alloc = need;
		pgl_bind_level_ptrs_cube(tex, nlevels);
		return 1;
	}

	u8* neu = (u8*)PGL_MALLOC(need);
	if (!neu)
		return 0;

	if (tex->data) {
		size_t keep = pgl_rgba_bytes_cube_level(tex->w, tex->h);
		if (keep > need) keep = need;
		memcpy(neu, tex->data, keep);
		if (need > keep)
			memset(neu + keep, 0, need - keep);
	} else {
		memset(neu, 0, need);
	}

	tex->data = neu;
	tex->data_alloc = need;
	tex->user_owned = GL_FALSE;
	pgl_bind_level_ptrs_cube(tex, nlevels);
	return 1;
}

// May be NULL if incomplete/empty.
static u8* pgl_tex_level_data(const glTexture* tex, GLint level)
{
	PGL_ASSERT(tex && tex->data && tex->num_levels > 0);
	PGL_ASSERT(level >= 0 && level < tex->num_levels);
	return tex->levels[level].data;
}

static void pgl_tex_level_dims(const glTexture* tex, GLint level, GLsizei* w, GLsizei* h, GLsizei* d)
{
	PGL_ASSERT(tex && tex->num_levels > 0);
	PGL_ASSERT(level >= 0 && level < tex->num_levels);

	if (w) *w = tex->levels[level].w;
	if (h) *h = tex->levels[level].h;
	if (d) *d = (level == 0) ? tex->d : 1;
}

// Box-filter one 2D RGBA8 level into the next (handles NPOT edges)
static void pgl_box_filter_2d(const u8* src, GLsizei sw, GLsizei sh, u8* dst, GLsizei dw, GLsizei dh)
{
	for (GLsizei y = 0; y < dh; ++y) {
		GLsizei y0 = y * 2;
		GLsizei y1 = (y0 + 1 < sh) ? y0 + 1 : y0;
		for (GLsizei x = 0; x < dw; ++x) {
			GLsizei x0 = x * 2;
			GLsizei x1 = (x0 + 1 < sw) ? x0 + 1 : x0;

			unsigned sum[4] = {0, 0, 0, 0};
			int count = 0;
			for (GLsizei j = y0; j <= y1; ++j) {
				for (GLsizei i = x0; i <= x1; ++i) {
					const u8* p = src + ((size_t)j * (size_t)sw + (size_t)i) * 4;
					sum[0] += p[0];
					sum[1] += p[1];
					sum[2] += p[2];
					sum[3] += p[3];
					++count;
				}
			}
			u8* out = dst + ((size_t)y * (size_t)dw + (size_t)x) * 4;
			out[0] = (u8)(sum[0] / count);
			out[1] = (u8)(sum[1] / count);
			out[2] = (u8)(sum[2] / count);
			out[3] = (u8)(sum[3] / count);
		}
	}
}

// Same for 1D (average 2 texels, or 1 at the end)
static void pgl_box_filter_1d(const u8* src, GLsizei sw, u8* dst, GLsizei dw)
{
	for (GLsizei x = 0; x < dw; ++x) {
		GLsizei x0 = x * 2;
		GLsizei x1 = (x0 + 1 < sw) ? x0 + 1 : x0;
		unsigned sum[4] = {0, 0, 0, 0};
		int count = 0;
		for (GLsizei i = x0; i <= x1; ++i) {
			const u8* p = src + (size_t)i * 4;
			sum[0] += p[0];
			sum[1] += p[1];
			sum[2] += p[2];
			sum[3] += p[3];
			++count;
		}
		u8* out = dst + (size_t)x * 4;
		out[0] = (u8)(sum[0] / count);
		out[1] = (u8)(sum[1] / count);
		out[2] = (u8)(sum[2] / count);
		out[3] = (u8)(sum[3] / count);
	}
}

// default pass through shaders for index 0
PGLDEF void default_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	PGL_UNUSED(uniforms);

	builtins->gl_Position = vertex_attribs[PGL_ATTR_VERT];
}

PGLDEF void default_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	PGL_UNUSED(uniforms);

	vec4* fragcolor = &builtins->gl_FragColor;
	//wish I could use a compound literal, stupid C++ compatibility
	fragcolor->x = 1.0f;
	fragcolor->y = 0.0f;
	fragcolor->z = 0.0f;
	fragcolor->w = 1.0f;
}

// TODO Where to put this and what to name it? move this and all static functions to gl_internal.c?
#ifndef PGL_UNSAFE
static const char* pgl_err_strs[] =
{
	"GL_NO_ERROR",
	"GL_INVALID_ENUM",
	"GL_INVALID_VALUE",
	"GL_INVALID_OPERATION",
	"GL_INVALID_FRAMEBUFFER_OPERATION",
	"GL_OUT_OF_MEMORY"
};

PGLDEF void dflt_dbg_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	PGL_UNUSED(source);
	PGL_UNUSED(type);
	PGL_UNUSED(id);
	PGL_UNUSED(severity);
	PGL_UNUSED(length);
	PGL_UNUSED(userParam);

	fprintf(stderr, "%s\n", message);
}
#endif

static void init_glVertex_Attrib(glVertex_Attrib* v)
{
	/*
	GLint size;      // number of components 1-4
	GLenum type;     // GL_FLOAT, default
	GLsizei stride;  //
	GLsizei offset;  //
	GLboolean normalized;
	unsigned int buf;
	GLboolean enabled;
	GLuint divisor;
*/
	v->buf = 0;
	v->enabled = 0;
	v->divisor = 0;
}

// TODO these are currently equivalent to memset(0) or = {0}...
static void init_glVertex_Array(glVertex_Array* v)
{
	v->deleted = GL_FALSE;
	for (int i=0; i<GL_MAX_VERTEX_ATTRIBS; ++i)
		init_glVertex_Attrib(&v->vertex_attribs[i]);
}

#define GET_SHIFT(mask, shift) \
	do {\
	shift = 0;\
	while ((mask & 1) == 0) {\
		mask >>= 1;\
		++shift;\
	}\
	} while (0)


PGLDEF GLboolean init_glContext(glContext* context, pix_t** back, GLsizei w, GLsizei h)
{
	PGL_ERR_RET_VAL(!back, GL_INVALID_VALUE, GL_FALSE);
	PGL_ERR_RET_VAL((w < 0 || h < 0), GL_INVALID_VALUE, GL_FALSE);

	c = context;
	memset(c, 0, sizeof(glContext));

	if (w && h && *back != NULL) {
		c->user_alloced_backbuf = GL_TRUE;
		c->back_buffer.buf = (u8*)*back;
		c->back_buffer.w = w;
		c->back_buffer.h = h;
		c->back_buffer.lastrow = c->back_buffer.buf + (h-1)*w*sizeof(pix_t);
	}

	c->xmin = 0;
	c->ymin = 0;
	c->width = w;
	c->height = h;

	c->color_mask = ~0;

	//initialize all vectors
	cvec_glVertex_Array(&c->vertex_arrays, 0, 3);
	cvec_glBuffer(&c->buffers, 0, 3);
	cvec_glProgram(&c->programs, 0, 3);
	cvec_glTexture(&c->textures, 0, 1);
	cvec_glFBO(&c->framebuffers, 0, 4);
	cvec_glRenderbuffer(&c->renderbuffers, 0, 4);
	cvec_glVertex(&c->glverts, 0, 10);

	c->bound_framebuffer = 0;
	c->bound_renderbuffer = 0;
	c->fbo_redirected = GL_FALSE;
	c->mrt_active = GL_FALSE;
	c->fbo_color_is_rt = GL_FALSE;
#ifndef PGL_NO_DEPTH_NO_STENCIL
	c->zbuf_float = GL_FALSE;
#endif
	c->default_num_draw_buffers = 1;
	c->default_draw_buffers[0] = GL_BACK;
	for (int i = 1; i < GL_MAX_DRAW_BUFFERS; ++i)
		c->default_draw_buffers[i] = GL_NONE;
	c->num_draw_buffers = 1;
	c->draw_buffers[0] = GL_BACK;
	for (int i = 1; i < GL_MAX_DRAW_BUFFERS; ++i)
		c->draw_buffers[i] = GL_NONE;
	c->default_read_buffer = GL_BACK;
	c->read_buffer = GL_BACK;
	memset(c->mrt_color, 0, sizeof(c->mrt_color));

	// If not pre-allocating max, need to track size and edit glUseProgram and pglSetInterp
	c->vs_output.output_buf = (float*)PGL_MALLOC(PGL_MAX_VERTICES * GL_MAX_VERTEX_OUTPUT_COMPONENTS * sizeof(float));
	PGL_ERR_RET_VAL(!c->vs_output.output_buf, GL_OUT_OF_MEMORY, GL_FALSE);

	c->clear_color = 0;
	SET_V4(c->blend_color, 0, 0, 0, 0);
	c->point_size = 1.0f;
	c->line_width = 1.0f;
	c->clear_depth = 1.0f;
	c->depth_range_near = 0.0f;
	c->depth_range_far = 1.0f;
	make_viewport_m4(c->vp_mat, 0, 0, w, h, 1);

	//set flags
	//TODO match order in structure definition
	c->provoking_vert = GL_LAST_VERTEX_CONVENTION;
	c->cull_mode = GL_BACK;
	c->cull_face = GL_FALSE;
	c->front_face = GL_CCW;
	c->depth_test = GL_FALSE;
	c->fragdepth_or_discard = GL_FALSE;
	c->depth_clamp = GL_FALSE;
	c->depth_mask = GL_TRUE;
	c->blend = GL_FALSE;
	c->logic_ops = GL_FALSE;
	c->poly_offset_pt = GL_FALSE;
	c->poly_offset_line = GL_FALSE;
	c->poly_offset_fill = GL_FALSE;
	c->scissor_test = GL_FALSE;

#ifndef PGL_NO_STENCIL
	c->clear_stencil = 0;

	c->stencil_test = GL_FALSE;
	c->stencil_writemask = -1; // all 1s for the masks
	c->stencil_writemask_back = -1;
	c->stencil_ref = 0;
	c->stencil_ref_back = 0;
	c->stencil_valuemask = -1;
	c->stencil_valuemask_back = -1;
	c->stencil_func = GL_ALWAYS;
	c->stencil_func_back = GL_ALWAYS;
	c->stencil_sfail = GL_KEEP;
	c->stencil_dpfail = GL_KEEP;
	c->stencil_dppass = GL_KEEP;
	c->stencil_sfail_back = GL_KEEP;
	c->stencil_dpfail_back = GL_KEEP;
	c->stencil_dppass_back = GL_KEEP;
#endif

	c->logic_func = GL_COPY;
	c->blend_sRGB = GL_ONE;
	c->blend_sA = GL_ONE;
	c->blend_dRGB = GL_ZERO;
	c->blend_dA = GL_ZERO;
	c->blend_eqRGB = GL_FUNC_ADD;
	c->blend_eqA = GL_FUNC_ADD;
	c->depth_func = GL_LESS;
	c->line_smooth = GL_FALSE;
	c->poly_mode_front = GL_FILL;
	c->poly_mode_back = GL_FILL;
	c->point_spr_origin = GL_UPPER_LEFT;

	c->poly_factor = 0.0f;
	c->poly_units = 0.0f;

	c->scissor_lx = 0;
	c->scissor_ly = 0;
	c->scissor_w = w;
	c->scissor_h = h;

	// According to refpages https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glPixelStore.xhtml
	c->unpack_alignment = 4;
	c->pack_alignment = 4;

	c->draw_triangle_front = draw_triangle_fill;
	c->draw_triangle_back = draw_triangle_fill;

	c->error = GL_NO_ERROR;
#ifndef PGL_UNSAFE
	c->dbg_callback = dflt_dbg_callback;
	c->dbg_output = GL_TRUE;
#else
	c->dbg_callback = NULL;
	c->dbg_output = GL_FALSE;
#endif

	// program 0 is supposed to be undefined but not invalid so I'll
	// just make it default, no transform, just draws things red
	glProgram tmp_prog = { default_vs, default_fs, NULL, 0, {0}, GL_FALSE, GL_FALSE };
	cvec_push_glProgram(&c->programs, tmp_prog);
	glUseProgram(0);

	// setup default vertex_array (vao) at position 0
	// we're like a compatibility profile for this but come on
	// no reason not to have this imo
	// https://www.opengl.org/wiki/Vertex_Specification#Vertex_Array_Object
	glVertex_Array tmp_va;
	init_glVertex_Array(&tmp_va);
	cvec_push_glVertex_Array(&c->vertex_arrays, tmp_va);
	c->cur_vertex_array = 0;

	// buffer 0 is invalid
	glBuffer tmp_buf = {0};
	tmp_buf.user_owned = GL_TRUE;
	tmp_buf.deleted = GL_FALSE;
	cvec_push_glBuffer(&c->buffers, tmp_buf);

	// From glBindTexture():
	// "The value zero is reserved to represent the default texture for each texture target."
	// "In effect, the texture targets become aliases for the textures currently bound to them, and the texture name zero refers to the default textures that were bound to them at initialization."
	//
	// ... which means we can't use the 0 index at all as it can obviously only
	// be one type/target at a time and it would be a pain regardless
	// Still we might as well initialize it since something has to be there
	glTexture tmp_tex;
	INIT_TEX(&tmp_tex, GL_TEXTURE_UNBOUND);
	cvec_push_glTexture(&c->textures, tmp_tex);

	// Initialize the actual default textures..
	// TODO Should I initialize them as their actual types? no
	// Should I do the non-spec white pixel thing?
	for (int i=0; i<GL_NUM_TEXTURE_TYPES-GL_TEXTURE_UNBOUND-1; i++) {
		INIT_TEX(&c->default_textures[i], GL_TEXTURE_UNBOUND);
	}

	// default texture (0) is bound to all targets initially
	memset(c->bound_textures, 0, sizeof(c->bound_textures));

	// invalid buffer (0) bound initially
	memset(c->bound_buffers, 0, sizeof(c->bound_buffers));

	// DRY, do all buffer allocs/init in here
	if (w && h && !pglResizeFramebuffer(w, h)) {
#ifndef PGL_NO_DEPTH_NO_STENCIL
		PGL_FREE(c->zbuf.buf);
#if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
		PGL_FREE(c->stencil_buf.buf);
#endif
#endif
		if (!c->user_alloced_backbuf) {
			PGL_FREE(c->back_buffer.buf);
		}
		return GL_FALSE;
	}

	*back = (pix_t*)c->back_buffer.buf;

	return GL_TRUE;
}

PGLDEF void free_glContext(glContext* ctx)
{
	int i;
	// If an FBO is bound, restore window surfaces so we free the real buffers
	if (ctx->fbo_redirected) {
		ctx->back_buffer = ctx->window_back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
		ctx->zbuf = ctx->window_zbuf;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
		ctx->stencil_buf = ctx->window_stencil_buf;
#  endif
#endif
		ctx->fbo_redirected = GL_FALSE;
	}

#ifndef PGL_NO_DEPTH_NO_STENCIL
	PGL_FREE(ctx->zbuf.buf);
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
	PGL_FREE(ctx->stencil_buf.buf);
#  endif
	PGL_FREE(ctx->fbo_scratch_z.buf);
	ctx->fbo_scratch_z.buf = NULL;
#endif
	if (!ctx->user_alloced_backbuf) {
		PGL_FREE(ctx->back_buffer.buf);
	}

	for (i=0; i<ctx->buffers.size; ++i) {
		if (!ctx->buffers.a[i].user_owned) {
			PGL_FREE(ctx->buffers.a[i].data);
		}
	}

	for (i=0; i<ctx->textures.size; ++i) {
		pgl_free_texture_images(&ctx->textures.a[i]);
	}
	for (i=0; i<GL_NUM_TEXTURE_TYPES-GL_TEXTURE_UNBOUND-1; ++i) {
		pgl_free_texture_images(&ctx->default_textures[i]);
	}

	//free vectors
	cvec_free_glVertex_Array(&ctx->vertex_arrays);
	cvec_free_glBuffer(&ctx->buffers);
	cvec_free_glProgram(&ctx->programs);
	cvec_free_glTexture(&ctx->textures);
	cvec_free_glFBO(&ctx->framebuffers);
	for (i = 0; i < ctx->renderbuffers.size; ++i) {
		if (!ctx->renderbuffers.a[i].user_owned)
			PGL_FREE(ctx->renderbuffers.a[i].data);
	}
	cvec_free_glRenderbuffer(&ctx->renderbuffers);
	cvec_free_glVertex(&ctx->glverts);

	PGL_FREE(ctx->vs_output.output_buf);

	if (c == ctx) {
		c = NULL;
	}
}

PGLDEF void set_glContext(glContext* context)
{
	c = context;
}

PGLDEF GLboolean pglResizeFramebuffer(GLsizei w, GLsizei h)
{
	PGL_ERR_RET_VAL((w < 0 || h < 0), GL_INVALID_VALUE, GL_FALSE);

	// Resize the *window* default surfaces, not a bound FBO's attachments.
	glFramebuffer* bb = c->fbo_redirected ? &c->window_back_buffer : &c->back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
	glFramebuffer* zb = c->fbo_redirected ? &c->window_zbuf : &c->zbuf;
#else
	glFramebuffer* zb = NULL;
	PGL_UNUSED(zb);
#endif

	// TODO C standard doesn't guarantee that passing the same size to
	// realloc is a no-op and will return the same pointer
	// NOTE checking zbuf because of the separation between pglSetBackBuffer()
	// and pglResizeFramebuffer(). If the former is called before the latter
	// backbuf dimensions would compare the same to the new size even when
	// we still need to update stencil and zbuf
#ifndef PGL_NO_DEPTH_NO_STENCIL
	if (w == zb->w && h == zb->h) {
		return GL_TRUE; // no resize necessary = success to me
	}
#else
	if (w == bb->w && h == bb->h) {
		return GL_TRUE;
	}
#endif

	u8* tmp;

	if (!c->user_alloced_backbuf) {
		tmp = (u8*)PGL_REALLOC(bb->buf, w*h * sizeof(pix_t));
		PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);
		bb->buf = tmp;
		bb->w = w;
		bb->h = h;
		bb->lastrow = bb->buf + (h-1)*w*sizeof(pix_t);
		if (!c->fbo_redirected)
			c->back_buffer = *bb;
	}

#ifdef PGL_D24S8
	tmp = (u8*)PGL_REALLOC(zb->buf, w*h * sizeof(u32));
	PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);

	zb->buf = tmp;
	zb->w = w;
	zb->h = h;
	zb->lastrow = zb->buf + (h-1)*w*sizeof(u32);
	if (!c->fbo_redirected)
		c->zbuf = *zb;

	// not checking for NO_STENCIL here because it makes no sense not to
	// have it if you're already using the space
	// D24S8: stencil is packed into the same buffer (window surfaces only)
	if (!c->fbo_redirected) {
		c->stencil_buf.buf = tmp;
		c->stencil_buf.w = w;
		c->stencil_buf.h = h;
		c->stencil_buf.lastrow = c->stencil_buf.buf + (h-1)*w*sizeof(u32);
	} else {
		// window_zbuf updated; stencil shares that storage when restored
		c->window_zbuf = *zb;
	}
#elif defined(PGL_D16)
	tmp = (u8*)PGL_REALLOC(zb->buf, w*h * sizeof(u16));
	PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);

	zb->buf = tmp;
	zb->w = w;
	zb->h = h;
	zb->lastrow = zb->buf + (h-1)*w*sizeof(u16);
	if (!c->fbo_redirected)
		c->zbuf = *zb;
	else
		c->window_zbuf = *zb;

#ifndef PGL_NO_STENCIL
	{
		glFramebuffer* sb = c->fbo_redirected ? &c->window_stencil_buf : &c->stencil_buf;
		tmp = (u8*)PGL_REALLOC(sb->buf, w*h);
		PGL_ERR_RET_VAL(!tmp, GL_OUT_OF_MEMORY, GL_FALSE);
		sb->buf = tmp;
		sb->w = w;
		sb->h = h;
		sb->lastrow = sb->buf + (h-1)*w;
		if (!c->fbo_redirected)
			c->stencil_buf = *sb;
	}
#endif
#endif

	if (c->scissor_test) {
		int ux = c->scissor_lx+c->scissor_w;
		int uy = c->scissor_ly+c->scissor_h;

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

	return GL_TRUE;
}



PGLDEF GLubyte* glGetString(GLenum name)
{
	static GLubyte vendor[] = "Robert Winkler (robertwinkler.com)";
	static GLubyte renderer[] = "PortableGL 0.101.0";
	static GLubyte version[] = "0.101.0";
	static GLubyte shading_language[] = "C/C++";

	switch (name) {
	case GL_VENDOR:                   return vendor;
	case GL_RENDERER:                 return renderer;
	case GL_VERSION:                  return version;
	case GL_SHADING_LANGUAGE_VERSION: return shading_language;
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
		return NULL;
	}
}

PGLDEF GLenum glGetError(void)
{
	GLenum err = c->error;
	c->error = GL_NO_ERROR;
	return err;
}

PGLDEF void glGenVertexArrays(GLsizei n, GLuint* arrays)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);

	glVertex_Array tmp = {0};
	//init_glVertex_Array(&tmp);
	tmp.deleted = GL_FALSE;

	//fill up empty slots first
	--n;
	for (int i=1; i<c->vertex_arrays.size && n>=0; ++i) {
		if (c->vertex_arrays.a[i].deleted) {
			c->vertex_arrays.a[i] = tmp;
			arrays[n--] = i;
		}
	}

	for (; n>=0; --n) {
		cvec_push_glVertex_Array(&c->vertex_arrays, tmp);
		arrays[n] = c->vertex_arrays.size-1;
	}
}

PGLDEF void glDeleteVertexArrays(GLsizei n, const GLuint* arrays)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	for (int i=0; i<n; ++i) {
		if (!arrays[i] || arrays[i] >= c->vertex_arrays.size)
			continue;

		// NOTE/TODO: This is non-standard behavior even in a compatibility profile but it
		// is similar to (from the user's perspective) how GL handles DeleteProgram called on
		// the active program.  So instead of getting a blank screen immediately, you just
		// free up the name moving the current vao to the default 0. Of course if you're switching
		// between VAOs and bind to the old name, you will get a GL error even if it still works
		// (because VAOS are POD and I don't overwrite it)... so maybe I should just have an
		// error here
		if (arrays[i] == c->cur_vertex_array) {
			memcpy(&c->vertex_arrays.a[0], &c->vertex_arrays.a[arrays[i]], sizeof(glVertex_Array));
			c->cur_vertex_array = 0;
		}

		c->vertex_arrays.a[arrays[i]].deleted = GL_TRUE;
	}
}

PGLDEF void glGenBuffers(GLsizei n, GLuint* buffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	//fill up empty slots first
	int j = 0;
	for (int i=1; i<c->buffers.size && j<n; ++i) {
		if (c->buffers.a[i].deleted) {
			c->buffers.a[i].deleted = GL_FALSE;
			buffers[j++] = i;
		}
	}

	if (j != n) {
		int s = c->buffers.size;
		cvec_extend_glBuffer(&c->buffers, n-j);
		for (int i=s; j<n; i++) {
			c->buffers.a[i].data = NULL;
			c->buffers.a[i].deleted = GL_FALSE;
			c->buffers.a[i].user_owned = GL_FALSE;
			buffers[j++] = i;
		}
	}
}

PGLDEF void glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	GLenum type;
	for (int i=0; i<n; ++i) {
		if (!buffers[i] || buffers[i] >= c->buffers.size)
			continue;

		// NOTE(rswinkle): type is stored as correct index not the raw enum value so no need to
		// subtract here see glBindBuffer
		type = c->buffers.a[buffers[i]].type;
		if (buffers[i] == c->bound_buffers[type])
			c->bound_buffers[type] = 0;

		if (!c->buffers.a[buffers[i]].user_owned) {
			PGL_FREE(c->buffers.a[buffers[i]].data);
		}
		c->buffers.a[buffers[i]].data = NULL;
		c->buffers.a[buffers[i]].deleted = GL_TRUE;
		c->buffers.a[buffers[i]].user_owned = GL_FALSE;
	}
}

PGLDEF void glGenTextures(GLsizei n, GLuint* textures)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	int j = 0;
	for (int i=1; i<c->textures.size && j<n; ++i) {
		if (c->textures.a[i].deleted) {
			c->textures.a[i].deleted = GL_FALSE;
			c->textures.a[i].type = GL_TEXTURE_UNBOUND;
			textures[j++] = i;
		}
	}
	if (j != n) {
		int s = c->textures.size;
		cvec_extend_glTexture(&c->textures, n-j);
		for (int i=s; j<n; i++) {
			c->textures.a[i].deleted = GL_FALSE;
			c->textures.a[i].type = GL_TEXTURE_UNBOUND;
			c->textures.a[i].user_owned = GL_FALSE;
			c->textures.a[i].data = NULL;
			c->textures.a[i].data_alloc = 0;
			c->textures.a[i].num_levels = 0;
			memset(c->textures.a[i].levels, 0, sizeof(c->textures.a[i].levels));
			textures[j++] = i;
		}
	}
}

PGLDEF void glCreateTextures(GLenum target, GLsizei n, GLuint* textures)
{
	PGL_ERR((target < GL_TEXTURE_1D || target >= GL_NUM_TEXTURE_TYPES), GL_INVALID_ENUM);
	PGL_ERR(n < 0, GL_INVALID_VALUE);

	target -= GL_TEXTURE_UNBOUND + 1;
	int j = 0;
	for (int i=1; i<c->textures.size && j<n; ++i) {
		if (c->textures.a[i].deleted) {
			INIT_TEX(&c->textures.a[i], target);
			textures[j++] = i;
		}
	}
	if (j != n) {
		int s = c->textures.size;
		cvec_extend_glTexture(&c->textures, n-j);
		for (int i=s; j<n; i++) {
			INIT_TEX(&c->textures.a[i], target);
			textures[j++] = i;
		}
	}
}

PGLDEF void glDeleteTextures(GLsizei n, const GLuint* textures)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	GLenum type;
	for (int i=0; i<n; ++i) {
		if (!textures[i] || textures[i] >= c->textures.size)
			continue;

		// NOTE(rswinkle): type is stored as correct index not the raw enum value
		// so no need to subtract here see glBindTexture
		type = c->textures.a[textures[i]].type;
		if (textures[i] == c->bound_textures[type])
			c->bound_textures[type] = 0;

		pgl_free_texture_images(&c->textures.a[textures[i]]);

		c->textures.a[textures[i]].type = GL_TEXTURE_UNBOUND;
		c->textures.a[textures[i]].deleted = GL_TRUE;
	}
}

PGLDEF void glBindVertexArray(GLuint array)
{
	PGL_ERR((array >= c->vertex_arrays.size || c->vertex_arrays.a[array].deleted), GL_INVALID_OPERATION);

	c->cur_vertex_array = array;
	c->bound_buffers[GL_ELEMENT_ARRAY_BUFFER-GL_ARRAY_BUFFER] = c->vertex_arrays.a[array].element_buffer;
}

PGLDEF void glBindBuffer(GLenum target, GLuint buffer)
{
	PGL_ERR(target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER, GL_INVALID_ENUM);

	PGL_ERR((buffer >= c->buffers.size || c->buffers.a[buffer].deleted), GL_INVALID_OPERATION);

	target -= GL_ARRAY_BUFFER;
	c->bound_buffers[target] = buffer;

	// Note type isn't set till binding and we're not storing the raw
	// enum but the enum - GL_ARRAY_BUFFER so it's an index into c->bound_buffers
	// TODO need to see what's supposed to happen if you try to bind
	// a buffer to multiple targets
	c->buffers.a[buffer].type = target;

	if (target == GL_ELEMENT_ARRAY_BUFFER - GL_ARRAY_BUFFER) {
		c->vertex_arrays.a[c->cur_vertex_array].element_buffer = buffer;
	}
}

// TODO reuse code, call glNamedBufferData() internally, remove duplicated error checks?
PGLDEF void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	//TODO check for usage later
	PGL_UNUSED(usage);

	PGL_ERR((target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER), GL_INVALID_ENUM);
	PGL_ERR(size < 0, GL_INVALID_VALUE);

	target -= GL_ARRAY_BUFFER;
	PGL_ERR(!c->bound_buffers[target], GL_INVALID_OPERATION);

	// the spec says any pre-existing data store is deleted but there's no reason to
	// c->buffers.a[c->bound_buffers[target]].data is always NULL or valid
	u8* tmp = (u8*)PGL_REALLOC(c->buffers.a[c->bound_buffers[target]].data, size);
	PGL_ERR(!tmp, GL_OUT_OF_MEMORY);

	c->buffers.a[c->bound_buffers[target]].data = tmp;

	if (data) {
		memcpy(c->buffers.a[c->bound_buffers[target]].data, data, size);
	}

	c->buffers.a[c->bound_buffers[target]].user_owned = GL_FALSE;
	c->buffers.a[c->bound_buffers[target]].size = size;
}

PGLDEF void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	PGL_ERR(target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER, GL_INVALID_ENUM);
	PGL_ERR((offset < 0 || size < 0), GL_INVALID_VALUE);

	target -= GL_ARRAY_BUFFER;

	PGL_ERR(!c->bound_buffers[target], GL_INVALID_OPERATION);
	PGL_ERR((offset + size > c->buffers.a[c->bound_buffers[target]].size), GL_INVALID_VALUE);

	memcpy(&c->buffers.a[c->bound_buffers[target]].data[offset], data, size);
}

PGLDEF void glNamedBufferData(GLuint buffer, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	//check for usage later
	PGL_UNUSED(usage);

	PGL_ERR((!buffer || buffer >= c->buffers.size || c->buffers.a[buffer].deleted), GL_INVALID_OPERATION);
	PGL_ERR(size < 0, GL_INVALID_VALUE);

	//always NULL or valid
	PGL_FREE(c->buffers.a[buffer].data);

	c->buffers.a[buffer].data = (u8*)PGL_MALLOC(size);
	PGL_ERR(!c->buffers.a[buffer].data, GL_OUT_OF_MEMORY);

	if (data) {
		memcpy(c->buffers.a[buffer].data, data, size);
	}

	c->buffers.a[buffer].user_owned = GL_FALSE;
	c->buffers.a[buffer].size = size;
}

PGLDEF void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	PGL_ERR((!buffer || buffer >= c->buffers.size || c->buffers.a[buffer].deleted), GL_INVALID_OPERATION);
	PGL_ERR((offset < 0 || size < 0), GL_INVALID_VALUE);
	PGL_ERR((offset + size > c->buffers.a[buffer].size), GL_INVALID_VALUE);

	memcpy(&c->buffers.a[buffer].data[offset], data, size);
}

// TODO see page 136-7 of spec
PGLDEF void glBindTexture(GLenum target, GLuint texture)
{
	PGL_ERR((target < GL_TEXTURE_1D || target >= GL_NUM_TEXTURE_TYPES), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;

	PGL_ERR((texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_VALUE);

	if (texture) {
		GLenum type = c->textures.a[texture].type;
		PGL_ERR((type != GL_TEXTURE_UNBOUND && type != target), GL_INVALID_OPERATION);

		if (type == GL_TEXTURE_UNBOUND) {
			INIT_TEX(&c->textures.a[texture], target);
		}
	}
	c->bound_textures[target] = texture;
}

static void set_texparami(glTexture* tex, GLenum pname, GLint param)
{
	/*
	PGL_ERR((pname != GL_TEXTURE_MIN_FILTER && pname != GL_TEXTURE_MAG_FILTER &&
	         pname != GL_TEXTURE_WRAP_S && pname != GL_TEXTURE_WRAP_T &&
	         pname != GL_TEXTURE_WRAP_R), GL_INVALID_ENUM);
	         */

	// Store full min_filter enums (including *MIPMAP*); sampling maps them to
	// within-level NEAREST/LINEAR.  texture*Lod uses them for explicit LOD.
	if (pname == GL_TEXTURE_MIN_FILTER) {
		// RECTANGLE: only NEAREST or LINEAR
		if (tex->type == GL_TEXTURE_RECTANGLE - (GL_TEXTURE_UNBOUND + 1)) {
			PGL_ERR((param != GL_NEAREST && param != GL_LINEAR), GL_INVALID_ENUM);
		} else {
			switch (param) {
			case GL_NEAREST:
			case GL_LINEAR:
			case GL_NEAREST_MIPMAP_NEAREST:
			case GL_NEAREST_MIPMAP_LINEAR:
			case GL_LINEAR_MIPMAP_NEAREST:
			case GL_LINEAR_MIPMAP_LINEAR:
				break;
			default:
				PGL_SET_ERR_RET(GL_INVALID_ENUM);
			}
		}
		tex->min_filter = param;
	} else if (pname == GL_TEXTURE_MAG_FILTER) {
		// Mag filter is only NEAREST or LINEAR
		PGL_ERR((param != GL_NEAREST && param != GL_LINEAR), GL_INVALID_ENUM);
		tex->mag_filter = param;
	} else if (pname == GL_TEXTURE_WRAP_S) {
		PGL_ERR((param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
#ifdef PGL_CORE_PROFILE
		// Core: RECTANGLE wrap is only CLAMP_TO_EDGE / CLAMP_TO_BORDER
		PGL_ERR((tex->type == GL_TEXTURE_RECTANGLE - (GL_TEXTURE_UNBOUND + 1) &&
		         param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER), GL_INVALID_ENUM);
#endif
		tex->wrap_s = param;
	} else if (pname == GL_TEXTURE_WRAP_T) {
		PGL_ERR((param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
#ifdef PGL_CORE_PROFILE
		PGL_ERR((tex->type == GL_TEXTURE_RECTANGLE - (GL_TEXTURE_UNBOUND + 1) &&
		         param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER), GL_INVALID_ENUM);
#endif
		tex->wrap_t = param;
	} else if (pname == GL_TEXTURE_WRAP_R) {
		PGL_ERR((param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
		tex->wrap_r = param;
	} else {
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

// TODO handle ParameterI*() functions correctly
static void get_texparami(glTexture* tex, GLenum pname, GLenum type, GLvoid* params)
{
	GLenum val;
	switch (pname) {
	case GL_TEXTURE_MIN_FILTER: val = tex->min_filter; break;
	case GL_TEXTURE_MAG_FILTER: val = tex->mag_filter; break;
	case GL_TEXTURE_WRAP_S:
		PGL_ERR((pname != GL_REPEAT && pname != GL_CLAMP_TO_EDGE && pname != GL_CLAMP_TO_BORDER && pname != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
		val = tex->wrap_s;
		break;
	case GL_TEXTURE_WRAP_T:
		PGL_ERR((pname != GL_REPEAT && pname != GL_CLAMP_TO_EDGE && pname != GL_CLAMP_TO_BORDER && pname != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
		val = tex->wrap_t;
		break;
	case GL_TEXTURE_WRAP_R:
		PGL_ERR((pname != GL_REPEAT && pname != GL_CLAMP_TO_EDGE && pname != GL_CLAMP_TO_BORDER && pname != GL_MIRRORED_REPEAT), GL_INVALID_ENUM);
		val = tex->wrap_r;
		break;
	default:
		PGL_SET_ERR_RET(GL_INVALID_ENUM);
	}

	if (type == GL_INT) {
		*(GLint*)params = val;
	} else {
		*(GLuint*)params = val;
	}
}

PGLDEF void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	//shift to range 0 - NUM_TEXTURES-1 to access bound_textures array
	target -= GL_TEXTURE_UNBOUND + 1;

	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	set_texparami(tex, pname, param);
}

PGLDEF void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	PGL_ERR((pname != GL_TEXTURE_BORDER_COLOR), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;
	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	memcpy(&tex->border_color, params, sizeof(GLfloat)*4);
#endif
}
PGLDEF void glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	PGL_ERR((pname != GL_TEXTURE_BORDER_COLOR), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;
	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}

	tex->border_color.x = (2*params[0] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.y = (2*params[1] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.z = (2*params[2] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.w = (2*params[3] + 1)/(UINT32_MAX - 1.0f);
#endif
}

// NOTE: I added the !texture checks to the glTextureParameter*() functions
// even though it's not in the spec because there's no way to know which
// default texture (0) target you're referring to
PGLDEF void glTextureParameteri(GLuint texture, GLenum pname, GLint param)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	set_texparami(&c->textures.a[texture], pname, param);
}

PGLDEF void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	memcpy(&c->textures.a[texture].border_color, params, sizeof(GLfloat)*4);
#endif
}

PGLDEF void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);

	glTexture* tex = &c->textures.a[texture];
	tex->border_color.x = (2*params[0] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.y = (2*params[1] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.z = (2*params[2] + 1)/(UINT32_MAX - 1.0f);
	tex->border_color.w = (2*params[3] + 1)/(UINT32_MAX - 1.0f);
#endif
}

PGLDEF void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	PGL_ERR((pname != GL_TEXTURE_BORDER_COLOR), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;
	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	memcpy(params, &tex->border_color, sizeof(GLfloat)*4);
#endif
}

PGLDEF void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;

	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	get_texparami(tex, pname, GL_INT, (GLvoid*)params);
}

PGLDEF void glGetTexParameterIiv(GLenum target, GLenum pname, GLint* params)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;

	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	get_texparami(tex, pname, GL_INT, (GLvoid*)params);
}

PGLDEF void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint* params)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	target -= GL_TEXTURE_UNBOUND + 1;

	glTexture* tex = NULL;
	if (c->bound_textures[target]) {
		tex = &c->textures.a[c->bound_textures[target]];
	} else {
		tex = &c->default_textures[target];
	}
	get_texparami(tex, pname, GL_UNSIGNED_INT, (GLvoid*)params);
}

PGLDEF void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params)
{
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	memcpy(params, &c->textures.a[texture].border_color, sizeof(GLfloat)*4);
#endif
}

PGLDEF void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	get_texparami(&c->textures.a[texture], pname, GL_UNSIGNED_INT, (GLvoid*)params);
}

PGLDEF void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	get_texparami(&c->textures.a[texture], pname, GL_UNSIGNED_INT, (GLvoid*)params);
}

PGLDEF void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);
	get_texparami(&c->textures.a[texture], pname, GL_UNSIGNED_INT, (GLvoid*)params);
}


PGLDEF void glPixelStorei(GLenum pname, GLint param)
{
	PGL_ERR((pname != GL_UNPACK_ALIGNMENT && pname != GL_PACK_ALIGNMENT), GL_INVALID_ENUM);

	PGL_ERR((param != 1 && param != 2 && param != 4 && param != 8), GL_INVALID_VALUE);

	// TODO eliminate branch? or use PGL_SET_ERR in else
	if (pname == GL_UNPACK_ALIGNMENT) {
		c->unpack_alignment = param;
	} else if (pname == GL_PACK_ALIGNMENT) {
		c->pack_alignment = param;
	}

}

// TODO check preprocessor output
#define CHECK_FORMAT_GET_COMP(format, components) \
	do { \
	switch (format) { \
	case GL_RED: \
	case GL_ALPHA: \
	case GL_LUMINANCE: \
	case PGL_ONE_ALPHA: \
		components = 1; \
		break; \
	case GL_RG: \
	case GL_LUMINANCE_ALPHA: \
		components = 2; \
		break; \
	case GL_RGB: \
	case GL_BGR: \
		components = 3; \
		break; \
	case GL_RGBA: \
	case GL_BGRA: \
		components = 4; \
		break; \
	default: \
		PGL_SET_ERR_RET(GL_INVALID_ENUM); \
	} \
	} while (0)

// Copy height rows of tightly packed dst pixels from unpack-aligned src.
static void pgl_copy_unpack_rows(u8* dst, const u8* src, int width, int height, int bpp, int src_pitch)
{
	int row_bytes = width * bpp;
	for (int y = 0; y < height; ++y)
		memcpy(dst + (size_t)y * (size_t)row_bytes, src + (size_t)y * (size_t)src_pitch, (size_t)row_bytes);
}

// True if format is valid for GL_FLOAT storage (matches pglTextureImage* matrix).
static GLboolean pgl_teximage_float_format_ok(GLenum format)
{
	return format == GL_RED || format == GL_RG || format == GL_RGBA ||
	       format == GL_DEPTH_COMPONENT;
}

PGLDEF void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(internalformat);
	PGL_UNUSED(border);

	PGL_ERR(target != GL_TEXTURE_1D, GL_INVALID_ENUM);
	PGL_ERR(level < 0, GL_INVALID_VALUE);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE && type != GL_FLOAT, GL_INVALID_ENUM);

	int components;
	if (type == GL_FLOAT) {
		PGL_ERR(!pgl_teximage_float_format_ok(format), GL_INVALID_ENUM);
		components = pgl_format_components(format);
	} else {
#ifdef PGL_DONT_CONVERT_TEXTURES
		PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
		components = 4;
#else
		CHECK_FORMAT_GET_COMP(format, components);
#endif
	}

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	PGL_ERR(level >= PGL_MAX_MIPMAP_LEVELS, GL_INVALID_VALUE);
	// Float / non-RGBA8 storage: only level 0 for now (mip chain helpers are RGBA8-centric)
	PGL_ERR(level > 0 && type == GL_FLOAT, GL_INVALID_OPERATION);

	if (level == 0) {
		if (!tex->user_owned)
			PGL_FREE(tex->data);

		tex->w = width;
		tex->h = 1;
		tex->d = 1;

		if (type == GL_FLOAT) {
			pgl_tex_set_format(tex, format, GL_FLOAT);
			int bpp = pgl_tex_bytes_per_pixel(tex);
			size_t nbytes = (size_t)width * (size_t)bpp;
			tex->data = (u8*)PGL_MALLOC(nbytes ? nbytes : 1);
			PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
			tex->data_alloc = nbytes;
			if (data) {
				int src_pitch = width * bpp; // 1D: no row padding beyond unpack for single row
				int byte_width = width * bpp;
				int pad = byte_width % c->unpack_alignment;
				if (pad)
					src_pitch = byte_width + c->unpack_alignment - pad;
				pgl_copy_unpack_rows(tex->data, (const u8*)data, width, 1, bpp, src_pitch);
			} else {
				memset(tex->data, 0, nbytes ? nbytes : 1);
			}
		} else {
			size_t nbytes = pgl_rgba_bytes_1d(width);
			tex->data = (u8*)PGL_MALLOC(nbytes);
			PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
			tex->data_alloc = nbytes;
			if (data) {
				convert_format_to_packed_rgba(tex->data, (u8*)data, width, 1, width*components, format);
			}
			pgl_tex_set_format(tex, GL_RGBA, GL_UNSIGNED_BYTE);
		}

		tex->user_owned = GL_FALSE;
		tex->num_levels = 1;
		pgl_set_level0_desc(tex);
	} else {
		// Higher levels require a defined base level (RGBA8 U8 only)
		PGL_ERR(!tex->data || tex->w <= 0, GL_INVALID_OPERATION);
		PGL_ERR(tex->datatype != GL_UNSIGNED_BYTE || tex->components != 4, GL_INVALID_OPERATION);
		PGL_ERR(width != pgl_mip_dim(tex->w, level), GL_INVALID_VALUE);

		// Call alloc outside PGL_ERR (PGL_UNSAFE empties the macro and would skip alloc)
		if (!pgl_alloc_mip_chain_1d(tex, level + 1)) {
			PGL_SET_ERR_RET(GL_OUT_OF_MEMORY);
		}

		if (data) {
			convert_format_to_packed_rgba(tex->levels[level].data, (u8*)data, width, 1, width*components, format);
		}
	}
}

PGLDEF void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(internalformat);
	PGL_UNUSED(border);

	// TODO GL_TEXTURE_1D_ARRAY
	PGL_ERR((target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_1D_ARRAY &&
	         target != GL_TEXTURE_RECTANGLE &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z), GL_INVALID_ENUM);

	PGL_ERR(level < 0, GL_INVALID_VALUE);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);

	PGL_ERR(type != GL_UNSIGNED_BYTE && type != GL_FLOAT, GL_INVALID_ENUM);

	// RECTANGLE and cubemap faces: only level 0 for now
	if (target != GL_TEXTURE_2D && target != GL_TEXTURE_1D_ARRAY) {
		PGL_ERR(level != 0, GL_INVALID_VALUE);
	}

	// Cubemaps remain U8 RGBA (with optional convert) only
	if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
		PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);
	}

	int components;
	if (type == GL_FLOAT) {
		PGL_ERR(!pgl_teximage_float_format_ok(format), GL_INVALID_ENUM);
		components = pgl_format_components(format);
	} else {
#ifdef PGL_DONT_CONVERT_TEXTURES
		PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
		components = 4;
#else
		CHECK_FORMAT_GET_COMP(format, components);
#endif
	}

	// Have to handle cubemaps specially since they have 1 real target
	// and 6 pseudo targets
	int target_idx;
	if (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
		//target is 2D, 1D_ARRAY, or RECTANGLE
		target_idx = target-GL_TEXTURE_UNBOUND-1;
	} else {
		target_idx = GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1;
	}
	int cur_tex_i = c->bound_textures[target_idx];

	// Have to handle 0 specially as well
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	int src_bpp = (type == GL_FLOAT) ? components * (int)sizeof(float) : components;
	int byte_width = width * src_bpp;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	PGL_ERR(level >= PGL_MAX_MIPMAP_LEVELS, GL_INVALID_VALUE);
	PGL_ERR(level > 0 && type == GL_FLOAT, GL_INVALID_OPERATION);

	if (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X) {
		//target is 2D, 1D_ARRAY, or RECTANGLE
		if (level == 0) {
			if (!tex->user_owned)
				PGL_FREE(tex->data);

			tex->w = width;
			tex->h = height;
			tex->d = 1;

			if (type == GL_FLOAT) {
				pgl_tex_set_format(tex, format, GL_FLOAT);
				int bpp = pgl_tex_bytes_per_pixel(tex);
				size_t nbytes = (size_t)width * (size_t)height * (size_t)bpp;
				tex->data = (u8*)PGL_MALLOC(nbytes ? nbytes : 1);
				PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
				tex->data_alloc = nbytes;
				if (data)
					pgl_copy_unpack_rows(tex->data, (const u8*)data, width, height, bpp, padded_row_len);
				else
					memset(tex->data, 0, nbytes ? nbytes : 1);
			} else {
				size_t nbytes = pgl_rgba_bytes_2d(width, height);
				tex->data = (u8*)PGL_MALLOC(nbytes);
				PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
				tex->data_alloc = nbytes;

				if (data) {
					convert_format_to_packed_rgba(tex->data, (u8*)data, width, height, padded_row_len, format);
				}
				pgl_tex_set_format(tex, GL_RGBA, GL_UNSIGNED_BYTE);
			}

			tex->user_owned = GL_FALSE;
			tex->num_levels = 1;
			pgl_set_level0_desc(tex);
		} else {
			// Higher mip levels (2D / 1D_ARRAY only) — RGBA8 U8 only
			PGL_ERR(!tex->data || tex->w <= 0 || tex->h <= 0, GL_INVALID_OPERATION);
			PGL_ERR(tex->datatype != GL_UNSIGNED_BYTE || tex->components != 4, GL_INVALID_OPERATION);
			PGL_ERR(width != pgl_mip_dim(tex->w, level) || height != pgl_mip_dim(tex->h, level), GL_INVALID_VALUE);

			if (!pgl_alloc_mip_chain_2d(tex, level + 1)) {
				PGL_SET_ERR_RET(GL_OUT_OF_MEMORY);
			}

			if (data) {
				convert_format_to_packed_rgba(tex->levels[level].data, (u8*)data, width, height, padded_row_len, format);
			}
		}

	} else {  //CUBE_MAP (level 0 only, U8 RGBA storage)
		// If we're reusing a texture, and we haven't already loaded
		// one of the planes of the cubemap, data is either NULL or valid
		if (!tex->w) {
			if (!tex->user_owned)
				PGL_FREE(tex->data);
			tex->data = NULL;
			tex->data_alloc = 0;
			memset(tex->levels, 0, sizeof(tex->levels));
		}

		// TODO specs say INVALID_VALUE, man/ref pages say INVALID_ENUM?
		// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
		PGL_ERR(width != height, GL_INVALID_VALUE);

		size_t mem_size = (size_t)width * height * 6 * 4;
		if (tex->w == 0) {
			tex->w = width;
			tex->h = width; //same cause square
			tex->d = 1;

			tex->data = (u8*)PGL_MALLOC(mem_size);
			PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
			tex->data_alloc = mem_size;
			tex->num_levels = 1;
			pgl_tex_set_format(tex, GL_RGBA, GL_UNSIGNED_BYTE);
			pgl_set_level0_desc(tex);
		} else if (tex->w != width) {
			//TODO spec doesn't say all sides must have same dimensions but it makes sense
			//and this site suggests it http://www.opengl.org/wiki/Cubemap_Texture
			PGL_SET_ERR_RET(GL_INVALID_VALUE);
		}

		//use target as plane index
		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X;

		int p = height*width*4;
		u8* texdata = tex->data;

		if (data) {
			convert_format_to_packed_rgba(&texdata[target*p], (u8*)data, width, height, padded_row_len, format);
		}

		tex->user_owned = GL_FALSE;
	} //end CUBE_MAP
}

PGLDEF void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(level);
	PGL_UNUSED(internalformat);
	PGL_UNUSED(border);

	PGL_ERR((target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY), GL_INVALID_ENUM);
	PGL_ERR(type != GL_UNSIGNED_BYTE && type != GL_FLOAT, GL_INVALID_ENUM);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((depth < 0 || depth > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);

	int components;
	if (type == GL_FLOAT) {
		PGL_ERR(!pgl_teximage_float_format_ok(format), GL_INVALID_ENUM);
		components = pgl_format_components(format);
	} else {
#ifdef PGL_DONT_CONVERT_TEXTURES
		PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
		components = 4;
#else
		CHECK_FORMAT_GET_COMP(format, components);
#endif
	}

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	// 3D mips not supported yet; level is still ignored but base is level 0
	if (!tex->user_owned)
		PGL_FREE(tex->data);

	tex->w = width;
	tex->h = height;
	tex->d = depth;

	int src_bpp = (type == GL_FLOAT) ? components * (int)sizeof(float) : components;
	int byte_width = width * src_bpp;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	if (type == GL_FLOAT) {
		pgl_tex_set_format(tex, format, GL_FLOAT);
		int bpp = pgl_tex_bytes_per_pixel(tex);
		size_t nbytes = (size_t)width * (size_t)height * (size_t)depth * (size_t)bpp;
		tex->data = (u8*)PGL_MALLOC(nbytes ? nbytes : 1);
		PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
		tex->data_alloc = nbytes;
		if (data) {
			// Treat as height*depth rows of width texels (same as U8 path)
			pgl_copy_unpack_rows(tex->data, (const u8*)data, width, height * depth, bpp, padded_row_len);
		} else {
			memset(tex->data, 0, nbytes ? nbytes : 1);
		}
	} else {
		size_t nbytes = (size_t)width * height * depth * 4;
		tex->data = (u8*)PGL_MALLOC(nbytes);
		PGL_ERR(!tex->data, GL_OUT_OF_MEMORY);
		tex->data_alloc = nbytes;

		if (data) {
			convert_format_to_packed_rgba(tex->data, (u8*)data, width, height*depth, padded_row_len, format);
		}
		pgl_tex_set_format(tex, GL_RGBA, GL_UNSIGNED_BYTE);
	}

	tex->user_owned = GL_FALSE;
	tex->num_levels = 1;
	pgl_set_level0_desc(tex);
}

PGLDEF void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_ERR(target != GL_TEXTURE_1D, GL_INVALID_ENUM);
	PGL_ERR(level < 0, GL_INVALID_VALUE);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	PGL_ERR(level >= tex->num_levels || !tex->levels[level].data, GL_INVALID_OPERATION);

	GLsizei tw = tex->levels[level].w;
	u8* level_data = tex->levels[level].data;

	PGL_ERR((xoffset < 0 || xoffset + width > tw), GL_INVALID_VALUE);

	u32* texdata = (u32*)level_data;
	convert_format_to_packed_rgba((u8*)&texdata[xoffset], (u8*)data, width, 1, width*components, format);
}

PGLDEF void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data)
{
	// TODO GL_TEXTURE_1D_ARRAY
	PGL_ERR((target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	         target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	         target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z), GL_INVALID_ENUM);

	PGL_ERR(level < 0, GL_INVALID_VALUE);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);

	// Cubemap: only level 0
	if (target != GL_TEXTURE_2D) {
		PGL_ERR(level != 0, GL_INVALID_VALUE);
	}

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	// Have to handle cubemaps specially since they have 1 real target
	// and 6 pseudo targets
	int target_idx;
	if (target == GL_TEXTURE_2D) {
		target_idx = target-GL_TEXTURE_UNBOUND-1;
	} else {
		target_idx = GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1;
	}
	int cur_tex_i = c->bound_textures[target_idx];

	// Have to handle 0 specially as well
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	u8* d = (u8*)data;

	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	if (target == GL_TEXTURE_2D) {
		PGL_ERR(level >= tex->num_levels || !tex->levels[level].data, GL_INVALID_OPERATION);

		GLsizei tw = tex->levels[level].w;
		GLsizei th = tex->levels[level].h;
		u8* level_data = tex->levels[level].data;

		PGL_ERR((xoffset < 0 || xoffset + width > tw || yoffset < 0 || yoffset + height > th), GL_INVALID_VALUE);

		u32* texdata = (u32*)level_data;
		int w = tw;

		// TODO maybe better to covert the whole input image if
		// necessary then do the original memcpy's even with
		// the extra alloc and free
		for (int i=0; i<height; ++i) {
			convert_format_to_packed_rgba((u8*)&texdata[(yoffset+i)*w + xoffset], &d[i*padded_row_len], width, 1, padded_row_len, format);
		}

	} else {  //CUBE_MAP
		u32* texdata = (u32*)tex->data;

		int w = tex->w;

		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X; //use target as plane index

		int p = w*w;

		for (int i=0; i<height; ++i) {
			convert_format_to_packed_rgba((u8*)&texdata[p*target + (yoffset+i)*w + xoffset], &d[i*padded_row_len], width, 1, padded_row_len, format);
		}
	} //end CUBE_MAP
}

PGLDEF void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_UNUSED(level);

	PGL_ERR((target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY), GL_INVALID_ENUM);
	PGL_ERR((width < 0 || width > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((height < 0 || height > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR((depth < 0 || depth > PGL_MAX_TEXTURE_SIZE), GL_INVALID_VALUE);
	PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);

	int components;
#ifdef PGL_DONT_CONVERT_TEXTURES
	PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);
	components = 4;
#else
	CHECK_FORMAT_GET_COMP(format, components);
#endif

	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	int target_idx = target-GL_TEXTURE_UNBOUND-1;
	int cur_tex_i = c->bound_textures[target_idx];
	glTexture* tex = NULL;
	if (cur_tex_i) {
		tex = &c->textures.a[cur_tex_i];
	} else {
		tex = &c->default_textures[target_idx];
	}

	PGL_ERR((xoffset < 0 || xoffset + width > tex->w ||
	         yoffset < 0 || yoffset + height > tex->h ||
	         zoffset < 0 || zoffset + depth > tex->d), GL_INVALID_VALUE);

	int w = tex->w;
	int h = tex->h;
	int p = w*h;
	int pp = h*padded_row_len;
	u8* d = (u8*)data;
	u32* texdata = (u32*)tex->data;
	u8* out;
	u8* in;

	for (int j=0; j<depth; ++j) {
		for (int i=0; i<height; ++i) {
			out = (u8*)&texdata[(zoffset+j)*p + (yoffset+i)*w + xoffset];
			in = &d[j*pp + i*padded_row_len];
			convert_format_to_packed_rgba(out, in, width, 1, padded_row_len, format);
		}
	}
}

// 1D/2D/CUBE_MAP.  Builds a full RGBA8 box-filtered chain from level 0 into one
// contiguous allocation.  Cubemap levels pack 6 faces each (~4/3 of L0 size).
// 3D/rectangle not supported.
static void pgl_generate_mipmap_tex(glTexture* tex, GLenum target)
{
	PGL_ERR(!tex->data || tex->w <= 0, GL_INVALID_OPERATION);
	if (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP) {
		PGL_ERR(tex->h <= 0, GL_INVALID_OPERATION);
	}

	if (target == GL_TEXTURE_1D) {
		if (tex->w <= 1) {
			tex->num_levels = 1;
			pgl_set_level0_desc(tex);
			return;
		}

		int levels = 1;
		GLsizei dim = tex->w;
		while (dim > 1) {
			dim = dim / 2;
			levels++;
		}
		if (levels > PGL_MAX_MIPMAP_LEVELS)
			levels = PGL_MAX_MIPMAP_LEVELS;

		if (!pgl_alloc_mip_chain_1d(tex, levels)) {
			PGL_SET_ERR_RET(GL_OUT_OF_MEMORY);
		}

		for (int level = 1; level < levels; ++level) {
			pgl_box_filter_1d(
				tex->levels[level - 1].data, tex->levels[level - 1].w,
				tex->levels[level].data, tex->levels[level].w);
		}
		return;
	}

	if (target == GL_TEXTURE_CUBE_MAP) {
		// Faces are square; filter each of the 6 faces independently per level
		if (tex->w <= 1 && tex->h <= 1) {
			tex->num_levels = 1;
			pgl_set_level0_desc(tex);
			return;
		}

		int levels = 1;
		GLsizei cw = tex->w, ch = tex->h;
		while (cw > 1 || ch > 1) {
			cw = cw > 1 ? cw / 2 : 1;
			ch = ch > 1 ? ch / 2 : 1;
			levels++;
		}
		if (levels > PGL_MAX_MIPMAP_LEVELS)
			levels = PGL_MAX_MIPMAP_LEVELS;

		if (!pgl_alloc_mip_chain_cube(tex, levels)) {
			PGL_SET_ERR_RET(GL_OUT_OF_MEMORY);
		}

		for (int level = 1; level < levels; ++level) {
			GLsizei sw = tex->levels[level - 1].w;
			GLsizei sh = tex->levels[level - 1].h;
			GLsizei dw = tex->levels[level].w;
			GLsizei dh = tex->levels[level].h;
			size_t src_face = pgl_rgba_bytes_2d(sw, sh);
			size_t dst_face = pgl_rgba_bytes_2d(dw, dh);
			const u8* src = tex->levels[level - 1].data;
			u8* dst = tex->levels[level].data;
			for (int face = 0; face < 6; ++face) {
				pgl_box_filter_2d(src + (size_t)face * src_face, sw, sh,
				                  dst + (size_t)face * dst_face, dw, dh);
			}
		}
		return;
	}

	// GL_TEXTURE_2D
	if (tex->w <= 1 && tex->h <= 1) {
		tex->num_levels = 1;
		pgl_set_level0_desc(tex);
		return;
	}

	int levels = 1;
	GLsizei cw = tex->w, ch = tex->h;
	while (cw > 1 || ch > 1) {
		cw = cw > 1 ? cw / 2 : 1;
		ch = ch > 1 ? ch / 2 : 1;
		levels++;
	}
	if (levels > PGL_MAX_MIPMAP_LEVELS)
		levels = PGL_MAX_MIPMAP_LEVELS;

	if (!pgl_alloc_mip_chain_2d(tex, levels)) {
		PGL_SET_ERR_RET(GL_OUT_OF_MEMORY);
	}

	for (int level = 1; level < levels; ++level) {
		pgl_box_filter_2d(
			tex->levels[level - 1].data, tex->levels[level - 1].w, tex->levels[level - 1].h,
			tex->levels[level].data, tex->levels[level].w, tex->levels[level].h);
	}
}

// DSA: texture must be a non-zero existing object (not default texture 0)
PGLDEF void glGenerateTextureMipmap(GLuint texture)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted),
	        GL_INVALID_OPERATION);

	glTexture* tex = &c->textures.a[texture];
	// type is stored as target - GL_TEXTURE_UNBOUND - 1
	GLenum target = tex->type + GL_TEXTURE_UNBOUND + 1;
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_CUBE_MAP), GL_INVALID_OPERATION);

	pgl_generate_mipmap_tex(tex, target);
}

PGLDEF void glGenerateMipmap(GLenum target)
{
	PGL_ERR((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	int target_idx = target - GL_TEXTURE_UNBOUND - 1;
	GLuint cur_tex = c->bound_textures[target_idx];
	if (cur_tex) {
		glGenerateTextureMipmap(cur_tex);
	} else {
		// Default texture for this target (DSA path rejects texture 0 regardless
		// of Core or Compatibility because it was defined against Core)
		//
		// Core profile removed default textures (0 is "unbound" instead)
		// Compatibility kept it but it's "Legacy"
		//
		// but since PGL is more Compatibility-ish, we need to allow it here
		// TODO PGL_CORE macro to enforce strict Core compliance?
		pgl_generate_mipmap_tex(&c->default_textures[target_idx], target);
	}
}

PGLDEF void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer)
{
	// See Section 2.8 pages 37-38 of 3.3 compatiblity spec
	//
	// Compare with Section 2.8 page 29 of 3.3 core spec
	// plus section E.2.2, pg 344 (VAOs required for everything, no default/0 VAO)
	//
	// GLES 2 and 3 match 3.3 compatibility profile
	//
	// Basically, core got rid of client arrays entirely, while compatibility
	// allows them for the default/0 VAO.
	//
	// So for now I've decided to match the compatibility profile
	// but you can easily remove c->cur_vertex_array from the check
	// below to enable client arrays for all VAOs; there's not really
	// any downside in PGL, it's all RAM.
	PGL_ERR((c->cur_vertex_array && !c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER] && pointer),
	        GL_INVALID_OPERATION);

	PGL_ERR(stride < 0, GL_INVALID_VALUE);
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	PGL_ERR((size < 1 || size > 4), GL_INVALID_VALUE);

	int type_sz = 4;
	switch (type) {
	case GL_BYTE:           type_sz = sizeof(GLbyte); break;
	case GL_UNSIGNED_BYTE:  type_sz = sizeof(GLubyte); break;
	case GL_SHORT:          type_sz = sizeof(GLshort); break;
	case GL_UNSIGNED_SHORT: type_sz = sizeof(GLushort); break;
	case GL_INT:            type_sz = sizeof(GLint); break;
	case GL_UNSIGNED_INT:   type_sz = sizeof(GLuint); break;

	case GL_FLOAT:  type_sz = sizeof(GLfloat); break;
	case GL_DOUBLE: type_sz = sizeof(GLdouble); break;

	default:
		PGL_SET_ERR_RET(GL_INVALID_ENUM);
	}

	glVertex_Attrib* v = &(c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index]);
	v->size = size;
	v->type = type;
	v->normalized = normalized;
	v->stride = (stride) ? stride : size*type_sz;

	// offset can still really be a pointer if using the 0 VAO and no bound ARRAY_BUFFER.
	v->offset = (GLsizeiptr)pointer;
	// I put ARRAY_BUFFER-itself instead of 0 to reinforce that bound_buffers is indexed that way, buffer type - GL_ARRAY_BUFFER
	v->buf = c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER];
}

PGLDEF void glEnableVertexAttribArray(GLuint index)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_TRUE;
}

PGLDEF void glDisableVertexAttribArray(GLuint index)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_FALSE;
}

PGLDEF void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);

	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_TRUE;
}

PGLDEF void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);
	PGL_ERR((vaobj >= c->vertex_arrays.size || c->vertex_arrays.a[vaobj].deleted), GL_INVALID_OPERATION);
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_FALSE;
}

PGLDEF void glVertexAttribDivisor(GLuint index, GLuint divisor)
{
	PGL_ERR(index >= GL_MAX_VERTEX_ATTRIBS, GL_INVALID_VALUE);

	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].divisor = divisor;
}



//TODO(rswinkle): Why is first, an index, a GLint and not GLuint or GLsizei?
PGLDEF void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR(count < 0, GL_INVALID_VALUE);
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);

	if (!count)
		return;

	run_pipeline(mode, (GLvoid*)(GLintptr)first, count, 0, 0, GL_FALSE);
}

PGLDEF void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR(drawcount < 0, GL_INVALID_VALUE);

	for (GLsizei i=0; i<drawcount; i++) {
		if (!count[i]) continue;
		run_pipeline(mode, (GLvoid*)(GLintptr)first[i], count[i], 0, 0, GL_FALSE);
	}
}

PGLDEF void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR(count < 0, GL_INVALID_VALUE);

	// TODO error not in the spec but says type must be one of these ... strange
	PGL_ERR((type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT), GL_INVALID_ENUM);
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);

	if (!count)
		return;

	run_pipeline(mode, indices, count, 0, 0, type);
}

// TODO fix
PGLDEF void glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const GLvoid* const* indices, GLsizei drawcount)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR(drawcount < 0, GL_INVALID_VALUE);

	// TODO error not in the spec but says type must be one of these ... strange
	PGL_ERR((type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT), GL_INVALID_ENUM);

	for (GLsizei i=0; i<drawcount; i++) {
		if (!count[i]) continue;
		run_pipeline(mode, indices[i], count[i], 0, 0, type);
	}
}

PGLDEF void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR((count < 0 || instancecount < 0), GL_INVALID_VALUE);

	if (!count || !instancecount)
		return;

	for (GLsizei instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, (GLvoid*)(GLintptr)first, count, instance, 0, GL_FALSE);
	}
}

PGLDEF void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR((count < 0 || instancecount < 0), GL_INVALID_VALUE);

	if (!count || !instancecount)
		return;

	for (GLsizei instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, (GLvoid*)(GLintptr)first, count, instance, baseinstance, GL_FALSE);
	}
}


PGLDEF void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR((count < 0 || instancecount < 0), GL_INVALID_VALUE);

	// NOTE: error not in the spec but says type must be one of these ... strange
	PGL_ERR((type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT), GL_INVALID_ENUM);

	if (!count || !instancecount)
		return;

	for (GLsizei instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, indices, count, instance, 0, type);
	}
}

PGLDEF void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instancecount, GLuint baseinstance)
{
	PGL_ERR((mode < GL_POINTS || mode > GL_TRIANGLE_FAN), GL_INVALID_ENUM);
	PGL_ERR((count < 0 || instancecount < 0), GL_INVALID_VALUE);

	//error not in the spec but says type must be one of these ... strange
	PGL_ERR((type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT), GL_INVALID_ENUM);

	if (!count || !instancecount)
		return;

	for (GLsizei instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, indices, count, instance, baseinstance, GL_TRUE);
	}
}

PGLDEF void glDebugMessageCallback(GLDEBUGPROC callback, void* userParam)
{
	c->dbg_callback = callback;
	c->dbg_userparam = userParam;
}

PGLDEF void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	PGL_ERR((width < 0 || height < 0), GL_INVALID_VALUE);

	// TODO: Do I need a full matrix? Also I don't actually
	// use these values anywhere else so why save them?  See ref pages or TinyGL for alternative
	make_viewport_m4(c->vp_mat, x, y, width, height, 1);
	c->xmin = x;
	c->ymin = y;
	c->width = width;
	c->height = height;
}

PGLDEF void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	red = clamp_01(red);
	green = clamp_01(green);
	blue = clamp_01(blue);
	alpha = clamp_01(alpha);

	//vec4 tmp = { red, green, blue, alpha };
	//c->clear_color = vec4_to_Color(tmp);
	c->clear_color = RGBA_TO_PIXEL(red*PGL_RMAX, green*PGL_GMAX, blue*PGL_BMAX, alpha*PGL_AMAX);
}

PGLDEF void glClearDepthf(GLfloat depth)
{
	c->clear_depth = clamp_01(depth);
}

PGLDEF void glClearDepth(GLdouble depth)
{
	c->clear_depth = clamp_01(depth);
}

PGLDEF void glDepthFunc(GLenum func)
{
	PGL_ERR((func < GL_LESS || func > GL_NEVER), GL_INVALID_ENUM);

	c->depth_func = func;
}

PGLDEF void glDepthRangef(GLfloat nearVal, GLfloat farVal)
{
	c->depth_range_near = clamp_01(nearVal);
	c->depth_range_far = clamp_01(farVal);
}

PGLDEF void glDepthRange(GLdouble nearVal, GLdouble farVal)
{
	c->depth_range_near = clamp_01(nearVal);
	c->depth_range_far = clamp_01(farVal);
}

PGLDEF void glDepthMask(GLboolean flag)
{
	c->depth_mask = flag;
}

PGLDEF void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
#ifndef PGL_DISABLE_COLOR_MASK
	// !! ensures 1 or 0
	red = !!red;
	green = !!green;
	blue = !!blue;
	alpha = !!alpha;

	// By multiplying by the pixel format masks there's no need to shift them
	pix_t rmask = red*PGL_RMASK;
	pix_t gmask = green*PGL_GMASK;
	pix_t bmask = blue*PGL_BMASK;
	pix_t amask = alpha*PGL_AMASK;
	c->color_mask = rmask | gmask | bmask | amask;
#endif
}

PGLDEF void glClear(GLbitfield mask)
{
	// TODO: If a buffer is not present, then a glClear directed at that buffer has no effect.
	// right now they're all always present

	PGL_ERR((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)), GL_INVALID_VALUE);
	PGL_ERR(!pgl_draw_framebuffer_ok(), GL_INVALID_FRAMEBUFFER_OPERATION);

	// NOTE: All buffers should have the same dimensions/size
	int sz = c->ux * c->uy;
	int w = c->back_buffer.w;

	pix_t color = c->clear_color;

#ifndef PGL_DISABLE_COLOR_MASK
	// clear out channels not enabled for writing
	// TODO are these casts really necessary?
	color &= (pix_t)c->color_mask;
	// used to erase channels to be written
	pix_t clear_mask = ~((pix_t)c->color_mask);
	pix_t tmp;
#endif

#ifndef PGL_NO_DEPTH_NO_STENCIL
	u32 cd = (u32)(c->clear_depth * PGL_MAX_Z) << PGL_ZSHIFT;
#  ifndef PGL_NO_STENCIL
    u8 cs = c->clear_stencil;
#  endif
#endif
	if (!c->scissor_test) {
		if (mask & GL_COLOR_BUFFER_BIT) {
			if (c->fbo_color_is_rt) {
				// Clear FBO color attachments in their storage format (from packed clear_color)
				Color cc = PIXEL_TO_COLOR(c->clear_color);
				float fr = cc.r / (float)PGL_RMAX, fg = cc.g / (float)PGL_GMAX;
				float fb = cc.b / (float)PGL_BMAX, fa = cc.a / (float)PGL_AMAX;
				for (GLsizei di = 0; di < c->num_draw_buffers; ++di) {
					if (c->draw_buffers[di] == GL_NONE) continue;
					int att = (int)(c->draw_buffers[di] - GL_COLOR_ATTACHMENT0);
					PGL_ASSERT(att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS);
					// Desktop completeness: non-NONE draw buffer ⇒ attached image
					PGL_ASSERT(c->mrt_color[att].buf);
					pglColorRT* rt = &c->mrt_color[att];
					int bsz = rt->w * rt->h;
					if (rt->datatype == GL_FLOAT) {
						PGL_ASSERT(rt->components > 0);
						const int nc = rt->components;
						float* p = (float*)rt->buf;
						for (int i = 0; i < bsz; ++i) {
							float* t = p + i * nc;
							t[0] = fr;
							if (nc > 1) t[1] = fg;
							if (nc > 2) t[2] = fb;
							if (nc > 3) t[3] = fa;
						}
					} else {
						Color col = VEC4_TO_COLOR(make_v4(fr, fg, fb, fa));
						Color* p = (Color*)rt->buf;
						for (int i = 0; i < bsz; ++i)
							p[i] = col;
					}
				}
			} else {
				pix_t* buf = (pix_t*)c->back_buffer.buf;
				int bsz = c->back_buffer.w * c->back_buffer.h;
				for (int i = 0; i < bsz; ++i) {
#ifdef PGL_DISABLE_COLOR_MASK
					buf[i] = color;
#else
					tmp = buf[i];
					tmp &= clear_mask;
					buf[i] = tmp | color;
#endif
				}
			}
		}
#ifndef PGL_NO_DEPTH_NO_STENCIL
		if (mask & GL_DEPTH_BUFFER_BIT && c->depth_mask) {
			if (c->zbuf_float) {
				float* z = (float*)c->zbuf.buf;
				int zsz = c->zbuf.w * c->zbuf.h;
				for (int i = 0; i < zsz; ++i)
					z[i] = c->clear_depth;
			} else {
				for (int i=0; i < sz; ++i) {
					SET_Z_PRESHIFTED_TOP(i, cd);
				}
			}
		}

#ifndef PGL_NO_STENCIL
		if (mask & GL_STENCIL_BUFFER_BIT) {
#  ifdef PGL_D16
			memset(c->stencil_buf.buf, cs, sz);
#  else
			for (int i=0; i < sz; ++i) {
				SET_STENCIL_TOP(i, cs);
			}
#  endif
		}
#  endif
#endif
	} else {
		// TODO this code is correct with or without scissor
		// enabled, test performance difference with above before
		// getting rid of above
		if (mask & GL_COLOR_BUFFER_BIT) {
			if (c->fbo_color_is_rt) {
				Color cc = PIXEL_TO_COLOR(c->clear_color);
				float fr = cc.r / (float)PGL_RMAX, fg = cc.g / (float)PGL_GMAX;
				float fb = cc.b / (float)PGL_BMAX, fa = cc.a / (float)PGL_AMAX;
				for (GLsizei di = 0; di < c->num_draw_buffers; ++di) {
					if (c->draw_buffers[di] == GL_NONE) continue;
					int att = (int)(c->draw_buffers[di] - GL_COLOR_ATTACHMENT0);
					PGL_ASSERT(att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS);
					PGL_ASSERT(c->mrt_color[att].buf);
					pglColorRT* rt = &c->mrt_color[att];
					int bw = rt->w;
					for (int y = c->ly; y < c->uy; ++y) {
						for (int x = c->lx; x < c->ux; ++x) {
							int i = -y * bw + x;
							if (rt->datatype == GL_FLOAT) {
								PGL_ASSERT(rt->components > 0);
								const int nc = rt->components;
								float* t = (float*)rt->lastrow + i * nc;
								t[0] = fr;
								if (nc > 1) t[1] = fg;
								if (nc > 2) t[2] = fb;
								if (nc > 3) t[3] = fa;
							} else {
								((Color*)rt->lastrow)[i] = VEC4_TO_COLOR(make_v4(fr, fg, fb, fa));
							}
						}
					}
				}
			} else {
				for (int y = c->ly; y < c->uy; ++y) {
					for (int x = c->lx; x < c->ux; ++x) {
						int i = -y * w + x;
#ifdef PGL_DISABLE_COLOR_MASK
						((pix_t*)c->back_buffer.lastrow)[i] = color;
#else
						tmp = ((pix_t*)c->back_buffer.lastrow)[i];
						tmp &= clear_mask;
						((pix_t*)c->back_buffer.lastrow)[i] = tmp | color;
#endif
					}
				}
			}
		}
#ifndef PGL_NO_DEPTH_NO_STENCIL
		if (mask & GL_DEPTH_BUFFER_BIT && c->depth_mask) {
			for (int y=c->ly; y<c->uy; ++y) {
				for (int x=c->lx; x<c->ux; ++x) {
					int i = -y*w + x;
					SET_Z_PRESHIFTED(i, cd);
				}
			}
		}
#  ifndef PGL_NO_STENCIL
		if (mask & GL_STENCIL_BUFFER_BIT) {
			for (int y=c->ly; y<c->uy; ++y) {
				for (int x=c->lx; x<c->ux; ++x) {
					int i = -y*w + x;
					SET_STENCIL(i, cs);
				}
			}
		}
#  endif
#endif
	}
}

PGLDEF void glEnable(GLenum cap)
{
	switch (cap) {
	case GL_CULL_FACE:
		c->cull_face = GL_TRUE;
		break;
	case GL_DEPTH_TEST:
		c->depth_test = GL_TRUE;
		break;
	case GL_DEPTH_CLAMP:
		c->depth_clamp = GL_TRUE;
		break;
	case GL_LINE_SMOOTH:
		// TODO implementation needs work/upgrade
		c->line_smooth = GL_TRUE;
		break;
	case GL_BLEND:
		c->blend = GL_TRUE;
		break;
	case GL_COLOR_LOGIC_OP:
		c->logic_ops = GL_TRUE;
		break;
	case GL_POLYGON_OFFSET_POINT:
		c->poly_offset_pt = GL_TRUE;
		break;
	case GL_POLYGON_OFFSET_LINE:
		c->poly_offset_line = GL_TRUE;
		break;
	case GL_POLYGON_OFFSET_FILL:
		c->poly_offset_fill = GL_TRUE;
		break;
	case GL_SCISSOR_TEST: {
		c->scissor_test = GL_TRUE;
		int ux = c->scissor_lx+c->scissor_w;
		int uy = c->scissor_ly+c->scissor_h;
		c->lx = MAX(c->scissor_lx, 0);
		c->ly = MAX(c->scissor_ly, 0);
		c->ux = MIN(ux, c->back_buffer.w);
		c->uy = MIN(uy, c->back_buffer.h);
	} break;
	case GL_STENCIL_TEST:
#ifndef PGL_NO_STENCIL
		c->stencil_test = GL_TRUE;
#endif
		break;
	case GL_DEBUG_OUTPUT:
		c->dbg_output = GL_TRUE;
		break;
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF void glDisable(GLenum cap)
{
	switch (cap) {
	case GL_CULL_FACE:
		c->cull_face = GL_FALSE;
		break;
	case GL_DEPTH_TEST:
		c->depth_test = GL_FALSE;
		break;
	case GL_DEPTH_CLAMP:
		c->depth_clamp = GL_FALSE;
		break;
	case GL_LINE_SMOOTH:
		c->line_smooth = GL_FALSE;
		break;
	case GL_BLEND:
		c->blend = GL_FALSE;
		break;
	case GL_COLOR_LOGIC_OP:
		c->logic_ops = GL_FALSE;
		break;
	case GL_POLYGON_OFFSET_POINT:
		c->poly_offset_pt = GL_FALSE;
		break;
	case GL_POLYGON_OFFSET_LINE:
		c->poly_offset_line = GL_FALSE;
		break;
	case GL_POLYGON_OFFSET_FILL:
		c->poly_offset_fill = GL_FALSE;
		break;
	case GL_SCISSOR_TEST:
		c->scissor_test = GL_FALSE;
		c->lx = 0;
		c->ly = 0;
		c->ux = c->back_buffer.w;
		c->uy = c->back_buffer.h;
		break;
	case GL_STENCIL_TEST:
#ifndef PGL_NO_STENCIL
		c->stencil_test = GL_FALSE;
#endif
		break;
	case GL_DEBUG_OUTPUT:
		c->dbg_output = GL_FALSE;
		break;
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF GLboolean glIsEnabled(GLenum cap)
{
	// make up my own enum for this?  rename member as no_early_z?
	//GLboolean fragdepth_or_discard;
	switch (cap) {
	case GL_DEPTH_TEST: return c->depth_test;
	case GL_LINE_SMOOTH: return c->line_smooth;
	case GL_CULL_FACE: return c->cull_face;
	case GL_DEPTH_CLAMP: return c->depth_clamp;
	case GL_BLEND: return c->blend;
	case GL_COLOR_LOGIC_OP: return c->logic_ops;
	case GL_POLYGON_OFFSET_POINT: return c->poly_offset_pt;
	case GL_POLYGON_OFFSET_LINE: return c->poly_offset_line;
	case GL_POLYGON_OFFSET_FILL: return c->poly_offset_fill;
	case GL_SCISSOR_TEST: return c->scissor_test;
#ifndef PGL_NO_STENCIL
	case GL_STENCIL_TEST: return c->stencil_test;
#endif
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}

	return GL_FALSE;
}

PGLDEF GLboolean glIsProgram(GLuint program)
{
	if (!program || program >= c->programs.size || c->programs.a[program].deleted) {
		return GL_FALSE;
	}

	return GL_TRUE;
}

PGLDEF void glGetBooleanv(GLenum pname, GLboolean* data)
{
	// not sure it's worth adding every enum, spec says
	// gelGet* will convert/map types if they don't match the function
	switch (pname) {
	case GL_DEPTH_TEST:           *data = c->depth_test;       break;
	case GL_LINE_SMOOTH:          *data = c->line_smooth;      break;
	case GL_CULL_FACE:            *data = c->cull_face;        break;
	case GL_DEPTH_CLAMP:          *data = c->depth_clamp;      break;
	case GL_BLEND:                *data = c->blend;            break;
	case GL_COLOR_LOGIC_OP:       *data = c->logic_ops;        break;
	case GL_POLYGON_OFFSET_POINT: *data = c->poly_offset_pt;  break;
	case GL_POLYGON_OFFSET_LINE:  *data = c->poly_offset_line; break;
	case GL_POLYGON_OFFSET_FILL:  *data = c->poly_offset_fill; break;
	case GL_SCISSOR_TEST:         *data = c->scissor_test;     break;
#ifndef PGL_NO_STENCIL
	case GL_STENCIL_TEST:         *data = c->stencil_test;     break;
#endif
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF void glGetFloatv(GLenum pname, GLfloat* data)
{
	switch (pname) {
	case GL_POLYGON_OFFSET_FACTOR:         *data = c->poly_factor;         break;
	case GL_POLYGON_OFFSET_UNITS:          *data = c->poly_units;          break;
	case GL_POINT_SIZE:                    *data = c->point_size;          break;
	case GL_LINE_WIDTH:                    *data = c->line_width;          break;
	case GL_DEPTH_CLEAR_VALUE:             *data = c->clear_depth;         break;
	case GL_SMOOTH_LINE_WIDTH_GRANULARITY: *data = PGL_SMOOTH_GRANULARITY; break;

	case GL_MAX_TEXTURE_SIZE:         *data = PGL_MAX_TEXTURE_SIZE;         break;
	case GL_MAX_3D_TEXTURE_SIZE:      *data = PGL_MAX_3D_TEXTURE_SIZE;      break;
	case GL_MAX_ARRAY_TEXTURE_LAYERS: *data = PGL_MAX_ARRAY_TEXTURE_LAYERS; break;

	case GL_ALIASED_LINE_WIDTH_RANGE:
		data[0] = 1.0f;
		data[1] = PGL_MAX_ALIASED_WIDTH;
		break;

	case GL_SMOOTH_LINE_WIDTH_RANGE:
		data[0] = 1.0f;
		data[1] = PGL_MAX_SMOOTH_WIDTH;
		break;

	case GL_DEPTH_RANGE:
		data[0] = c->depth_range_near;
		data[1] = c->depth_range_near;
		break;
	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF void glGetIntegerv(GLenum pname, GLint* data)
{
	// TODO maybe make all the enum/int member names match the associated ENUM?
	switch (pname) {
#ifndef PGL_NO_STENCIL
	case GL_STENCIL_WRITE_MASK:       data[0] = c->stencil_writemask; break;
	case GL_STENCIL_REF:              data[0] = c->stencil_ref; break;
	case GL_STENCIL_VALUE_MASK:       data[0] = c->stencil_valuemask; break;
	case GL_STENCIL_FUNC:             data[0] = c->stencil_func; break;
	case GL_STENCIL_FAIL:             data[0] = c->stencil_sfail; break;
	case GL_STENCIL_PASS_DEPTH_FAIL:  data[0] = c->stencil_dpfail; break;
	case GL_STENCIL_PASS_DEPTH_PASS:  data[0] = c->stencil_dppass; break;

	case GL_STENCIL_BACK_WRITE_MASK:      data[0] = c->stencil_writemask_back; break;
	case GL_STENCIL_BACK_REF:             data[0] = c->stencil_ref_back; break;
	case GL_STENCIL_BACK_VALUE_MASK:      data[0] = c->stencil_valuemask_back; break;
	case GL_STENCIL_BACK_FUNC:            data[0] = c->stencil_func_back; break;
	case GL_STENCIL_BACK_FAIL:            data[0] = c->stencil_sfail_back; break;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL: data[0] = c->stencil_dpfail_back; break;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS: data[0] = c->stencil_dppass_back; break;
#endif

	case GL_LOGIC_OP_MODE:             data[0] = c->logic_func; break;

	//TODO implement glBlendFuncSeparate and glBlendEquationSeparate
	case GL_BLEND_SRC_RGB:             data[0] = c->blend_sRGB; break;
	case GL_BLEND_SRC_ALPHA:           data[0] = c->blend_sA; break;
	case GL_BLEND_DST_RGB:             data[0] = c->blend_dRGB; break;
	case GL_BLEND_DST_ALPHA:           data[0] = c->blend_dA; break;

	case GL_BLEND_EQUATION_RGB:        data[0] = c->blend_eqRGB; break;
	case GL_BLEND_EQUATION_ALPHA:      data[0] = c->blend_eqA; break;

	case GL_CULL_FACE_MODE:            data[0] = c->cull_mode; break;
	case GL_FRONT_FACE:                data[0] = c->front_face; break;
	case GL_DEPTH_FUNC:                data[0] = c->depth_func; break;
	case GL_POINT_SPRITE_COORD_ORIGIN: data[0] = c->point_spr_origin; break;
	case GL_PROVOKING_VERTEX:          data[0] = c->provoking_vert; break;

	case GL_MAX_TEXTURE_SIZE:         data[0] = PGL_MAX_TEXTURE_SIZE;         break;
	case GL_MAX_3D_TEXTURE_SIZE:      data[0] = PGL_MAX_3D_TEXTURE_SIZE;      break;
	case GL_MAX_ARRAY_TEXTURE_LAYERS: data[0] = PGL_MAX_ARRAY_TEXTURE_LAYERS; break;

	case GL_MAX_DEBUG_MESSAGE_LENGTH: data[0] = PGL_MAX_DEBUG_MESSAGE_LENGTH; break;

	case GL_POLYGON_MODE:
		data[0] = c->poly_mode_front;
		data[1] = c->poly_mode_back;
		break;

	case GL_VIEWPORT:
		data[0] = c->xmin;
		data[1] = c->ymin;
		data[2] = c->width;
		data[3] = c->height;
		break;

	case GL_SCISSOR_BOX:
		data[0] = c->scissor_lx;
		data[1] = c->scissor_ly;
		data[2] = c->scissor_w;
		data[3] = c->scissor_h;
		break;

	// TODO decide if 3.2 is the best approximation
	case GL_MAJOR_VERSION:             data[0] = 3; break;
	case GL_MINOR_VERSION:             data[0] = 2; break;

	case GL_ARRAY_BUFFER_BINDING:
		data[0] = c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER];
		break;

	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		data[0] = c->bound_buffers[GL_ELEMENT_ARRAY_BUFFER-GL_ARRAY_BUFFER];
		break;

	case GL_VERTEX_ARRAY_BINDING:
		data[0] = c->cur_vertex_array;
		break;

	case GL_CURRENT_PROGRAM:
		data[0] = c->cur_program;
		break;


	case GL_TEXTURE_BINDING_1D:        data[0] = c->bound_textures[GL_TEXTURE_1D-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_2D:        data[0] = c->bound_textures[GL_TEXTURE_2D-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_3D:        data[0] = c->bound_textures[GL_TEXTURE_3D-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_1D_ARRAY:  data[0] = c->bound_textures[GL_TEXTURE_1D_ARRAY-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_2D_ARRAY:  data[0] = c->bound_textures[GL_TEXTURE_2D_ARRAY-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_RECTANGLE: data[0] = c->bound_textures[GL_TEXTURE_RECTANGLE-GL_TEXTURE_UNBOUND-1]; break;
	case GL_TEXTURE_BINDING_CUBE_MAP:  data[0] = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1]; break;

	default:
		PGL_SET_ERR(GL_INVALID_ENUM);
	}
}

PGLDEF void glCullFace(GLenum mode)
{
	PGL_ERR((mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK), GL_INVALID_ENUM);
	c->cull_mode = mode;
}

PGLDEF void glFrontFace(GLenum mode)
{
	PGL_ERR((mode != GL_CCW && mode != GL_CW), GL_INVALID_ENUM);
	c->front_face = mode;
}

PGLDEF void glPolygonMode(GLenum face, GLenum mode)
{
	// TODO only support FRONT_AND_BACK like OpenGL 3/4 and OpenGL ES 2/3 ...
	// or keep support for FRONT and BACK like OpenGL 1 and 2?
	// Make final decision before version 1.0.0
	PGL_ERR(((face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) ||
	         (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL)), GL_INVALID_ENUM);

	if (mode == GL_POINT) {
		if (face == GL_FRONT) {
			c->poly_mode_front = mode;
			c->draw_triangle_front = draw_triangle_point;
		} else if (face == GL_BACK) {
			c->poly_mode_back = mode;
			c->draw_triangle_back = draw_triangle_point;
		} else {
			c->poly_mode_front = mode;
			c->poly_mode_back = mode;
			c->draw_triangle_front = draw_triangle_point;
			c->draw_triangle_back = draw_triangle_point;
		}
	} else if (mode == GL_LINE) {
		if (face == GL_FRONT) {
			c->poly_mode_front = mode;
			c->draw_triangle_front = draw_triangle_line;
		} else if (face == GL_BACK) {
			c->poly_mode_back = mode;
			c->draw_triangle_back = draw_triangle_line;
		} else {
			c->poly_mode_front = mode;
			c->poly_mode_back = mode;
			c->draw_triangle_front = draw_triangle_line;
			c->draw_triangle_back = draw_triangle_line;
		}
	} else  {
		if (face == GL_FRONT) {
			c->poly_mode_front = mode;
			c->draw_triangle_front = draw_triangle_fill;
		} else if (face == GL_BACK) {
			c->poly_mode_back = mode;
			c->draw_triangle_back = draw_triangle_fill;
		} else {
			c->poly_mode_front = mode;
			c->poly_mode_back = mode;
			c->draw_triangle_front = draw_triangle_fill;
			c->draw_triangle_back = draw_triangle_fill;
		}
	}
}

PGLDEF void glLineWidth(GLfloat width)
{
	PGL_ERR(width <= 0.0f, GL_INVALID_VALUE);
	c->line_width = width;
}

PGLDEF void glPointSize(GLfloat size)
{
	PGL_ERR(size <= 0.0f, GL_INVALID_VALUE);
	c->point_size = size;
}

PGLDEF void glPointParameteri(GLenum pname, GLint param)
{
	//also GL_POINT_FADE_THRESHOLD_SIZE
	PGL_ERR((pname != GL_POINT_SPRITE_COORD_ORIGIN ||
	        (param != GL_LOWER_LEFT && param != GL_UPPER_LEFT)), GL_INVALID_ENUM);

	c->point_spr_origin = param;
}

PGLDEF void glProvokingVertex(GLenum provokeMode)
{
	PGL_ERR((provokeMode != GL_FIRST_VERTEX_CONVENTION && provokeMode != GL_LAST_VERTEX_CONVENTION), GL_INVALID_ENUM);

	c->provoking_vert = provokeMode;
}


// Shader functions
PGLDEF GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean fragdepth_or_discard)
{
	// Using glAttachShader error if shader is not a shader object which
	// is the closest analog
	PGL_ERR_RET_VAL((!vertex_shader || !fragment_shader), GL_INVALID_OPERATION, 0);

	PGL_ERR_RET_VAL((n < 0 || n > GL_MAX_VERTEX_OUTPUT_COMPONENTS), GL_INVALID_VALUE, 0);

	glProgram tmp = {vertex_shader, fragment_shader, NULL, n, {0}, fragdepth_or_discard, GL_FALSE };
	for (int i=0; i<n; ++i) {
		tmp.interpolation[i] = interpolation[i];
	}

	for (int i=1; i<c->programs.size; ++i) {
		if (c->programs.a[i].deleted && (GLuint)i != c->cur_program) {
			c->programs.a[i] = tmp;
			return i;
		}
	}

	cvec_push_glProgram(&c->programs, tmp);
	return c->programs.size-1;
}

// Doesn't really do anything except mark for re-use, you
// could still use it even if it wasn't current as long as
// no new program get's assigned to the same spot
PGLDEF void glDeleteProgram(GLuint program)
{
	// This check isn't really necessary since "deleting" only marks it
	// and CreateProgram will never overwrite the 0/default shader
	if (!program)
		return;

	PGL_ERR(program >= c->programs.size, GL_INVALID_VALUE);

	c->programs.a[program].deleted = GL_TRUE;
}

PGLDEF void glUseProgram(GLuint program)
{
	// Not a problem if program is marked "deleted" already
	PGL_ERR(program >= c->programs.size, GL_INVALID_VALUE);

	c->vs_output.size = c->programs.a[program].vs_output_size;
	// c->vs_output.output_buf was pre-allocated to max size needed in init_glContext
	// otherwise would need to assure it's at least
	// c->vs_output_size * PGL_MAX_VERTS * sizeof(float) right here
	c->vs_output.interpolation = c->programs.a[program].interpolation;
	c->fragdepth_or_discard = c->programs.a[program].fragdepth_or_discard;

	c->cur_program = program;
}

PGLDEF void pglSetUniform(void* uniform)
{
	//TODO check for NULL? definitely if I ever switch to storing a local
	//copy in glProgram
	c->programs.a[c->cur_program].uniform = uniform;
}

PGLDEF void pglSetProgramUniform(GLuint program, void* uniform)
{
	// can set uniform for a "deleted" program ... but maybe I should still check and just
	// make an exception if it's the current program?
	PGL_ERR(program >= c->programs.size, GL_INVALID_OPERATION);

	c->programs.a[program].uniform = uniform;
}


PGLDEF void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	PGL_ERR((sfactor < GL_ZERO || sfactor >= NUM_BLEND_FUNCS || dfactor < GL_ZERO || dfactor >= NUM_BLEND_FUNCS), GL_INVALID_ENUM);

	c->blend_sRGB = sfactor;
	c->blend_sA = sfactor;
	c->blend_dRGB = dfactor;
	c->blend_dA = dfactor;
}

PGLDEF void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	PGL_ERR((srcRGB < GL_ZERO || srcRGB >= NUM_BLEND_FUNCS ||
	         dstRGB < GL_ZERO || dstRGB >= NUM_BLEND_FUNCS ||
	         srcAlpha < GL_ZERO || srcAlpha >= NUM_BLEND_FUNCS ||
	         dstAlpha < GL_ZERO || dstAlpha >= NUM_BLEND_FUNCS), GL_INVALID_ENUM);

	c->blend_sRGB = srcRGB;
	c->blend_sA = srcAlpha;
	c->blend_dRGB = dstRGB;
	c->blend_dA = dstAlpha;
}

PGLDEF void glBlendEquation(GLenum mode)
{
	PGL_ERR((mode < GL_FUNC_ADD || mode >= NUM_BLEND_EQUATIONS), GL_INVALID_ENUM);

	c->blend_eqRGB = mode;
	c->blend_eqA = mode;
}

PGLDEF void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	PGL_ERR((modeRGB < GL_FUNC_ADD || modeRGB >= NUM_BLEND_EQUATIONS ||
	    modeAlpha < GL_FUNC_ADD || modeAlpha >= NUM_BLEND_EQUATIONS), GL_INVALID_ENUM);

	c->blend_eqRGB = modeRGB;
	c->blend_eqA = modeAlpha;
}

PGLDEF void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	SET_V4(c->blend_color, clamp_01(red), clamp_01(green), clamp_01(blue), clamp_01(alpha));
}

PGLDEF void glLogicOp(GLenum opcode)
{
	PGL_ERR((opcode < GL_CLEAR || opcode > GL_INVERT), GL_INVALID_ENUM);
	c->logic_func = opcode;
}

PGLDEF void glPolygonOffset(GLfloat factor, GLfloat units)
{
	c->poly_factor = factor;
	c->poly_units = units;
}

PGLDEF void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	PGL_ERR((width < 0 || height < 0), GL_INVALID_VALUE);

	c->scissor_lx = x;
	c->scissor_ly = y;
	c->scissor_w = width;
	c->scissor_h = height;
	int ux = x+width;
	int uy = y+height;

	c->lx = MAX(x, 0);
	c->ly = MAX(y, 0);
	c->ux = MIN(ux, c->back_buffer.w);
	c->uy = MIN(uy, c->back_buffer.h);
}

#ifndef PGL_NO_STENCIL
PGLDEF void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	PGL_ERR((func < GL_LESS || func > GL_NEVER), GL_INVALID_ENUM);

	c->stencil_func = func;
	c->stencil_func_back = func;

	// TODO clamp byte function?
	clampi(ref, 0, 255);

	c->stencil_ref = ref;
	c->stencil_ref_back = ref;

	c->stencil_valuemask = mask;
	c->stencil_valuemask_back = mask;
}

PGLDEF void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	PGL_ERR((face < GL_FRONT || face > GL_FRONT_AND_BACK), GL_INVALID_ENUM);
	PGL_ERR((func < GL_LESS || func > GL_NEVER), GL_INVALID_ENUM);

	// TODO clamp byte function?
	clampi(ref, 0, 255);

	// Any better way to do this? I don't call glStencilFunc in case
	// I ever want/need debugging/logging info to show the function call
	if (face == GL_FRONT) {
		c->stencil_func = func;
		c->stencil_ref = ref;
		c->stencil_valuemask = mask;
	} else if (face == GL_BACK) {
		c->stencil_func_back = func;
		c->stencil_ref_back = ref;
		c->stencil_valuemask_back = mask;
	} else {
		c->stencil_func = func;
		c->stencil_ref = ref;
		c->stencil_valuemask = mask;

		c->stencil_func_back = func;
		c->stencil_ref_back = ref;
		c->stencil_valuemask_back = mask;
	}
}

PGLDEF void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)
{
	PGL_ERR((((sfail < GL_INVERT || sfail > GL_DECR_WRAP) && sfail != GL_ZERO) ||
	        ((dpfail < GL_INVERT || dpfail > GL_DECR_WRAP) && dpfail != GL_ZERO) ||
	        ((dppass < GL_INVERT || dppass > GL_DECR_WRAP) && dppass != GL_ZERO)), GL_INVALID_ENUM);

	c->stencil_sfail = sfail;
	c->stencil_dpfail = dpfail;
	c->stencil_dppass = dppass;

	c->stencil_sfail_back = sfail;
	c->stencil_dpfail_back = dpfail;
	c->stencil_dppass_back = dppass;
}

PGLDEF void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
	PGL_ERR((face < GL_FRONT || face > GL_FRONT_AND_BACK), GL_INVALID_ENUM);

	PGL_ERR((((sfail < GL_INVERT || sfail > GL_DECR_WRAP) && sfail != GL_ZERO) ||
	        ((dpfail < GL_INVERT || dpfail > GL_DECR_WRAP) && dpfail != GL_ZERO) ||
	        ((dppass < GL_INVERT || dppass > GL_DECR_WRAP) && dppass != GL_ZERO)), GL_INVALID_ENUM);


	if (face == GL_FRONT) {
		c->stencil_sfail = sfail;
		c->stencil_dpfail = dpfail;
		c->stencil_dppass = dppass;
	} else if (face == GL_BACK) {
		c->stencil_sfail_back = sfail;
		c->stencil_dpfail_back = dpfail;
		c->stencil_dppass_back = dppass;
	} else {
		c->stencil_sfail = sfail;
		c->stencil_dpfail = dpfail;
		c->stencil_dppass = dppass;

		c->stencil_sfail_back = sfail;
		c->stencil_dpfail_back = dpfail;
		c->stencil_dppass_back = dppass;
	}
}

PGLDEF void glClearStencil(GLint s)
{
	c->clear_stencil = s & PGL_STENCIL_MASK;
}

PGLDEF void glStencilMask(GLuint mask)
{
	c->stencil_writemask = mask;
	c->stencil_writemask_back = mask;
}

PGLDEF void glStencilMaskSeparate(GLenum face, GLuint mask)
{
	PGL_ERR((face < GL_FRONT || face > GL_FRONT_AND_BACK), GL_INVALID_ENUM);

	if (face == GL_FRONT) {
		c->stencil_writemask = mask;
	} else if (face == GL_BACK) {
		c->stencil_writemask_back = mask;
	} else {
		c->stencil_writemask = mask;
		c->stencil_writemask_back = mask;
	}
}
#endif


// Just wrap my pgl extension getter, unmap does nothing
PGLDEF void* glMapBuffer(GLenum target, GLenum access)
{
	PGL_ERR_RET_VAL((target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER), GL_INVALID_ENUM, NULL);

	PGL_ERR_RET_VAL((access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE), GL_INVALID_ENUM, NULL);

	// adjust to access bound_buffers
	target -= GL_ARRAY_BUFFER;

	void* data = NULL;
	pglGetBufferData(c->bound_buffers[target], &data);
	return data;
}

PGLDEF void* glMapNamedBuffer(GLuint buffer, GLenum access)
{
	// TODO pglGetBufferData will verify buffer is valid, hmm
	PGL_ERR_RET_VAL((access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE), GL_INVALID_ENUM, NULL);

	void* data = NULL;
	pglGetBufferData(buffer, &data);
	return data;
}


#ifndef PGL_EXCLUDE_STUBS

// Stubs to let real OpenGL libs compile with minimal modifications/ifdefs
// add what you need

PGLDEF const GLubyte* glGetStringi(GLenum name, GLuint index) { return NULL; }

PGLDEF void glColorMaski(GLuint buf, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}

// glGenerateMipmap / glGenerateTextureMipmap are implemented in gl_impl.c

PGLDEF void glGetDoublev(GLenum pname, GLdouble* params) { }
PGLDEF void glGetInteger64v(GLenum pname, GLint64* params) { }

// Drawbuffers (glDrawBuffers implemented in gl_fbo.c)
PGLDEF void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum* bufs) {}

// Framebuffers/Renderbuffers (core FBO API implemented in gl_fbo.c)
PGLDEF void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
PGLDEF void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer) {}

PGLDEF void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {}
PGLDEF void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) {}

PGLDEF void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum mode) {}

PGLDEF void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}
PGLDEF void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}

PGLDEF void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
PGLDEF void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}

PGLDEF void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value) {}
PGLDEF void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value) {}
PGLDEF void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value) {}
PGLDEF void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {}
PGLDEF void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint* value) {}
PGLDEF void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint* value) {}
PGLDEF void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat* value) {}
PGLDEF void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {}

PGLDEF void glGetProgramiv(GLuint program, GLenum pname, GLint* params) { }
PGLDEF void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog) { }
PGLDEF void glAttachShader(GLuint program, GLuint shader) { }
PGLDEF void glCompileShader(GLuint shader) { }
PGLDEF void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog) { }
PGLDEF void glLinkProgram(GLuint program) { }
PGLDEF void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length) { }
PGLDEF void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) { }
PGLDEF void glDeleteShader(GLuint shader) { }
PGLDEF void glDetachShader(GLuint program, GLuint shader) { }

PGLDEF GLuint glCreateProgram(void) { return 0; }
PGLDEF GLuint glCreateShader(GLenum shaderType) { return 0; }
PGLDEF GLint glGetUniformLocation(GLuint program, const GLchar* name) { return 0; }
PGLDEF GLint glGetAttribLocation(GLuint program, const GLchar* name) { return 0; }

PGLDEF GLboolean glUnmapBuffer(GLenum target) { return GL_TRUE; }
PGLDEF GLboolean glUnmapNamedBuffer(GLuint buffer) { return GL_TRUE; }

// TODO?

PGLDEF void glActiveTexture(GLenum texture) { }
PGLDEF void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {}

PGLDEF void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {}

// TODO what the heck are these?
PGLDEF void glTexParameterliv(GLenum target, GLenum pname, const GLint* params) {}
PGLDEF void glTexParameterluiv(GLenum target, GLenum pname, const GLuint* params) {}

PGLDEF void glTextureParameterliv(GLuint texture, GLenum pname, const GLint* params) {}
PGLDEF void glTextureParameterluiv(GLuint texture, GLenum pname, const GLuint* params) {}

PGLDEF void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data) {}
PGLDEF void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data) {}
PGLDEF void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data) {}

PGLDEF void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) { }
PGLDEF void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer) { }


PGLDEF void glUniform1f(GLint location, GLfloat v0) { }
PGLDEF void glUniform2f(GLint location, GLfloat v0, GLfloat v1) { }
PGLDEF void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) { }
PGLDEF void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) { }

PGLDEF void glUniform1i(GLint location, GLint v0) { }
PGLDEF void glUniform2i(GLint location, GLint v0, GLint v1) { }
PGLDEF void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) { }
PGLDEF void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) { }

PGLDEF void glUniform1ui(GLint location, GLuint v0) { }
PGLDEF void glUniform2ui(GLint location, GLuint v0, GLuint v1) { }
PGLDEF void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) { }
PGLDEF void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) { }

PGLDEF void glUniform1fv(GLint location, GLsizei count, const GLfloat* value) { }
PGLDEF void glUniform2fv(GLint location, GLsizei count, const GLfloat* value) { }
PGLDEF void glUniform3fv(GLint location, GLsizei count, const GLfloat* value) { }
PGLDEF void glUniform4fv(GLint location, GLsizei count, const GLfloat* value) { }

PGLDEF void glUniform1iv(GLint location, GLsizei count, const GLint* value) { }
PGLDEF void glUniform2iv(GLint location, GLsizei count, const GLint* value) { }
PGLDEF void glUniform3iv(GLint location, GLsizei count, const GLint* value) { }
PGLDEF void glUniform4iv(GLint location, GLsizei count, const GLint* value) { }

PGLDEF void glUniform1uiv(GLint location, GLsizei count, const GLuint* value) { }
PGLDEF void glUniform2uiv(GLint location, GLsizei count, const GLuint* value) { }
PGLDEF void glUniform3uiv(GLint location, GLsizei count, const GLuint* value) { }
PGLDEF void glUniform4uiv(GLint location, GLsizei count, const GLuint* value) { }

PGLDEF void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
PGLDEF void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
PGLDEF void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
PGLDEF void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
PGLDEF void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
PGLDEF void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
PGLDEF void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
PGLDEF void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
PGLDEF void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }

#endif
// Framebuffer objects (color/depth/stencil attach, MRT, renderbuffers, readback).
// Relies on amalgamation after gl_impl.c for PGL_ERR and texture RT helpers.

static void pgl_init_fbo(glFBO* f)
{
	memset(f, 0, sizeof(*f));
	f->deleted = GL_FALSE;
	f->status_dirty = GL_TRUE;
	f->status = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
	// Default draw buffer state (per-framebuffer)
	f->num_draw_buffers = 1;
	f->draw_buffers[0] = GL_COLOR_ATTACHMENT0;
	for (int i = 1; i < GL_MAX_DRAW_BUFFERS; ++i)
		f->draw_buffers[i] = GL_NONE;
	f->read_buffer = GL_COLOR_ATTACHMENT0;
}

static size_t pgl_z_bytes_per_pixel(void)
{
#ifdef PGL_NO_DEPTH_NO_STENCIL
	return 0;
#elif defined(PGL_D16)
	return sizeof(u16);
#else
	return sizeof(u32); // D24S8
#endif
}

static GLboolean pgl_tex_is_2d_level0(const glTexture* t)
{
	if (!t || t->deleted)
		return GL_FALSE;
	// type is stored as index (see glBindTexture), not raw enum
	GLenum target = t->type + GL_TEXTURE_UNBOUND + 1;
	if (target != GL_TEXTURE_2D && target != GL_TEXTURE_RECTANGLE)
		return GL_FALSE;
	if (!t->data || t->w <= 0 || t->h <= 0 || t->num_levels < 1)
		return GL_FALSE;
	return GL_TRUE;
}

// Depth attachment texture: explicit depth format, or color-sized storage large enough to alias Z.
static GLboolean pgl_tex_ok_depth_attach(const glTexture* t)
{
	if (!pgl_tex_is_2d_level0(t))
		return GL_FALSE;
	if (t->is_depth)
		return GL_TRUE;
	// Legacy: RGBA8/float buffer large enough for window Z packing
	size_t need = (size_t)t->w * (size_t)t->h * pgl_z_bytes_per_pixel();
	if (!need)
		return GL_FALSE;
	size_t have = (size_t)t->w * (size_t)t->h * (size_t)pgl_tex_bytes_per_pixel(t);
	return have >= need;
}

static GLboolean pgl_rb_ok_depth(const glRenderbuffer* rb)
{
	if (!rb || rb->deleted || !rb->data || rb->w <= 0 || rb->h <= 0)
		return GL_FALSE;
	return rb->internalformat == GL_DEPTH_COMPONENT ||
	       rb->internalformat == GL_DEPTH_COMPONENT16 ||
	       rb->internalformat == GL_DEPTH_COMPONENT24 ||
	       rb->internalformat == GL_DEPTH_COMPONENT32 ||
	       rb->internalformat == GL_DEPTH_COMPONENT32F;
}

static GLenum pgl_fbo_compute_status(glFBO* f)
{
	int n_attach = 0;
	GLsizei aw = 0, ah = 0;
	GLboolean have_size = GL_FALSE;

	for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i) {
		if (!f->color[i].tex)
			continue;
		if (f->color[i].level != 0)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		if (f->color[i].tex >= c->textures.size)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		glTexture* t = &c->textures.a[f->color[i].tex];
		if (!pgl_tex_is_2d_level0(t) || t->is_depth)
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		// Drawable color: U8 RGBA, or float R/RG/RGBA (no RGB32F)
		if (t->datatype == GL_FLOAT) {
			if (t->components != 1 && t->components != 2 && t->components != 4)
				return GL_FRAMEBUFFER_UNSUPPORTED;
		} else if (t->datatype != GL_UNSIGNED_BYTE || t->components != 4) {
			return GL_FRAMEBUFFER_UNSUPPORTED;
		}
		if (!have_size) {
			aw = t->w;
			ah = t->h;
			have_size = GL_TRUE;
		} else if (t->w != aw || t->h != ah) {
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
		}
		n_attach++;
	}

	if (f->depth.tex || f->depth.rb) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		return GL_FRAMEBUFFER_UNSUPPORTED;
#else
		GLsizei dw = 0, dh = 0;
		if (f->depth.tex) {
			if (f->depth.level != 0)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			if (f->depth.tex >= c->textures.size)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			glTexture* t = &c->textures.a[f->depth.tex];
			if (!pgl_tex_ok_depth_attach(t))
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			dw = t->w;
			dh = t->h;
		} else {
			if (f->depth.rb >= c->renderbuffers.size)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			glRenderbuffer* rb = &c->renderbuffers.a[f->depth.rb];
			if (!pgl_rb_ok_depth(rb))
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			dw = rb->w;
			dh = rb->h;
		}
		if (!have_size) {
			aw = dw;
			ah = dh;
			have_size = GL_TRUE;
		} else if (dw != aw || dh != ah) {
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
		}
		n_attach++;
#endif
	}

	if (f->stencil.tex || f->stencil.rb) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		return GL_FRAMEBUFFER_UNSUPPORTED;
#else
		// Phase D middle ground: stencil RB, or packed with D24S8 depth (same buffer)
		if (f->stencil.tex) {
			// Only allow stencil tex if same as depth tex under D24S8 pack
			if (!f->depth.tex || f->stencil.tex != f->depth.tex)
				return GL_FRAMEBUFFER_UNSUPPORTED;
		} else if (f->stencil.rb) {
			if (f->stencil.rb >= c->renderbuffers.size)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			glRenderbuffer* rb = &c->renderbuffers.a[f->stencil.rb];
			if (rb->deleted || !rb->data)
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
#  ifndef PGL_D16
			// D24S8: stencil may share depth RB if same object
			if (f->depth.rb && f->depth.rb == f->stencil.rb) {
				/* ok packed */
			} else if (rb->w != aw || rb->h != ah) {
				return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
			}
#  else
			if (rb->w != aw || rb->h != ah)
				return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
#  endif
		}
		n_attach++;
#endif
	}

	if (!n_attach)
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;

	// Need at least one color attachment (depth-only not useful for drawable FBOs yet)
	if (!n_attach || !have_size)
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;

	// Must have a color attachment if we counted only depth above... re-check colors
	{
		int n_color = 0;
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i)
			if (f->color[i].tex) n_color++;
		if (!n_color)
			return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
	}

	// Desktop completeness: every non-GL_NONE DRAW_BUFFERi must name a color
	// attachment that has an image (already validated above if present).
	for (GLsizei i = 0; i < f->num_draw_buffers; ++i) {
		GLenum db = f->draw_buffers[i];
		if (db == GL_NONE)
			continue;
		int att = (int)(db - GL_COLOR_ATTACHMENT0);
		if (att < 0 || att >= GL_MAX_COLOR_ATTACHMENTS || !f->color[att].tex)
			return GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER;
	}

	// Same for READ_BUFFER (GL_NONE is allowed — reads are a no-op).
	if (f->read_buffer != GL_NONE) {
		int att = (int)(f->read_buffer - GL_COLOR_ATTACHMENT0);
		if (att < 0 || att >= GL_MAX_COLOR_ATTACHMENTS || !f->color[att].tex)
			return GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER;
	}

	return GL_FRAMEBUFFER_COMPLETE;
}

static void pgl_fbo_update_status(glFBO* f)
{
	f->status = pgl_fbo_compute_status(f);
	f->status_dirty = GL_FALSE;
}

static glFBO* pgl_get_bound_user_fbo(void)
{
	if (!c->bound_framebuffer)
		return NULL;
	if (c->bound_framebuffer >= c->framebuffers.size)
		return NULL;
	glFBO* f = &c->framebuffers.a[c->bound_framebuffer];
	if (f->deleted)
		return NULL;
	return f;
}

// True if draws may proceed (default FB or complete user FBO).
static GLboolean pgl_draw_framebuffer_ok(void)
{
	if (!c->bound_framebuffer)
		return GL_TRUE;
	glFBO* f = pgl_get_bound_user_fbo();
	if (!f)
		return GL_FALSE;
	if (f->status_dirty)
		pgl_fbo_update_status(f);
	return f->status == GL_FRAMEBUFFER_COMPLETE;
}

#ifndef PGL_NO_DEPTH_NO_STENCIL
static GLboolean pgl_ensure_fbo_scratch_z(GLsizei w, GLsizei h)
{
	size_t zb = pgl_z_bytes_per_pixel();
	if (!zb || w <= 0 || h <= 0)
		return GL_FALSE;
	size_t need = (size_t)w * (size_t)h * zb;
	if (c->fbo_scratch_z.buf && c->fbo_scratch_z.w == w && c->fbo_scratch_z.h == h)
		return GL_TRUE;
	u8* p = (u8*)PGL_REALLOC(c->fbo_scratch_z.buf, need);
	if (!p)
		return GL_FALSE;
	c->fbo_scratch_z.buf = p;
	c->fbo_scratch_z.w = w;
	c->fbo_scratch_z.h = h;
	c->fbo_scratch_z.lastrow = p + (size_t)(h - 1) * (size_t)w * zb;
	return GL_TRUE;
}
#endif

// Resolve mrt_color[] from FBO color attachments (texture format bpp, not pix_t).
static void pgl_apply_color_attachments(glFBO* f)
{
	for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i) {
		memset(&c->mrt_color[i], 0, sizeof(c->mrt_color[i]));
		if (!f->color[i].tex)
			continue;
		glTexture* t = &c->textures.a[f->color[i].tex];
		pgl_tex_mark_render_target(t);
		int bpp = pgl_tex_bytes_per_pixel(t);
		c->mrt_color[i].buf = t->data;
		c->mrt_color[i].w = t->w;
		c->mrt_color[i].h = t->h;
		c->mrt_color[i].datatype = t->datatype;
		c->mrt_color[i].components = t->components;
		c->mrt_color[i].lastrow =
			t->data + (size_t)(t->h - 1) * (size_t)t->w * (size_t)bpp;
	}

	c->num_draw_buffers = f->num_draw_buffers;
	for (GLsizei i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
		c->draw_buffers[i] = (i < f->num_draw_buffers) ? f->draw_buffers[i] : (GLenum)GL_NONE;
	c->read_buffer = f->read_buffer;
	c->fbo_color_is_rt = GL_TRUE;

	// Dimension surface for scissor/viewport macros (buf may be unused for color writes)
	pglColorRT* primary = NULL;
	for (GLsizei i = 0; i < c->num_draw_buffers; ++i) {
		if (c->draw_buffers[i] == GL_NONE)
			continue;
		int att = (int)(c->draw_buffers[i] - GL_COLOR_ATTACHMENT0);
		if (att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS && c->mrt_color[att].buf) {
			primary = &c->mrt_color[att];
			break;
		}
	}
	if (!primary) {
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i) {
			if (c->mrt_color[i].buf) {
				primary = &c->mrt_color[i];
				break;
			}
		}
	}
	if (primary) {
		// Keep back_buffer dimensions in sync for raster bounds; do not use as pix_t target
		c->back_buffer.w = primary->w;
		c->back_buffer.h = primary->h;
		c->back_buffer.buf = primary->buf;
		c->back_buffer.lastrow = primary->lastrow;
	}

	{
		int n_active = 0;
		for (GLsizei i = 0; i < c->num_draw_buffers; ++i)
			if (c->draw_buffers[i] != GL_NONE)
				n_active++;
		c->mrt_active = (n_active > 1) ? GL_TRUE : GL_FALSE;
	}
}

// Point active draw surfaces at bound FBO attachments (or restore window).
static void pgl_apply_draw_framebuffer(void)
{
	if (!c->bound_framebuffer) {
		if (c->fbo_redirected) {
			c->back_buffer = c->window_back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
			c->zbuf = c->window_zbuf;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
			c->stencil_buf = c->window_stencil_buf;
#  endif
#endif
			c->fbo_redirected = GL_FALSE;
		}
		// Default FB: pix_t color, no MRT
		c->mrt_active = GL_FALSE;
		c->fbo_color_is_rt = GL_FALSE;
#ifndef PGL_NO_DEPTH_NO_STENCIL
		c->zbuf_float = GL_FALSE;
#endif
		c->num_draw_buffers = c->default_num_draw_buffers;
		for (GLsizei i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
			c->draw_buffers[i] = c->default_draw_buffers[i];
		c->read_buffer = c->default_read_buffer;
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i)
			memset(&c->mrt_color[i], 0, sizeof(c->mrt_color[i]));
		return;
	}

	glFBO* f = pgl_get_bound_user_fbo();
	if (!f)
		return;
	if (f->status_dirty)
		pgl_fbo_update_status(f);
	if (f->status != GL_FRAMEBUFFER_COMPLETE) {
		// Not drawable/readable: drop RT color routing so we never keep stale mrt_*
		// after draw/read buffer or attach changes. Window backup stays until unbind.
		c->fbo_color_is_rt = GL_FALSE;
		c->mrt_active = GL_FALSE;
		for (int i = 0; i < GL_MAX_COLOR_ATTACHMENTS; ++i)
			memset(&c->mrt_color[i], 0, sizeof(c->mrt_color[i]));
		return;
	}

	if (!c->fbo_redirected) {
		c->window_back_buffer = c->back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
		c->window_zbuf = c->zbuf;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
		c->window_stencil_buf = c->stencil_buf;
#  endif
#endif
		c->fbo_redirected = GL_TRUE;
	}

	pgl_apply_color_attachments(f);

#ifndef PGL_NO_DEPTH_NO_STENCIL
	GLsizei dw = c->back_buffer.w;
	GLsizei dh = c->back_buffer.h;
	c->zbuf_float = GL_FALSE;
	if (f->depth.tex) {
		glTexture* dt = &c->textures.a[f->depth.tex];
		pgl_tex_mark_render_target(dt);
		// Explicit depth textures use their own bpp; legacy "alias color storage as Z"
		// must use window Z packing (D16=2, D24S8=4), not the color texel's bpp.
		size_t zb;
		if (dt->is_depth)
			zb = (size_t)pgl_tex_bytes_per_pixel(dt);
		else
			zb = pgl_z_bytes_per_pixel();
		c->zbuf.buf = dt->data;
		c->zbuf.w = dt->w;
		c->zbuf.h = dt->h;
		c->zbuf.lastrow = dt->data + (size_t)(dt->h - 1) * (size_t)dt->w * zb;
		if (dt->is_depth && dt->datatype == GL_FLOAT)
			c->zbuf_float = GL_TRUE;
#  if defined(PGL_D24S8)
		if (!c->zbuf_float && (!f->stencil.rb || f->stencil.tex == f->depth.tex)) {
			c->stencil_buf.buf = dt->data;
			c->stencil_buf.w = dt->w;
			c->stencil_buf.h = dt->h;
			c->stencil_buf.lastrow = c->zbuf.lastrow;
		}
#  endif
	} else if (f->depth.rb) {
		glRenderbuffer* rb = &c->renderbuffers.a[f->depth.rb];
		c->zbuf.buf = rb->data;
		c->zbuf.w = rb->w;
		c->zbuf.h = rb->h;
		c->zbuf.lastrow = rb->lastrow;
		if (rb->internalformat == GL_DEPTH_COMPONENT32F)
			c->zbuf_float = GL_TRUE;
#  if defined(PGL_D24S8)
		if (!c->zbuf_float && f->stencil.rb == f->depth.rb) {
			c->stencil_buf = c->zbuf;
		}
#  endif
	} else {
		if (pgl_ensure_fbo_scratch_z(dw, dh))
			c->zbuf = c->fbo_scratch_z;
#  if defined(PGL_D24S8)
		c->stencil_buf.buf = c->zbuf.buf;
		c->stencil_buf.w = c->zbuf.w;
		c->stencil_buf.h = c->zbuf.h;
		c->stencil_buf.lastrow = c->zbuf.lastrow;
#  endif
	}
#  if !defined(PGL_NO_STENCIL) && defined(PGL_D16)
	if (f->stencil.rb && f->stencil.rb != f->depth.rb) {
		glRenderbuffer* srb = &c->renderbuffers.a[f->stencil.rb];
		c->stencil_buf.buf = srb->data;
		c->stencil_buf.w = srb->w;
		c->stencil_buf.h = srb->h;
		c->stencil_buf.lastrow = srb->lastrow;
	}
#  endif
#endif
}

static void pgl_fbo_mark_dirty(glFBO* f)
{
	f->status_dirty = GL_TRUE;
	// If this FBO is bound and was applied, re-apply after status update on next bind/check
}

PGLDEF void glGenFramebuffers(GLsizei n, GLuint* ids)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	if (!n)
		return;

	// Ensure index 0 exists as a deleted placeholder so names start at 1
	if (c->framebuffers.size == 0) {
		cvec_extend_glFBO(&c->framebuffers, 1);
		pgl_init_fbo(&c->framebuffers.a[0]);
		c->framebuffers.a[0].deleted = GL_TRUE; // never used
	}

	int j = 0;
	for (int i = 1; i < c->framebuffers.size && j < n; ++i) {
		if (c->framebuffers.a[i].deleted) {
			pgl_init_fbo(&c->framebuffers.a[i]);
			ids[j++] = (GLuint)i;
		}
	}
	if (j != n) {
		int s = (int)c->framebuffers.size;
		cvec_extend_glFBO(&c->framebuffers, n - j);
		for (int i = s; j < n; ++i) {
			pgl_init_fbo(&c->framebuffers.a[i]);
			ids[j++] = (GLuint)i;
		}
	}
}

PGLDEF void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	for (int i = 0; i < n; ++i) {
		GLuint id = framebuffers[i];
		if (!id || id >= c->framebuffers.size)
			continue;
		if (c->framebuffers.a[id].deleted)
			continue;
		if (c->bound_framebuffer == id) {
			c->bound_framebuffer = 0;
			pgl_apply_draw_framebuffer();
		}
		c->framebuffers.a[id].deleted = GL_TRUE;
	}
}

PGLDEF GLboolean glIsFramebuffer(GLuint framebuffer)
{
	if (!framebuffer || framebuffer >= c->framebuffers.size)
		return GL_FALSE;
	return !c->framebuffers.a[framebuffer].deleted;
}

PGLDEF void glBindFramebuffer(GLenum target, GLuint framebuffer)
{
	PGL_ERR(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER &&
	        target != GL_READ_FRAMEBUFFER, GL_INVALID_ENUM);

	if (framebuffer != 0) {
		PGL_ERR(framebuffer >= c->framebuffers.size ||
		        c->framebuffers.a[framebuffer].deleted, GL_INVALID_OPERATION);
	}

	// v1: DRAW and READ share one bind
	c->bound_framebuffer = framebuffer;
	pgl_apply_draw_framebuffer();
}

PGLDEF void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                                   GLuint texture, GLint level)
{
	PGL_ERR(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER &&
	        target != GL_READ_FRAMEBUFFER, GL_INVALID_ENUM);
	PGL_ERR(!c->bound_framebuffer, GL_INVALID_OPERATION); // cannot attach to default FB

	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR(!f, GL_INVALID_OPERATION);

	if (texture != 0) {
		PGL_ERR(textarget != GL_TEXTURE_2D && textarget != GL_TEXTURE_RECTANGLE, GL_INVALID_OPERATION);
		PGL_ERR(level != 0, GL_INVALID_VALUE); // v1: level 0 only
		PGL_ERR(texture >= c->textures.size || c->textures.a[texture].deleted, GL_INVALID_VALUE);
	}

	if (attachment == GL_COLOR_ATTACHMENT0 ||
	    (attachment >= GL_COLOR_ATTACHMENT1 &&
	     attachment < GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS)) {
		int idx = (int)(attachment - GL_COLOR_ATTACHMENT0);
		f->color[idx].tex = texture;
		f->color[idx].rb = 0;
		f->color[idx].level = level;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
	} else if (attachment == GL_DEPTH_ATTACHMENT) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.tex = texture;
		f->depth.rb = 0;
		f->depth.level = level;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
#endif
	} else if (attachment == GL_STENCIL_ATTACHMENT) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		// Packed path: same texture as depth (D24S8). Separate stencil textures unsupported.
		f->stencil.tex = texture;
		f->stencil.rb = 0;
		f->stencil.level = level;
#endif
	} else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.tex = texture;
		f->depth.rb = 0;
		f->depth.level = level;
		f->stencil.tex = texture;
		f->stencil.rb = 0;
		f->stencil.level = level;
		if (texture)
			pgl_tex_mark_render_target(&c->textures.a[texture]);
#endif
	} else {
		PGL_ERR(1, GL_INVALID_ENUM);
	}

	pgl_fbo_mark_dirty(f);
	// Re-apply if still bound so draw targets update
	if (c->bound_framebuffer)
		pgl_apply_draw_framebuffer();
}

// Convenience: DSA-ish path used by some ports; maps to bound-FBO style after bind.
PGLDEF void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level)
{
	// Assume 2D; validate on attach
	glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, texture, level);
}

PGLDEF GLenum glCheckFramebufferStatus(GLenum target)
{
	PGL_ERR_RET_VAL(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER &&
	                target != GL_READ_FRAMEBUFFER, GL_INVALID_ENUM, 0);

	if (!c->bound_framebuffer)
		return GL_FRAMEBUFFER_COMPLETE; // default FB always complete when context exists

	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR_RET_VAL(!f, GL_INVALID_OPERATION, 0);
	pgl_fbo_update_status(f);
	return f->status;
}

// Select which color attachments receive FS outputs (gl_FragData[i] → bufs[i]).
// State is per-framebuffer (default FB uses context default_draw_buffers).
PGLDEF void glDrawBuffers(GLsizei n, const GLenum* bufs)
{
	PGL_ERR(n < 0 || n > GL_MAX_DRAW_BUFFERS, GL_INVALID_VALUE);
	PGL_ERR(!bufs && n > 0, GL_INVALID_VALUE);

	// Validate entries; reject duplicates (except GL_NONE)
	GLboolean seen[GL_MAX_COLOR_ATTACHMENTS];
	memset(seen, 0, sizeof(seen));

	for (GLsizei i = 0; i < n; ++i) {
		GLenum b = bufs[i];
		if (b == GL_NONE)
			continue;

		if (!c->bound_framebuffer) {
			// Default FB: only GL_BACK (and treat COLOR_ATTACHMENT0 as synonym)
			PGL_ERR(b != GL_BACK && b != GL_COLOR_ATTACHMENT0, GL_INVALID_ENUM);
			PGL_ERR(n != 1, GL_INVALID_OPERATION); // single buffer only
		} else {
			PGL_ERR(b < GL_COLOR_ATTACHMENT0 ||
			        b >= GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS, GL_INVALID_ENUM);
			int att = (int)(b - GL_COLOR_ATTACHMENT0);
			PGL_ERR(seen[att], GL_INVALID_OPERATION); // duplicate
			seen[att] = GL_TRUE;
		}
	}

	if (!c->bound_framebuffer) {
		c->default_num_draw_buffers = n > 0 ? n : 1;
		if (n <= 0) {
			c->default_draw_buffers[0] = GL_BACK;
		} else {
			for (GLsizei i = 0; i < n; ++i)
				c->default_draw_buffers[i] = bufs[i];
			for (GLsizei i = n; i < GL_MAX_DRAW_BUFFERS; ++i)
				c->default_draw_buffers[i] = GL_NONE;
		}
		c->num_draw_buffers = c->default_num_draw_buffers;
		for (GLsizei i = 0; i < GL_MAX_DRAW_BUFFERS; ++i)
			c->draw_buffers[i] = c->default_draw_buffers[i];
		c->mrt_active = GL_FALSE;
		return;
	}

	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR(!f, GL_INVALID_OPERATION);

	f->num_draw_buffers = n > 0 ? n : 1;
	if (n <= 0) {
		f->draw_buffers[0] = GL_COLOR_ATTACHMENT0;
		for (int i = 1; i < GL_MAX_DRAW_BUFFERS; ++i)
			f->draw_buffers[i] = GL_NONE;
	} else {
		for (GLsizei i = 0; i < n; ++i)
			f->draw_buffers[i] = bufs[i];
		for (GLsizei i = n; i < GL_MAX_DRAW_BUFFERS; ++i)
			f->draw_buffers[i] = GL_NONE;
	}

	// Draw buffers affect completeness (INCOMPLETE_DRAW_BUFFER)
	pgl_fbo_mark_dirty(f);
	// Refresh back_buffer / mrt_active if this FBO is complete and bound
	pgl_apply_draw_framebuffer();
}

// Renderbuffers, framebuffer renderbuffer attach, read buffer/pixels

static void pgl_init_rb(glRenderbuffer* rb)
{
	memset(rb, 0, sizeof(*rb));
	rb->deleted = GL_FALSE;
	rb->user_owned = GL_FALSE;
}

PGLDEF void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	if (!n) return;

	if (c->renderbuffers.size == 0) {
		cvec_extend_glRenderbuffer(&c->renderbuffers, 1);
		pgl_init_rb(&c->renderbuffers.a[0]);
		c->renderbuffers.a[0].deleted = GL_TRUE;
	}

	int j = 0;
	for (int i = 1; i < c->renderbuffers.size && j < n; ++i) {
		if (c->renderbuffers.a[i].deleted) {
			pgl_init_rb(&c->renderbuffers.a[i]);
			renderbuffers[j++] = (GLuint)i;
		}
	}
	if (j != n) {
		int s = (int)c->renderbuffers.size;
		cvec_extend_glRenderbuffer(&c->renderbuffers, n - j);
		for (int i = s; j < n; ++i) {
			pgl_init_rb(&c->renderbuffers.a[i]);
			renderbuffers[j++] = (GLuint)i;
		}
	}
}

PGLDEF void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
	PGL_ERR(n < 0, GL_INVALID_VALUE);
	for (int i = 0; i < n; ++i) {
		GLuint id = renderbuffers[i];
		if (!id || id >= c->renderbuffers.size) continue;
		glRenderbuffer* rb = &c->renderbuffers.a[id];
		if (rb->deleted) continue;
		if (!rb->user_owned)
			PGL_FREE(rb->data);
		if (c->bound_renderbuffer == id)
			c->bound_renderbuffer = 0;
		pgl_init_rb(rb);
		rb->deleted = GL_TRUE;
	}
}

PGLDEF GLboolean glIsRenderbuffer(GLuint renderbuffer)
{
	if (!renderbuffer || renderbuffer >= c->renderbuffers.size)
		return GL_FALSE;
	return !c->renderbuffers.a[renderbuffer].deleted;
}

PGLDEF void glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
	PGL_ERR(target != GL_RENDERBUFFER, GL_INVALID_ENUM);
	if (renderbuffer != 0) {
		PGL_ERR(renderbuffer >= c->renderbuffers.size ||
		        c->renderbuffers.a[renderbuffer].deleted, GL_INVALID_OPERATION);
	}
	c->bound_renderbuffer = renderbuffer;
}

PGLDEF void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	PGL_ERR(target != GL_RENDERBUFFER, GL_INVALID_ENUM);
	PGL_ERR(!c->bound_renderbuffer, GL_INVALID_OPERATION);
	PGL_ERR(width < 0 || height < 0, GL_INVALID_VALUE);
	PGL_ERR(internalformat != GL_DEPTH_COMPONENT &&
	        internalformat != GL_DEPTH_COMPONENT16 &&
	        internalformat != GL_DEPTH_COMPONENT24 &&
	        internalformat != GL_DEPTH_COMPONENT32 &&
	        internalformat != GL_DEPTH_COMPONENT32F &&
	        internalformat != GL_STENCIL_INDEX8, GL_INVALID_ENUM);

	glRenderbuffer* rb = &c->renderbuffers.a[c->bound_renderbuffer];
	size_t bpp = pgl_z_bytes_per_pixel();
	if (internalformat == GL_DEPTH_COMPONENT32F)
		bpp = sizeof(float);
	else if (internalformat == GL_STENCIL_INDEX8)
		bpp = 1;
	else if (!bpp)
		bpp = sizeof(u32);

	size_t need = (size_t)width * (size_t)height * bpp;
	if (!rb->user_owned)
		PGL_FREE(rb->data);
	rb->data = (u8*)PGL_MALLOC(need ? need : 1);
	PGL_ERR(!rb->data, GL_OUT_OF_MEMORY);
	memset(rb->data, 0, need ? need : 1);
	rb->data_alloc = need;
	rb->user_owned = GL_FALSE;
	rb->w = width;
	rb->h = height;
	rb->internalformat = internalformat;
	rb->lastrow = rb->data + (size_t)(height > 0 ? height - 1 : 0) * (size_t)width * bpp;
}

PGLDEF void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	PGL_ERR(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER &&
	        target != GL_READ_FRAMEBUFFER, GL_INVALID_ENUM);
	PGL_ERR(renderbuffertarget != GL_RENDERBUFFER, GL_INVALID_ENUM);
	PGL_ERR(!c->bound_framebuffer, GL_INVALID_OPERATION);

	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR(!f, GL_INVALID_OPERATION);

	if (renderbuffer != 0) {
		PGL_ERR(renderbuffer >= c->renderbuffers.size ||
		        c->renderbuffers.a[renderbuffer].deleted, GL_INVALID_OPERATION);
	}

	if (attachment == GL_DEPTH_ATTACHMENT) {
#ifdef PGL_NO_DEPTH_NO_STENCIL
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.rb = renderbuffer;
		f->depth.tex = 0;
		f->depth.level = 0;
#endif
	} else if (attachment == GL_STENCIL_ATTACHMENT) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->stencil.rb = renderbuffer;
		f->stencil.tex = 0;
		f->stencil.level = 0;
#endif
	} else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
#if defined(PGL_NO_STENCIL) || defined(PGL_NO_DEPTH_NO_STENCIL)
		PGL_ERR(1, GL_INVALID_ENUM);
#else
		f->depth.rb = renderbuffer;
		f->depth.tex = 0;
		f->stencil.rb = renderbuffer;
		f->stencil.tex = 0;
#endif
	} else {
		// Color renderbuffers not implemented (use textures)
		PGL_ERR(1, GL_INVALID_ENUM);
	}

	pgl_fbo_mark_dirty(f);
	if (c->bound_framebuffer)
		pgl_apply_draw_framebuffer();
}

PGLDEF void glReadBuffer(GLenum mode)
{
	if (!c->bound_framebuffer) {
		PGL_ERR(mode != GL_BACK && mode != GL_COLOR_ATTACHMENT0, GL_INVALID_ENUM);
		c->default_read_buffer = (mode == GL_COLOR_ATTACHMENT0) ? GL_BACK : mode;
		c->read_buffer = c->default_read_buffer;
		return;
	}
	PGL_ERR(mode != GL_NONE &&
	        (mode < GL_COLOR_ATTACHMENT0 ||
	         mode >= GL_COLOR_ATTACHMENT0 + GL_MAX_COLOR_ATTACHMENTS), GL_INVALID_ENUM);
	glFBO* f = pgl_get_bound_user_fbo();
	PGL_ERR(!f, GL_INVALID_OPERATION);
	f->read_buffer = mode;
	c->read_buffer = mode;
	// Read buffer affects completeness (INCOMPLETE_READ_BUFFER)
	pgl_fbo_mark_dirty(f);
	pgl_apply_draw_framebuffer();
}

// Thin glReadPixels: RGBA U8 or float RGBA/R from current read color buffer.
// (x,y) is GL bottom-left origin; converts to storage with invert for RTs.
PGLDEF void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, GLvoid* data)
{
	PGL_ERR(width < 0 || height < 0, GL_INVALID_VALUE);
	PGL_ERR(!data, GL_INVALID_VALUE);
	PGL_ERR(format != GL_RGBA && format != GL_RED, GL_INVALID_ENUM);
	PGL_ERR(type != GL_UNSIGNED_BYTE && type != GL_FLOAT, GL_INVALID_ENUM);
	// User FBO must be complete (includes DRAW_BUFFER / READ_BUFFER rules)
	PGL_ERR(c->bound_framebuffer && !pgl_draw_framebuffer_ok(),
	        GL_INVALID_FRAMEBUFFER_OPERATION);

	if (!width || !height)
		return;

	// Resolve source
	u8* src_base = NULL;
	GLsizei sw = 0, sh = 0;
	GLenum src_type = GL_UNSIGNED_BYTE;
	int src_comp = 4;
	GLboolean invert = GL_FALSE;

	if (!c->bound_framebuffer || !c->fbo_color_is_rt) {
		src_base = c->back_buffer.buf;
		sw = c->back_buffer.w;
		sh = c->back_buffer.h;
		// Window is top-down memory; GL y=0 is bottom → invert
		invert = GL_TRUE;
		src_type = GL_UNSIGNED_BYTE;
		// will convert from pix_t
	} else {
		GLenum rb = c->read_buffer;
		if (rb == GL_NONE)
			return;
		int att = (int)(rb - GL_COLOR_ATTACHMENT0);
		// Completeness guarantees a non-NONE read buffer has an attachment
		PGL_ASSERT(att >= 0 && att < GL_MAX_COLOR_ATTACHMENTS);
		PGL_ASSERT(c->mrt_color[att].buf);
		pglColorRT* rt = &c->mrt_color[att];
		src_base = rt->buf;
		sw = rt->w;
		sh = rt->h;
		src_type = rt->datatype;
		src_comp = rt->components;
		invert = GL_TRUE; // RT lastrow / invert_y: GL y=0 = bottom
	}

	for (GLsizei row = 0; row < height; ++row) {
		for (GLsizei col = 0; col < width; ++col) {
			GLint sx = x + col;
			GLint sy_gl = y + row; // bottom-left origin
			if (sx < 0 || sy_gl < 0 || sx >= sw || sy_gl >= sh)
				continue;
			GLint sy = invert ? (sh - 1 - sy_gl) : sy_gl;
			int idx = sy * sw + sx;
			float fr = 0, fg = 0, fb = 0, fa = 1;

			if (!c->bound_framebuffer || !c->fbo_color_is_rt) {
				pix_t p = ((pix_t*)src_base)[idx];
				Color colc = PIXEL_TO_COLOR(p);
				fr = colc.r / (float)PGL_RMAX;
				fg = colc.g / (float)PGL_GMAX;
				fb = colc.b / (float)PGL_BMAX;
				fa = colc.a / (float)PGL_AMAX;
			} else if (src_type == GL_FLOAT) {
				const float* f = (const float*)src_base + idx * src_comp;
				fr = f[0];
				if (src_comp > 1) fg = f[1];
				if (src_comp > 2) fb = f[2];
				if (src_comp > 3) fa = f[3];
			} else {
				Color colc = ((Color*)src_base)[idx];
				fr = colc.r / 255.f;
				fg = colc.g / 255.f;
				fb = colc.b / 255.f;
				fa = colc.a / 255.f;
			}

			size_t out_i = (size_t)row * (size_t)width + (size_t)col;
			if (type == GL_UNSIGNED_BYTE) {
				u8* o = (u8*)data;
				if (format == GL_RED) {
					o[out_i] = (u8)(fr * 255.f);
				} else {
					o[out_i * 4 + 0] = (u8)(fr * 255.f);
					o[out_i * 4 + 1] = (u8)(fg * 255.f);
					o[out_i * 4 + 2] = (u8)(fb * 255.f);
					o[out_i * 4 + 3] = (u8)(fa * 255.f);
				}
			} else {
				float* o = (float*)data;
				if (format == GL_RED) {
					o[out_i] = fr;
				} else {
					o[out_i * 4 + 0] = fr;
					o[out_i * 4 + 1] = fg;
					o[out_i * 4 + 2] = fb;
					o[out_i * 4 + 3] = fa;
				}
			}
		}
	}
}


/*************************************
 *  GLSL(ish) functions
 *************************************/

/*
float clampf_01(float f)
{
	if (f < 0.0f) return 0.0f;
	if (f > 1.0f) return 1.0f;
	return f;
}

float clampf(float f, float min, float max)
{
	if (f < min) return min;
	if (f > max) return max;
	return f;
}

int clampi(int i, int min, int max)
{
	if (i < min) return min;
	if (i > max) return max;
	return i;
}
*/


// TODO maybe I should put this in crsw_math/rsw_math? static inline?
// guarantees positive mod result
#define positive_mod(a, b) (((a) % (b) + (b)) % (b))

// if I only wanted to support power of 2 textures...
#define positive_mod_pow_of_2(i, n) ((i) & ((n) - 1) + (n)) & ((n) - 1)

// TODO should this be in rsw_math
#define mirror(i) (i) >= 0 ? (i) : -(1 + (i))

// See page 174 of GL 3.3 core spec.
static int wrap(int i, int size, GLenum mode)
{
	switch (mode)
	{
	case GL_REPEAT:
		return positive_mod(i, size);

	// Border is too much of a pain to implement with render to
	// texture.  Trade offs in poor performance or ugly extra code
	// for a feature that almost no one actually uses and even
	// when it is used (barring rare/odd uv coordinates) it's not
	// even noticable.
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	case GL_CLAMP_TO_BORDER:
		if (i >= 0 && i < size) return i;
		return -1;
		// Would use if we went back to literally surrounding textures with a border
		//return clampi(i, -1, size);
#else
	case GL_CLAMP_TO_BORDER:  // just so stuff that uses it compiles
#endif
	case GL_CLAMP_TO_EDGE:
		return clampi(i, 0, size-1);

	case GL_MIRRORED_REPEAT: {
		int sz2 = 2*size;
		i = positive_mod(i, sz2);
		i -= size;
		i = mirror(i);
		i = size - 1 - i;
		return i;
	} break;
	default:
		PGL_ASSERT(0 && "ERROR: unknown wrap mode!");
		return 0;
	}
}
#undef imod
#undef positive_mod
#undef positive_mod_pow_of_2


// hmm should I have these take a glTexture* somehow?
// It would save the check for 0 for every single access

// used in the following texture access functions
// Not sure if it's actually necessary since wrap() clamps
#define EPSILON 0.000001

// Texture filter arithmetic: float by default (soft-float / no-double platforms).
// Define PGL_DOUBLE_TEX_FILTER before including PGL to use double for UV scaling,
// lerp weights, and the color mix — fewer off-by-one results after the truncating
// [0,1]<->[0,255] conversion (see commit f66741f5).
#ifdef PGL_DOUBLE_TEX_FILTER
typedef double pgl_texf;
#define pgl_tex_floor(x) floor(x)
#define pgl_tex_modf(x, ip) modf((x), (ip))
#else
typedef float pgl_texf;
#define pgl_tex_floor(x) floorf(x)
#define pgl_tex_modf(x, ip) modff((x), (ip))
#endif

// Map MIN_FILTER to within-level NEAREST vs LINEAR
static GLenum pgl_within_level_filter(GLenum min_filter)
{
	switch (min_filter) {
	case GL_NEAREST:
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
		return GL_NEAREST;
	default:
		return GL_LINEAR;
	}
}

// True when min filter blends between two mip levels (trilinear / "mip linear")
static int pgl_is_mip_linear_filter(GLenum min_filter)
{
	return min_filter == GL_NEAREST_MIPMAP_LINEAR ||
	       min_filter == GL_LINEAR_MIPMAP_LINEAR;
}

// Explicit λ → single integer level for *MIPMAP_NEAREST (round).
// For *MIPMAP_LINEAR the lower level is floor(lod); caller blends with floor+1.
static int pgl_lod_to_level(const glTexture* t, float lod)
{
	PGL_ASSERT(t && t->num_levels > 1);

	int max_level = t->num_levels - 1;
	int level;
	if (pgl_is_mip_linear_filter(t->min_filter))
		level = (int)floorf(lod);
	else
		level = (int)floorf(lod + 0.5f);

	if (level < 0) level = 0;
	if (level > max_level) level = max_level;
	return level;
}

static int pgl_is_mip_min_filter(GLenum min_filter)
{
	return min_filter == GL_NEAREST_MIPMAP_NEAREST ||
	       min_filter == GL_NEAREST_MIPMAP_LINEAR ||
	       min_filter == GL_LINEAR_MIPMAP_NEAREST ||
	       min_filter == GL_LINEAR_MIPMAP_LINEAR;
}

// Incomplete for mip sampling: MIN_FILTER is a *MIPMAP* mode but no chain
// (num_levels <= 1).  Under PGL_CORE_PROFILE → black; otherwise L0 fallback.
static int pgl_incomplete_mip_returns_black(const glTexture* t)
{
#ifdef PGL_CORE_PROFILE
	return pgl_is_mip_min_filter(t->min_filter) && t->num_levels <= 1;
#else
	PGL_UNUSED(t);
	return 0;
#endif
}

static vec4 pgl_lerp_v4(vec4 a, vec4 b, float t)
{
	// a*(1-t) + b*t
	a = scale_v4(a, 1.0f - t);
	b = scale_v4(b, t);
	return add_v4s(a, b);
}

// Phase 2B: λ from per-triangle UV/pixel scale and base-level size.
// ρ ≈ mip_uv_per_px * max(w,h); λ = log2(ρ).  λ<=0 => magnification.
static float pgl_auto_lod(const glTexture* t, GLsizei dim0, GLsizei dim1)
{
	float dim = (float)((dim0 > dim1) ? dim0 : dim1);
	if (dim < 1.0f)
		dim = 1.0f;
	float rho = c->mip_uv_per_px * dim;
	// Avoid -inf; tiny ρ => strong magnification (negative λ)
	if (rho < 1e-10f)
		return -16.0f;
	return log2f(rho);
}

// Resolve user texture name → glTexture* (0 = default unit for the target).
static glTexture* pgl_tex_1d(GLuint tex)
{
	if (tex)
		return &c->textures.a[tex];
	return &c->default_textures[GL_TEXTURE_1D - GL_TEXTURE_1D];
}
static glTexture* pgl_tex_2d(GLuint tex)
{
	if (tex)
		return &c->textures.a[tex];
	return &c->default_textures[GL_TEXTURE_2D - GL_TEXTURE_1D];
}
static glTexture* pgl_tex_cube(GLuint tex)
{
	if (tex)
		return &c->textures.a[tex];
	return &c->default_textures[GL_TEXTURE_CUBE_MAP - GL_TEXTURE_1D];
}

// λ = log2(ρ); clamp tiny ρ to avoid -inf
static float pgl_lambda_from_rho(float rho)
{
	if (rho < 1e-10f)
		return -16.0f;
	return log2f(rho);
}

// Isotropic LOD from screen-space UV derivatives in texel units:
// ρx = length( (dU/dx * w, dV/dx * h) ), ρy similarly, ρ = max(ρx, ρy).
static float pgl_lod_from_grad_dims(float w, float h,
                                    float dUdx, float dVdx, float dUdy, float dVdy)
{
	float ux = dUdx * w, vx = dVdx * h;
	float uy = dUdy * w, vy = dVdy * h;
	float rho_x = sqrtf(ux * ux + vx * vx);
	float rho_y = sqrtf(uy * uy + vy * vy);
	return pgl_lambda_from_rho(rho_x > rho_y ? rho_x : rho_y);
}

static float pgl_lod_from_grad1d_dim(float w, float dUdx, float dUdy)
{
	float ax = fabsf(dUdx * w);
	float ay = fabsf(dUdy * w);
	return pgl_lambda_from_rho(ax > ay ? ax : ay);
}

// --- Public LOD helpers -------------------------------------------------------
// These take a real texture object name only.  Default texture 0 is not accepted:
// it is ambiguous (one name per target).  Use typed samplers (texture2D etc.) for 0.
// PGL_ERR_RET_VAL logs GL_INVALID_VALUE in debug; the check vanishes under PGL_UNSAFE.

PGLDEF float pgl_lod_grad1D(GLuint tex, float dUdx, float dUdy)
{
	PGL_ERR_RET_VAL(!tex, GL_INVALID_VALUE, -16.0f);
	glTexture* t = &c->textures.a[tex];
	float w = (float)(t->w > 0 ? t->w : 1);
	return pgl_lod_from_grad1d_dim(w, dUdx, dUdy);
}

PGLDEF float pgl_lod_grad(GLuint tex, float dUdx, float dVdx, float dUdy, float dVdy)
{
	PGL_ERR_RET_VAL(!tex, GL_INVALID_VALUE, -16.0f);
	glTexture* t = &c->textures.a[tex];
	float w = (float)(t->w > 0 ? t->w : 1);
	float h = (float)(t->h > 0 ? t->h : 1);
	return pgl_lod_from_grad_dims(w, h, dUdx, dVdx, dUdy, dVdy);
}

// λ for UV = fragCoord / rt_size (0–1 across the render target).
PGLDEF float pgl_lod_screen_wh(GLuint tex, float rt_w, float rt_h)
{
	PGL_ERR_RET_VAL(!tex, GL_INVALID_VALUE, -16.0f);
	glTexture* t = &c->textures.a[tex];
	if (rt_w < 1.0f) rt_w = 1.0f;
	if (rt_h < 1.0f) rt_h = 1.0f;
	float tw = (float)(t->w > 0 ? t->w : 1);
	float th = (float)(t->h > 0 ? t->h : 1);
	// dU/dx = 1/rt_w, dV/dy = 1/rt_h  →  ρ = max(tw/rt_w, th/rt_h)
	float rho_x = tw / rt_w;
	float rho_y = th / rt_h;
	return pgl_lambda_from_rho(rho_x > rho_y ? rho_x : rho_y);
}

PGLDEF float pgl_lod_uv_scale_wh(GLuint tex, float scale, float rt_w, float rt_h)
{
	PGL_ERR_RET_VAL(!tex, GL_INVALID_VALUE, -16.0f);
	float s = fabsf(scale);
	if (s < 1e-10f)
		s = 1e-10f;
	// UV' = s * UV_screen  →  derivatives × |s|  →  λ' = λ + log2(|s|)
	return pgl_lod_screen_wh(tex, rt_w, rt_h) + log2f(s);
}

// Current color buffer size (active RT after FBO bind / pglSetBackBuffer).
PGLDEF float pgl_lod_screen(GLuint tex)
{
	return pgl_lod_screen_wh(tex, (float)c->back_buffer.w, (float)c->back_buffer.h);
}

PGLDEF float pgl_lod_uv_scale(GLuint tex, float scale)
{
	return pgl_lod_uv_scale_wh(tex, scale, (float)c->back_buffer.w, (float)c->back_buffer.h);
}

// Load one texel as vec4.
// Color: U8 RGBA or float R/RG/RGBA (missing channels → 0, alpha → 1).
// Depth: .r = depth in [0,1] (float store as-is; integer pack normalized by PGL_MAX_Z).
static inline vec4 pgl_load_texel(const glTexture* t, const u8* data, int idx)
{
	if (t->is_depth) {
		float d = 0.f;
		if (t->datatype == GL_FLOAT) {
			d = ((const float*)data)[idx];
		} else {
#ifndef PGL_NO_DEPTH_NO_STENCIL
#  ifdef PGL_D16
			d = ((const u16*)data)[idx] / (float)PGL_MAX_Z;
#  else
			u32 z = ((const u32*)data)[idx];
			d = (float)(z >> PGL_ZSHIFT) / (float)PGL_MAX_Z;
#  endif
#endif
		}
		if (d < 0.f) d = 0.f;
		if (d > 1.f) d = 1.f;
		return make_v4(d, 0.f, 0.f, 1.f);
	}
	if (t->datatype == GL_FLOAT) {
		PGL_ASSERT(t->components > 0);
		const int nc = t->components;
		const float* f = (const float*)data + idx * nc;
		float r = f[0], g = 0.f, b = 0.f, a = 1.f;
		if (nc > 1) g = f[1];
		if (nc > 2) b = f[2];
		if (nc > 3) a = f[3];
		return make_v4(r, g, b, a);
	}
	// U8: currently only RGBA8 tightly packed as Color
	return Color_to_v4(((Color*)data)[idx]);
}

// Logical (x,y) -> tightly packed linear index for one 2D level.
// invert_y RTs: fragCoord/sample y=0 is bottom of image = memory row h-1
// (matches glFramebuffer lastrow writes). Uploaded textures: y=0 = data row 0.
static inline int pgl_tex_index_2d(const glTexture* t, int x, int y, int w, int h)
{
	if (t->invert_y)
		y = h - 1 - y;
	return y * w + x;
}

// Sample one 1D level with NEAREST or LINEAR (filter != NEAREST => LINEAR)
static vec4 pgl_sample_1d_level(const glTexture* t, const u8* data, int w, float x, GLenum filter)
{
	int i0, i1;
	pgl_texf ww = w - EPSILON;
	pgl_texf xw = (pgl_texf)x * ww;

	if (filter == GL_NEAREST) {
		i0 = wrap((int)pgl_tex_floor(xw), w, t->wrap_s);
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if (i0 < 0) return t->border_color;
#endif
		return pgl_load_texel(t, data, i0);
	}

	// LINEAR
	// This seems right to me since pixel centers are 0.5 but
	// this isn't exactly what's described in the spec or FoCG
	i0 = wrap((int)pgl_tex_floor(xw - (pgl_texf)0.5), w, t->wrap_s);
	i1 = wrap((int)pgl_tex_floor(xw + (pgl_texf)0.499999), w, t->wrap_s);

	pgl_texf tmp2;
	pgl_texf alpha = pgl_tex_modf(xw + (pgl_texf)0.5, &tmp2);
	if (alpha < 0) ++alpha;

#ifdef PGL_HERMITE_SMOOTHING
	alpha = alpha * alpha * (3 - 2 * alpha);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	vec4 ci, ci1;
	if (i0 < 0) ci = t->border_color;
	else ci = pgl_load_texel(t, data, i0);
	if (i1 < 0) ci1 = t->border_color;
	else ci1 = pgl_load_texel(t, data, i1);
#else
	vec4 ci = pgl_load_texel(t, data, i0);
	vec4 ci1 = pgl_load_texel(t, data, i1);
#endif

#ifdef PGL_DOUBLE_TEX_FILTER
	{
		vec4 r;
		pgl_texf w0 = 1 - alpha, w1 = alpha;
		r.x = (float)(ci.x * w0 + ci1.x * w1);
		r.y = (float)(ci.y * w0 + ci1.y * w1);
		r.z = (float)(ci.z * w0 + ci1.z * w1);
		r.w = (float)(ci.w * w0 + ci1.w * w1);
		return r;
	}
#else
	ci = scale_v4(ci, (float)(1 - alpha));
	ci1 = scale_v4(ci1, (float)alpha);
	return add_v4s(ci, ci1);
#endif
}

// Sample one 2D level with NEAREST or LINEAR
static vec4 pgl_sample_2d_level(const glTexture* t, const u8* data, int w, int h, float x, float y, GLenum filter)
{
	int i0, j0, i1, j1;
	pgl_texf dw = w - EPSILON;
	pgl_texf dh = h - EPSILON;
	pgl_texf xw = (pgl_texf)x * dw;
	pgl_texf yh = (pgl_texf)y * dh;

	if (filter == GL_NEAREST) {
		i0 = wrap((int)pgl_tex_floor(xw), w, t->wrap_s);
		j0 = wrap((int)pgl_tex_floor(yh), h, t->wrap_t);
#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0) < 0) return t->border_color;
#endif
		return pgl_load_texel(t, data, pgl_tex_index_2d(t, i0, j0, w, h));
	}

	// LINEAR
	// This seems right to me since pixel centers are 0.5 but
	// this isn't exactly what's described in the spec or FoCG
	i0 = wrap((int)pgl_tex_floor(xw - (pgl_texf)0.5), w, t->wrap_s);
	j0 = wrap((int)pgl_tex_floor(yh - (pgl_texf)0.5), h, t->wrap_t);
	i1 = wrap((int)pgl_tex_floor(xw + (pgl_texf)0.499999), w, t->wrap_s);
	j1 = wrap((int)pgl_tex_floor(yh + (pgl_texf)0.499999), h, t->wrap_t);

	pgl_texf tmp2;
	pgl_texf alpha = pgl_tex_modf(xw + (pgl_texf)0.5, &tmp2);
	pgl_texf beta = pgl_tex_modf(yh + (pgl_texf)0.5, &tmp2);
	if (alpha < 0) ++alpha;
	if (beta < 0) ++beta;

	//hermite smoothing is optional
	//looks like my nvidia implementation doesn't do it
	//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
	alpha = alpha * alpha * (3 - 2 * alpha);
	beta = beta * beta * (3 - 2 * beta);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
	vec4 cij, ci1j, cij1, ci1j1;
	if ((i0 | j0) < 0) cij = t->border_color;
	else cij = pgl_load_texel(t, data, pgl_tex_index_2d(t, i0, j0, w, h));
	if ((i1 | j0) < 0) ci1j = t->border_color;
	else ci1j = pgl_load_texel(t, data, pgl_tex_index_2d(t, i1, j0, w, h));
	if ((i0 | j1) < 0) cij1 = t->border_color;
	else cij1 = pgl_load_texel(t, data, pgl_tex_index_2d(t, i0, j1, w, h));
	if ((i1 | j1) < 0) ci1j1 = t->border_color;
	else ci1j1 = pgl_load_texel(t, data, pgl_tex_index_2d(t, i1, j1, w, h));
#else
	vec4 cij = pgl_load_texel(t, data, pgl_tex_index_2d(t, i0, j0, w, h));
	vec4 ci1j = pgl_load_texel(t, data, pgl_tex_index_2d(t, i1, j0, w, h));
	vec4 cij1 = pgl_load_texel(t, data, pgl_tex_index_2d(t, i0, j1, w, h));
	vec4 ci1j1 = pgl_load_texel(t, data, pgl_tex_index_2d(t, i1, j1, w, h));
#endif

#ifdef PGL_DOUBLE_TEX_FILTER
	{
		vec4 r;
		pgl_texf w00 = (1 - alpha) * (1 - beta);
		pgl_texf w10 = alpha * (1 - beta);
		pgl_texf w01 = (1 - alpha) * beta;
		pgl_texf w11 = alpha * beta;
		r.x = (float)(cij.x * w00 + ci1j.x * w10 + cij1.x * w01 + ci1j1.x * w11);
		r.y = (float)(cij.y * w00 + ci1j.y * w10 + cij1.y * w01 + ci1j1.y * w11);
		r.z = (float)(cij.z * w00 + ci1j.z * w10 + cij1.z * w01 + ci1j1.z * w11);
		r.w = (float)(cij.w * w00 + ci1j.w * w10 + cij1.w * w01 + ci1j1.w * w11);
		return r;
	}
#else
	// float path: same style as pre-mipmap texture2D (f66741f5+)
	cij = scale_v4(cij, (float)((1 - alpha) * (1 - beta)));
	ci1j = scale_v4(ci1j, (float)(alpha * (1 - beta)));
	cij1 = scale_v4(cij1, (float)((1 - alpha) * beta));
	ci1j1 = scale_v4(ci1j1, (float)(alpha * beta));

	cij = add_v4s(cij, ci1j);
	cij = add_v4s(cij, cij1);
	cij = add_v4s(cij, ci1j1);
	return cij;
#endif
}

// Sample one mip level (by index) with within-level filter from min_filter
static vec4 pgl_sample_1d_level_idx(const glTexture* t, int level, float x)
{
	u8* data = pgl_tex_level_data(t, level);
	PGL_ASSERT(data);

	GLsizei w;
	pgl_tex_level_dims(t, level, &w, NULL, NULL);
	return pgl_sample_1d_level(t, data, w, x, pgl_within_level_filter(t->min_filter));
}

static vec4 pgl_sample_2d_level_idx(const glTexture* t, int level, float x, float y)
{
	u8* data = pgl_tex_level_data(t, level);
	PGL_ASSERT(data);

	GLsizei w, h;
	pgl_tex_level_dims(t, level, &w, &h, NULL);
	return pgl_sample_2d_level(t, data, w, h, x, y, pgl_within_level_filter(t->min_filter));
}

// Minify path: *MIPMAP_NEAREST → one level; *MIPMAP_LINEAR → two levels + lerp (trilinear).
// Only used when min filter is a mip mode, chain exists, and λ/lod > 0.
static vec4 pgl_sample_1d_minify(const glTexture* t, float x, float lod)
{
	int max_level = t->num_levels - 1;
	if (max_level <= 0)
		return pgl_sample_1d_level_idx(t, 0, x);

	// Non-trilinear: single rounded/floored level (same as before)
	if (!pgl_is_mip_linear_filter(t->min_filter))
		return pgl_sample_1d_level_idx(t, pgl_lod_to_level(t, lod), x);

	// Trilinear: blend floor(lod) and floor(lod)+1
	if (lod < 0.0f)
		lod = 0.0f;
	if (lod >= (float)max_level)
		return pgl_sample_1d_level_idx(t, max_level, x);

	int l0 = (int)floorf(lod);
	float frac = lod - (float)l0;
	if (frac <= 0.0f)
		return pgl_sample_1d_level_idx(t, l0, x);
	if (frac >= 1.0f)
		return pgl_sample_1d_level_idx(t, l0 + 1, x);

	vec4 c0 = pgl_sample_1d_level_idx(t, l0, x);
	vec4 c1 = pgl_sample_1d_level_idx(t, l0 + 1, x);
	return pgl_lerp_v4(c0, c1, frac);
}

static vec4 pgl_sample_2d_minify(const glTexture* t, float x, float y, float lod)
{
	int max_level = t->num_levels - 1;
	if (max_level <= 0)
		return pgl_sample_2d_level_idx(t, 0, x, y);

	if (!pgl_is_mip_linear_filter(t->min_filter))
		return pgl_sample_2d_level_idx(t, pgl_lod_to_level(t, lod), x, y);

	if (lod < 0.0f)
		lod = 0.0f;
	if (lod >= (float)max_level)
		return pgl_sample_2d_level_idx(t, max_level, x, y);

	int l0 = (int)floorf(lod);
	float frac = lod - (float)l0;
	if (frac <= 0.0f)
		return pgl_sample_2d_level_idx(t, l0, x, y);
	if (frac >= 1.0f)
		return pgl_sample_2d_level_idx(t, l0 + 1, x, y);

	vec4 c0 = pgl_sample_2d_level_idx(t, l0, x, y);
	vec4 c1 = pgl_sample_2d_level_idx(t, l0 + 1, x, y);
	return pgl_lerp_v4(c0, c1, frac);
}

PGLDEF vec4 texture1D(GLuint tex, float x)
{
	glTexture* t = pgl_tex_1d(tex);

	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	// Mip path only when a chain exists and MIN_FILTER is a *MIPMAP* mode
	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter)) {
		float lambda = pgl_auto_lod(t, t->w, 1);
		if (lambda <= 0.0f)
			return pgl_sample_1d_level(t, t->data, t->w, x, t->mag_filter);
		return pgl_sample_1d_minify(t, x, lambda);
	}

	// Incomplete mip filter: Core → black; compat → L0 + mag
	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_1d_level(t, t->data, t->w, x, t->mag_filter);
}

PGLDEF vec4 texture1DLod(GLuint tex, float x, float lod)
{
	glTexture* t = pgl_tex_1d(tex);

	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	// λ ≤ 0 → magnify base with MAG_FILTER; λ > 0 → minify (same as texture1D auto)
	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter)) {
		if (lod <= 0.0f)
			return pgl_sample_1d_level(t, t->data, t->w, x, t->mag_filter);
		return pgl_sample_1d_minify(t, x, lod);
	}

	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	// Non-mip or incomplete-compat: within-level filter from min_filter
	return pgl_sample_1d_level(t, t->data, t->w, x, pgl_within_level_filter(t->min_filter));
}

PGLDEF vec4 texture2D(GLuint tex, float x, float y)
{
	glTexture* t = pgl_tex_2d(tex);

	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter)) {
		float lambda = pgl_auto_lod(t, t->w, t->h);
		if (lambda <= 0.0f)
			return pgl_sample_2d_level(t, t->data, t->w, t->h, x, y, t->mag_filter);
		return pgl_sample_2d_minify(t, x, y, lambda);
	}

	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_2d_level(t, t->data, t->w, t->h, x, y, t->mag_filter);
}

PGLDEF vec4 texture2DLod(GLuint tex, float x, float y, float lod)
{
	glTexture* t = pgl_tex_2d(tex);

	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	// λ ≤ 0 → magnify base with MAG_FILTER; λ > 0 → minify (same as texture2D auto)
	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter)) {
		if (lod <= 0.0f)
			return pgl_sample_2d_level(t, t->data, t->w, t->h, x, y, t->mag_filter);
		return pgl_sample_2d_minify(t, x, y, lod);
	}

	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_2d_level(t, t->data, t->w, t->h, x, y,
	                           pgl_within_level_filter(t->min_filter));
}

PGLDEF vec4 texture1DGrad(GLuint tex, float x, float dPdx, float dPdy)
{
	glTexture* t = pgl_tex_1d(tex);
	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter)) {
		float lod = pgl_lod_from_grad1d_dim((float)t->w, dPdx, dPdy);
		if (lod <= 0.0f)
			return pgl_sample_1d_level(t, t->data, t->w, x, t->mag_filter);
		return pgl_sample_1d_minify(t, x, lod);
	}

	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_1d_level(t, t->data, t->w, x, pgl_within_level_filter(t->min_filter));
}

PGLDEF vec4 texture2DGrad(GLuint tex, float x, float y,
                          float dUdx, float dVdx, float dUdy, float dVdy)
{
	glTexture* t = pgl_tex_2d(tex);
	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	if (t->num_levels > 1 && pgl_is_mip_min_filter(t->min_filter)) {
		float lod = pgl_lod_from_grad_dims((float)t->w, (float)t->h,
		                                   dUdx, dVdx, dUdy, dVdy);
		if (lod <= 0.0f)
			return pgl_sample_2d_level(t, t->data, t->w, t->h, x, y, t->mag_filter);
		return pgl_sample_2d_minify(t, x, y, lod);
	}

	if (pgl_incomplete_mip_returns_black(t))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_2d_level(t, t->data, t->w, t->h, x, y,
	                           pgl_within_level_filter(t->min_filter));
}

PGLDEF vec4 texture3D(GLuint tex, float x, float y, float z)
{
	int i0, j0, i1, j1, k0, k1;

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_3D-GL_TEXTURE_1D];
	}
	float dw = t->w - EPSILON;
	float dh = t->h - EPSILON;
	float dd = t->d - EPSILON;

	int w = t->w;
	int h = t->h;
	int d = t->d;
	int plane = w * t->h;
	float xw = x * dw;
	float yh = y * dh;
	float zd = z * dd;


	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floorf(xw), w, t->wrap_s);
		j0 = wrap(floorf(yh), h, t->wrap_t);
		k0 = wrap(floorf(zd), d, t->wrap_r);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0 | k0) < 0) return t->border_color;
#endif

		return pgl_load_texel(t, t->data, k0*plane + j0*w + i0);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floorf(xw - 0.5f), w, t->wrap_s);
		j0 = wrap(floorf(yh - 0.5f), h, t->wrap_t);
		k0 = wrap(floorf(zd - 0.5f), d, t->wrap_r);
		i1 = wrap(floorf(xw + 0.499999f), w, t->wrap_s);
		j1 = wrap(floorf(yh + 0.499999f), h, t->wrap_t);
		k1 = wrap(floorf(zd + 0.499999f), d, t->wrap_r);

		float tmp2;
		float alpha = modff(xw+0.5f, &tmp2);
		float beta = modff(yh+0.5f, &tmp2);
		float gamma = modff(zd+0.5f, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;
		if (gamma < 0) ++gamma;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
		gamma = gamma*gamma * (3 - 2*gamma);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		vec4 cijk, ci1jk, cij1k, ci1j1k, cijk1, ci1jk1, cij1k1, ci1j1k1;
		if ((i0 | j0 | k0) < 0) cijk = t->border_color;
		else cijk = pgl_load_texel(t, t->data, k0*plane + j0*w + i0);

		if ((i1 | j0 | k0) < 0) ci1jk = t->border_color;
		else ci1jk = pgl_load_texel(t, t->data, k0*plane + j0*w + i1);

		if ((i0 | j1 | k0) < 0) cij1k = t->border_color;
		else cij1k = pgl_load_texel(t, t->data, k0*plane + j1*w + i0);

		if ((i1 | j1 | k0) < 0) ci1j1k = t->border_color;
		else ci1j1k = pgl_load_texel(t, t->data, k0*plane + j1*w + i1);

		if ((i0 | j0 | k1) < 0) cijk1 = t->border_color;
		else cijk1 = pgl_load_texel(t, t->data, k1*plane + j0*w + i0);

		if ((i1 | j0 | k1) < 0) ci1jk1 = t->border_color;
		else ci1jk1 = pgl_load_texel(t, t->data, k1*plane + j0*w + i1);

		if ((i0 | j1 | k1) < 0) cij1k1 = t->border_color;
		else cij1k1 = pgl_load_texel(t, t->data, k1*plane + j1*w + i0);

		if ((i1 | j1 | k1) < 0) ci1j1k1 = t->border_color;
		else ci1j1k1 = pgl_load_texel(t, t->data, k1*plane + j1*w + i1);
#else
		vec4 cijk = pgl_load_texel(t, t->data, k0*plane + j0*w + i0);
		vec4 ci1jk = pgl_load_texel(t, t->data, k0*plane + j0*w + i1);
		vec4 cij1k = pgl_load_texel(t, t->data, k0*plane + j1*w + i0);
		vec4 ci1j1k = pgl_load_texel(t, t->data, k0*plane + j1*w + i1);
		vec4 cijk1 = pgl_load_texel(t, t->data, k1*plane + j0*w + i0);
		vec4 ci1jk1 = pgl_load_texel(t, t->data, k1*plane + j0*w + i1);
		vec4 cij1k1 = pgl_load_texel(t, t->data, k1*plane + j1*w + i0);
		vec4 ci1j1k1 = pgl_load_texel(t, t->data, k1*plane + j1*w + i1);
#endif

		cijk = scale_v4(cijk, (1-alpha)*(1-beta)*(1-gamma));
		ci1jk = scale_v4(ci1jk, alpha*(1-beta)*(1-gamma));
		cij1k = scale_v4(cij1k, (1-alpha)*beta*(1-gamma));
		ci1j1k = scale_v4(ci1j1k, alpha*beta*(1-gamma));
		cijk1 = scale_v4(cijk1, (1-alpha)*(1-beta)*gamma);
		ci1jk1 = scale_v4(ci1jk1, alpha*(1-beta)*gamma);
		cij1k1 = scale_v4(cij1k1, (1-alpha)*beta*gamma);
		ci1j1k1 = scale_v4(ci1j1k1, alpha*beta*gamma);

		cijk = add_v4s(cijk, ci1jk);
		cijk = add_v4s(cijk, cij1k);
		cijk = add_v4s(cijk, ci1j1k);
		cijk = add_v4s(cijk, cijk1);
		cijk = add_v4s(cijk, ci1jk1);
		cijk = add_v4s(cijk, cij1k1);
		cijk = add_v4s(cijk, ci1j1k1);

		return cijk;
	}
}

// for now this should work
PGLDEF vec4 texture2DArray(GLuint tex, float x, float y, int z)
{
	int i0, j0, i1, j1;

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_2D_ARRAY-GL_TEXTURE_1D];
	}
	Color* texdata = (Color*)t->data;
	int w = t->w;
	int h = t->h;

	float dw = w - EPSILON;
	float dh = h - EPSILON;

	int plane = w * h;
	float xw = x * dw;
	float yh = y * dh;


	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floorf(xw), w, t->wrap_s);
		j0 = wrap(floorf(yh), h, t->wrap_t);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0) < 0) return t->border_color;
#endif
		return Color_to_v4(texdata[z*plane + j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floorf(xw - 0.5f), w, t->wrap_s);
		j0 = wrap(floorf(yh - 0.5f), h, t->wrap_t);
		i1 = wrap(floorf(xw + 0.499999f), w, t->wrap_s);
		j1 = wrap(floorf(yh + 0.499999f), h, t->wrap_t);

		float tmp2;
		float alpha = modff(xw+0.5f, &tmp2);
		float beta = modff(yh+0.5f, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		vec4 cij, ci1j, cij1, ci1j1;
		if ((i0 | j0) < 0) cij = t->border_color;
		else cij = Color_to_v4(texdata[z*plane + j0*w + i0]);

		if ((i1 | j0) < 0) ci1j = t->border_color;
		else ci1j = Color_to_v4(texdata[z*plane + j0*w + i1]);

		if ((i0 | j1) < 0) cij1 = t->border_color;
		else cij1 = Color_to_v4(texdata[z*plane + j1*w + i0]);

		if ((i1 | j1) < 0) ci1j1 = t->border_color;
		else ci1j1 = Color_to_v4(texdata[z*plane + j1*w + i1]);
#else
		vec4 cij = Color_to_v4(texdata[z*plane + j0*w + i0]);
		vec4 ci1j = Color_to_v4(texdata[z*plane + j0*w + i1]);
		vec4 cij1 = Color_to_v4(texdata[z*plane + j1*w + i0]);
		vec4 ci1j1 = Color_to_v4(texdata[z*plane + j1*w + i1]);
#endif

		cij = scale_v4(cij, (1-alpha)*(1-beta));
		ci1j = scale_v4(ci1j, alpha*(1-beta));
		cij1 = scale_v4(cij1, (1-alpha)*beta);
		ci1j1 = scale_v4(ci1j1, alpha*beta);

		cij = add_v4s(cij, ci1j);
		cij = add_v4s(cij, cij1);
		cij = add_v4s(cij, ci1j1);

		return cij;
	}
}

PGLDEF vec4 texture_rect(GLuint tex, float x, float y)
{
	int i0, j0, i1, j1;

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_RECTANGLE-GL_TEXTURE_1D];
	}
	Color* texdata = (Color*)t->data;

	int w = t->w;
	int h = t->h;

	float xw = x;
	float yh = y;

	//TODO don't just use mag_filter all the time?
	//is it worth bothering?
	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floorf(xw), w, t->wrap_s);
		j0 = wrap(floorf(yh), h, t->wrap_t);

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		if ((i0 | j0) < 0) return t->border_color;
#endif
		return Color_to_v4(texdata[j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floorf(xw - 0.5f), w, t->wrap_s);
		j0 = wrap(floorf(yh - 0.5f), h, t->wrap_t);
		i1 = wrap(floorf(xw + 0.499999f), w, t->wrap_s);
		j1 = wrap(floorf(yh + 0.499999f), h, t->wrap_t);

		float tmp2;
		float alpha = modff(xw+0.5f, &tmp2);
		float beta = modff(yh+0.5f, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef PGL_HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
#endif

#ifdef PGL_ENABLE_CLAMP_TO_BORDER
		vec4 cij, ci1j, cij1, ci1j1;
		if ((i0 | j0) < 0) cij = t->border_color;
		else cij = Color_to_v4(texdata[j0*w + i0]);

		if ((i1 | j0) < 0) ci1j = t->border_color;
		else ci1j = Color_to_v4(texdata[j0*w + i1]);

		if ((i0 | j1) < 0) cij1 = t->border_color;
		else cij1 = Color_to_v4(texdata[j1*w + i0]);

		if ((i1 | j1) < 0) ci1j1 = t->border_color;
		else ci1j1 = Color_to_v4(texdata[j1*w + i1]);
#else
		vec4 cij = Color_to_v4(texdata[j0*w + i0]);
		vec4 ci1j = Color_to_v4(texdata[j0*w + i1]);
		vec4 cij1 = Color_to_v4(texdata[j1*w + i0]);
		vec4 ci1j1 = Color_to_v4(texdata[j1*w + i1]);
#endif

		cij = scale_v4(cij, (1-alpha)*(1-beta));
		ci1j = scale_v4(ci1j, alpha*(1-beta));
		cij1 = scale_v4(cij1, (1-alpha)*beta);
		ci1j1 = scale_v4(ci1j1, alpha*beta);

		cij = add_v4s(cij, ci1j);
		cij = add_v4s(cij, cij1);
		cij = add_v4s(cij, ci1j1);

		return cij;
	}
}

// Sample one face of a cubemap level (level_data points at the 6-face pack).
// face is 0..5; x,y are [0,1] face UVs.  filter is NEAREST or LINEAR.
static vec4 pgl_sample_cube_face(const glTexture* t, const u8* level_data,
                                 int w, int h, int face, float x, float y, GLenum filter)
{
	Color* texdata = (Color*)level_data;
	float dw = w - EPSILON;
	float dh = h - EPSILON;
	int plane = w * h;
	float xw = x * dw;
	float yh = y * dh;
	int i0, j0, i1, j1;

	if (filter == GL_NEAREST) {
		i0 = wrap(floorf(xw), w, t->wrap_s);
		j0 = wrap(floorf(yh), h, t->wrap_t);
		return Color_to_v4(texdata[face * plane + j0 * w + i0]);
	}

	// LINEAR
	i0 = wrap(floorf(xw - 0.5f), w, t->wrap_s);
	j0 = wrap(floorf(yh - 0.5f), h, t->wrap_t);
	i1 = wrap(floorf(xw + 0.499999f), w, t->wrap_s);
	j1 = wrap(floorf(yh + 0.499999f), h, t->wrap_t);

	float tmp2;
	float alpha = modff(xw + 0.5f, &tmp2);
	float beta = modff(yh + 0.5f, &tmp2);
	if (alpha < 0) ++alpha;
	if (beta < 0) ++beta;

#ifdef PGL_HERMITE_SMOOTHING
	alpha = alpha * alpha * (3 - 2 * alpha);
	beta = beta * beta * (3 - 2 * beta);
#endif

	vec4 cij = Color_to_v4(texdata[face * plane + j0 * w + i0]);
	vec4 ci1j = Color_to_v4(texdata[face * plane + j0 * w + i1]);
	vec4 cij1 = Color_to_v4(texdata[face * plane + j1 * w + i0]);
	vec4 ci1j1 = Color_to_v4(texdata[face * plane + j1 * w + i1]);

	cij = scale_v4(cij, (1 - alpha) * (1 - beta));
	ci1j = scale_v4(ci1j, alpha * (1 - beta));
	cij1 = scale_v4(cij1, (1 - alpha) * beta);
	ci1j1 = scale_v4(ci1j1, alpha * beta);

	cij = add_v4s(cij, ci1j);
	cij = add_v4s(cij, cij1);
	cij = add_v4s(cij, ci1j1);
	return cij;
}

static vec4 pgl_sample_cube_level_idx(const glTexture* t, int level, int face, float x, float y)
{
	u8* data = pgl_tex_level_data(t, level);
	PGL_ASSERT(data);

	GLsizei w, h;
	pgl_tex_level_dims(t, level, &w, &h, NULL);
	return pgl_sample_cube_face(t, data, w, h, face, x, y,
	                            pgl_within_level_filter(t->min_filter));
}

// Minify path for cubemaps (same LOD rules as 2D: nearest level or trilinear)
static vec4 pgl_sample_cube_minify(const glTexture* t, int face, float x, float y, float lod)
{
	int max_level = t->num_levels - 1;
	if (max_level <= 0)
		return pgl_sample_cube_level_idx(t, 0, face, x, y);

	if (!pgl_is_mip_linear_filter(t->min_filter))
		return pgl_sample_cube_level_idx(t, pgl_lod_to_level(t, lod), face, x, y);

	if (lod < 0.0f)
		lod = 0.0f;
	if (lod >= (float)max_level)
		return pgl_sample_cube_level_idx(t, max_level, face, x, y);

	int l0 = (int)floorf(lod);
	float frac = lod - (float)l0;
	if (frac <= 0.0f)
		return pgl_sample_cube_level_idx(t, l0, face, x, y);
	if (frac >= 1.0f)
		return pgl_sample_cube_level_idx(t, l0 + 1, face, x, y);

	vec4 c0 = pgl_sample_cube_level_idx(t, l0, face, x, y);
	vec4 c1 = pgl_sample_cube_level_idx(t, l0 + 1, face, x, y);
	return pgl_lerp_v4(c0, c1, frac);
}

// Select cubemap face and face UV in [0,1] from a direction (x,y,z).
// Returns face index 0..5; writes *out_s, *out_t.
static int pgl_cube_select_face_st(float x, float y, float z, float* out_s, float* out_t)
{
	float x_mag = (x < 0) ? -x : x;
	float y_mag = (y < 0) ? -y : y;
	float z_mag = (z < 0) ? -z : z;

	float s, t, maxv;
	int p;

	//there should be a better/shorter way to do this ...
	if (x_mag > y_mag) {
		if (x_mag > z_mag) {  //x largest
			maxv = x_mag;
			t = -y;
			if (x_mag == x) {
				p = 0;
				s = -z;
			} else {
				p = 1;
				s = z;
			}
		} else { //z largest
			maxv = z_mag;
			t = -y;
			if (z_mag == z) {
				p = 4;
				s = x;
			} else {
				p = 5;
				s = -x;
			}
		}
	} else {
		if (y_mag > z_mag) {  //y largest
			maxv = y_mag;
			s = x;
			if (y_mag == y) {
				p = 2;
				t = z;
			} else {
				p = 3;
				t = -z;
			}
		} else { //z largest
			maxv = z_mag;
			t = -y;
			if (z_mag == z) {
				p = 4;
				s = x;
			} else {
				p = 5;
				s = -x;
			}
		}
	}

	// TODO As I understand this, this prevents x and y from ever being
	// outside [0, 1] so there's no need for me to put CLAMP_TO_BORDER ifdefs
	// in here, since even CLAMP_TO_EDGE should never happen.
	if (maxv < 1e-20f)
		maxv = 1e-20f;
	*out_s = (s / maxv + 1.0f) / 2.0f;
	*out_t = (t / maxv + 1.0f) / 2.0f;
	return p;
}

// Project direction onto a *fixed* face (for gradient FD without face flips).
// Face major-axis formulas match the select path above.
static void pgl_cube_face_st(int face, float x, float y, float z, float* out_s, float* out_t)
{
	float s, t, maxv;
	switch (face) {
	case 0: // +X
		maxv = (x < 0) ? -x : x;
		if (maxv < 1e-20f) maxv = 1e-20f;
		s = -z; t = -y;
		break;
	case 1: // -X
		maxv = (x < 0) ? -x : x;
		if (maxv < 1e-20f) maxv = 1e-20f;
		s = z; t = -y;
		break;
	case 2: // +Y
		maxv = (y < 0) ? -y : y;
		if (maxv < 1e-20f) maxv = 1e-20f;
		s = x; t = z;
		break;
	case 3: // -Y
		maxv = (y < 0) ? -y : y;
		if (maxv < 1e-20f) maxv = 1e-20f;
		s = x; t = -z;
		break;
	case 4: // +Z
		maxv = (z < 0) ? -z : z;
		if (maxv < 1e-20f) maxv = 1e-20f;
		s = x; t = -y;
		break;
	default: // -Z
		maxv = (z < 0) ? -z : z;
		if (maxv < 1e-20f) maxv = 1e-20f;
		s = -x; t = -y;
		break;
	}
	*out_s = (s / maxv + 1.0f) / 2.0f;
	*out_t = (t / maxv + 1.0f) / 2.0f;
}

// Explicit λ for cubemap Lod/Grad: λ ≤ 0 → MAG on base; λ > 0 → minify
static vec4 pgl_sample_cube_with_lod(glTexture* tex, int face, float s, float t, float lod)
{
	if (tex->num_levels > 1 && pgl_is_mip_min_filter(tex->min_filter)) {
		if (lod <= 0.0f)
			return pgl_sample_cube_face(tex, tex->data, tex->w, tex->h, face, s, t, tex->mag_filter);
		return pgl_sample_cube_minify(tex, face, s, t, lod);
	}
	if (pgl_incomplete_mip_returns_black(tex))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);
	return pgl_sample_cube_face(tex, tex->data, tex->w, tex->h, face, s, t,
	                            pgl_within_level_filter(tex->min_filter));
}

PGLDEF vec4 texture_cubemap(GLuint texture, float x, float y, float z)
{
	glTexture* tex = pgl_tex_cube(texture);

	if (!tex->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	float s, t;
	int face = pgl_cube_select_face_st(x, y, z, &s, &t);

	if (tex->num_levels > 1 && pgl_is_mip_min_filter(tex->min_filter)) {
		// Auto LOD from per-triangle UV scale and face base size (same ρ as 2D)
		float lambda = pgl_auto_lod(tex, tex->w, tex->h);
		if (lambda <= 0.0f)
			return pgl_sample_cube_face(tex, tex->data, tex->w, tex->h, face, s, t, tex->mag_filter);
		return pgl_sample_cube_minify(tex, face, s, t, lambda);
	}

	// Incomplete mip filter: Core → black; compat → L0 + mag
	if (pgl_incomplete_mip_returns_black(tex))
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_sample_cube_face(tex, tex->data, tex->w, tex->h, face, s, t, tex->mag_filter);
}

PGLDEF vec4 texture_cubemapLod(GLuint texture, float x, float y, float z, float lod)
{
	glTexture* tex = pgl_tex_cube(texture);
	if (!tex->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	float s, t;
	int face = pgl_cube_select_face_st(x, y, z, &s, &t);
	return pgl_sample_cube_with_lod(tex, face, s, t, lod);
}

PGLDEF vec4 texture_cubemapGrad(GLuint texture, float x, float y, float z,
                                float dPdx_x, float dPdx_y, float dPdx_z,
                                float dPdy_x, float dPdy_y, float dPdy_z)
{
	glTexture* tex = pgl_tex_cube(texture);
	if (!tex->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	float s0, t0, s1, t1, s2, t2;
	int face = pgl_cube_select_face_st(x, y, z, &s0, &t0);

	// Same-face finite difference of the nonlinear face projection → ∂(s,t)/∂screen
	pgl_cube_face_st(face, x + dPdx_x, y + dPdx_y, z + dPdx_z, &s1, &t1);
	pgl_cube_face_st(face, x + dPdy_x, y + dPdy_y, z + dPdy_z, &s2, &t2);

	float dUdx = s1 - s0, dVdx = t1 - t0;
	float dUdy = s2 - s0, dVdy = t2 - t0;
	float lod = pgl_lod_from_grad_dims((float)tex->w, (float)tex->h,
	                                   dUdx, dVdx, dUdy, dVdy);
	return pgl_sample_cube_with_lod(tex, face, s0, t0, lod);
}

PGLDEF vec4 texelFetch1D(GLuint tex, int x, int lod)
{
	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_1D-GL_TEXTURE_1D];
	}

	if (lod < 0)
		lod = 0;
	u8* data = pgl_tex_level_data(t, lod);
	if (!data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	GLsizei w;
	pgl_tex_level_dims(t, lod, &w, NULL, NULL);
	if (x < 0 || x >= w)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_load_texel(t, data, x);
}

PGLDEF vec4 texelFetch2D(GLuint tex, int x, int y, int lod)
{
	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_2D-GL_TEXTURE_1D];
	}

	if (lod < 0)
		lod = 0;
	u8* data = pgl_tex_level_data(t, lod);
	if (!data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	GLsizei w, h;
	pgl_tex_level_dims(t, lod, &w, &h, NULL);
	if (x < 0 || x >= w || y < 0 || y >= h)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	return pgl_load_texel(t, data, pgl_tex_index_2d(t, x, y, w, h));
}

PGLDEF vec4 texelFetch3D(GLuint tex, int x, int y, int z, int lod)
{
	// 3D mipmaps not supported yet; lod other than 0 still reads level 0
	PGL_UNUSED(lod);

	glTexture* t = NULL;
	if (tex) {
		t = &c->textures.a[tex];
	} else {
		t = &c->default_textures[GL_TEXTURE_3D-GL_TEXTURE_1D];
	}
	if (!t->data)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	int w = t->w;
	int h = t->h;
	int d = t->d;
	if (x < 0 || x >= w || y < 0 || y >= h || z < 0 || z >= d)
		return make_v4(0.0f, 0.0f, 0.0f, 1.0f);

	int plane = w * h;
	// 3D has no invert_y (no FBO-as-3D); linear index
	return pgl_load_texel(t, t->data, z * plane + y * w + x);
}

// Real texture object only (not default name 0 — ambiguous across targets).
// Debug: GL_INVALID_VALUE + log; under PGL_UNSAFE the check is compiled out.
PGLDEF ivec3 textureSize(GLuint tex, GLint lod)
{
	PGL_ERR_RET_VAL(!tex, GL_INVALID_VALUE, make_iv3(0, 0, 0));
	glTexture* t = &c->textures.a[tex];

	if (lod < 0)
		lod = 0;

	// Clamp to last defined level (3D/array have only level 0 for now)
	if (t->num_levels > 0 && lod >= t->num_levels)
		lod = t->num_levels - 1;

	GLsizei w = 0, h = 0, d = 0;
	pgl_tex_level_dims(t, lod, &w, &h, &d);
	return make_iv3(w, h, d);
}

#undef EPSILON


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
PGLDEF void pglClearScreen(void)
{
	memset(c->back_buffer.buf, 255, c->back_buffer.w * c->back_buffer.h * sizeof(pix_t));
}

PGLDEF void pglSetInterp(GLsizei n, GLenum* interpolation)
{
	c->programs.a[c->cur_program].vs_output_size = n;
	c->vs_output.size = n;

	memcpy(c->programs.a[c->cur_program].interpolation, interpolation, n*sizeof(GLenum));

	// c->vs_output.output_buf was pre-allocated to max size needed in init_glContext
	// otherwise would need to assure it's at least
	// c->vs_output_size * PGL_MAX_VERTS * sizeof(float) right here

	//vs_output.interpolation would be already pointing at current program's array
	//unless the programs array was realloced since the last glUseProgram because
	//they've created a bunch of programs.  Unlikely they'd be changing a shader
	//before creating all their shaders but whatever.
	c->vs_output.interpolation = c->programs.a[c->cur_program].interpolation;
}


// Uses default_vs for vertex shader (passes vertex unchanged, no other attributes or outputs)
// This function is designed to be used with pglDrawFrame(), you don't need it for pglDrawFrame2()
PGLDEF GLuint pglCreateFragProgram(frag_func fragment_shader, GLboolean fragdepth_or_discard)
{
	// Using glAttachShader error if shader is not a shader object which
	// is the closest analog
	PGL_ERR_RET_VAL((!fragment_shader), GL_INVALID_OPERATION, 0);

	glProgram tmp = {default_vs, fragment_shader, NULL, 0, {0}, fragdepth_or_discard, GL_FALSE };

	for (int i=1; i<c->programs.size; ++i) {
		if (c->programs.a[i].deleted && (GLuint)i != c->cur_program) {
			c->programs.a[i] = tmp;
			return i;
		}
	}

	cvec_push_glProgram(&c->programs, tmp);
	return c->programs.size-1;
}

//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
//
// TODO worth another draw_pixel() that never does fragment_processing?
// worth duplicating the loops for initial discard check?
PGLDEF void pglDrawFrame(void)
{
	frag_func frag_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniforms = c->programs.a[c->cur_program].uniform;

	Shader_Builtins builtins;
	//#pragma omp parallel for private(builtins)
	for (int y=0; y<c->back_buffer.h; ++y) {
		for (int x=0; x<c->back_buffer.w; ++x) {

			//ignore z and w components
			builtins.gl_FragCoord.x = x + 0.5f;
			builtins.gl_FragCoord.y = y + 0.5f;

			builtins.discard = GL_FALSE;
			frag_shader(NULL, &builtins, uniforms);
			if (!builtins.discard)
				draw_pixel(builtins.gl_FragColor, x, y, 0.0f, GL_FALSE);  //scissor/stencil/depth aren't used for pglDrawFrame
		}
	}

}

PGLDEF void pglDrawFrame2(frag_func frag_shader, void* uniforms)
{
	Shader_Builtins builtins;
	//#pragma omp parallel for private(builtins)
	for (int y=0; y<c->back_buffer.h; ++y) {
		for (int x=0; x<c->back_buffer.w; ++x) {

			//ignore z and w components
			builtins.gl_FragCoord.x = x + 0.5f;
			builtins.gl_FragCoord.y = y + 0.5f;

			builtins.discard = GL_FALSE;
			frag_shader(NULL, &builtins, uniforms);
			if (!builtins.discard)
				draw_pixel(builtins.gl_FragColor, x, y, 0.0f, GL_FALSE);  //scissor/stencil/depth aren't used for pglDrawFrame
		}
	}
}

PGLDEF void pglBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage)
{
	//TODO check for usage later
	PGL_UNUSED(usage);

	PGL_ERR((target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER), GL_INVALID_ENUM);

	target -= GL_ARRAY_BUFFER;
	PGL_ERR(!c->bound_buffers[target], GL_INVALID_OPERATION);

	// data can't be null for user_owned data
	PGL_ERR(!data, GL_INVALID_VALUE);

	// TODO Should I change this in spec functions too?  Or just say don't mix them
	// otherwise bad things/undefined behavior??
	if (!c->buffers.a[c->bound_buffers[target]].user_owned) {
		free(c->buffers.a[c->bound_buffers[target]].data);
	}

	// user_owned buffer, just assign the pointer, will not free
	c->buffers.a[c->bound_buffers[target]].data = (u8*)data;

	c->buffers.a[c->bound_buffers[target]].user_owned = GL_TRUE;
	c->buffers.a[c->bound_buffers[target]].size = size;

	if (target == GL_ELEMENT_ARRAY_BUFFER) {
		c->vertex_arrays.a[c->cur_vertex_array].element_buffer = c->bound_buffers[target];
	}
}

// pglTex*/pglTextureImage*: map user memory (no copy). Format matrix matches
// glTexImage* storage (U8 RGBA or float R/RG/RGBA/depth); no conversion.
// Cubemap mapping remains packed U8 RGBA only.

// Shared validation for mapped pglTextureImage* (2D path is the reference).
// On failure sets error and returns GL_TRUE so caller can return.
#define PGL_TEXIMAGE_MAP_VALIDATE(format, type) do { \
	PGL_ERR((type) != GL_UNSIGNED_BYTE && (type) != GL_FLOAT, GL_INVALID_ENUM); \
	PGL_ERR((format) != GL_RGBA && (format) != GL_RG && (format) != GL_RED && \
	        (format) != GL_DEPTH_COMPONENT, GL_INVALID_ENUM); \
	PGL_ERR((type) == GL_UNSIGNED_BYTE && (format) != GL_RGBA && \
	        (format) != GL_DEPTH_COMPONENT, GL_INVALID_OPERATION); \
	PGL_ERR((type) == GL_FLOAT && (format) == GL_RGB, GL_INVALID_ENUM); \
} while (0)

PGLDEF void pglTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_ERR(target != GL_TEXTURE_1D, GL_INVALID_ENUM);
	GLuint cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	pglTextureImage1D(cur_tex, level, internalformat, width, border, format, type, data);
}

PGLDEF void pglTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	// NOTE, since this is mapping data, the entire cubemap has to already be arranged in memory in the correct order and we only
	// accept GL_TEXTURE_CUBE_MAP, not any of the individual planes as that wouldn't make sense
	PGL_ERR((target != GL_TEXTURE_2D &&
	         target != GL_TEXTURE_RECTANGLE &&
	         target != GL_TEXTURE_CUBE_MAP), GL_INVALID_ENUM);

	GLuint cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	pglTextureImage2D(cur_tex, level, internalformat, width, height, border, format, type, data);
}

PGLDEF void pglTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	PGL_ERR((target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY), GL_INVALID_ENUM);

	GLuint cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	pglTextureImage3D(cur_tex, level, internalformat, width, height, depth, border, format, type, data);
}

PGLDEF void pglTextureImage1D(GLuint texture, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	// User-owned mapping is level 0 only; higher levels use glTexImage*
	PGL_UNUSED(internalformat);

	PGL_ERR(border, GL_INVALID_VALUE);
	PGL_ERR(level != 0, GL_INVALID_VALUE);
	PGL_TEXIMAGE_MAP_VALIDATE(format, type);

	// data can't be null for user_owned data
	PGL_ERR(!data, GL_INVALID_VALUE);

	// I do not support DSA mapping of default texture 0, for convenience, no way to know which target it was
	// and I don't want to duplicate code or add extra functions
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);

	glTexture* tex = &c->textures.a[texture];
	if (!tex->user_owned)
		free(tex->data);

	tex->w = width;
	tex->h = 1;
	tex->d = 1;

	tex->data = (u8*)data;
	tex->data_alloc = 0;
	tex->user_owned = GL_TRUE;
	pgl_tex_set_format(tex, format, type);
	tex->num_levels = 1;
	pgl_set_level0_desc(tex);
}

PGLDEF void pglTextureImage2D(GLuint texture, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	// User-owned mapping is level 0 only; higher levels use glTexImage*
	// Color: U8 RGBA, or float R/RG/RGBA (R32F/RG32F/RGBA32F). No RGB32F.
	// Depth: GL_DEPTH_COMPONENT + GL_FLOAT or U8 Z pack.
	PGL_UNUSED(internalformat);

	PGL_ERR(border, GL_INVALID_VALUE);
	PGL_ERR(level != 0, GL_INVALID_VALUE);
	PGL_TEXIMAGE_MAP_VALIDATE(format, type);

	// data can't be null for user_owned data
	PGL_ERR(!data, GL_INVALID_VALUE);

	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);

	glTexture* tex = &c->textures.a[texture];
	// have to convert type back from offset to actual enum value
	GLenum target = tex->type + GL_TEXTURE_UNBOUND + 1;
	if (target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE) {
		if (!tex->user_owned)
			free(tex->data);

		tex->w = width;
		tex->h = height;
		tex->d = 1;

		// If you're using these pgl mapped functions, it assumes you are respecting
		// your own current unpack alignment settings already
		tex->data = (u8*)data;
		tex->data_alloc = 0;
		tex->user_owned = GL_TRUE;
		pgl_tex_set_format(tex, format, type);
		tex->num_levels = 1;
		pgl_set_level0_desc(tex);

	} else {  //CUBE_MAP
		// We only accept all the data already arranged, since we're mapping,
		// no individual planes/copying
		// Cubemaps remain UNSIGNED_BYTE RGBA only for now
		PGL_ERR(type != GL_UNSIGNED_BYTE, GL_INVALID_ENUM);
		PGL_ERR(format != GL_RGBA, GL_INVALID_ENUM);

		if (!tex->user_owned)
			free(tex->data);

		//TODO spec says INVALID_VALUE, man pages say INVALID_ENUM ?
		PGL_ERR(width != height, GL_INVALID_VALUE);

		tex->w = width;
		tex->h = height;
		tex->d = 1;

		tex->data = (u8*)data;
		tex->data_alloc = 0;
		tex->user_owned = GL_TRUE;
		pgl_tex_set_format(tex, GL_RGBA, GL_UNSIGNED_BYTE);
		tex->num_levels = 1;
		pgl_set_level0_desc(tex);

	} //end CUBE_MAP

}

PGLDEF void pglTextureImage3D(GLuint texture, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	// User-owned mapping is level 0 only (3D mips not supported yet)
	PGL_UNUSED(internalformat);

	PGL_ERR(border, GL_INVALID_VALUE);
	PGL_ERR(level != 0, GL_INVALID_VALUE);
	PGL_TEXIMAGE_MAP_VALIDATE(format, type);

	// data can't be null for user_owned data
	PGL_ERR(!data, GL_INVALID_VALUE);

	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);

	glTexture* tex = &c->textures.a[texture];
	if (!tex->user_owned)
		free(tex->data);

	tex->w = width;
	tex->h = height;
	tex->d = depth;

	tex->data = (u8*)data;
	tex->data_alloc = 0;
	tex->user_owned = GL_TRUE;
	pgl_tex_set_format(tex, format, type);
	tex->num_levels = 1;
	pgl_set_level0_desc(tex);

}

PGLDEF void pglGetBufferData(GLuint buffer, GLvoid** data)
{
	// why'd you even call it?
	PGL_ERR(!data, GL_INVALID_VALUE);

	// matching error code of binding invalid buffecr
	PGL_ERR((!buffer || buffer >= c->buffers.size || c->buffers.a[buffer].deleted),
	        GL_INVALID_OPERATION);

	*data = c->buffers.a[buffer].data;
}

PGLDEF void pglGetTextureData(GLuint texture, GLvoid** data)
{
	// why'd you even call it?
	PGL_ERR(!data, GL_INVALID_VALUE);

	// TODO texture 0?
	PGL_ERR((texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION);

	*data = c->textures.a[texture].data;
}

PGLDEF const glTexture* pglGetTexture(GLuint texture)
{
	// TODO texture 0?
	PGL_ERR_RET_VAL((texture >= c->textures.size || c->textures.a[texture].deleted), GL_INVALID_OPERATION, NULL);

	return &c->textures.a[texture];
}

// TODO hmm, void*, or u8*, or GLvoid*?
GLvoid* pglGetBackBuffer(void)
{
	return c->back_buffer.buf;
}

PGLDEF void pglSetBackBuffer(GLvoid* backbuf, GLsizei w, GLsizei h, GLboolean user_owned)
{
	// Always update the window default color surface. If an FBO is bound,
	// active back_buffer stays on the attachment until bind 0.
	glFramebuffer* bb = c->fbo_redirected ? &c->window_back_buffer : &c->back_buffer;
	bb->w = w;
	bb->h = h;
	bb->buf = (u8*)backbuf;
	bb->lastrow = bb->buf + (h-1)*w*sizeof(pix_t);
	if (!c->fbo_redirected)
		c->back_buffer = *bb;

	c->user_alloced_backbuf = user_owned;
}

PGLDEF void pglSetTexBackBuffer(GLuint texture)
{
	// NOTE, I do not support texture 0
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted ||
	         c->textures.a[texture].type+GL_TEXTURE_UNBOUND+1 != GL_TEXTURE_2D), GL_INVALID_OPERATION);
	glTexture* t = &c->textures.a[texture];
	// Draw uses lastrow FB indexing; sample must match (Phase A RT origin).
	pgl_tex_mark_render_target(t);
	// Color write path still uses sizeof(pix_t) strides — U8 RGBA RTs only for draw.
	pglSetBackBuffer((GLvoid*)t->data, t->w, t->h, t->user_owned);
}

// Mark a 2D texture as a render target without changing the current back buffer.
// Sample/fetch use lastrow indexing so they match fragCoord lastrow writes into
// the same memory (multipass / mapped float or U8 buffers). Call after the
// texture has L0 storage (e.g. after pglTextureImage2D). Remap/resize keeps
// invert_y and refreshes lastrow via pgl_set_level0_desc.
PGLDEF void pglTextureAsRenderTarget(GLuint texture)
{
	PGL_ERR((!texture || texture >= c->textures.size || c->textures.a[texture].deleted ||
	         c->textures.a[texture].type+GL_TEXTURE_UNBOUND+1 != GL_TEXTURE_2D), GL_INVALID_OPERATION);
	pgl_tex_mark_render_target(&c->textures.a[texture]);
}


// Not sure where else to put these two functions, they're helper/stopgap
// measures to deal with PGL only supporting RGBA but they're
// also useful functions on their own and not really "extensions"
// so I don't feel right putting them here or giving them a pgl prefix.
//
// Takes an image with GL_UNSIGNED_BYTE channels in
// a format other than packed GL_RGBA and returns it in (tightly packed) GL_RGBA
// (with the same rules as GLSL texture access for filling the other channels).
// See section 3.6.2 page 65 of the OpenGL ES 2.0.25 spec pdf
//
// IOW this creates an image that will give you the same values in the
// shader that you would have gotten had you used the unsupported
// format.  Passing in a GL_RGBA where pitch == w*4 reduces to a single memcpy
//
// If output is NULL, it will allocate the output image for you
// pitch is the length of a row in bytes.
//
// Returns the resulting packed RGBA image
PGLDEF u8* convert_format_to_packed_rgba(u8* output, u8* input, int w, int h, int pitch, GLenum format)
{
	int i, j, size = w*h;
	int rb = pitch;
	u8* out = output;
	if (!out) {
		out = (u8*)PGL_MALLOC(size*4);
		PGL_ERR_RET_VAL(!out, GL_OUT_OF_MEMORY, NULL);
	}
	memset(out, 0, size*4);

	u8* p = out;

	if (format == PGL_ONE_ALPHA) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = UINT8_MAX;
				p[1] = UINT8_MAX;
				p[2] = UINT8_MAX;
				p[3] = input[i*rb+j];
			}
		}
	} else if (format == GL_ALPHA) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[3] = input[i*rb+j];
			}
		}
	} else if (format == GL_LUMINANCE) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j];
				p[1] = input[i*rb+j];
				p[2] = input[i*rb+j];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_RED) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_LUMINANCE_ALPHA) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*2];
				p[1] = input[i*rb+j*2];
				p[2] = input[i*rb+j*2];
				p[3] = input[i*rb+j*2+1];
			}
		}
	} else if (format == GL_RG) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*2];
				p[1] = input[i*rb+j*2+1];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_RGB) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*3];
				p[1] = input[i*rb+j*3+1];
				p[2] = input[i*rb+j*3+2];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_BGR) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*3+2];
				p[1] = input[i*rb+j*3+1];
				p[2] = input[i*rb+j*3];
				p[3] = UINT8_MAX;
			}
		}
	} else if (format == GL_BGRA) {
		for (i=0; i<h; ++i) {
			for (j=0; j<w; ++j, p+=4) {
				p[0] = input[i*rb+j*4+2];
				p[1] = input[i*rb+j*4+1];
				p[2] = input[i*rb+j*4];
				p[3] = input[i*rb+j*4+3];
			}
		}
	} else if (format == GL_RGBA) {
		if (pitch == w*4) {
			// Just a plain copy
			memcpy(out, input, w*h*4);
		} else {
			// get rid of row padding
			int bw = w*4;
			for (i=0; i<h; ++i) {
				memcpy(&out[i*bw], &input[i*rb], bw);
			}
		}
	} else {
		PGL_ASSERT(0 && "ERROR: Unrecognized or unsupported input format!");
		free(out);
		out = NULL;
	}
	return out;
}

// pass in packed single channel 8 bit image where background=0, foreground=255
// and get a packed 4-channel rgba image using the colors provided
PGLDEF u8* convert_grayscale_to_rgba(u8* input, int size, u32 bg_rgba, u32 text_rgba)
{
	float rb, gb, bb, ab, rt, gt, bt, at;

	u8* tmp = (u8*)&bg_rgba;
	rb = tmp[0];
	gb = tmp[1];
	bb = tmp[2];
	ab = tmp[3];

	tmp = (u8*)&text_rgba;
	rt = tmp[0];
	gt = tmp[1];
	bt = tmp[2];
	at = tmp[3];

	//printf("background = (%f, %f, %f, %f)\ntext = (%f, %f, %f, %f)\n", rb, gb, bb, ab, rt, gt, bt, at);

	u8* color_image = (u8*)PGL_MALLOC(size * 4);
	PGL_ERR_RET_VAL(!color_image, GL_OUT_OF_MEMORY, NULL);
	float t;
	for (int i=0; i<size; ++i) {
		t = (input[i] - 0) / 255.0;
		color_image[i*4] = rt * t + rb * (1 - t);
		color_image[i*4+1] = gt * t + gb * (1 - t);
		color_image[i*4+2] = bt * t + bb * (1 - t);
		color_image[i*4+3] = at * t + ab * (1 - t);
	}


	return color_image;
}


// Just a convenience to have default textures return a specific color, like white here,
// so a textured shading algorithm will look untextured if you bind texture 0
PGLDEF int setup_default_textures(void)
{
	// just 1 white pixel
	// Could make it static and map it so we don't have a 12 tiny allocations
	GLuint image[1] = {
		0xFFFFFFFF
	};
	int w = 1;
	int h = 1;
	int d = 1;
	int frames = 1;
	// If this was called in init_glContext() or immediately after we would know
	// 0 was already bound
	glBindTexture(GL_TEXTURE_1D, 0);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_COMPRESSED_RGBA, w, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glBindTexture(GL_TEXTURE_2D, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glBindTexture(GL_TEXTURE_3D, 0);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_COMPRESSED_RGBA, w, h, d, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glBindTexture(GL_TEXTURE_1D_ARRAY, 0);
	glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, GL_COMPRESSED_RGBA, w, frames, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_COMPRESSED_RGBA, w, h, frames, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	GLenum cube[6] =
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};
	for (int i=0; i<6; i++) {
		glTexImage2D(cube[i], 0, GL_COMPRESSED_RGBA, w, h, 0,
		             GL_RGBA, GL_UNSIGNED_BYTE, image);
	}

	return GL_TRUE;
}

PGLDEF void put_pixel(Color color, int x, int y)
{
	//u32* dest = &((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	pix_t* dest = &((pix_t*)c->back_buffer.buf)[y*c->back_buffer.w + x];
	//*dest = (u32)color.a << PGL_ASHIFT | (u32)color.r << PGL_RSHIFT | (u32)color.g << PGL_GSHIFT | (u32)color.b << PGL_BSHIFT;
	*dest = RGBA_TO_PIXEL(color.r, color.g, color.b, color.a);
}

PGLDEF void put_pixel_blend(vec4 src, int x, int y)
{
	//u32* dest = &((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	u32* dest = &((u32*)c->back_buffer.buf)[y*c->back_buffer.w + x];

	//Color dest_color = make_Color((*dest & PGL_RMASK) >> PGL_RSHIFT, (*dest & PGL_GMASK) >> PGL_GSHIFT, (*dest & PGL_BMASK) >> PGL_BSHIFT, (*dest & PGL_AMASK) >> PGL_ASHIFT);
	Color dest_color = PIXEL_TO_COLOR(*dest);

	vec4 dst = Color_to_v4(dest_color);

	// standard alpha blending xyzw = rgba
	vec4 final;
	final.x = src.x * src.w + dst.x * (1.0f - src.w);
	final.y = src.y * src.w + dst.y * (1.0f - src.w);
	final.z = src.z * src.w + dst.z * (1.0f - src.w);
	final.w = src.w + dst.w * (1.0f - src.w);

	Color color = v4_to_Color(final);
	//*dest = (u32)color.a << PGL_ASHIFT | (u32)color.r << PGL_RSHIFT | (u32)color.g << PGL_GSHIFT | (u32)color.b << PGL_BSHIFT;
	*dest = RGBA_TO_PIXEL(color.r, color.g, color.b, color.a);
}

PGLDEF void put_wide_line_simple(Color the_color, float width, float x1, float y1, float x2, float y2)
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

	vec2 ab = make_v2(line.A, line.B);
	normalize_v2(&ab);

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

PGLDEF void put_wide_line(Color color1, Color color2, float width, float x1, float y1, float x2, float y2)
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

	vec4 c1 = Color_to_v4(color1);
	vec4 c2 = Color_to_v4(color2);

	// need half the width to calculate
	width /= 2.0f;

	float m = (y2-y1)/(x2-x1);
	Line line = make_Line(x1, y1, x2, y2);
	normalize_line(&line);
	vec2 c;

	vec2 ab = sub_v2s(b, a);
	vec2 ac;

	float dot_abab = dot_v2s(ab, ab);

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

	float x, y, e, dist, t;
	float w2 = width*width;
	//int last = 1;
	Color out_c;

	for (y = y_min; y <= y_max; ++y) {
		c.y = y;
		for (x = x_min; x <= x_max; x++) {
			// TODO optimize
			c.x = x;
			ac = sub_v2s(c, a);
			e = dot_v2s(ac, ab);
			
			// c lies past the ends of the segment ab
			if (e <= 0.0f || e >= dot_abab) {
				continue;
			}

			// can do this because we normalized the line equation
			// TODO square or fabsf?
			dist = line_func(&line, c.x, c.y);
			if (dist*dist < w2) {
				t = e / dot_abab;
				out_c = v4_to_Color(mixf_v4(c1, c2, t));
				put_pixel(out_c, x, y);
			}
		}
	}
}

//Should I have it take a glFramebuffer as paramater?
PGLDEF void put_line(Color the_color, float x1, float y1, float x2, float y2)
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

	x_min = floorf(x_min) + 0.5f;
	x_max = floorf(x_max) + 0.5f;
	y_min = floorf(y_min) + 0.5f;
	y_max = floorf(y_max) + 0.5f;

	//4 cases based on slope
	if (m <= -1) {           //(-infinite, -1]
		x = x_min;
		for (y=y_max; y>=y_min; --y) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+0.5f, y-1) < 0)
				x++;
		}
	} else if (m <= 0) {     //(-1, 0]
		y = y_max;
		for (x=x_min; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+1, y-0.5f) > 0)
				y--;
		}
	} else if (m <= 1) {     //(0, 1]
		y = y_min;
		for (x=x_min; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+1, y+0.5f) < 0)
				y++;
		}

	} else {                 //(1, +infinite)
		x = x_min;
		for (y=y_min; y<=y_max; ++y) {
			put_pixel(the_color, x, y);
			if (line_func(&line, x+0.5f, y+1) > 0)
				x++;
		}
	}
}


// can't think of a better/cleaner way to do this than these lines
#define CLIP_TRIANGLE() \
	do { \
	x_min = MIN(p1.x, p2.x); \
	x_max = MAX(p1.x, p2.x); \
	y_min = MIN(p1.y, p2.y); \
	y_max = MAX(p1.y, p2.y); \
 \
	x_min = MIN(p3.x, x_min); \
	x_max = MAX(p3.x, x_max); \
	y_min = MIN(p3.y, y_min); \
	y_max = MAX(p3.y, y_max); \
 \
	x_min = MAX(c->lx, x_min); \
	x_max = MIN(c->ux, x_max); \
	y_min = MAX(c->ly, y_min); \
	y_max = MIN(c->uy, y_max); \
	} while (0)

#define MAKE_IMPLICIT_LINES() \
	do { \
	l12 = make_Line(p1.x, p1.y, p2.x, p2.y); \
	l23 = make_Line(p2.x, p2.y, p3.x, p3.y); \
	l31 = make_Line(p3.x, p3.y, p1.x, p1.y); \
	} while (0)

#define ANY_COLORS_NOT_WHITE(c) \
	(c0.r != 255 || c1.r != 255 || c2.r != 255 || \
	c0.g != 255 || c1.g != 255 || c2.g != 255 || \
	c0.b != 255 || c1.b != 255 || c2.b != 255)

PGLDEF void put_triangle_uniform(vec4 color, vec2 p1, vec2 p2, vec2 p3)
{
	float x_min,x_max,y_min,y_max;
	Line l12, l23, l31;
	float alpha, beta, gamma;

	CLIP_TRIANGLE();
	MAKE_IMPLICIT_LINES();

	x_min = floorf(x_min) + 0.5f;
	y_min = floorf(y_min) + 0.5f;

	for (float y=y_min; y<y_max; ++y) {
		for (float x=x_min; x<x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
				    (beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					// blend
					put_pixel_blend(color, x, y);
					//put_pixel(color, x, y);
				}
			}
		}
	}
}

PGLDEF void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3)
{
	float x_min,x_max,y_min,y_max;
	Line l12, l23, l31;
	float alpha, beta, gamma;
	Color col;
	col.a = 255; // hmm

	CLIP_TRIANGLE();
	MAKE_IMPLICIT_LINES();

	x_min = floorf(x_min) + 0.5f;
	y_min = floorf(y_min) + 0.5f;

	for (float y=y_min; y<y_max; ++y) {
		for (float x=x_min; x<x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
				    (beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					//calculate interoplation here
					col.r = alpha*c1.r + beta*c2.r + gamma*c3.r;
					col.g = alpha*c1.g + beta*c2.g + gamma*c3.g;
					col.b = alpha*c1.b + beta*c2.b + gamma*c3.b;
					//col.a = alpha*c1.a + beta*c2.a + gamma*c3.a;
					//put_pixel_blend(c, x, y);
					put_pixel(col, x, y);
				}
			}
		}
	}
}

PGLDEF void put_triangle_tex(int tex, vec2 uv1, vec2 uv2, vec2 uv3, vec2 p1, vec2 p2, vec2 p3)
{
	float x_min,x_max,y_min,y_max;
	Line l12, l23, l31;
	float alpha, beta, gamma;

	CLIP_TRIANGLE();
	MAKE_IMPLICIT_LINES();

#if 0
	print_v2(p1, " p1\n");
	print_v2(p2, " p2\n");
	print_v2(p3, " p3\n");
	print_v2(uv1, " uv1\n");
	print_v2(uv2, " uv2\n");
	print_v2(uv3, " uv3\n");
#endif

	x_min = floorf(x_min) + 0.5f;
	y_min = floorf(y_min) + 0.5f;
	vec2 uv;

	for (float y=y_min; y<y_max; ++y) {
		for (float x=x_min; x<x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
				    (beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					//calculate interoplation here
					uv = add_v2s(scale_v2(uv1, alpha), scale_v2(uv2, beta));
					uv = add_v2s(uv, scale_v2(uv3, gamma));
					put_pixel_blend(texture2D(tex, uv.x, uv.y), x, y);
				}
			}
		}
	}
}

PGLDEF void put_triangle_tex_modulate(int tex, vec2 uv1, vec2 uv2, vec2 uv3, vec2 p1, vec2 p2, vec2 p3, Color c1, Color c2, Color c3)
{
	float x_min,x_max,y_min,y_max;
	Line l12, l23, l31;
	float alpha, beta, gamma;
	Color col;

	CLIP_TRIANGLE();
	MAKE_IMPLICIT_LINES();

#if 0
	print_v2(p1, " p1\n");
	print_v2(p2, " p2\n");
	print_v2(p3, " p3\n");
	print_v2(uv1, " uv1\n");
	print_v2(uv2, " uv2\n");
	print_v2(uv3, " uv3\n");
	print_Color(c1, " c1\n");
	print_Color(c2, " c2\n");
	print_Color(c3, " c3\n");
#endif

	x_min = floorf(x_min) + 0.5f;
	y_min = floorf(y_min) + 0.5f;
	vec2 uv;

	for (float y=y_min; y<y_max; ++y) {
		for (float x=x_min; x<x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
				    (beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					//calculate interoplation here
					uv = add_v2s(scale_v2(uv1, alpha), scale_v2(uv2, beta));
					uv = add_v2s(uv, scale_v2(uv3, gamma));

					col.r = alpha*c1.r + beta*c2.r + gamma*c3.r;
					col.g = alpha*c1.g + beta*c2.g + gamma*c3.g;
					col.b = alpha*c1.b + beta*c2.b + gamma*c3.b;
					col.a = alpha*c1.a + beta*c2.a + gamma*c3.a;
					vec4 cv = Color_to_v4(col);
					// texture2D without mip_uv_per_px setup → always LOD 0 (see pgl_draw_geometry_raw)
					vec4 texcolor = texture2D(tex, uv.x, uv.y);
					
					put_pixel_blend(mult_v4s(cv, texcolor), x, y);
				}
			}
		}
	}
}

#define COLOR_EQ(c1, c2) ((c1).r == (c2).r && (c1).g == (c2).g && (c1).b == (c2).b && (c1).a == (c2).a)


// TODO Color* or vec4*? float* for xy/uv or vec2*?
//
// SDL_RenderGeometryRaw-style immediate path: rasterizes triangles itself and
// multiplies vertex color by texture2D(tex, uv).
//
// No automatic mip LOD: unlike draw_triangle_fill, this never sets
// c->mip_uv_per_px, so texture2D always treats λ as magnification (level 0).
// *MIPMAP* min filters only change within-level NEAREST vs LINEAR on L0.
// Explicit LOD would require texture2DLod in a programmable FS (or changing
// this helper); we intentionally keep it simple like SDL's 2D geometry API.
PGLDEF void pgl_draw_geometry_raw(int tex, const float* xy, int xy_stride, const Color* color, int color_stride, const float* uv, int uv_stride, int n_verts, const void* indices, int n_indices, int sz_indices)
{
	int i,j;
	float* x;
	float* u;
	int count = indices ? n_indices : n_verts;

	// TODO make PGL_INVALID_VALUE et all?
	PGL_ERR(!xy, GL_INVALID_VALUE);

	// Matching SDL_RenderGeometryRaw but I feel like they should be able to pass
	// NULL and just use the texture
	PGL_ERR(!color, GL_INVALID_VALUE);


	PGL_ERR(count % 3, GL_INVALID_VALUE);
	PGL_ERR(!(sz_indices==1 || sz_indices==2 || sz_indices==4), GL_INVALID_VALUE);

	if (n_verts < 3) return;

	PGL_ASSERT((PGL_MAX_VERTICES * GL_MAX_VERTEX_OUTPUT_COMPONENTS * sizeof(float))/sizeof(pgl_copy_data) >= (size_t)count);
	// Allow default texture 0?  many implementations return black (0,0,0,1) when sampling
	// tex 0
	if (tex > 0) {
		PGL_ERR(!uv, GL_INVALID_VALUE);

		PGL_ERR((tex >= c->textures.size || c->textures.a[tex].deleted), GL_INVALID_VALUE);

		PGL_ERR(c->textures.a[tex].type != GL_TEXTURE_2D-(GL_TEXTURE_UNBOUND+1), GL_INVALID_OPERATION);

		pgl_copy_data* verts = (pgl_copy_data*)&c->vs_output.output_buf[0];
		for (i=0; i<count; ++i) {
			if (sz_indices == 1) j = ((GLubyte*)indices)[i];
			else if (sz_indices == 2) j = ((GLushort*)indices)[i];
			else if (sz_indices == 4) j = ((GLuint*)indices)[i];
			else j = i;

			x = (float*)((u8*)xy + j*xy_stride);
			u = (float*)((u8*)uv + j*uv_stride);

			verts[i].c = *(Color*)((u8*)color + j*color_stride);

			// TODO convert to ints here for efficiency or leave floats for
			// flexibility/subpixel accuracy?
			verts[i].src.x = u[0];
			verts[i].src.y = u[1];

			// scale?
			verts[i].dst.x = x[0];
			verts[i].dst.y = x[1];
		}

		vec4 tex_color;
		pgl_copy_data* p = verts;
		int is_uniform = GL_FALSE;
		int has_modulation = GL_FALSE;
		int tex_uniform = GL_FALSE;
		for (i=0; i<count; i+=3, p+=3) {
			is_uniform = (COLOR_EQ(p[0].c, p[1].c) && COLOR_EQ(p[1].c, p[2].c));
			if (is_uniform) {
				has_modulation = (p[0].c.r != 255 || p[0].c.g != 255 || p[0].c.b != 255 || p[0].c.a != 255);
			} else {
				has_modulation = GL_TRUE;
			}
			tex_uniform = (equal_v2s(p[0].src, p[1].src) && equal_v2s(p[1].src, p[2].src));
			if (tex_uniform) tex_color = texture2D(tex, p[0].src.x, p[0].src.y);

			if (has_modulation) {
				if (is_uniform) {
					if (tex_uniform) {
						// uniform color triangle, likely uniform color rect
						vec4 color = mult_v4s(tex_color, Color_to_v4(p[0].c));
						put_triangle_uniform(color, p[0].dst, p[1].dst, p[2].dst);
					} else {
						// need another variant that takes a single color so only
						// interpolates uv
						put_triangle_tex_modulate(tex, p[0].src, p[1].src, p[2].src, p[0].dst, p[1].dst, p[2].dst, p[0].c, p[1].c, p[2].c);
					}
				} else {
					put_triangle_tex_modulate(tex, p[0].src, p[1].src, p[2].src, p[0].dst, p[1].dst, p[2].dst, p[0].c, p[1].c, p[2].c);
				}
			} else {
				if (tex_uniform) {
					// uniform color triangle, likely uniform color rect
					put_triangle_uniform(tex_color, p[0].dst, p[1].dst, p[2].dst);
				} else {
					put_triangle_tex(tex, p[0].src, p[1].src, p[2].src, p[0].dst, p[1].dst, p[2].dst);
				}
			}
		}
	} else {
		pgl_fill_data* verts = (pgl_fill_data*)&c->vs_output.output_buf[0];
		for (i=0; i<count; ++i) {
			if (sz_indices == 1) j = ((GLubyte*)indices)[i];
			else if (sz_indices == 2) j = ((GLushort*)indices)[i];
			else if (sz_indices == 4) j = ((GLuint*)indices)[i];
			else j = i;

			x = (float*)((u8*)xy + j*xy_stride);
			verts[i].c = *(Color*)((u8*)color + j*color_stride);

			// scale?
			verts[i].dst.x = x[0];
			verts[i].dst.y = x[1];
		}

		pgl_fill_data* p = verts;
		for (i=0; i<count; i+=3, p+=3) {
			put_triangle(p[0].c, p[1].c, p[2].c, p[0].dst, p[1].dst, p[2].dst);
		}
	}
}



#define plot(X,Y,D) do{ c.w = (D); put_pixel_blend(c, X, Y); } while (0)

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
PGLDEF void put_aa_line(vec4 c, float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	if (fabs(dx) > fabs(dy)) {
		if (x2 < x1) {
			swap_(x1, x2);
			swap_(y1, y2);
		}
		float gradient = dy / dx;
		float xend = round_(x1);
		float yend = y1 + gradient*(xend - x1);
		float xgap = rfpart_(x1 + 0.5);
		int xpxl1 = xend;
		int ypxl1 = ipart_(yend);
		plot(xpxl1, ypxl1, rfpart_(yend)*xgap);
		plot(xpxl1, ypxl1+1, fpart_(yend)*xgap);
		printf("xgap = %f\n", xgap);
		printf("%f %f\n", rfpart_(yend), fpart_(yend));
		printf("%f %f\n", rfpart_(yend)*xgap, fpart_(yend)*xgap);
		float intery = yend + gradient;

		xend = round_(x2);
		yend = y2 + gradient*(xend - x2);
		xgap = fpart_(x2+0.5);
		int xpxl2 = xend;
		int ypxl2 = ipart_(yend);
		plot(xpxl2, ypxl2, rfpart_(yend) * xgap);
		plot(xpxl2, ypxl2 + 1, fpart_(yend) * xgap);

		int x;
		for(x=xpxl1+1; x < xpxl2; x++) {
			plot(x, ipart_(intery), rfpart_(intery));
			plot(x, ipart_(intery) + 1, fpart_(intery));
			intery += gradient;
		}
	} else {
		if ( y2 < y1 ) {
			swap_(x1, x2);
			swap_(y1, y2);
		}
		float gradient = dx / dy;
		float yend = round_(y1);
		float xend = x1 + gradient*(yend - y1);
		float ygap = rfpart_(y1 + 0.5);
		int ypxl1 = yend;
		int xpxl1 = ipart_(xend);
		plot(xpxl1, ypxl1, rfpart_(xend)*ygap);
		plot(xpxl1 + 1, ypxl1, fpart_(xend)*ygap);
		float interx = xend + gradient;

		yend = round_(y2);
		xend = x2 + gradient*(yend - y2);
		ygap = fpart_(y2+0.5);
		int ypxl2 = yend;
		int xpxl2 = ipart_(xend);
		plot(xpxl2, ypxl2, rfpart_(xend) * ygap);
		plot(xpxl2 + 1, ypxl2, fpart_(xend) * ygap);

		int y;
		for(y=ypxl1+1; y < ypxl2; y++) {
			plot(ipart_(interx), y, rfpart_(interx));
			plot(ipart_(interx) + 1, y, fpart_(interx));
			interx += gradient;
		}
	}
}


PGLDEF void put_aa_line_interp(vec4 c1, vec4 c2, float x1, float y1, float x2, float y2)
{
	vec4 c;
	float t;

	float dx = x2 - x1;
	float dy = y2 - y1;

	if (fabs(dx) > fabs(dy)) {
		if (x2 < x1) {
			swap_(x1, x2);
			swap_(y1, y2);
			swap_(c1, c2);
		}

		vec2 p1 = { x1, y1 }, p2 = { x2, y2 };
		vec2 pr, sub_p2p1 = sub_v2s(p2, p1);
		float line_length_squared = len_v2(sub_p2p1);
		line_length_squared *= line_length_squared;

		c = c1;

		float gradient = dy / dx;
		float xend = round_(x1);
		float yend = y1 + gradient*(xend - x1);
		float xgap = rfpart_(x1 + 0.5);
		int xpxl1 = xend;
		int ypxl1 = ipart_(yend);
		plot(xpxl1, ypxl1, rfpart_(yend)*xgap);
		plot(xpxl1, ypxl1+1, fpart_(yend)*xgap);
		printf("xgap = %f\n", xgap);
		printf("%f %f\n", rfpart_(yend), fpart_(yend));
		printf("%f %f\n", rfpart_(yend)*xgap, fpart_(yend)*xgap);
		float intery = yend + gradient;

		c = c2;
		xend = round_(x2);
		yend = y2 + gradient*(xend - x2);
		xgap = fpart_(x2+0.5);
		int xpxl2 = xend;
		int ypxl2 = ipart_(yend);
		plot(xpxl2, ypxl2, rfpart_(yend) * xgap);
		plot(xpxl2, ypxl2 + 1, fpart_(yend) * xgap);

		int x;
		for(x=xpxl1+1; x < xpxl2; x++) {
			pr.x = x;
			pr.y = intery;
			t = dot_v2s(sub_v2s(pr, p1), sub_p2p1) / line_length_squared;
			c = mixf_v4(c1, c2, t);

			plot(x, ipart_(intery), rfpart_(intery));
			plot(x, ipart_(intery) + 1, fpart_(intery));
			intery += gradient;
		}
	} else {
		if ( y2 < y1 ) {
			swap_(x1, x2);
			swap_(y1, y2);
			swap_(c1, c2);
		}

		vec2 p1 = { x1, y1 }, p2 = { x2, y2 };
		vec2 pr, sub_p2p1 = sub_v2s(p2, p1);
		float line_length_squared = len_v2(sub_p2p1);
		line_length_squared *= line_length_squared;

		c = c1;

		float gradient = dx / dy;
		float yend = round_(y1);
		float xend = x1 + gradient*(yend - y1);
		float ygap = rfpart_(y1 + 0.5);
		int ypxl1 = yend;
		int xpxl1 = ipart_(xend);
		plot(xpxl1, ypxl1, rfpart_(xend)*ygap);
		plot(xpxl1 + 1, ypxl1, fpart_(xend)*ygap);
		float interx = xend + gradient;


		c = c2;
		yend = round_(y2);
		xend = x2 + gradient*(yend - y2);
		ygap = fpart_(y2+0.5);
		int ypxl2 = yend;
		int xpxl2 = ipart_(xend);
		plot(xpxl2, ypxl2, rfpart_(xend) * ygap);
		plot(xpxl2 + 1, ypxl2, fpart_(xend) * ygap);

		int y;
		for(y=ypxl1+1; y < ypxl2; y++) {
			pr.x = interx;
			pr.y = y;
			t = dot_v2s(sub_v2s(pr, p1), sub_p2p1) / line_length_squared;
			c = mixf_v4(c1, c2, t);

			plot(ipart_(interx), y, rfpart_(interx));
			plot(ipart_(interx) + 1, y, fpart_(interx));
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



// Collection of standard shaders based on
// https://github.com/rswinkle/oglsuperbible5/blob/master/Src/GLTools/src/GLShaderManager.cpp
//
// Meant to ease the transition from old fixed function a little.  You might be able
// to get away without writing any new shaders, but you'll still need to use uniforms
// and enable attributes etc. things unless you write a full compatibility layer

// Identity Shader, no transformation, uniform color
static void pgl_identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	PGL_UNUSED(uniforms);
	builtins->gl_Position = vertex_attribs[PGL_ATTR_VERT];
}

static void pgl_identity_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	builtins->gl_FragColor = ((pgl_uniforms*)uniforms)->color;
}

// Flat Shader, Applies the uniform model view matrix transformation, uniform color
static void flat_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	builtins->gl_Position = mult_m4_v4(*((mat4*)uniforms), vertex_attribs[PGL_ATTR_VERT]);
}

// flat_fs is identical to pgl_identity_fs

// Shaded Shader, interpolates per vertex colors
static void pgl_shaded_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((vec4*)vs_output)[0] = vertex_attribs[PGL_ATTR_COLOR]; //color

	builtins->gl_Position = mult_m4_v4(*((mat4*)uniforms), vertex_attribs[PGL_ATTR_VERT]);
}

static void pgl_shaded_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(uniforms);
	builtins->gl_FragColor = ((vec4*)fs_input)[0];
}

// Default Light Shader
// simple diffuse directional light, vertex based shading
// uniforms:
// mat4 mvp_mat
// mat3 normal_mat
// vec4 color
//
// attributes:
// vec4 vertex
// vec3 normal
static void pgl_dflt_light_vs(float* vs_output, vec4* v_attrs, Shader_Builtins* builtins, void* uniforms)
{
	pgl_uniforms* u = (pgl_uniforms*)uniforms;

	vec3 norm = norm_v3(mult_m3_v3(u->normal_mat, *(vec3*)&v_attrs[PGL_ATTR_NORMAL]));

	vec3 light_dir = { 0.0f, 0.0f, 1.0f };
	float tmp = dot_v3s(norm, light_dir);
	float fdot = MAX(0.0f, tmp);

	vec4 c = u->color;

	// outgoing fragcolor to be interpolated
	((vec4*)vs_output)[0] = make_v4(c.x*fdot, c.y*fdot, c.z*fdot, c.w);

	builtins->gl_Position = mult_m4_v4(u->mvp_mat, v_attrs[PGL_ATTR_VERT]);
}

// default_light_fs is the same as pgl_shaded_fs

// Point Light Diff Shader
// point light, diffuse lighting only
// uniforms:
// mat4 mvp_mat
// mat4 mv_mat
// mat3 normal_mat
// vec4 color
// vec3 light_pos
//
// attributes:
// vec4 vertex
// vec3 normal
static void pgl_pnt_light_diff_vs(float* vs_output, vec4* v_attrs, Shader_Builtins* builtins, void* uniforms)
{
	pgl_uniforms* u = (pgl_uniforms*)uniforms;

	vec3 norm = norm_v3(mult_m3_v3(u->normal_mat, *(vec3*)&v_attrs[PGL_ATTR_NORMAL]));

	vec4 ec_pos = mult_m4_v4(u->mv_mat, v_attrs[PGL_ATTR_VERT]);
	vec3 ec_pos3 = v4_to_v3h(ec_pos);

	vec3 light_dir = norm_v3(sub_v3s(u->light_pos, ec_pos3));

	float tmp = dot_v3s(norm, light_dir);
	float fdot = MAX(0.0f, tmp);

	vec4 c = u->color;

	// outgoing fragcolor to be interpolated
	((vec4*)vs_output)[0] = make_v4(c.x*fdot, c.y*fdot, c.z*fdot, c.w);

	builtins->gl_Position = mult_m4_v4(u->mvp_mat, v_attrs[PGL_ATTR_VERT]);
}

// point_light_diff_fs is the same as pgl_shaded_fs


// Texture Replace Shader
// Just paste the texture on the triangles
// uniforms:
// mat4 mvp_mat
// GLuint tex0
//
// attributes:
// vec4 vertex
// vec2 texcoord0
static void pgl_tex_rplc_vs(float* vs_output, vec4* v_attrs, Shader_Builtins* builtins, void* uniforms)
{
	pgl_uniforms* u = (pgl_uniforms*)uniforms;

	((vec2*)vs_output)[0] = *(vec2*)&v_attrs[PGL_ATTR_TEXCOORD0]; //tex_coords

	builtins->gl_Position = mult_m4_v4(u->mvp_mat, v_attrs[PGL_ATTR_VERT]);

}

static void pgl_tex_rplc_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];
	GLuint tex = ((pgl_uniforms*)uniforms)->tex0;

	builtins->gl_FragColor = texture2D(tex, tex_coords.x, tex_coords.y);
}



// Texture Rect Replace Shader
// Just paste the texture on the triangles except using rect textures
// uniforms:
// mat4 mvp_mat
// GLuint tex0
//
// attributes:
// vec4 vertex
// vec2 texcoord0

// texture_rect_rplc_vs is the same as pgl_tex_rplc_vs
static void pgl_tex_rect_rplc_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec2 tex_coords = ((vec2*)fs_input)[0];
	GLuint tex = ((pgl_uniforms*)uniforms)->tex0;

	builtins->gl_FragColor = texture_rect(tex, tex_coords.x, tex_coords.y);
}


// Texture Modulate Shader
// Paste texture on triangles but multiplied by a uniform color
// uniforms:
// mat4 mvp_mat
// GLuint tex0
//
// attributes:
// vec4 vertex
// vec2 texcoord0

// texture_modulate_vs is the same as pgl_tex_rplc_vs

static void pgl_tex_modulate_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_uniforms* u = (pgl_uniforms*)uniforms;

	vec2 tex_coords = ((vec2*)fs_input)[0];

	GLuint tex = u->tex0;

	builtins->gl_FragColor = mult_v4s(u->color, texture2D(tex, tex_coords.x, tex_coords.y));
}


// Texture Point Light Diff
// point light, diffuse only with texture
// uniforms:
// mat4 mvp_mat
// mat4 mv_mat
// mat3 normal_mat
// vec4 color
// vec3 light_pos
//
// attributes:
// vec4 vertex
// vec3 normal
static void pgl_tex_pnt_light_diff_vs(float* vs_output, vec4* v_attrs, Shader_Builtins* builtins, void* uniforms)
{
	pgl_uniforms* u = (pgl_uniforms*)uniforms;

	vec3 norm = norm_v3(mult_m3_v3(u->normal_mat, *(vec3*)&v_attrs[PGL_ATTR_NORMAL]));

	vec4 ec_pos = mult_m4_v4(u->mv_mat, v_attrs[PGL_ATTR_VERT]);
	vec3 ec_pos3 = v4_to_v3h(ec_pos);

	vec3 light_dir = norm_v3(sub_v3s(u->light_pos, ec_pos3));

	float tmp = dot_v3s(norm, light_dir);
	float fdot = MAX(0.0f, tmp);

	vec4 c = u->color;

	// outgoing fragcolor to be interpolated
	((vec4*)vs_output)[0] = make_v4(c.x*fdot, c.y*fdot, c.z*fdot, c.w);
	// fragcolor takes up 4 floats, ie 2*sizeof(vec2)
	((vec2*)vs_output)[2] =  *(vec2*)&v_attrs[PGL_ATTR_TEXCOORD0];

	builtins->gl_Position = mult_m4_v4(u->mvp_mat, v_attrs[PGL_ATTR_VERT]);
}


static void pgl_tex_pnt_light_diff_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_uniforms* u = (pgl_uniforms*)uniforms;

	vec2 tex_coords = ((vec2*)fs_input)[2];

	GLuint tex = u->tex0;

	builtins->gl_FragColor = mult_v4s(((vec4*)fs_input)[0], texture2D(tex, tex_coords.x, tex_coords.y));
}


PGLDEF void pgl_init_std_shaders(GLuint programs[PGL_NUM_SHADERS])
{
	pgl_prog_info std_shaders[PGL_NUM_SHADERS] =
	{
		{ pgl_identity_vs, pgl_identity_fs, 0, {0}, GL_FALSE },
		{ flat_vs, pgl_identity_fs, 0, {0}, GL_FALSE },
		{ pgl_shaded_vs, pgl_shaded_fs, 4, {PGL_SMOOTH4}, GL_FALSE },
		{ pgl_dflt_light_vs, pgl_shaded_fs, 4, {PGL_SMOOTH4}, GL_FALSE },
		{ pgl_pnt_light_diff_vs, pgl_shaded_fs, 4, {PGL_SMOOTH4}, GL_FALSE },
		{ pgl_tex_rplc_vs, pgl_tex_rplc_fs, 2, {PGL_SMOOTH2}, GL_FALSE },
		{ pgl_tex_rplc_vs, pgl_tex_modulate_fs, 2, {PGL_SMOOTH2}, GL_FALSE },
		{ pgl_tex_pnt_light_diff_vs, pgl_tex_pnt_light_diff_fs, 6, {PGL_SMOOTH4, PGL_SMOOTH2}, GL_FALSE },


		{ pgl_tex_rplc_vs, pgl_tex_rect_rplc_fs, 2, {PGL_SMOOTH2}, GL_FALSE }
	};

	for (int i=0; i<PGL_NUM_SHADERS; i++) {
		pgl_prog_info* p = &std_shaders[i];
		programs[i] = pglCreateProgram(p->vs, p->fs, p->vs_out_sz, p->interp, p->uses_fragdepth_or_discard);
	}
}




#undef PORTABLEGL_IMPLEMENTATION
#endif

#ifdef PGL_PREFIX_TYPES
#undef vec2
#undef vec3
#undef vec4
#undef ivec2
#undef ivec3
#undef ivec4
#undef uvec2
#undef uvec3
#undef uvec4
#undef bvec2
#undef bvec3
#undef bvec4
#undef mat2
#undef mat3
#undef mat4
#undef Color
#undef Line
#undef Plane
#endif

#if defined(PGL_PREFIX_GLSL) || defined(PGL_SUFFIX_GLSL)
#undef smoothstep
#undef clamp_01
#undef clamp_01_v4
#undef clamp
#undef clampi
#endif

