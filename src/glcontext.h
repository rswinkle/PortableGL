
typedef struct glContext
{
	mat4 vp_mat;

	int x_min, y_min;
	size_t x_max, y_max;

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

	void* uniform;

	vec4 vertex_attribs_vs[GL_MAX_VERTEX_ATTRIBS];
	Shader_Builtins builtins;
	Vertex_Shader_output vs_output;
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	unsigned int provoking_vert;
	GLboolean depth_test;
	GLboolean line_smooth;
	GLboolean cull_face;
	GLboolean fragdepth_or_discard;
	GLboolean depth_clamp;
	GLboolean depth_mask;
	GLboolean blend;
	GLboolean logic_ops;
	GLboolean poly_offset;
	GLboolean scissor_test;
	GLboolean stencil_test;
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

	// I really need to decide whether to use GLtypes or plain C types
	GLfloat poly_factor;
	GLfloat poly_units;

	GLint scissor_lx;
	GLint scissor_ly;
	GLsizei scissor_ux;
	GLsizei scissor_uy;

	GLint unpack_alignment;
	GLint pack_alignment;

	Color clear_color;
	vec4 blend_color;
	float point_size;
	float clear_depth;
	float depth_range_near;
	float depth_range_far;

	draw_triangle_func draw_triangle_front;
	draw_triangle_func draw_triangle_back;

	glFramebuffer zbuf;
	glFramebuffer back_buffer;

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

