
// Collection of standard shaders based on
// https://github.com/rswinkle/oglsuperbible5/blob/master/Src/GLTools/src/GLShaderManager.cpp
//
// Meant to ease the transition from old fixed function a little.  You might be able
// to get away without writing any new shaders, but you'll still need to use uniforms
// and enable attributes etc. things unless you write a full compatibility layer

// Identity Shader, no transformation, uniform color
static void pgl_identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = vertex_attribs[PGL_ATTR_VERT];
}

static void pgl_identity_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_FragColor = ((pgl_uniforms*)uniforms)->color;
}

// Flat Shader, Applies the uniform model view matrix transformation, uniform color
static void flat_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), vertex_attribs[PGL_ATTR_VERT]);
}

// flat_fs is identical to pgl_identity_fs

// Shaded Shader, interpolates per vertex colors
static void pgl_shaded_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	((vec4*)vs_output)[0] = vertex_attribs[PGL_ATTR_COLOR]; //color

	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), vertex_attribs[PGL_ATTR_VERT]);
}

static void pgl_shaded_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
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

	vec3 norm = norm_vec3(mult_mat3_vec3(u->normal_mat, *(vec3*)&v_attrs[PGL_ATTR_NORMAL]));

	vec3 light_dir = { 0.0f, 0.0f, 1.0f };
	float tmp = dot_vec3s(norm, light_dir);
	float fdot = MAX(0.0f, tmp);

	vec4 c = u->color;

	// outgoing fragcolor to be interpolated
	((vec4*)vs_output)[0] = make_vec4(c.x*fdot, c.y*fdot, c.z*fdot, c.w);

	builtins->gl_Position = mult_mat4_vec4(u->mvp_mat, v_attrs[PGL_ATTR_VERT]);
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

	vec3 norm = norm_vec3(mult_mat3_vec3(u->normal_mat, *(vec3*)&v_attrs[PGL_ATTR_NORMAL]));

	vec4 ec_pos = mult_mat4_vec4(u->mv_mat, v_attrs[PGL_ATTR_VERT]);
	vec3 ec_pos3 = vec4_to_vec3h(ec_pos);

	vec3 light_dir = norm_vec3(sub_vec3s(u->light_pos, ec_pos3));

	float tmp = dot_vec3s(norm, light_dir);
	float fdot = MAX(0.0f, tmp);

	vec4 c = u->color;

	// outgoing fragcolor to be interpolated
	((vec4*)vs_output)[0] = make_vec4(c.x*fdot, c.y*fdot, c.z*fdot, c.w);

	builtins->gl_Position = mult_mat4_vec4(u->mvp_mat, v_attrs[PGL_ATTR_VERT]);
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

	builtins->gl_Position = mult_mat4_vec4(u->mvp_mat, v_attrs[PGL_ATTR_VERT]);

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

	builtins->gl_FragColor = mult_vec4s(u->color, texture2D(tex, tex_coords.x, tex_coords.y));
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

	vec3 norm = norm_vec3(mult_mat3_vec3(u->normal_mat, *(vec3*)&v_attrs[PGL_ATTR_NORMAL]));

	vec4 ec_pos = mult_mat4_vec4(u->mv_mat, v_attrs[PGL_ATTR_VERT]);
	vec3 ec_pos3 = vec4_to_vec3h(ec_pos);

	vec3 light_dir = norm_vec3(sub_vec3s(u->light_pos, ec_pos3));

	float tmp = dot_vec3s(norm, light_dir);
	float fdot = MAX(0.0f, tmp);

	vec4 c = u->color;

	// outgoing fragcolor to be interpolated
	((vec4*)vs_output)[0] = make_vec4(c.x*fdot, c.y*fdot, c.z*fdot, c.w);
	// fragcolor takes up 4 floats, ie 2*sizeof(vec2)
	((vec2*)vs_output)[2] =  *(vec2*)&v_attrs[PGL_ATTR_TEXCOORD0];

	builtins->gl_Position = mult_mat4_vec4(u->mvp_mat, v_attrs[PGL_ATTR_VERT]);
}


static void pgl_tex_pnt_light_diff_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	pgl_uniforms* u = (pgl_uniforms*)uniforms;

	vec2 tex_coords = ((vec2*)fs_input)[2];

	GLuint tex = u->tex0;

	builtins->gl_FragColor = mult_vec4s(((vec4*)fs_input)[0], texture2D(tex, tex_coords.x, tex_coords.y));
}


void pgl_init_std_shaders(GLuint programs[PGL_NUM_SHADERS])
{
	pgl_prog_info std_shaders[PGL_NUM_SHADERS] =
	{
		{ pgl_identity_vs, pgl_identity_fs, 0, {0}, GL_FALSE },
		{ flat_vs, pgl_identity_fs, 0, {0}, GL_FALSE },
		{ pgl_shaded_vs, pgl_shaded_fs, 4, {SMOOTH, SMOOTH, SMOOTH, SMOOTH}, GL_FALSE },
		{ pgl_dflt_light_vs, pgl_shaded_fs, 4, {SMOOTH, SMOOTH, SMOOTH, SMOOTH}, GL_FALSE },
		{ pgl_pnt_light_diff_vs, pgl_shaded_fs, 4, {SMOOTH, SMOOTH, SMOOTH, SMOOTH}, GL_FALSE },
		{ pgl_tex_rplc_vs, pgl_tex_rplc_fs, 2, {SMOOTH, SMOOTH}, GL_FALSE },
		{ pgl_tex_rplc_vs, pgl_tex_modulate_fs, 2, {SMOOTH, SMOOTH}, GL_FALSE },
		{ pgl_tex_pnt_light_diff_vs, pgl_tex_pnt_light_diff_fs, 6, {SMOOTH, SMOOTH, SMOOTH, SMOOTH, SMOOTH, SMOOTH}, GL_FALSE },


		{ pgl_tex_rplc_vs, pgl_tex_rect_rplc_fs, 2, {SMOOTH, SMOOTH}, GL_FALSE }
	};

	for (int i=0; i<PGL_NUM_SHADERS; i++) {
		pgl_prog_info* p = &std_shaders[i];
		programs[i] = pglCreateProgram(p->vs, p->fs, p->vs_out_sz, p->interp, p->uses_fragdepth_or_discard);
	}
}




