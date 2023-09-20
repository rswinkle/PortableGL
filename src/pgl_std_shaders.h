
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

enum {
	PGL_ATTR_VERT,
	PGL_ATTR_COLOR,
	PGL_ATTR_NORMAL,
	PGL_ATTR_TEXCOORD0,
	PGL_ATTR_TEXCOORD1
};
