
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

	GLuint cur_vertex_array;
	GLuint bound_buffers[GL_NUM_BUFFER_TYPES-GL_ARRAY_BUFFER];
	GLuint bound_textures[GL_NUM_TEXTURE_TYPES-GL_TEXTURE_UNBOUND-1];
	GLuint cur_texture2D;
	GLuint cur_program;

	GLenum error;

	// TODO make some or all of these locals, measure performance
	// impact. Would be necessary in the long term if I ever
	// parallelize more
	vec4 vertex_attribs_vs[GL_MAX_VERTEX_ATTRIBS];
	Shader_Builtins builtins;
	Vertex_Shader_output vs_output;
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

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

	// stencil test requires a lot of state, especially for
	// something that I think will rarely be used... is it even worth having?
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

	GLenum logic_func;
	GLenum blend_sfactor;
	GLenum blend_dfactor;
	GLenum blend_equation;
	GLenum cull_mode;
	GLenum front_face;
	GLenum poly_mode_front;
	GLenum poly_mode_back;
	GLenum depth_func;
	GLenum point_spr_origin;
	GLenum provoking_vert;

	// I really need to decide whether to use GLtypes or plain C types
	GLfloat poly_factor;
	GLfloat poly_units;

	GLint scissor_lx;
	GLint scissor_ly;
	GLsizei scissor_w;
	GLsizei scissor_h;

	GLint unpack_alignment;
	GLint pack_alignment;

	GLint clear_stencil;
	Color clear_color;
	vec4 blend_color;
	GLfloat point_size;
	GLfloat line_width;
	GLfloat clear_depth;
	GLfloat depth_range_near;
	GLfloat depth_range_far;

	draw_triangle_func draw_triangle_front;
	draw_triangle_func draw_triangle_back;

	glFramebuffer zbuf;
	glFramebuffer back_buffer;
	glFramebuffer stencil_buf;

	int user_alloced_backbuf;
	int bitdepth;
	u32 Rmask;
	u32 Gmask;
	u32 Bmask;
	u32 Amask;
	int Rshift;
	int Gshift;
	int Bshift;
	int Ashift;
	


	cvector_glVertex glverts;
} glContext;

