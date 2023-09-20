
typedef struct pgl_uniforms
{
	mat4 mvp_mat;
	mat4 mv_mat;
	mat4 p_mat;
	mat3 normal_mat;
	vec4 v_color;

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

void pgl_init_std_shaders(GLuint programs[PGL_NUM_SHADERS]);
