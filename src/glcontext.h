
typedef struct glContext
{
	mat4 vp_mat;

	// Viewport rectangle. Filled triangles intersect their bbox with this
	// as well as with lx/ux/ly/uy. Lines and points still use lx/uy only.
	GLint xmin, ymin;
	GLsizei width, height;

	// Clip-space guard, from the viewport matrix. Not read by the clipper yet.
	float guard_ndc_left, guard_ndc_right, guard_ndc_bottom, guard_ndc_top;

	// Raster clip rect: the framebuffer, intersected with the scissor when
	// the scissor test is on. Not the viewport.
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
	GLboolean dbg_output_sync; // GL_DEBUG_OUTPUT_SYNCHRONOUS; callbacks are always sync

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
	GLboolean blend[GL_MAX_DRAW_BUFFERS];
	GLboolean logic_ops;
	GLboolean poly_offset_pt;
	GLboolean poly_offset_line;
	GLboolean poly_offset_fill;
	GLboolean scissor_test;
	GLboolean cube_map_seamless; // GL_TEXTURE_CUBE_MAP_SEAMLESS; LINEAR cube filter only

#ifndef PGL_DISABLE_COLOR_MASK
	GLboolean color_writemask[GL_MAX_DRAW_BUFFERS][4];
	pix_t color_mask_pix[GL_MAX_DRAW_BUFFERS];
	u32 color_mask_u8[GL_MAX_DRAW_BUFFERS];
#endif

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
	GLenum blend_sRGB[GL_MAX_DRAW_BUFFERS];
	GLenum blend_sA[GL_MAX_DRAW_BUFFERS];
	GLenum blend_dRGB[GL_MAX_DRAW_BUFFERS];
	GLenum blend_dA[GL_MAX_DRAW_BUFFERS];
	GLenum blend_eqRGB[GL_MAX_DRAW_BUFFERS];
	GLenum blend_eqA[GL_MAX_DRAW_BUFFERS];
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
	// When bound_draw_framebuffer != 0, back_buffer/zbuf may point at attachments;
	// window_* hold the default surfaces to restore on bind 0.
	cvector_glFBO framebuffers;
	GLuint bound_draw_framebuffer;
	GLuint bound_read_framebuffer;
	GLboolean fbo_redirected;
	glFramebuffer window_back_buffer;
#ifndef PGL_NO_DEPTH_NO_STENCIL
	glFramebuffer window_zbuf;
#  if defined(PGL_D16) && !defined(PGL_NO_STENCIL)
	glFramebuffer window_stencil_buf;
#  endif
	// When bound FBO depth is float depth texture, depth test uses float compares.
	GLboolean zbuf_float;
	// Spec: depth/stencil tests are implicitly disabled if that buffer is absent.
	GLboolean has_depth_buf;
	GLboolean has_stencil_buf;
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

	// One chunk of pgl_tri records. Points store a glverts index per survivor.
	// Lines store pgl_line (two indices plus provoke) in the same bytes.
	u8* prim_buf;
	pgl_clip_arena clip_arena;
} glContext;

