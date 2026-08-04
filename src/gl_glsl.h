


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
