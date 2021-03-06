
#include "gl.h"

#include "crsw_math.c"

#include <stdarg.h>





/******** GL type vectors ************/

#define RESIZE(a) ((a)*2)

CVEC_NEW_DEFS(glVertex_Array, RESIZE)
CVEC_NEW_DEFS(glBuffer, RESIZE)
CVEC_NEW_DEFS(glTexture, RESIZE)
CVEC_NEW_DEFS(glProgram, RESIZE)
CVEC_NEW_DEFS(glVertex, RESIZE)


/******************************************
 * GL_IMPLEMENTATION
 ******************************************/

#include <stdio.h>
#include <assert.h>
#include <float.h>


static glContext* c;

#ifdef DEBUG
#define IS_VALID(target, error, ...) is_valid(target, error, __VA_ARGS__)
#else
#define IS_VALID(target, error, ...) 1
#endif

int is_valid(GLenum target, GLenum error, int n, ...)
{
	va_list argptr;

	va_start(argptr, n);
	for (int i=0; i<n; ++i) {
		if (target == va_arg(argptr, GLenum)) {
			return 1;
		}
	}
	va_end(argptr);

	if (!c->error) {
		c->error = error;
	}
	return 0;
}





static Color blend_pixel(vec4 src, vec4 dst);
static void draw_pixel_vec2(vec4 cf, vec2 pos);
static void draw_pixel(vec4 cf, int x, int y);
static void run_pipeline(GLenum mode, GLint first, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements);

static void draw_triangle_clip(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke, int clip_bit);
static void draw_triangle_point(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_line(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_fill(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke);
static void draw_triangle_final(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);
static void draw_triangle(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);

static void draw_line_clip(glVertex* v1, glVertex* v2);
static void draw_line_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke);
static void draw_line_smooth_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke);



/* example pass through shaders  */
void default_vp(float* vs_output, void* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	builtins->gl_Position = mult_mat4_vec4(*((mat4*)uniforms), ((vec4*)vertex_attribs)[0]);
}

//void (*fragment_shader)(vec4* vs_input, Shader_Builtins* builtins, vec4* fragcolor, void* uniforms);
void default_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec4* fragcolor = &builtins->gl_FragColor;
	//wish I could use a compound literal, stupid C++ compatibility
	fragcolor->x = 1.0f;
	fragcolor->y = 0.0f;
	fragcolor->z = 0.0f;
	fragcolor->w = 1.0f;
}


void init_glVertex_Array(glVertex_Array* v)
{
	v->deleted = GL_FALSE;
	for (int i=0; i<GL_MAX_VERTEX_ATTRIBS; ++i)
		init_glVertex_Attrib(&v->vertex_attribs[i]);
}

void init_glVertex_Attrib(glVertex_Attrib* v)
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


#define GET_SHIFT(mask, shift) \
	do {\
	shift = 0;\
	while ((mask & 1) == 0) {\
		mask >>= 1;\
		++shift;\
	}\
	} while (0)


int init_glContext(glContext* context, u32* back, int w, int h, int bitdepth, u32 Rmask, u32 Gmask, u32 Bmask, u32 Amask)
{
	context->zbuf.buf = (u8*) malloc(w*h * sizeof(float));
	if (!context->zbuf.buf)
		return 0;

	if (!back) {
		back = (u32*) malloc(w * h * sizeof(u32));
		if (!back)
			return 0;
	}
	context->x_min = 0;
	context->y_min = 0;
	context->x_max = w;
	context->y_max = h;

	context->zbuf.w = w;
	context->zbuf.h = h;
	context->zbuf.lastrow = context->zbuf.buf + (h-1)*w*sizeof(float);

	context->back_buffer.w = w;
	context->back_buffer.h = h;
	context->back_buffer.buf = (u8*) back;
	context->back_buffer.lastrow = context->back_buffer.buf + (h-1)*w*sizeof(u32);

	context->bitdepth = bitdepth; //not used yet
	context->Rmask = Rmask;
	context->Gmask = Gmask;
	context->Bmask = Bmask;
	context->Amask = Amask;
	GET_SHIFT(Rmask, context->Rshift);
	GET_SHIFT(Gmask, context->Gshift);
	GET_SHIFT(Bmask, context->Bshift);
	GET_SHIFT(Amask, context->Ashift);

	//initialize all vectors
	cvec_glVertex_Array(&context->vertex_arrays, 0, 3);
	cvec_glBuffer(&context->buffers, 0, 3);
	cvec_glProgram(&context->programs, 0, 3);
	cvec_glTexture(&context->textures, 0, 1);
	cvec_glVertex(&context->glverts, 0, 10);

	//might as well just set it to MAX_VERTICES * MAX_OUTPUT_COMPONENTS
	cvec_float(&context->vs_output.output_buf, 0, 0);


	context->clear_color = make_Color(0, 0, 0, 0);
	SET_VEC4(context->blend_color, 0, 0, 0, 0);
	context->point_size = 1.0f;
	context->clear_depth = 1.0f;
	context->depth_range_near = 0.0f;
	context->depth_range_far = 1.0f;
	make_viewport_matrix(context->vp_mat, 0, 0, w, h, 1);


	//set flags
	//TODO match order in structure definition
	context->provoking_vert = GL_LAST_VERTEX_CONVENTION;
	context->cull_mode = GL_BACK;
	context->cull_face = GL_FALSE;
	context->front_face = GL_CCW;
	context->depth_test = GL_FALSE;
	context->frag_depth_used = GL_FALSE;
	context->depth_clamp = GL_FALSE;
	context->blend = GL_FALSE;
	context->blend_sfactor = GL_ONE;
	context->blend_dfactor = GL_ZERO;
	context->blend_equation = GL_FUNC_ADD;
	context->depth_func = GL_LESS;
	context->line_smooth = GL_FALSE;
	context->poly_mode_front = GL_FILL;
	context->poly_mode_back = GL_FILL;
	context->point_spr_origin = GL_UPPER_LEFT;

	context->draw_triangle_front = draw_triangle_fill;
	context->draw_triangle_back = draw_triangle_fill;

	context->error = GL_NO_ERROR;

	//program 0 is supposed to be undefined but not invalid so I'll
	//just make it default
	glProgram tmp_prog = { default_vp, default_fs, NULL, GL_FALSE };
	cvec_push_glProgram(&context->programs, tmp_prog);
	context->cur_program = 0;

	//setup default vertex_array (vao) at position 0
	//we're like a compatibility profile for this but come on
	//no reason not to have this imo
	//https://www.opengl.org/wiki/Vertex_Specification#Vertex_Array_Object
	glVertex_Array tmp_va;
	init_glVertex_Array(&tmp_va);
	cvec_push_glVertex_Array(&context->vertex_arrays, tmp_va);
	context->cur_vertex_array = 0; 

	//setup buffers and textures
	//need to push back once since 0 is invalid
	//valid buffers have to start at position 1
	glBuffer tmp_buf;
	tmp_buf.mapped = GL_TRUE;
	tmp_buf.deleted = GL_FALSE;

	glTexture tmp_tex;
	tmp_tex.mapped = GL_TRUE;
	tmp_tex.deleted = GL_FALSE;
	tmp_tex.type = GL_TEXTURE_UNBOUND;
	tmp_tex.data = NULL;
	tmp_tex.w = 0;
	tmp_tex.h = 0;
	tmp_tex.d = 0;
	cvec_push_glBuffer(&context->buffers, tmp_buf);
	cvec_push_glTexture(&context->textures, tmp_tex);

	return 0;
}

void free_glContext(glContext* context)
{
	int i;

	free(context->zbuf.buf);

	for (i=0; i<context->buffers.size; ++i) {
		if (!context->buffers.a[i].mapped) {
			printf("freeing buffer %d\n", i);
			free(context->buffers.a[i].data);
		}
	}

	for (i=0; i<context->textures.size; ++i) {
		if (!context->textures.a[i].mapped) {
			printf("freeing texture %d\n", i);
			free(context->textures.a[i].data);
		}
	}

	//free vectors
	cvec_free_glVertex_Array(&context->vertex_arrays);
	cvec_free_glBuffer(&context->buffers);
	cvec_free_glProgram(&context->programs);
	cvec_free_glTexture(&context->textures);
	cvec_free_glVertex(&context->glverts);

	cvec_free_float(&context->vs_output.output_buf);
}

void set_glContext(glContext* context)
{
	c = context;
}

void resize_framebuffer(size_t w, size_t h)
{
	u8* tmp;
	tmp = (u8*) realloc(c->zbuf.buf, w*h * sizeof(float));
	if (!tmp) {
		if (c->error == GL_NO_ERROR)
			c->error = GL_OUT_OF_MEMORY;
		return;
	}
	c->zbuf.buf = tmp;
	c->zbuf.w = w;
	c->zbuf.h = h;
	c->zbuf.lastrow = c->zbuf.buf + (h-1)*w*sizeof(float);

	tmp = (u8*) realloc(c->back_buffer.buf, w*h * sizeof(u32));
	if (!tmp) {
		if (c->error == GL_NO_ERROR)
			c->error = GL_OUT_OF_MEMORY;
		return;
	}
	c->back_buffer.buf = tmp;
	c->back_buffer.w = w;
	c->back_buffer.h = h;
	c->back_buffer.lastrow = c->back_buffer.buf + (h-1)*w*sizeof(u32);
}



GLubyte* glGetString(GLenum name)
{
	static GLubyte vendor[] = "Robert Winkler";
	static GLubyte renderer[] = "PortableGL";
	static GLubyte version[] = "OpenGL 3.x-ish PortableGL 0.5";
	static GLubyte shading_language[] = "C/C++";

	switch (name) {
	case GL_VENDOR:                   return vendor;
	case GL_RENDERER:                 return renderer;
	case GL_VERSION:                  return version;
	case GL_SHADING_LANGUAGE_VERSION: return shading_language;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return 0;
	}
}

GLenum glGetError()
{
	GLenum err = c->error;
	c->error = GL_NO_ERROR;
	return err;
}


void glGenVertexArrays(GLsizei n, GLuint* arrays)
{
	glVertex_Array tmp;
	init_glVertex_Array(&tmp);

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

void glDeleteVertexArrays(GLsizei n, const GLuint* arrays)
{
	for (int i=0; i<n; ++i) {
		if (!arrays[i] || arrays[i] >= c->vertex_arrays.size)
			continue;

		if (arrays[i] == c->cur_vertex_array) {
			//TODO check if memcpy isn't enough
			memcpy(&c->vertex_arrays.a[0], &c->vertex_arrays.a[arrays[i]], sizeof(glVertex_Array));
			c->cur_vertex_array = 0;
		}

		c->vertex_arrays.a[arrays[i]].deleted = GL_TRUE;
	}
}

void glGenBuffers(GLsizei n, GLuint* buffers)
{
	glBuffer tmp;
	tmp.mapped = GL_TRUE;  //TODO not really but I use this to know whether to free or not
	tmp.data = NULL;
	tmp.deleted = GL_FALSE;

	//fill up empty slots first
	--n;
	for (int i=1; i<c->buffers.size && n>=0; ++i) {
		if (c->buffers.a[i].deleted) {
			c->buffers.a[i] = tmp;
			buffers[n--] = i;
		}
	}

	for (; n>=0; --n) {
		cvec_push_glBuffer(&c->buffers, tmp);
		buffers[n] = c->buffers.size-1;
	}
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	GLenum type;
	for (int i=0; i<n; ++i) {
		if (!buffers[i] || buffers[i] >= c->buffers.size)
			continue;

		// NOTE(rswinkse): type is stored as correct index not the raw enum value so no need to
		// subtract here see glBindBuffer
		type = c->buffers.a[buffers[i]].type;
		if (buffers[i] == c->bound_buffers[type])
			c->bound_buffers[type] = 0;

		if (!c->buffers.a[buffers[i]].mapped) {
			free(c->buffers.a[buffers[i]].data);
			c->buffers.a[buffers[i]].data = NULL;
		}

		c->buffers.a[buffers[i]].deleted = GL_TRUE;
	}
}

void glGenTextures(GLsizei n, GLuint* textures)
{
	glTexture tmp;
	//SET_VEC4(tmp.border_color, 0, 0, 0, 0);
	tmp.mag_filter = GL_LINEAR;
	tmp.min_filter = GL_LINEAR; //NOTE: spec says should be mipmap_linear
	tmp.wrap_s = GL_REPEAT;
	tmp.wrap_t = GL_REPEAT;
	tmp.data = NULL;
	tmp.deleted = GL_FALSE;
	tmp.mapped = GL_TRUE;
	tmp.type = GL_TEXTURE_UNBOUND;
	tmp.w = 0;
	tmp.h = 0;
	tmp.d = 0;

	--n;
	for (int i=0; i<c->textures.size && n>=0; ++i) {
		if (c->textures.a[i].deleted) {
			c->textures.a[i] = tmp;
			textures[n--] = i;
		}
	}
	for (; n>=0; --n) {
		cvec_push_glTexture(&c->textures, tmp);
		textures[n] = c->textures.size-1;
	}
}

void glDeleteTextures(GLsizei n, GLuint* textures)
{
	GLenum type;
	for (int i=0; i<n; ++i) {
		if (!textures[i] || textures[i] >= c->textures.size)
			continue;

		// NOTE(rswinkle): type is stored as correct index not the raw enum value so no need to
		// subtract here see glBindTexture
		type = c->textures.a[textures[i]].type;
		if (textures[i] == c->bound_textures[type])
			c->bound_textures[type] = 0;

		if (!c->textures.a[textures[i]].mapped) {
			free(c->textures.a[textures[i]].data);
			c->textures.a[textures[i]].data = NULL;
		}

		c->textures.a[textures[i]].deleted = GL_TRUE;
	}
}

void glBindVertexArray(GLuint array)
{
	if (array < c->vertex_arrays.size && c->vertex_arrays.a[array].deleted == GL_FALSE) {
		c->cur_vertex_array = array;
	} else if (!c->error) {
		c->error = GL_INVALID_OPERATION;
	}
}

void glBindBuffer(GLenum target, GLuint buffer)
{
//GL_ARRAY_BUFFER, GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, GL_ELEMENT_ARRAY_BUFFER,
//GL_PIXEL_PACK_BUFFER, GL_PIXEL_UNPACK_BUFFER, GL_TEXTURE_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER, or GL_UNIFORM_BUFFER.
	if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	target -= GL_ARRAY_BUFFER;
	if (buffer < c->buffers.size && c->buffers.a[buffer].deleted == GL_FALSE) {
		c->bound_buffers[target] = buffer;
	} else if (!c->error) {
		c->error = GL_INVALID_OPERATION;
	}
}

void glBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage)
{
	if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//check for usage later

	target -= GL_ARRAY_BUFFER;
	if (c->bound_buffers[target] == 0) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	//always NULL or valid
	free(c->buffers.a[c->bound_buffers[target]].data);

	if (!(c->buffers.a[c->bound_buffers[target]].data = (u8*) malloc(size))) {
		if (!c->error)
			c->error = GL_OUT_OF_MEMORY;
		// GL state is undefined from here on
		return;
	}

	if (data) {
		memcpy(c->buffers.a[c->bound_buffers[target]].data, data, size);
	}

	c->buffers.a[c->bound_buffers[target]].mapped = GL_FALSE;
	c->buffers.a[c->bound_buffers[target]].size = size;

	if (target == GL_ELEMENT_ARRAY_BUFFER) {
		c->vertex_arrays.a[c->cur_vertex_array].element_buffer = c->bound_buffers[target];
	}
}

void glBufferSubData(GLenum target, GLsizei offset, GLsizei size, const GLvoid* data)
{
	if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	target -= GL_ARRAY_BUFFER;
	if (c->bound_buffers[target] == 0) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	if (offset + size > c->buffers.a[c->bound_buffers[target]].size) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	memcpy(&c->buffers.a[c->bound_buffers[target]].data[offset], data, size);
}

void glBindTexture(GLenum target, GLuint texture)
{
	if (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_CUBE_MAP) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	target -= GL_TEXTURE_UNBOUND + 1;

	if (texture < c->textures.size && c->textures.a[texture].deleted == GL_FALSE) {
		if (c->textures.a[texture].type == GL_TEXTURE_UNBOUND) {
			c->bound_textures[target] = texture;
			c->textures.a[texture].type = target;
		} else if (c->textures.a[texture].type == target) {
			c->bound_textures[target] = texture;
		} else if (!c->error) {
			c->error = GL_INVALID_OPERATION;
		}
	} else if (!c->error) {
		c->error = GL_INVALID_VALUE;
	}
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	//GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_RECTANGLE, or GL_TEXTURE_CUBE_MAP.
	//will add others as they're implemented
	if (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_CUBE_MAP) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	
	//shift to range 0 - NUM_TEXTURES-1 to access bound_textures array
	target -= GL_TEXTURE_UNBOUND + 1;


//GL_TEXTURE_BASE_LEVEL, GL_TEXTURE_COMPARE_FUNC, GL_TEXTURE_COMPARE_MODE, GL_TEXTURE_LOD_BIAS, GL_TEXTURE_MIN_FILTER,
//GL_TEXTURE_MAG_FILTER, GL_TEXTURE_MIN_LOD, GL_TEXTURE_MAX_LOD, GL_TEXTURE_MAX_LEVEL, GL_TEXTURE_SWIZZLE_R,
//GL_TEXTURE_SWIZZLE_G, GL_TEXTURE_SWIZZLE_B, GL_TEXTURE_SWIZZLE_A, GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, or GL_TEXTURE_WRAP_R.
	if (pname != GL_TEXTURE_MIN_FILTER && pname != GL_TEXTURE_MAG_FILTER &&
	    pname != GL_TEXTURE_WRAP_S && pname != GL_TEXTURE_WRAP_T && pname != GL_TEXTURE_WRAP_R) {

		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (pname == GL_TEXTURE_MIN_FILTER) {
		if(param != GL_NEAREST && param != GL_LINEAR && param != GL_NEAREST_MIPMAP_NEAREST &&
		   param != GL_NEAREST_MIPMAP_LINEAR && param != GL_LINEAR_MIPMAP_NEAREST &&
		   param != GL_LINEAR_MIPMAP_LINEAR) {
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}

		//TODO mipmapping isn't actually supported, not sure it's worth trouble/perf hit
		//just adding the enums to make porting easier
		if (param == GL_NEAREST_MIPMAP_NEAREST || param == GL_NEAREST_MIPMAP_LINEAR)
			param = GL_NEAREST;
		if (param == GL_LINEAR_MIPMAP_NEAREST || param == GL_LINEAR_MIPMAP_LINEAR)
			param = GL_LINEAR;

		c->textures.a[c->bound_textures[target]].min_filter = param;

	} else if (pname == GL_TEXTURE_MAG_FILTER) {
		if(param != GL_NEAREST && param != GL_LINEAR) {
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}
		c->textures.a[c->bound_textures[target]].mag_filter = param;
	} else if (pname == GL_TEXTURE_WRAP_S) {
		if(param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT) {
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}
		c->textures.a[c->bound_textures[target]].wrap_s = param;
	} else if (pname == GL_TEXTURE_WRAP_T) {
		if(param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT) {
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}
		c->textures.a[c->bound_textures[target]].wrap_t = param;
	} else if (pname == GL_TEXTURE_WRAP_R) {
		if(param != GL_REPEAT && param != GL_CLAMP_TO_EDGE && param != GL_CLAMP_TO_BORDER && param != GL_MIRRORED_REPEAT) {
			if (!c->error)
				c->error = GL_INVALID_ENUM;
			return;
		}
		c->textures.a[c->bound_textures[target]].wrap_r = param;
	}
}

void glPixelStorei(GLenum pname, GLint param)
{
	if (pname != GL_UNPACK_ALIGNMENT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (param != 1 && param != 2 && param != 4 && param != 8) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	
	c->unpack_alignment = param;

}

void glGenerateMipmap(GLenum target)
{
	if (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_CUBE_MAP) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//TODO not implemented, not sure it's worth it.  This stub is just to
	//make porting real OpenGL programs easier.
	//For example mipmap generation code see
	//https://github.com/thebeast33/cro_lib/blob/master/cro_mipmap.h
}


#define imod(a, b) (a) - (b) * ((a)/(b))

int wrap(int i, int size, GLenum mode)
{
	int tmp, tmp2;
	switch (mode)
	{
	case GL_REPEAT:
		tmp = imod(i, size);
		if (tmp < 0) tmp = size + tmp;
		return tmp;

	// Border is too much of a pain to implement with render to
	// texture.  Trade offs in poor performance or ugly extra code
	// for a feature that almost no one actually uses and even
	// when it is used (barring rare/odd uv coordinates) it's not
	// even noticable.
	//case GL_CLAMP_TO_BORDER:
		//return clampi(i, -1, size);

	case GL_CLAMP_TO_BORDER:  // just so stuff that uses it compiles
	case GL_CLAMP_TO_EDGE:
		return clampi(i, 0, size-1);
	

	case GL_MIRRORED_REPEAT:
		if (i < 0) i = -i;
		tmp = i / size;
		tmp2 = i / (2*size);  // TODO what was this for?
		if (tmp % 2)
			return (size-1) - (i - tmp * size);
		else
			return i - tmp * size;

		return tmp;
	default:
		//should never happen, get rid of compile warning
		assert(0);
		return 0;
	}
}
#undef imod


// used in the following 4 texture access functions
// Not sure if it's actually necessary since wrap() clamps
#define EPSILON 0.000001
vec4 texture1D(GLuint tex, float x)
{
	int i0, i1;

	glTexture* t = &c->textures.a[tex];
	Color* texdata = (Color*)t->data;

	double w = t->w - EPSILON;

	double xw = x * w;

	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), t->w, t->wrap_s);

		return Color_to_vec4(texdata[i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), t->w, t->wrap_s);
		i1 = wrap(floor(xw + 0.499999), t->w, t->wrap_s);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		if (alpha < 0) ++alpha;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
#endif

		vec4 ci = Color_to_vec4(texdata[i0]);
		vec4 ci1 = Color_to_vec4(texdata[i1]);

		ci = scale_vec4(ci, (1-alpha));
		ci1 = scale_vec4(ci1, alpha);

		ci = add_vec4s(ci, ci1);

		return ci;
	}
}

vec4 texture2D(GLuint tex, float x, float y)
{
	int i0, j0, i1, j1;

	glTexture* t = &c->textures.a[tex];
	Color* texdata = (Color*)t->data;

	double dw = t->w - EPSILON;
	double dh = t->h - EPSILON;

	int w = t->w;
	int h = t->h;

	double xw = x * dw;
	double yh = y * dh;

	//TODO don't just use mag_filter all the time?
	//is it worth bothering?
	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), w, t->wrap_s);
		j0 = wrap(floor(yh), h, t->wrap_t);

		return Color_to_vec4(texdata[j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), w, t->wrap_s);
		j0 = wrap(floor(yh - 0.5), h, t->wrap_t);
		i1 = wrap(floor(xw + 0.499999), w, t->wrap_s);
		j1 = wrap(floor(yh + 0.499999), h, t->wrap_t);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		double beta = modf(yh+0.5, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
#endif

		vec4 cij = Color_to_vec4(texdata[j0*w + i0]);
		vec4 ci1j = Color_to_vec4(texdata[j0*w + i1]);
		vec4 cij1 = Color_to_vec4(texdata[j1*w + i0]);
		vec4 ci1j1 = Color_to_vec4(texdata[j1*w + i1]);

		cij = scale_vec4(cij, (1-alpha)*(1-beta));
		ci1j = scale_vec4(ci1j, alpha*(1-beta));
		cij1 = scale_vec4(cij1, (1-alpha)*beta);
		ci1j1 = scale_vec4(ci1j1, alpha*beta);

		cij = add_vec4s(cij, ci1j);
		cij = add_vec4s(cij, cij1);
		cij = add_vec4s(cij, ci1j1);

		return cij;
	}
}

vec4 texture3D(GLuint tex, float x, float y, float z)
{
	int i0, j0, i1, j1, k0, k1;

	glTexture* t = &c->textures.a[tex];
	Color* texdata = (Color*)t->data;

	double dw = t->w - EPSILON;
	double dh = t->h - EPSILON;
	double dd = t->d - EPSILON;

	int w = t->w;
	int h = t->h;
	int d = t->d;
	int plane = w * t->h;
	double xw = x * dw;
	double yh = y * dh;
	double zd = z * dd;


	if (t->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), w, t->wrap_s);
		j0 = wrap(floor(yh), h, t->wrap_t);
		k0 = wrap(floor(zd), d, t->wrap_r);

		return Color_to_vec4(texdata[k0*plane + j0*w + i0]);

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), w, t->wrap_s);
		j0 = wrap(floor(yh - 0.5), h, t->wrap_t);
		k0 = wrap(floor(zd - 0.5), d, t->wrap_r);
		i1 = wrap(floor(xw + 0.499999), w, t->wrap_s);
		j1 = wrap(floor(yh + 0.499999), h, t->wrap_t);
		k1 = wrap(floor(zd + 0.499999), d, t->wrap_r);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		double beta = modf(yh+0.5, &tmp2);
		double gamma = modf(zd+0.5, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;
		if (gamma < 0) ++gamma;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
		gamma = gamma*gamma * (3 - 2*gamma);
#endif

		vec4 cijk = Color_to_vec4(texdata[k0*plane + j0*w + i0]);
		vec4 ci1jk = Color_to_vec4(texdata[k0*plane + j0*w + i1]);
		vec4 cij1k = Color_to_vec4(texdata[k0*plane + j1*w + i0]);
		vec4 ci1j1k = Color_to_vec4(texdata[k0*plane + j1*w + i1]);
		vec4 cijk1 = Color_to_vec4(texdata[k1*plane + j0*w + i0]);
		vec4 ci1jk1 = Color_to_vec4(texdata[k1*plane + j0*w + i1]);
		vec4 cij1k1 = Color_to_vec4(texdata[k1*plane + j1*w + i0]);
		vec4 ci1j1k1 = Color_to_vec4(texdata[k1*plane + j1*w + i1]);

		cijk = scale_vec4(cijk, (1-alpha)*(1-beta)*(1-gamma));
		ci1jk = scale_vec4(ci1jk, alpha*(1-beta)*(1-gamma));
		cij1k = scale_vec4(cij1k, (1-alpha)*beta*(1-gamma));
		ci1j1k = scale_vec4(ci1j1k, alpha*beta*(1-gamma));
		cijk1 = scale_vec4(cijk1, (1-alpha)*(1-beta)*gamma);
		ci1jk1 = scale_vec4(ci1jk1, alpha*(1-beta)*gamma);
		cij1k1 = scale_vec4(cij1k1, (1-alpha)*beta*gamma);
		ci1j1k1 = scale_vec4(ci1j1k1, alpha*beta*gamma);

		cijk = add_vec4s(cijk, ci1jk);
		cijk = add_vec4s(cijk, cij1k);
		cijk = add_vec4s(cijk, ci1j1k);
		cijk = add_vec4s(cijk, cijk1);
		cijk = add_vec4s(cijk, ci1jk1);
		cijk = add_vec4s(cijk, cij1k1);
		cijk = add_vec4s(cijk, ci1j1k1);

		return cijk;
	}
}

vec4 texture_cubemap(GLuint texture, float x, float y, float z)
{
	glTexture* tex = &c->textures.a[texture];
	Color* texdata = (Color*)tex->data;

	float x_mag = (x < 0) ? -x : x;
	float y_mag = (y < 0) ? -y : y;
	float z_mag = (z < 0) ? -z : z;

	float s, t, max;

	int p, i0, j0, i1, j1;

	//there should be a better/shorter way to do this ...
	if (x_mag > y_mag) {
		if (x_mag > z_mag) {  //x largest
			max = x_mag;
			t = -y;
			if (x_mag == x) {
				p = 0;
				s = -z;
			} else {
				p = 1;
				s = z;
			}
		} else { //z largest
			max = z_mag;
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
			max = y_mag;
			s = x;
			if (y_mag == y) {
				p = 2;
				t = z;
			} else {
				p = 3;
				t = -z;
			}
		} else { //z largest
			max = z_mag;
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

	x = (s/max + 1.0f)/2.0f;
	y = (t/max + 1.0f)/2.0f;

	int w = tex->w;
	int h = tex->h;

	double dw = w - EPSILON;
	double dh = h - EPSILON;

	int plane = w*w;
	double xw = x * dw;
	double yh = y * dh;

	if (tex->mag_filter == GL_NEAREST) {
		i0 = wrap(floor(xw), w, tex->wrap_s);
		j0 = wrap(floor(yh), h, tex->wrap_t);

		vec4 tmpvec4 = Color_to_vec4(texdata[p*plane + j0*w + i0]);
		return tmpvec4;

	} else {
		// LINEAR
		// This seems right to me since pixel centers are 0.5 but
		// this isn't exactly what's described in the spec or FoCG
		i0 = wrap(floor(xw - 0.5), tex->w, tex->wrap_s);
		j0 = wrap(floor(yh - 0.5), tex->h, tex->wrap_t);
		i1 = wrap(floor(xw + 0.499999), tex->w, tex->wrap_s);
		j1 = wrap(floor(yh + 0.499999), tex->h, tex->wrap_t);

		double tmp2;
		double alpha = modf(xw+0.5, &tmp2);
		double beta = modf(yh+0.5, &tmp2);
		if (alpha < 0) ++alpha;
		if (beta < 0) ++beta;

		//hermite smoothing is optional
		//looks like my nvidia implementation doesn't do it
		//but it can look a little better
#ifdef HERMITE_SMOOTHING
		alpha = alpha*alpha * (3 - 2*alpha);
		beta = beta*beta * (3 - 2*beta);
#endif

		vec4 cij = Color_to_vec4(texdata[p*plane + j0*w + i0]);
		vec4 ci1j = Color_to_vec4(texdata[p*plane + j0*w + i1]);
		vec4 cij1 = Color_to_vec4(texdata[p*plane + j1*w + i0]);
		vec4 ci1j1 = Color_to_vec4(texdata[p*plane + j1*w + i1]);

		cij = scale_vec4(cij, (1-alpha)*(1-beta));
		ci1j = scale_vec4(ci1j, alpha*(1-beta));
		cij1 = scale_vec4(cij1, (1-alpha)*beta);
		ci1j1 = scale_vec4(ci1j1, alpha*beta);

		cij = add_vec4s(cij, ci1j);
		cij = add_vec4s(cij, cij1);
		cij = add_vec4s(cij, ci1j1);

		return cij;
	}
}

#undef EPSILON

void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	if (target != GL_TEXTURE_1D) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (border) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	//ignore level for now

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	c->textures.a[cur_tex].w = width;

	if (type != GL_UNSIGNED_BYTE) {

		return;
	}

	int components;
	if (format == GL_RED) components = 1;
	else if (format == GL_RG) components = 2;
	else if (format == GL_RGB || format == GL_BGR) components = 3;
	else if (format == GL_RGBA || format == GL_BGRA) components = 4;
	else {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (c->textures.a[cur_tex].data)
		free(c->textures.a[cur_tex].data);

	//TODO support other internal formats? components should be of internalformat not format
	if (!(c->textures.a[cur_tex].data = (u8*) malloc(width * components))) {
		if (!c->error)
			c->error = GL_OUT_OF_MEMORY;
		//undefined state now
		return;
	}

	u32* texdata = (u32*) c->textures.a[cur_tex].data;

	if (data)
		memcpy(&texdata[0], data, width*sizeof(u32));

	c->textures.a[cur_tex].mapped = GL_FALSE;

	//TODO
	//assume for now always RGBA coming in and that's what I'm storing it as
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	//GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_RECTANGLE, or GL_TEXTURE_CUBE_MAP.
	//will add others as they're implemented
	if (target != GL_TEXTURE_2D &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (border) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	//ignore level for now

	//TODO support other types
	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	int components;
	if (format == GL_RED) components = 1;
	else if (format == GL_RG) components = 2;
	else if (format == GL_RGB || format == GL_BGR) components = 3;
	else if (format == GL_RGBA || format == GL_BGRA) components = 4;
	else {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	int cur_tex;

	// If I ever support other alignments
	//int tmp = width % c->unpack_alignment;
	//int row_len = (!tmp) ? width : width + c->unpack_alignment - tmp;

	if (target == GL_TEXTURE_2D) {
		cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

		c->textures.a[cur_tex].w = width;
		c->textures.a[cur_tex].h = height;

		// either NULL or valid
		free(c->textures.a[cur_tex].data);

		//TODO support other internal formats? components should be of internalformat not format
		if (!(c->textures.a[cur_tex].data = (u8*) malloc(width*height * components))) {
			if (!c->error)
				c->error = GL_OUT_OF_MEMORY;
			//undefined state now
			return;
		}

		if (data)
			memcpy(c->textures.a[cur_tex].data, data, width*height*sizeof(u32));

		c->textures.a[cur_tex].mapped = GL_FALSE;

	} else {  //CUBE_MAP
		cur_tex = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1];

		if (width != height) {
			//TODO spec says INVALID_VALUE, man pages say INVALID_ENUM ?
			if (!c->error)
				c->error = GL_INVALID_VALUE;
			return;
		}

		int mem_size = width*height*6 * components;
		if (c->textures.a[cur_tex].w == 0) {
			c->textures.a[cur_tex].w = width;
			c->textures.a[cur_tex].h = width; //same cause square

			if (!(c->textures.a[cur_tex].data = (u8*) malloc(mem_size))) {
				if (!c->error)
					c->error = GL_OUT_OF_MEMORY;
				//undefined state now
				return;
			}
		} else if (c->textures.a[cur_tex].w != width) {
			//TODO spec doesn't say all sides must have same dimensions but it makes sense
			//and this site suggests it http://www.opengl.org/wiki/Cubemap_Texture
			if (!c->error)
				c->error = GL_INVALID_VALUE;
			return;
		}

		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X; //use target as plane index

		int p = width*height;
		u32* texdata = (u32*) c->textures.a[cur_tex].data;

		//printf("texdata etc =\n%lu\n%lu\n%lu\n", texdata, c->textures.a[cur_tex].data, c->textures.a[cur_tex].data + mem_size);
		
		if (data)
			memcpy(&texdata[target*p], data, width*height*sizeof(u32));

		c->textures.a[cur_tex].mapped = GL_FALSE;

	} //end CUBE_MAP
}

void glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data)
{
	//GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_RECTANGLE, or GL_TEXTURE_CUBE_MAP.
	//will add others as they're implemented
	if (target != GL_TEXTURE_3D) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (border) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	//ignore level for now

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	c->textures.a[cur_tex].w = width;
	c->textures.a[cur_tex].h = height;
	c->textures.a[cur_tex].d = depth;

	if (type != GL_UNSIGNED_BYTE) {

		return;
	}

	// TODO add error?  only support GL_RGBA for now
	int components;
	if (format == GL_RED) components = 1;
	else if (format == GL_RG) components = 2;
	else if (format == GL_RGB || format == GL_BGR) components = 3;
	else if (format == GL_RGBA || format == GL_BGRA) components = 4;
	else {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (c->textures.a[cur_tex].data)
		free(c->textures.a[cur_tex].data);

	//TODO support other internal formats? components should be of internalformat not format
	if (!(c->textures.a[cur_tex].data = (u8*) malloc(width*height*depth * components))) {
		if (!c->error)
			c->error = GL_OUT_OF_MEMORY;
		//undefined state now
		return;
	}

	u32* texdata = (u32*) c->textures.a[cur_tex].data;

	if (data)
		memcpy(texdata, data, width*height*depth*sizeof(u32));

	c->textures.a[cur_tex].mapped = GL_FALSE;

	//TODO
	//assume for now always RGBA coming in and that's what I'm storing it as


}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* data)
{
	if (target != GL_TEXTURE_1D) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//ignore level for now

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	//only kind supported currently
	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//TODO
	if (format != GL_RGBA) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (xoffset < 0 || xoffset + width > c->textures.a[cur_tex].w) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	u32* texdata = (u32*) c->textures.a[cur_tex].data;
	memcpy(&texdata[xoffset], data, width*sizeof(u32));
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* data)
{
	//GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_RECTANGLE, or GL_TEXTURE_CUBE_MAP.
	//will add others as they're implemented
	if (target != GL_TEXTURE_2D &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
	    target != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
	    target != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//ignore level for now

	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (format != GL_RGBA) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	int cur_tex;
	u32* d = (u32*) data;

	if (target == GL_TEXTURE_2D) {
		cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];
		u32* texdata = (u32*) c->textures.a[cur_tex].data;

		if (xoffset < 0 || xoffset + width > c->textures.a[cur_tex].w || yoffset < 0 || yoffset + height > c->textures.a[cur_tex].h) {
			if (!c->error)
				c->error = GL_INVALID_VALUE;
			return;
		}

		int w = c->textures.a[cur_tex].w;

		for (int i=0; i<height; ++i) {
			memcpy(&texdata[(yoffset+i)*w + xoffset], &d[i*width], width*sizeof(u32));
		}

	} else {  //CUBE_MAP
		cur_tex = c->bound_textures[GL_TEXTURE_CUBE_MAP-GL_TEXTURE_UNBOUND-1];
		u32* texdata = (u32*) c->textures.a[cur_tex].data;

		int w = c->textures.a[cur_tex].w;

		target -= GL_TEXTURE_CUBE_MAP_POSITIVE_X; //use target as plane index

		int p = w*w;

		for (int i=0; i<height; ++i)
			memcpy(&texdata[p*target + (yoffset+i)*w + xoffset], &d[i*width], width*sizeof(u32));
	} //end CUBE_MAP
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data)
{
	//GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_RECTANGLE, or GL_TEXTURE_CUBE_MAP.
	//will add others as they're implemented
	if (target != GL_TEXTURE_3D) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//ignore level for now

	int cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

	if (type != GL_UNSIGNED_BYTE) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//TODO
	if (format != GL_RGBA) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (xoffset < 0 || xoffset + width > c->textures.a[cur_tex].w ||
	    yoffset < 0 || yoffset + height > c->textures.a[cur_tex].h ||
	    zoffset < 0 || zoffset + depth > c->textures.a[cur_tex].d) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	int w = c->textures.a[cur_tex].w;
	int h = c->textures.a[cur_tex].h;
	int p = w*h;
	u32* d = (u32*) data;
	u32* texdata = (u32*) c->textures.a[cur_tex].data;

	for (int j=0; j<depth; ++j) {
		for (int i=0; i<height; ++i) {
			memcpy(&texdata[(zoffset+j)*p + (yoffset+i)*w + xoffset], &d[j*width*height + i*width], width*sizeof(u32));
		}
	}
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLsizei offset)
{
	if (index >= GL_MAX_VERTEX_ATTRIBS || size < 1 || size > 4 || (!c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER] && offset)) {
		if (!c->error)
			c->error = GL_INVALID_OPERATION;
		return;
	}

	//TODO type Specifies the data type of each component in the array. The symbolic constants GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,
	//GL_UNSIGNED_SHORT, GL_INT, and GL_UNSIGNED_INT are accepted by both functions. Additionally GL_HALF_FLOAT, GL_FLOAT, GL_DOUBLE,
	//GL_INT_2_10_10_10_REV, and GL_UNSIGNED_INT_2_10_10_10_REV are accepted by glVertexAttribPointer. The initial value is GL_FLOAT.
	if (type != GL_FLOAT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	glVertex_Attrib* v = &(c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index]);
	v->size = size;
	v->type = type;

	//TODO expand for other types etc.
	v->stride = (stride) ? stride : size*sizeof(GLfloat);

	v->offset = offset;
	v->normalized = normalized;
	v->buf = c->bound_buffers[GL_ARRAY_BUFFER-GL_ARRAY_BUFFER]; //can be 0 if offset is 0/NULL
}

void glEnableVertexAttribArray(GLuint index)
{
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_TRUE;
}

void glDisableVertexAttribArray(GLuint index)
{
	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].enabled = GL_FALSE;
}

void glVertexAttribDivisor(GLuint index, GLuint divisor)
{
	if (index >= GL_MAX_VERTEX_ATTRIBS) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs[index].divisor = divisor;
}


//TODO not used
vec4 get_vertex_attrib_array(glVertex_Attrib* v, GLsizei i)
{
	//this line need work for future flexibility and handling more than floats
	u8* buf_pos = (u8*)c->buffers.a[v->buf].data + v->offset + v->stride*i;

	vec4 tmpvec4;
	memcpy(&tmpvec4, buf_pos, sizeof(float)*v->size);

	//c->cur_vertex_array->vertex_attribs[enabled[j]].buf->data;
	return tmpvec4;
}


/* this clip epsilon is needed to avoid some rounding errors after
   several clipping stages */

#define CLIP_EPSILON (1E-5)

static inline int gl_clipcode(vec4 pt)
{
	float w;

	w = pt.w * (1.0 + CLIP_EPSILON);
	return
		(((pt.z < -w) |
		( (pt.z >  w) << 1)) &
		(!c->depth_clamp |
		 !c->depth_clamp << 1)) |

		((pt.x < -w) << 2) |
		((pt.x >  w) << 3) |
		((pt.y < -w) << 4) |
		((pt.y >  w) << 5);

}


void do_vertex(glVertex_Attrib* v, int* enabled, unsigned int num_enabled, unsigned int i, unsigned int vert)
{
	GLuint buf;
	u8* buf_pos;
	vec4 tmpvec4;

	for (int j=0; j<num_enabled; ++j) {
		buf = v[enabled[j]].buf;

		buf_pos = (u8*)c->buffers.a[buf].data + v[enabled[j]].offset + v[enabled[j]].stride*i;

		SET_VEC4(tmpvec4, 0.0f, 0.0f, 0.0f, 1.0f);
		memcpy(&tmpvec4, buf_pos, sizeof(float)*v[enabled[j]].size);

		c->vertex_attribs_vs[enabled[j]] = tmpvec4;
	}

	float* vs_out = &c->vs_output.output_buf.a[vert*c->vs_output.size];
	c->programs.a[c->cur_program].vertex_shader(vs_out, c->vertex_attribs_vs, &c->builtins, c->programs.a[c->cur_program].uniform);

	c->glverts.a[vert].vs_out = vs_out;
	c->glverts.a[vert].clip_space = c->builtins.gl_Position;
	c->glverts.a[vert].edge_flag = 1;

	c->glverts.a[vert].clip_code = gl_clipcode(c->builtins.gl_Position);
}





void vertex_stage(GLint first, GLsizei count, GLsizei instance_id, GLuint base_instance, GLboolean use_elements)
{
	unsigned int i, j, vert, num_enabled;
	vec4 tmpvec4;
	u8* buf_pos;

	//save checking if enabled on every loop if we build this first
	//also initialize the vertex_attrib space
	float vec4_init[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	int enabled[GL_MAX_VERTEX_ATTRIBS];
	memset(enabled, 0, sizeof(int)*GL_MAX_VERTEX_ATTRIBS);
	glVertex_Attrib* v = c->vertex_arrays.a[c->cur_vertex_array].vertex_attribs;
	GLuint elem_buffer = c->vertex_arrays.a[c->cur_vertex_array].element_buffer;

	for (i=0, j=0; i<GL_MAX_VERTEX_ATTRIBS; ++i) {
		memcpy(&c->vertex_attribs_vs[i], vec4_init, sizeof(vec4));

		if (v[i].enabled) {
 		   	if (v[i].divisor == 0) {
				enabled[j++] = i;
				//printf("%d is enabled\n", i);
			} else if (!(instance_id % v[i].divisor)) {   //set instanced attributes if necessary
				int n = instance_id/v[i].divisor + base_instance;
				buf_pos = (u8*)c->buffers.a[v[i].buf].data + v[i].offset + v[i].stride*n;

				SET_VEC4(tmpvec4, 0.0f, 0.0f, 0.0f, 1.0f);

				memcpy(&tmpvec4, buf_pos, sizeof(float)*v[enabled[j]].size); //TODO why v[enabled[j]].size and not just v[i].size?

				//c->cur_vertex_array->vertex_attribs[enabled[j]].buf->data;

				c->vertex_attribs_vs[i] = tmpvec4;
			}
		}
	}
	num_enabled = j;

	cvec_reserve_glVertex(&c->glverts, count);
	c->builtins.gl_InstanceID = instance_id;

	if (!use_elements) {
		for (vert=0, i=first; i<first+count; ++i, ++vert) {
			do_vertex(v, enabled, num_enabled, i, vert);
		}
	} else {
		GLuint* uint_array = (GLuint*) c->buffers.a[elem_buffer].data;
		GLushort* ushort_array = (GLushort*) c->buffers.a[elem_buffer].data;
		GLubyte* ubyte_array = (GLubyte*) c->buffers.a[elem_buffer].data;
		if (c->buffers.a[elem_buffer].type == GL_UNSIGNED_BYTE) {
			for (vert=0, i=first; i<first+count; ++i, ++vert) {
				do_vertex(v, enabled, num_enabled, ubyte_array[i], vert);
			}
		} else if (c->buffers.a[elem_buffer].type == GL_UNSIGNED_SHORT) {
			for (vert=0, i=first; i<first+count; ++i, ++vert) {
				do_vertex(v, enabled, num_enabled, ushort_array[i], vert);
			}
		} else {
			for (vert=0, i=first; i<first+count; ++i, ++vert) {
				do_vertex(v, enabled, num_enabled, uint_array[i], vert);
			}
		}
	}
}





void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	run_pipeline(mode, first, count, 0, 0, GL_FALSE);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, GLsizei offset)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//error not in the spec but says type must be one of these ... strange
	if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;
	run_pipeline(mode, offset, count, 0, 0, GL_TRUE);
}

void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//TODO check for buffer mapped when I implement that according to spec
	//still want to do my own special map function to mean just use the pointer

	for (unsigned int instance = 0; instance < primcount; ++instance) {
		run_pipeline(mode, first, count, instance, 0, GL_FALSE);
	}
}

void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//TODO check for buffer mapped when I implement that according to spec
	//still want to do my own special map function to mean just use the pointer

	for (unsigned int instance = 0; instance < primcount; ++instance) {
		run_pipeline(mode, first, count, instance, baseinstance, GL_FALSE);
	}
}


void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei primcount)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//error not in the spec but says type must be one of these ... strange
	if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	//TODO check for buffer mapped when I implement that according to spec
	//still want to do my own special map function to mean just use the pointer

	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;

	for (unsigned int instance = 0; instance < primcount; ++instance) {
		run_pipeline(mode, offset, count, instance, 0, GL_TRUE);
	}
}

void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei primcount, GLuint baseinstance)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//error not in the spec but says type must be one of these ... strange
	if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	//TODO check for buffer mapped when I implement that according to spec
	//still want to do my own special map function to mean just use the pointer

	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;

	for (unsigned int instance = 0; instance < primcount; ++instance) {
		run_pipeline(mode, offset, count, instance, baseinstance, GL_TRUE);
	}
}

int is_front_facing(glVertex* v0, glVertex* v1, glVertex* v2)
{
	//according to docs culling is done based on window coordinates
	//See page 3.6.1 page 116 of glspec33.core for more on rasterization, culling etc.
	//
	//TODO bug where it doesn't correctly cull if part of the triangle goes behind eye
	vec3 normal, tmpvec3 = { 0, 0, 1 };

	vec3 p0 = vec4_to_vec3h(v0->screen_space);
	vec3 p1 = vec4_to_vec3h(v1->screen_space);
	vec3 p2 = vec4_to_vec3h(v2->screen_space);

	//float a;

	//method from spec
	//a = p0.x*p1.y - p1.x*p0.y + p1.x*p2.y - p2.x*p1.y + p2.x*p0.y - p0.x*p2.y;
	//a /= 2;

	normal = cross_product(sub_vec3s(p1, p0), sub_vec3s(p2, p0));

	if (c->front_face == GL_CW) {
		//a = -a;
		normal = negate_vec3(normal);
	}

	//if (a <= 0) {
	if (dot_vec3s(normal, tmpvec3) <= 0) {
		return 0;
	}

	return 1;
}

//TODO make fs_input static?  or a member of glContext?
static void draw_point(glVertex* vert)
{
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];

	vec3 point = vec4_to_vec3h(vert->screen_space);
	point.z = MAP(point.z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	//TODO not sure if I'm supposed to do this ... doesn't say to in spec but it is called depth clamping
	if (c->depth_clamp)
		point.z = clampf_01(point.z);

	if (c->depth_test && !c->frag_depth_used) {
		if (point.z > ((float*)c->zbuf.lastrow)[-(int)point.y*c->zbuf.w + (int)point.x]) {
			return;
		} else {
			((float*)c->zbuf.lastrow)[-(int)point.y*c->zbuf.w + (int)point.x] = point.z;
		}
	}

	//TODO why not just pass vs_output directly?  hmmm...
	memcpy(fs_input, vert->vs_out, c->vs_output.size*sizeof(float));

	//TODO set other builtins, FragCoord  etc.
	//
	//accounting for pixel centers at 0.5, using truncation
	float x = point.x + 0.5f;
	float y = point.y + 0.5f;
	float p_size = c->point_size;
	float origin = (c->point_spr_origin == GL_UPPER_LEFT) ? -1.0f : 1.0f;

	for (float i = y-p_size/2; i<y+p_size/2; ++i) {
		for (float j = x-p_size/2; j<x+p_size/2; ++j) {
			
			// per page 110 of 3.3 spec
			c->builtins.gl_PointCoord.x = 0.5f + ((int)j + 0.5f - point.x)/p_size;
			c->builtins.gl_PointCoord.y = 0.5f + origin * ((int)i + 0.5f - point.y)/p_size;

			c->builtins.discard = GL_FALSE;
			c->programs.a[c->cur_program].fragment_shader(fs_input, &c->builtins, c->programs.a[c->cur_program].uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, j, i);
		}
	}
}

static void run_pipeline(GLenum mode, GLint first, GLsizei count, GLsizei instance, GLuint base_instance, GLboolean use_elements)
{
	unsigned int i, vert;
	int provoke;

	assert(count <= MAX_VERTICES);

	vertex_stage(first, count, instance, base_instance, use_elements);

	//fragment portion
	if (mode == GL_POINTS) {
		for (vert=0, i=first; i<first+count; ++i, ++vert) {
			if (c->glverts.a[vert].clip_code)
				continue;

			c->glverts.a[vert].screen_space = mult_mat4_vec4(c->vp_mat, c->glverts.a[vert].clip_space);

			draw_point(&c->glverts.a[vert]);
		}
	} else if (mode == GL_LINES) {
		for (vert=0, i=first; i<first+count-1; i+=2, vert+=2) {
			draw_line_clip(&c->glverts.a[vert], &c->glverts.a[vert+1]);
		}
	} else if (mode == GL_LINE_STRIP) {
		for (vert=0, i=first; i<first+count-1; i++, vert++) {
			draw_line_clip(&c->glverts.a[vert], &c->glverts.a[vert+1]);
		}
	} else if (mode == GL_LINE_LOOP) {
		for (vert=0, i=first; i<first+count-1; i++, vert++) {
			draw_line_clip(&c->glverts.a[vert], &c->glverts.a[vert+1]);
		}
		//draw ending line from last to first point
		draw_line_clip(&c->glverts.a[count-1], &c->glverts.a[0]);

	} else if (mode == GL_TRIANGLES) {
		provoke = (c->provoking_vert == GL_LAST_VERTEX_CONVENTION) ? 2 : 0;

		for (vert=0, i=first; i<first+count-2; i+=3, vert+=3) {
			draw_triangle(&c->glverts.a[vert], &c->glverts.a[vert+1], &c->glverts.a[vert+2], vert+provoke);
		}

	} else if (mode == GL_TRIANGLE_STRIP) {
		unsigned int a=0, b=1, toggle = 0;
		provoke = (c->provoking_vert == GL_LAST_VERTEX_CONVENTION) ? 0 : -2;

		for (vert=2; vert<count; ++vert) {
			draw_triangle(&c->glverts.a[a], &c->glverts.a[b], &c->glverts.a[vert], vert+provoke);

			if (!toggle)
				a = vert;
			else
				b = vert;

			toggle = !toggle;
		}
	} else if (mode == GL_TRIANGLE_FAN) {
		provoke = (c->provoking_vert == GL_LAST_VERTEX_CONVENTION) ? 0 : -1;

		for (vert=2; vert<count; ++vert) {
			draw_triangle(&c->glverts.a[0], &c->glverts.a[vert-1], &c->glverts.a[vert], vert+provoke);
		}
	}
}




void glViewport(int x, int y, GLsizei width, GLsizei height)
{
	if (width < 0 || height < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	make_viewport_matrix(c->vp_mat, x, y, width, height, 1);
	c->x_min = x;
	c->y_min = y;
	c->x_max = x + width;
	c->y_max = y + height;
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	red = clampf_01(red);
	green = clampf_01(green);
	blue = clampf_01(blue);
	alpha = clampf_01(alpha);

	vec4 tmp = { red, green, blue, alpha };
	c->clear_color = vec4_to_Color(tmp);
}

void glClearDepth(GLclampf depth)
{
	c->clear_depth = clampf_01(depth);
}

void glDepthFunc(GLenum func)
{
	if (func != GL_NEVER && func != GL_ALWAYS && func != GL_LESS &&
	    func != GL_LEQUAL && func != GL_EQUAL && func != GL_GEQUAL &&
	    func != GL_GREATER && func != GL_NOTEQUAL) {
		if (!c->error)
			c->error =GL_INVALID_ENUM;

		return;
	}

	c->depth_func = func;
}

static int depthtest(float zval, float zbufval)
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
	return 0; //get rid of compile warning
}

void glDepthRange(GLclampf nearVal, GLclampf farVal)
{
	c->depth_range_near = clampf_01(nearVal);
	c->depth_range_far = clampf_01(farVal);
}

void glClear(GLbitfield mask)
{
	if (!(mask & (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		printf("failed to clear\n");
		return;
	}

	Color col = c->clear_color;
	if (mask & GL_COLOR_BUFFER_BIT) {
		for (int i=0; i<c->back_buffer.w*c->back_buffer.h; ++i) {
			((u32*)c->back_buffer.buf)[i] = col.a << c->Ashift | col.r << c->Rshift | col.g << c->Gshift | col.b << c->Bshift;
		}
	}

	if (mask & GL_DEPTH_BUFFER_BIT) {
		//TODO try a big memcpy or other way to clear it
		for (int i=0; i < c->zbuf.w * c->zbuf.h; ++i)
			((float*)c->zbuf.buf)[i] = c->clear_depth;
	}

	if (mask & GL_STENCIL_BUFFER_BIT) {

	}
}

void glEnable(GLenum cap)
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
		c->line_smooth = GL_TRUE;
		break;
	case GL_BLEND:
		c->blend = GL_TRUE;
		break;
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glDisable(GLenum cap)
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
	default:
		if (!c->error)
			c->error = GL_INVALID_ENUM;
	}
}

void glCullFace(GLenum mode)
{
	if (mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	c->cull_mode = mode;
}


void glFrontFace(GLenum mode)
{
	if (mode != GL_CCW && mode != GL_CW) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	c->front_face = mode;
}

void glPolygonMode(GLenum face, GLenum mode)
{
	if ((face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) ||
	    (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL)) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

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


void glPointSize(GLfloat size)
{
	if (size <= 0.0f) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	c->point_size = size;
}

void glPointParameteri(GLenum pname, GLint param)
{
	//also GL_POINT_FADE_THRESHOLD_SIZE
	if (pname != GL_POINT_SPRITE_COORD_ORIGIN || (param != GL_LOWER_LEFT && param != GL_UPPER_LEFT)) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}
	c->point_spr_origin = param;
}


void glProvokingVertex(GLenum provokeMode)
{
	if (provokeMode != GL_FIRST_VERTEX_CONVENTION && provokeMode != GL_LAST_VERTEX_CONVENTION) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	c->provoking_vert = provokeMode;
}

void clear_screen()
{
	memset(c->back_buffer.buf, 255, c->back_buffer.w * c->back_buffer.h * 4);
}


// Shader functions
GLuint pglCreateProgram(vert_func vertex_shader, frag_func fragment_shader, GLsizei n, GLenum* interpolation, GLboolean use_frag_depth)
{
	if (!vertex_shader || !fragment_shader) {
		//TODO set error? doesn't in spec but I'll think about it
		return 0;
	}

	if (n > GL_MAX_VERTEX_OUTPUT_COMPONENTS) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return 0;
	}

	glProgram tmp = {vertex_shader, fragment_shader, NULL, n, {0}, use_frag_depth, GL_FALSE };
	memcpy(tmp.interpolation, interpolation, n*sizeof(GLenum));

	for (int i=1; i<c->programs.size; ++i) {
		if (c->programs.a[i].deleted && i != c->cur_program) {
			c->programs.a[i] = tmp;
			return i;
		}
	}

	cvec_push_glProgram(&c->programs, tmp);
	return c->programs.size-1;
}

void glDeleteProgram(GLuint program)
{
	if (!program)
		return;

	if (program >= c->programs.size) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	c->programs.a[program].deleted = GL_TRUE;
}

void glUseProgram(GLuint program)
{
	if (program >= c->programs.size) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}

	c->vs_output.size = c->programs.a[program].vs_output_size;
	cvec_reserve_float(&c->vs_output.output_buf, c->vs_output.size * MAX_VERTICES);
	c->vs_output.interpolation = c->programs.a[program].interpolation;
	c->frag_depth_used = c->programs.a[program].use_frag_depth;

	c->cur_program = program;
}

void set_uniform(void* uniform)
{
	//TODO check for NULL? definitely if I ever switch to storing a local
	//copy in glProgram
	c->programs.a[c->cur_program].uniform = uniform;
}


//TODO rename?  interpolation only applies to vs output, ie it's done
//between the vs and fs.  So maybe call it vs_output_interp so
//it's not confused with the input vertex attributes
void set_vs_interpolation(GLsizei n, GLenum* interpolation)
{
	c->programs.a[c->cur_program].vs_output_size = n;
	c->vs_output.size = n;

	memcpy(c->programs.a[c->cur_program].interpolation, interpolation, n*sizeof(GLenum));
	cvec_reserve_float(&c->vs_output.output_buf, n * MAX_VERTICES);

	//vs_output.interpolation would be already pointing at current program's array
	//unless the programs array was realloced since the last glUseProgram because
	//they've created a bunch of programs.  Unlikely they'd be changing a shader
	//before creating all their shaders but whatever.
	c->vs_output.interpolation = c->programs.a[c->cur_program].interpolation;
}


//new TODO list
//Clipping for real!
//FBO's, render buffers, render to texture, multiple outputs etc.
//More texture formats?
//Geometry shader
//Optimize


static void setup_fs_input(float t, float* v1_out, float* v2_out, float wa, float wb, unsigned int provoke)
{
	float* vs_output = &c->vs_output.output_buf.a[0];

	float inv_wa = 1.0/wa;
	float inv_wb = 1.0/wb;

	for (int i=0; i<c->vs_output.size; ++i) {
		if (c->vs_output.interpolation[i] == SMOOTH) {
			c->fs_input[i] = (v1_out[i]*inv_wa + t*(v2_out[i]*inv_wb - v1_out[i]*inv_wa)) / (inv_wa + t*(inv_wb - inv_wa));

		} else if (c->vs_output.interpolation[i] == NOPERSPECTIVE) {
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

	//TODO ponder this
	unsigned int provoke;
	if (c->provoking_vert == GL_LAST_VERTEX_CONVENTION)
		provoke = (v2 - c->glverts.a)/sizeof(glVertex);
	else
		provoke = (v1 - c->glverts.a)/sizeof(glVertex);

	if (cc1 & cc2) {
		return;
	} else if ((cc1 | cc2) == 0) {
		t1 = mult_mat4_vec4(c->vp_mat, p1);
		t2 = mult_mat4_vec4(c->vp_mat, p2);

		//no need
		//memcpy(v1_out, v1->vs_out, c->vs_output.size*sizeof(float));
		//memcpy(v2_out, v2->vs_out, c->vs_output.size*sizeof(float));

		if (!c->line_smooth)
			draw_line_shader(t1, t2, v1->vs_out, v2->vs_out, provoke);
		else
			draw_line_smooth_shader(t1, t2, v1->vs_out, v2->vs_out, provoke);
	} else {

		d = sub_vec4s(p2, p1);

		tmin = 0;
		tmax = 1;
		if (clip_line( d.x+d.w, -p1.x-p1.w, &tmin, &tmax) &&
		    clip_line(-d.x+d.w,  p1.x-p1.w, &tmin, &tmax) &&
		    clip_line( d.y+d.w, -p1.y-p1.w, &tmin, &tmax) &&
		    clip_line(-d.y+d.w,  p1.y-p1.w, &tmin, &tmax) &&
			clip_line( d.z+d.w, -p1.z-p1.w, &tmin, &tmax) &&
			clip_line(-d.z+d.w,  p1.z-p1.w, &tmin, &tmax)) {

			//printf("%f %f\n", tmin, tmax);

			t1 = add_vec4s(p1, scale_vec4(d, tmin));
			t2 = add_vec4s(p1, scale_vec4(d, tmax));

			t1 = mult_mat4_vec4(c->vp_mat, t1);
			t2 = mult_mat4_vec4(c->vp_mat, t2);
			//print_vec4(t1, "\n");
			//print_vec4(t2, "\n");

			interpolate_clipped_line(v1, v2, v1_out, v2_out, tmin, tmax);

			if (!c->line_smooth)
				draw_line_shader(t1, t2, v1_out, v2_out, provoke);
			else
				draw_line_smooth_shader(t1, t2, v1_out, v2_out, provoke);
		}
	}
}


static void draw_line_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke)
{
	float tmp;
	float* tmp_ptr;

	vec3 hp1 = vec4_to_vec3h(v1);
	vec3 hp2 = vec4_to_vec3h(v2);

	//print_vec3(hp1, "\n");
	//print_vec3(hp2, "\n");

	float w1 = v1.w;
	float w2 = v2.w;

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
	vec2 pr, sub_p2p1 = sub_vec2s(p2, p1);
	float line_length_squared = length_vec2(sub_p2p1);
	line_length_squared *= line_length_squared;

	frag_func fragment_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniform = c->programs.a[c->cur_program].uniform;
	int frag_depth_used = c->programs.a[c->cur_program].use_frag_depth;

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

	//printf("%f %f %f %f   =\n", i_x1, i_y1, i_x2, i_y2);
	//printf("%f %f %f %f   x_min etc\n", x_min, x_max, y_min, y_max);

	z1 = MAP(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	z2 = MAP(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

	//4 cases based on slope
	if (m <= -1) {     //(-infinite, -1]
		//printf("slope <= -1\n");
		for (x = x_min, y = y_max; y>=y_min && x<=x_max; --y) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			if (!frag_depth_used && c->depth_test) {
				if (depthtest(z, ((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x])) {
					((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x] = z;
				} else {
					goto line_1;
				}
			}

			SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y);

line_1:
			if (line_func(&line, x+0.5f, y-1) < 0) //A*(x+0.5f) + B*(y-1) + C < 0)
				++x;
		}
	} else if (m <= 0) {     //(-1, 0]
		//printf("slope = (-1, 0]\n");
		for (x = x_min, y = y_max; x<=x_max && y>=y_min; ++x) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			if (!c->frag_depth_used && c->depth_test) {
				if (depthtest(z, ((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x])) {
					((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x] = z;
				} else {
					goto line_2;
				}
			}

			SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y);

line_2:
			if (line_func(&line, x+1, y-0.5f) > 0) //A*(x+1) + B*(y-0.5f) + C > 0)
				--y;
		}
	} else if (m <= 1) {     //(0, 1]
		//printf("slope = (0, 1]\n");
		for (x = x_min, y = y_min; x <= x_max && y <= y_max; ++x) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			if (!c->frag_depth_used && c->depth_test) {
				if (depthtest(z, ((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x])) {
					((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x] = z;
				} else {
					goto line_3;
				}
			}

			SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y);

line_3:
			if (line_func(&line, x+1, y+0.5f) < 0) //A*(x+1) + B*(y+0.5f) + C < 0)
				++y;
		}

	} else {    //(1, +infinite)
		//printf("slope > 1\n");
		for (x = x_min, y = y_min; y<=y_max && x <= x_max; ++y) {
			pr.x = x;
			pr.y = y;
			t = dot_vec2s(sub_vec2s(pr, p1), sub_p2p1) / line_length_squared;

			z = (1 - t) * z1 + t * z2;
			w = (1 - t) * w1 + t * w2;

			if (!c->frag_depth_used && c->depth_test) {
				if (depthtest(z, ((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x])) {
					((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x] = z;
				} else {
					goto line_4;
				}
			}

			SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y);

line_4:
			if (line_func(&line, x+0.5f, y+1) > 0) //A*(x+0.5f) + B*(y+1) + C > 0)
				++x;
		}
	}
}


static void draw_line_smooth_shader(vec4 v1, vec4 v2, float* v1_out, float* v2_out, unsigned int provoke)
{
	float tmp;
	float* tmp_ptr;

	frag_func fragment_shader = c->programs.a[c->cur_program].fragment_shader;
	void* uniform = c->programs.a[c->cur_program].uniform;
	int frag_depth_used = c->programs.a[c->cur_program].use_frag_depth;

	vec3 hp1 = vec4_to_vec3h(v1);
	vec3 hp2 = vec4_to_vec3h(v2);
	float x1 = hp1.x, x2 = hp2.x, y1 = hp1.y, y2 = hp2.y;
	float z1 = hp1.z, z2 = hp2.z;

	float w1 = v1.w;
	float w2 = v2.w;

	int x, j;

	int steep = fabsf(y2 - y1) > fabsf(x2 - x1);

	if (steep) {
		tmp = x1;
		x1 = y1;
		y1 = tmp;
		tmp = x2;
		x2 = y2;
		y2 = tmp;
	}
	if (x1 > x2) {
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

	float dx = x2 - x1;
	float dy = y2 - y1;
	float gradient = dy / dx;

	float xend = x1 + 0.5f;
	float yend = y1 + gradient * (xend - x1);

	float xgap = 1.0 - modff(x1 + 0.5, &tmp);
	float xpxl1 = xend;
	float ypxl1;
	modff(yend, &ypxl1);


	//choose to compare against just one pixel for depth test instead of both
	z1 = MAP(z1, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	if (steep) {
		if (!c->depth_test || (!frag_depth_used &&
			depthtest(z1, ((float*)c->zbuf.lastrow)[-(int)xpxl1*c->zbuf.w + (int)ypxl1]))) {

			if (!c->frag_depth_used && c->depth_test) { //hate this double check but depth buf is only update if enabled
				((float*)c->zbuf.lastrow)[-(int)xpxl1*c->zbuf.w + (int)ypxl1] = z1;
				((float*)c->zbuf.lastrow)[-(int)xpxl1*c->zbuf.w + (int)(ypxl1+1)] = z1;
			}

			SET_VEC4(c->builtins.gl_FragCoord, ypxl1, xpxl1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = (1.0 - modff(yend, &tmp)) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl1, xpxl1);

			SET_VEC4(c->builtins.gl_FragCoord, ypxl1+1, xpxl1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl1+1, xpxl1);
		}
	} else {
		if (!c->depth_test || (!frag_depth_used &&
			depthtest(z1, ((float*)c->zbuf.lastrow)[-(int)ypxl1*c->zbuf.w + (int)xpxl1]))) {

			if (!c->frag_depth_used && c->depth_test) { //hate this double check but depth buf is only update if enabled
				((float*)c->zbuf.lastrow)[-(int)ypxl1*c->zbuf.w + (int)xpxl1] = z1;
				((float*)c->zbuf.lastrow)[-(int)(ypxl1+1)*c->zbuf.w + (int)xpxl1] = z1;
			}

			SET_VEC4(c->builtins.gl_FragCoord, xpxl1, ypxl1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = (1.0 - modff(yend, &tmp)) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl1, ypxl1);

			SET_VEC4(c->builtins.gl_FragCoord, xpxl1, ypxl1+1, z1, 1/w1);
			setup_fs_input(0, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl1, ypxl1+1);
		}
	}


	float intery = yend + gradient; //first y-intersection for main loop


	xend = x2 + 0.5f;
	yend = y2 + gradient * (xend - x2);

	xgap = modff(x2 + 0.5, &tmp);
	float xpxl2 = xend;
	float ypxl2;
	modff(yend, &ypxl2);

	z2 = MAP(z2, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);
	if (steep) {
		if (!c->depth_test || (!frag_depth_used &&
			depthtest(z2, ((float*)c->zbuf.lastrow)[-(int)xpxl2*c->zbuf.w + (int)ypxl2]))) {

			if (!c->frag_depth_used && c->depth_test) {
				((float*)c->zbuf.lastrow)[-(int)xpxl2*c->zbuf.w + (int)ypxl2] = z2;
				((float*)c->zbuf.lastrow)[-(int)xpxl2*c->zbuf.w + (int)(ypxl2+1)] = z2;
			}

			SET_VEC4(c->builtins.gl_FragCoord, ypxl2, xpxl2, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = (1.0 - modff(yend, &tmp)) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl2, xpxl2);

			SET_VEC4(c->builtins.gl_FragCoord, ypxl2+1, xpxl2, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, ypxl2+1, xpxl2);
		}

	} else {
		if (!c->depth_test || (!frag_depth_used &&
			depthtest(z2, ((float*)c->zbuf.lastrow)[-(int)ypxl2*c->zbuf.w + (int)xpxl2]))) {

			if (!c->frag_depth_used && c->depth_test) {
				((float*)c->zbuf.lastrow)[-(int)ypxl2*c->zbuf.w + (int)xpxl2] = z2;
				((float*)c->zbuf.lastrow)[-(int)(ypxl2+1)*c->zbuf.w + (int)xpxl2] = z2;
			}

			SET_VEC4(c->builtins.gl_FragCoord, xpxl2, ypxl2, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = (1.0 - modff(yend, &tmp)) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl2, ypxl2);

			SET_VEC4(c->builtins.gl_FragCoord, xpxl2, ypxl2+1, z2, 1/w2);
			setup_fs_input(1, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(yend, &tmp) * xgap;
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, xpxl2, ypxl2+1);
		}
	}

	//use the fast, inaccurate calculation of t since this algorithm is already
	//slower than the normal line drawing, pg 111 glspec if I ever want to fix it
	float range = ceil(x2-x1);
	float t, z, w;
	for (j=1, x = xpxl1 + 1; x < xpxl2; ++x, ++j, intery += gradient) {
		t = j/range;

		z = (1 - t) * z1 + t * z2;
		w = (1 - t) * w1 + t * w2;

		if (steep) {
			if (!c->frag_depth_used && c->depth_test) {
				if (!depthtest(z, ((float*)c->zbuf.lastrow)[-(int)x*c->zbuf.w + (int)intery])) {
					continue;
				} else {
					((float*)c->zbuf.lastrow)[-(int)x*c->zbuf.w + (int)intery] = z;
					((float*)c->zbuf.lastrow)[-(int)x*c->zbuf.w + (int)(intery+1)] = z;
				}
			}

			SET_VEC4(c->builtins.gl_FragCoord, intery, x, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = 1.0 - modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, intery, x);

			SET_VEC4(c->builtins.gl_FragCoord, intery+1, x, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, intery+1, x);

		} else {
			if (!c->frag_depth_used && c->depth_test) {
				if (!depthtest(z, ((float*)c->zbuf.lastrow)[-(int)intery*c->zbuf.w + (int)x])) {
					continue;
				} else {
					((float*)c->zbuf.lastrow)[-(int)intery*c->zbuf.w + (int)x] = z;
					((float*)c->zbuf.lastrow)[-(int)(intery+1)*c->zbuf.w + (int)x] = z;
				}
			}

			SET_VEC4(c->builtins.gl_FragCoord, x, intery, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = 1.0 - modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, intery);

			SET_VEC4(c->builtins.gl_FragCoord, x, intery+1, z, 1/w);
			setup_fs_input(t, v1_out, v2_out, w1, w2, provoke);
			fragment_shader(c->fs_input, &c->builtins, uniform);
			//fragcolor.w = modff(intery, &tmp);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, intery+1);

		}
	}
}


static void draw_triangle(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke)
{
	int c_or, c_and;

	c_and = v0->clip_code & v1->clip_code & v2->clip_code;
	if (c_and != 0) {
		//printf("triangle outside\n");
		return;
	}

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
	v0->screen_space = mult_mat4_vec4(c->vp_mat, v0->clip_space);
	v1->screen_space = mult_mat4_vec4(c->vp_mat, v1->clip_space);
	v2->screen_space = mult_mat4_vec4(c->vp_mat, v2->clip_space);

	front_facing = is_front_facing(v0, v1, v2);
	if (c->cull_face) {
		if (c->cull_mode == GL_FRONT_AND_BACK)
			return;
		if (c->cull_mode == GL_BACK && !front_facing) {
			//printf("culling back face\n");
			return;
		}
		if (c->cull_mode == GL_FRONT && front_facing)
			return;
	}

	c->builtins.gl_FrontFacing = front_facing;

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
		//why is this correct for both SMOOTH and NOPERSPECTIVE?
		q->vs_out[i] = v0->vs_out[i] + (v1->vs_out[i] - v0->vs_out[i]) * t;

		//FLAT should be handled indirectly by the provoke index
		//nothing to do here unless I change that
	}
	
	q->clip_code = gl_clipcode(q->clip_space);
	/*
	 * this is done in draw_triangle currently ...
	q->screen_space = mult_mat4_vec4(c->vp_mat, q->clip_space);
	if (q->clip_code == 0)
		q->screen_space = mult_mat4_vec4(c->vp_mat, q->clip_space);
		*/
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
	print_vec4(v0->clip_space, "\n");
	print_vec4(v1->clip_space, "\n");
	print_vec4(v2->clip_space, "\n");
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
		while (clip_bit < 6 && (c_or & (1 << clip_bit)) == 0)  {
			++clip_bit;
		}

		/* this test can be true only in case of rounding errors */
		if (clip_bit == 6) {
#if 1
			printf("Error:\n");
			print_vec4(v0->clip_space, "\n");
			print_vec4(v1->clip_space, "\n");
			print_vec4(v2->clip_space, "\n");
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

			tmp2.edge_flag = 1;
			tmp1.edge_flag = 0;
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

			tmp1.edge_flag = 1;
			tmp2.edge_flag = q[2]->edge_flag;
			draw_triangle_clip(q[0], &tmp1, &tmp2, provoke, clip_bit+1);
		}
	}
}

static void draw_triangle_point(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke)
{
	//TODO provoke?
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	vec3 point;
	glVertex* vert[3] = { v0, v1, v2 };

	for (int i=0; i<3; ++i) {
		if (!vert[i]->edge_flag) //TODO doesn't work
			continue;

		point = vec4_to_vec3h(vert[i]->screen_space);
		point.z = MAP(point.z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far);

		//TODO not sure if I'm supposed to do this ... doesn't say to in spec but it is called depth clamping
		if (c->depth_clamp)
			point.z = clampf_01(point.z);

		if (c->depth_test && !c->frag_depth_used) {
			if (point.z > ((float*)c->zbuf.lastrow)[-(int)point.y*c->zbuf.w + (int)point.x]) {
				return;
			} else {
				((float*)c->zbuf.lastrow)[-(int)point.y*c->zbuf.w + (int)point.x] = point.z;
			}
		}
		for (int j=0; j<c->vs_output.size; ++j) {
			if (c->vs_output.interpolation[j] != FLAT) {
				fs_input[j] = vert[i]->vs_out[j]; //would be correct from clipping
			} else {
				fs_input[j] = c->vs_output.output_buf.a[provoke*c->vs_output.size + j];
			}
		}

		c->builtins.discard = GL_FALSE;
		c->programs.a[c->cur_program].fragment_shader(fs_input, &c->builtins, c->programs.a[c->cur_program].uniform);
		if (!c->builtins.discard)
			draw_pixel(c->builtins.gl_FragColor, point.x, point.y);
	}
}

static void draw_triangle_line(glVertex* v0, glVertex* v1,  glVertex* v2, unsigned int provoke)
{
	if (v0->edge_flag)
		draw_line_shader(v0->screen_space, v1->screen_space, v0->vs_out, v1->vs_out, provoke);
	if (v1->edge_flag)
		draw_line_shader(v1->screen_space, v2->screen_space, v1->vs_out, v2->vs_out, provoke);
	if (v2->edge_flag)
		draw_line_shader(v2->screen_space, v0->screen_space, v2->vs_out, v0->vs_out, provoke);
}


static void draw_triangle_fill(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke)
{
	vec4 p0 = v0->screen_space;
	vec4 p1 = v1->screen_space;
	vec4 p2 = v2->screen_space;

	vec3 hp0 = vec4_to_vec3h(p0);
	vec3 hp1 = vec4_to_vec3h(p1);
	vec3 hp2 = vec4_to_vec3h(p2);

	/*
	print_vec4(hp0, "\n");
	print_vec4(hp1, "\n");
	print_vec4(hp2, "\n");

	printf("%f %f %f\n", p0.w, p1.w, p2.w);
	print_vec3(hp0, "\n");
	print_vec3(hp1, "\n");
	print_vec3(hp2, "\n\n");
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



	/*
	 * testing without this
	x_min = MAX(0, x_min);
	x_max = MIN(c->back_buffer.w-1, x_max);
	y_min = MAX(0, y_min);
	y_max = MIN(c->back_buffer.h-1, y_max);

	x_min = MAX(c->x_min, x_min);
	x_max = MIN(c->x_max, x_max);
	y_min = MAX(c->y_min, y_min);
	y_max = MIN(c->y_max, y_max);
	*/

	//form implicit lines
	Line l01 = make_Line(hp0.x, hp0.y, hp1.x, hp1.y);
	Line l12 = make_Line(hp1.x, hp1.y, hp2.x, hp2.y);
	Line l20 = make_Line(hp2.x, hp2.y, hp0.x, hp0.y);

	float alpha, beta, gamma, tmp, tmp2, z;
	float fs_input[GL_MAX_VERTEX_OUTPUT_COMPONENTS];
	float perspective[GL_MAX_VERTEX_OUTPUT_COMPONENTS*3];
	float* vs_output = &c->vs_output.output_buf.a[0];

	for (int i=0; i<c->vs_output.size; ++i) {
		perspective[i] = v0->vs_out[i]/p0.w;
		perspective[GL_MAX_VERTEX_OUTPUT_COMPONENTS + i] = v1->vs_out[i]/p1.w;
		perspective[2*GL_MAX_VERTEX_OUTPUT_COMPONENTS + i] = v2->vs_out[i]/p2.w;
	}
	float inv_w0 = 1/p0.w;  //is this worth it?  faster than just dividing by w down below?
	float inv_w1 = 1/p1.w;
	float inv_w2 = 1/p2.w;

	float x;
	float y = floor(y_min) + 0.5; //center of min pixel

	for (; y<=y_max; ++y) {
		x = floor(x_min) + 0.5; //center of min pixel

		for (; x<=x_max; ++x) {
			//see page 117 of glspec for alternative method
			gamma = line_func(&l01, x, y)/line_func(&l01, hp2.x, hp2.y);
			beta = line_func(&l20, x, y)/line_func(&l20, hp1.x, hp1.y);
			alpha = 1 - beta - gamma;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l12, hp0.x, hp0.y) * line_func(&l12, -1, -1) > 0) &&
				    (beta  > 0 || line_func(&l20, hp1.x, hp1.y) * line_func(&l20, -1, -1) > 0) &&
				    (gamma > 0 || line_func(&l01, hp2.x, hp2.y) * line_func(&l01, -1, -1) > 0)) {
					//calculate interoplation here
					tmp2 = alpha*inv_w0 + beta*inv_w1 + gamma*inv_w2;

					z = alpha * hp0.z + beta * hp1.z + gamma * hp2.z;

					z = MAP(z, -1.0f, 1.0f, c->depth_range_near, c->depth_range_far); //TODO move out (ie can I map hp1.z etc.)?

					//technically depth test and scissoring should be done after the fragment shader
					//but that's a lot of uneccessary work if it fails
					if (!c->frag_depth_used && c->depth_test) {
						if (!depthtest(z, ((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x])) {
							//printf("depth fail %f %f\n", z, ((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x]);
							continue;
						} else {
							((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x] = z;
						}
					}

					for (int i=0; i<c->vs_output.size; ++i) {
						if (c->vs_output.interpolation[i] == SMOOTH) {
							tmp = alpha*perspective[i] + beta*perspective[GL_MAX_VERTEX_OUTPUT_COMPONENTS + i] + gamma*perspective[2*GL_MAX_VERTEX_OUTPUT_COMPONENTS + i];

							fs_input[i] = tmp/tmp2;

						} else if (c->vs_output.interpolation[i] == NOPERSPECTIVE) {
							fs_input[i] = alpha * v0->vs_out[i] + beta * v1->vs_out[i] + gamma * v2->vs_out[i];
						} else { // == FLAT
							fs_input[i] = vs_output[provoke*c->vs_output.size + i];
						}
					}

					SET_VEC4(c->builtins.gl_FragCoord, x, y, z, 1);
					c->builtins.discard = GL_FALSE;
					c->programs.a[c->cur_program].fragment_shader(fs_input, &c->builtins, c->programs.a[c->cur_program].uniform);
					if (!c->builtins.discard)
						draw_pixel(c->builtins.gl_FragColor, x, y);
				}
			}
		}
	}
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	if (sfactor < GL_ZERO || sfactor >= NUM_BLEND_FUNCS || dfactor < GL_ZERO || dfactor >= NUM_BLEND_FUNCS) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	c->blend_sfactor = sfactor;
	c->blend_dfactor = dfactor;
}

void glBlendEquation(GLenum mode)
{
	if (mode < GL_FUNC_ADD || mode >= NUM_BLEND_EQUATIONS ) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;

		return;
	}

	c->blend_equation = mode;
}

void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	SET_VEC4(c->blend_color, clampf_01(red), clampf_01(green), clampf_01(blue), clampf_01(alpha));
}

static Color blend_pixel(vec4 src, vec4 dst)
{
	vec4* cnst = &c->blend_color;
	float i = MIN(src.w, 1-dst.w);

	vec4 Cs, Cd;

	switch (c->blend_sfactor) {
	case GL_ZERO:                     SET_VEC4(Cs, 0,0,0,0);                                 break;
	case GL_ONE:                      SET_VEC4(Cs, 1,1,1,1);                                 break;
	case GL_SRC_COLOR:                Cs = src;                                              break;
	case GL_ONE_MINUS_SRC_COLOR:      SET_VEC4(Cs, 1-src.x,1-src.y,1-src.z,1-src.w);         break;
	case GL_DST_COLOR:                Cs = dst;                                              break;
	case GL_ONE_MINUS_DST_COLOR:      SET_VEC4(Cs, 1-dst.x,1-dst.y,1-dst.z,1-dst.w);         break;
	case GL_SRC_ALPHA:                SET_VEC4(Cs, src.w, src.w, src.w, src.w);              break;
	case GL_ONE_MINUS_SRC_ALPHA:      SET_VEC4(Cs, 1-src.w,1-src.w,1-src.w,1-src.w);         break;
	case GL_DST_ALPHA:                SET_VEC4(Cs, dst.w, dst.w, dst.w, dst.w);              break;
	case GL_ONE_MINUS_DST_ALPHA:      SET_VEC4(Cs, 1-dst.w,1-dst.w,1-dst.w,1-dst.w);         break;
	case GL_CONSTANT_COLOR:           Cs = *cnst;                                            break;
	case GL_ONE_MINUS_CONSTANT_COLOR: SET_VEC4(Cs, 1-cnst->x,1-cnst->y,1-cnst->z,1-cnst->w); break;
	case GL_CONSTANT_ALPHA:           SET_VEC4(Cs, cnst->w, cnst->w, cnst->w, cnst->w);      break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: SET_VEC4(Cs, 1-cnst->w,1-cnst->w,1-cnst->w,1-cnst->w); break;

	case GL_SRC_ALPHA_SATURATE:       SET_VEC4(Cs, i, i, i, 1);                              break;
	/*not implemented yet
	 * won't be until I implement dual source blending/dual output from frag shader
	 *https://www.opengl.org/wiki/Blending#Dual_Source_Blending
	case GL_SRC1_COLOR:               Cs =  break;
	case GL_ONE_MINUS_SRC1_COLOR:     Cs =  break;
	case GL_SRC1_ALPHA:               Cs =  break;
	case GL_ONE_MINUS_SRC1_ALPHA:     Cs =  break;
	*/
	default:
		//should never get here
		printf("error unrecognized blend_sfactor!\n");
		break;
	}

	switch (c->blend_dfactor) {
	case GL_ZERO:                     SET_VEC4(Cd, 0,0,0,0);                                 break;
	case GL_ONE:                      SET_VEC4(Cd, 1,1,1,1);                                 break;
	case GL_SRC_COLOR:                Cd = src;                                              break;
	case GL_ONE_MINUS_SRC_COLOR:      SET_VEC4(Cd, 1-src.x,1-src.y,1-src.z,1-src.w);         break;
	case GL_DST_COLOR:                Cd = dst;                                              break;
	case GL_ONE_MINUS_DST_COLOR:      SET_VEC4(Cd, 1-dst.x,1-dst.y,1-dst.z,1-dst.w);         break;
	case GL_SRC_ALPHA:                SET_VEC4(Cd, src.w, src.w, src.w, src.w);              break;
	case GL_ONE_MINUS_SRC_ALPHA:      SET_VEC4(Cd, 1-src.w,1-src.w,1-src.w,1-src.w);         break;
	case GL_DST_ALPHA:                SET_VEC4(Cd, dst.w, dst.w, dst.w, dst.w);              break;
	case GL_ONE_MINUS_DST_ALPHA:      SET_VEC4(Cd, 1-dst.w,1-dst.w,1-dst.w,1-dst.w);         break;
	case GL_CONSTANT_COLOR:           Cd = *cnst;                                            break;
	case GL_ONE_MINUS_CONSTANT_COLOR: SET_VEC4(Cd, 1-cnst->x,1-cnst->y,1-cnst->z,1-cnst->w); break;
	case GL_CONSTANT_ALPHA:           SET_VEC4(Cd, cnst->w, cnst->w, cnst->w, cnst->w);      break;
	case GL_ONE_MINUS_CONSTANT_ALPHA: SET_VEC4(Cd, 1-cnst->w,1-cnst->w,1-cnst->w,1-cnst->w); break;

	case GL_SRC_ALPHA_SATURATE:       SET_VEC4(Cd, i, i, i, 1);                              break;
	/*not implemented yet
	case GL_SRC_ALPHA_SATURATE:       Cd =  break;
	case GL_SRC1_COLOR:               Cd =  break;
	case GL_ONE_MINUS_SRC1_COLOR:     Cd =  break;
	case GL_SRC1_ALPHA:               Cd =  break;
	case GL_ONE_MINUS_SRC1_ALPHA:     Cd =  break;
	*/
	default:
		//should never get here
		printf("error unrecognized blend_dfactor!\n");
		break;
	}

	vec4 result;

	switch (c->blend_equation) {
	case GL_FUNC_ADD:
		result = add_vec4s(mult_vec4s(Cs, src), mult_vec4s(Cd, dst));
		break;
	case GL_FUNC_SUBTRACT:
		result = sub_vec4s(mult_vec4s(Cs, src), mult_vec4s(Cd, dst));
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		result = sub_vec4s(mult_vec4s(Cd, dst), mult_vec4s(Cs, src));
		break;
	case GL_MIN:
		SET_VEC4(result, MIN(src.x, dst.x), MIN(src.y, dst.y), MIN(src.z, dst.z), MIN(src.w, dst.w));
		break;
	case GL_MAX:
		SET_VEC4(result, MAX(src.x, dst.x), MAX(src.y, dst.y), MAX(src.z, dst.z), MAX(src.w, dst.w));
		break;
	default:
		//should never get here
		printf("error unrecognized blend_equation!\n");
		break;
	}

	return vec4_to_Color(result);
}



static void draw_pixel_vec2(vec4 cf, vec2 pos)
{
/*
 * spec pg 110:
Point rasterization produces a fragment for each framebuffer pixel whose center
lies inside a square centered at the point’s (x w , y w ), with side length equal to the
current point size.

for a 1 pixel size point there are only 3 edge cases where more than 1 pixel center (0.5, 0.5)
would fall on the very edge of a 1 pixel square.  I think just drawing the upper or upper
corner pixel in these cases is fine and makes sense since width and height are actually 0.01 less
than full, see make_viewport_matrix
TODO point size > 1
*/

	draw_pixel(cf, pos.x, pos.y);
}


static void draw_pixel(vec4 cf, int x, int y)
{
	//MSAA
	//Stencil Test

	//Depth test if necessary
	if (c->frag_depth_used && c->depth_test) {
		if (depthtest(c->builtins.gl_FragDepth, ((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x])) {
			((float*)c->zbuf.lastrow)[-(int)y*c->zbuf.w + (int)x] = c->builtins.gl_FragDepth;
		} else {
			return;
		}
	}

	//Blending
	Color color;
	u32* dest = &((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	color = make_Color((*dest & c->Rmask) >> c->Rshift, (*dest & c->Gmask) >> c->Gshift, (*dest & c->Bmask) >> c->Bshift, (*dest & c->Amask) >> c->Ashift);

	if (c->blend) {
		//TODO clamp in blend_pixel?  return the vec4 and clamp?
		color = blend_pixel(cf, Color_to_vec4(color));
	} else {
		cf.x = clampf_01(cf.x);
		cf.y = clampf_01(cf.y);
		cf.z = clampf_01(cf.z);
		cf.w = clampf_01(cf.w);
		color = vec4_to_Color(cf);
	}
	//this line neded the negation in the viewport matrix
	//((u32*)c->back_buffer.buf)[y*buf.w+x] = c.a << 24 | c.c << 16 | c.g << 8 | c.b;

	//Logic Ops
	//Dithering

	//((u32*)c->back_buffer.buf)[(buf.h-1-y)*buf.w + x] = c.a << 24 | c.c << 16 | c.g << 8 | c.b;
	//((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x] = c.a << 24 | c.c << 16 | c.g << 8 | c.b;
	*dest = color.a << c->Ashift | color.r << c->Rshift | color.g << c->Gshift | color.b << c->Bshift;
}


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

//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
void pglDrawFrame()
{
	frag_func frag_shader = c->programs.a[c->cur_program].fragment_shader;

	for (float y=0.5; y<c->back_buffer.h; ++y) {
		for (float x=0.5; x<c->back_buffer.w; ++x) {

			//ignore z and w components
			c->builtins.gl_FragCoord.x = x;
			c->builtins.gl_FragCoord.y = y;

			c->builtins.discard = GL_FALSE;
			frag_shader(NULL, &c->builtins, c->programs.a[c->cur_program].uniform);
			if (!c->builtins.discard)
				draw_pixel(c->builtins.gl_FragColor, x, y);
		}
	}

}



void put_pixel(Color color, int x, int y)
{
	u32* dest = &((u32*)c->back_buffer.lastrow)[-y*c->back_buffer.w + x];
	*dest = color.a << c->Ashift | color.r << c->Rshift | color.g << c->Gshift | color.b << c->Bshift;
}

//Should I have it take a glFramebuffer as paramater?
void put_line(Color the_color, float x1, float y1, float x2, float y2)
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
	float A = y1 - y2;
	float B = x2 - x1;
	float C = x1*y2 -x2*y1;

	int x, y;

	float x_min = MAX(0, MIN(x1, x2));
	float x_max = MIN(c->back_buffer.w-1, MAX(x1, x2));
	float y_min = MAX(0, MIN(y1, y2));
	float y_max = MIN(c->back_buffer.h-1, MAX(y1, y2));
	
	//4 cases based on slope
	if (m <= -1) {			//(-infinite, -1]
		x = x1;
		for (y=y_max; y>=y_min; --y) {
			put_pixel(the_color, x, y);
			if (A*(x+0.5f) + B*(y-1) + C < 0)
				x++;
		}
	} else if (m <= 0) {	//(-1, 0]
		y = y1;
		for (x=x_min; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (A*(x+1) + B*(y-0.5f) + C > 0)
				y--;
		}
	} else if (m <= 1) {	//(0, 1]
		y = y1;
		for (x=x_min; x<=x_max; ++x) {
			put_pixel(the_color, x, y);
			if (A*(x+1) + B*(y+0.5f) + C < 0)
				y++;
		}
		
	} else {				//(1, +infinite)
		x = x1;
		for (y=y_min; y<=y_max; ++y) {
			put_pixel(the_color, x, y);
			if (A*(x+0.5f) + B*(y+1) + C > 0)
				x++;
		}
	}
}

void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3)
{
	//can't think of a better/cleaner way to do this than these 8 lines
	float x_min = MIN(floor(p1.x), floor(p2.x));
	float x_max = MAX(ceil(p1.x), ceil(p2.x));
	float y_min = MIN(floor(p1.y), floor(p2.y));
	float y_max = MAX(ceil(p1.y), ceil(p2.y));
	
	x_min = MIN(floor(p3.x), x_min);
	x_max = MAX(ceil(p3.x),  x_max);
	y_min = MIN(floor(p3.y), y_min);
	y_max = MAX(ceil(p3.y),  y_max);

	x_min = MAX(0, x_min);
	x_max = MIN(c->back_buffer.w-1, x_max);
	y_min = MAX(0, y_min);
	y_max = MIN(c->back_buffer.h-1, y_max);
	
	//form implicit lines
	Line l12 = make_Line(p1.x, p1.y, p2.x, p2.y);
	Line l23 = make_Line(p2.x, p2.y, p3.x, p3.y);
	Line l31 = make_Line(p3.x, p3.y, p1.x, p1.y);
	
	float alpha, beta, gamma;
	Color c;

	float x, y;
	//y += 0.5f; //center of pixel
	
	// TODO(rswinkle): floor(  + 0.5f) like draw_triangle?
	for (y=y_min; y<=y_max; ++y) {
		for (x=x_min; x<=x_max; ++x) {
			gamma = line_func(&l12, x, y)/line_func(&l12, p3.x, p3.y);
			beta = line_func(&l31, x, y)/line_func(&l31, p2.x, p2.y);
			alpha = 1 - beta - gamma;
			
			if (alpha >= 0 && beta >= 0 && gamma >= 0)
				//if it's on the edge (==0), draw if the opposite vertex is on the same side as arbitrary point -1, -1
				//this is a deterministic way of choosing which triangle gets a pixel for trinagles that share
				//edges
				if ((alpha > 0 || line_func(&l23, p1.x, p1.y) * line_func(&l23, -1, -1) > 0) &&
					(beta >  0 || line_func(&l31, p2.x, p2.y) * line_func(&l31, -1, -1) > 0) &&
					(gamma > 0 || line_func(&l12, p3.x, p3.y) * line_func(&l12, -1, -1) > 0)) {
					//calculate interoplation here
						c.r = alpha*c1.r + beta*c2.r + gamma*c3.r;
						c.g = alpha*c1.g + beta*c2.g + gamma*c3.g;
						c.b = alpha*c1.b + beta*c2.b + gamma*c3.b;
						put_pixel(c, x, y);
				}
		}
	}
}








/*************************************
 *  GLSL(ish) functions
 *************************************/

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





// Stubs to let real OpenGL libs compile with minimal modifications/ifdefs
// add what you need

void glGetProgramiv(GLuint program, GLenum pname, GLint* params) { }
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog) { }
void glAttachShader(GLuint program, GLuint shader) { }
void glCompileShader(GLuint shader) { }
void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog) { }
void glLinkProgram(GLuint program) { }
void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length) { }
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) { }
void glDeleteShader(GLuint shader) { }

GLuint glCreateProgram() { return 0; }
GLuint glCreateShader(GLenum shaderType) { return 0; }
GLint glGetUniformLocation(GLuint program, const GLchar* name) { return 0; }

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params) { }

void glUniform1f(GLint location, GLfloat v0) { }
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) { }
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) { }
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) { }

void glUniform1i(GLint location, GLint v0) { }
void glUniform2i(GLint location, GLint v0, GLint v1) { }
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) { }
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) { }

void glUniform1ui(GLuint location, GLuint v0) { }
void glUniform2ui(GLuint location, GLuint v0, GLuint v1) { }
void glUniform3ui(GLuint location, GLuint v0, GLuint v1, GLuint v2) { }
void glUniform4ui(GLuint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) { }

void glUniform1fv(GLint location, GLsizei count, const GLfloat* value) { }
void glUniform2fv(GLint location, GLsizei count, const GLfloat* value) { }
void glUniform3fv(GLint location, GLsizei count, const GLfloat* value) { }
void glUniform4fv(GLint location, GLsizei count, const GLfloat* value) { }

void glUniform1iv(GLint location, GLsizei count, const GLint* value) { }
void glUniform2iv(GLint location, GLsizei count, const GLint* value) { }
void glUniform3iv(GLint location, GLsizei count, const GLint* value) { }
void glUniform4iv(GLint location, GLsizei count, const GLint* value) { }

void glUniform1uiv(GLint location, GLsizei count, const GLuint* value) { }
void glUniform2uiv(GLint location, GLsizei count, const GLuint* value) { }
void glUniform3uiv(GLint location, GLsizei count, const GLuint* value) { }
void glUniform4uiv(GLint location, GLsizei count, const GLuint* value) { }

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { }

























/**********Non-GL type vectors ***********************
 *
 *
 *
 ***********************************************
 */

CVEC_NEW_DEFS(float, RESIZE)
CVEC_NEW_DEFS(vec3, RESIZE)
CVEC_NEW_DEFS(vec4, RESIZE)


