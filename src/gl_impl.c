

#include <stdarg.h>


/******************************************
 * PORTABLEGL_IMPLEMENTATION
 ******************************************/

#include <stdio.h>
#include <assert.h>
#include <float.h>

// for CHAR_BIT
#include <limits.h>



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


int init_glContext(glContext* context, u32** back, int w, int h, int bitdepth, u32 Rmask, u32 Gmask, u32 Bmask, u32 Amask)
{
	if (bitdepth > 32 || !back)
		return 0;

	if (!*back) {
		int bytes_per_pixel = (bitdepth + CHAR_BIT-1) / CHAR_BIT;
		*back = (u32*) malloc(w * h * bytes_per_pixel);
		if (!*back)
			return 0;
	}

	context->zbuf.buf = (u8*) malloc(w*h * sizeof(float));
	if (!context->zbuf.buf)
		return 0;

	context->x_min = 0;
	context->y_min = 0;
	context->x_max = w;
	context->y_max = h;

	context->zbuf.w = w;
	context->zbuf.h = h;
	context->zbuf.lastrow = context->zbuf.buf + (h-1)*w*sizeof(float);

	context->back_buffer.w = w;
	context->back_buffer.h = h;
	context->back_buffer.buf = (u8*) *back;
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

	//TODO might as well just set it to MAX_VERTICES * MAX_OUTPUT_COMPONENTS
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

	// According to refpages https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glPixelStore.xhtml
	context->unpack_alignment = 4;
	context->pack_alignment = 4;

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

	return 1;
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

		// NOTE(rswinkle): type is stored as correct index not the raw enum value so no need to
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

		// Note type isn't set till binding and we're not storing the raw
		// enum but the enum - GL_ARRAY_BUFFER so it's an index into c->bound_buffers
		// TODO need to see what's supposed to happen if you try to bind
		// a buffer to multiple targets
		c->buffers.a[buffer].type = target;
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
	if (pname != GL_UNPACK_ALIGNMENT && pname != GL_PACK_ALIGNMENT) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	if (param != 1 && param != 2 && param != 4 && param != 8) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	
	if (pname == GL_UNPACK_ALIGNMENT) {
		c->unpack_alignment = param;
	} else if (pname == GL_PACK_ALIGNMENT) {
		c->pack_alignment = param;
	}

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

	// TODO I don't actually support anything other than GL_RGBA for input or
	// internal format ... so I should probably make the others errors and
	// I'm not even checking internalFormat currently..
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

	// TODO If I ever support type other than GL_UNSIGNED_BYTE (also using for both internalformat and format)
	int byte_width = width * components;
	int padding_needed = byte_width % c->unpack_alignment;
	int padded_row_len = (!padding_needed) ? byte_width : byte_width + c->unpack_alignment - padding_needed;

	if (target == GL_TEXTURE_2D) {
		cur_tex = c->bound_textures[target-GL_TEXTURE_UNBOUND-1];

		c->textures.a[cur_tex].w = width;
		c->textures.a[cur_tex].h = height;

		// either NULL or valid
		free(c->textures.a[cur_tex].data);

		//TODO support other internal formats? components should be of internalformat not format
		if (!(c->textures.a[cur_tex].data = (u8*) malloc(height * byte_width))) {
			if (!c->error)
				c->error = GL_OUT_OF_MEMORY;
			//undefined state now
			return;
		}

		if (data) {
			if (!padding_needed) {
				memcpy(c->textures.a[cur_tex].data, data, height*byte_width);
			} else {
				for (int i=0; i<height; ++i) {
					memcpy(&c->textures.a[cur_tex].data[i*byte_width], &((u8*)data)[i*padded_row_len], byte_width);
				}
			}
		}

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

		// TODO handle different format and internalFormat
		int p = height*byte_width;
		u8* texdata = c->textures.a[cur_tex].data;

		if (data) {
			if (!padding_needed) {
				memcpy(&texdata[target*p], data, height*byte_width);
			} else {
				for (int i=0; i<height; ++i) {
					memcpy(&texdata[target*p + i*byte_width], &((u8*)data)[i*padded_row_len], byte_width);
				}
			}
		}

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


void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	// TODO should I just make GLsizei an uint32_t rather than int32_t?
	if (count < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count)
		return;

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
	// TODO should I just make GLsizei an uint32_t rather than int32_t?
	if (count < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count)
		return;

	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;
	run_pipeline(mode, offset, count, 0, 0, GL_TRUE);
}

void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//TODO check for buffer mapped when I implement that according to spec
	//still want to do my own special map function to mean just use the pointer
	
	if (count < 0 || instancecount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count || !instancecount)
		return;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, first, count, instance, 0, GL_FALSE);
	}
}

void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)
{
	if (mode < GL_POINTS || mode > GL_TRIANGLE_FAN) {
		if (!c->error)
			c->error = GL_INVALID_ENUM;
		return;
	}

	//TODO check for buffer mapped when I implement that according to spec
	//still want to do my own special map function to mean just use the pointer
	if (count < 0 || instancecount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count || !instancecount)
		return;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, first, count, instance, baseinstance, GL_FALSE);
	}
}


void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei instancecount)
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
	if (count < 0 || instancecount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count || !instancecount)
		return;

	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, offset, count, instance, 0, GL_TRUE);
	}
}

void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, GLsizei offset, GLsizei instancecount, GLuint baseinstance)
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
	if (count < 0 || instancecount < 0) {
		if (!c->error)
			c->error = GL_INVALID_VALUE;
		return;
	}
	if (!count || !instancecount)
		return;

	c->buffers.a[c->vertex_arrays.a[c->cur_vertex_array].element_buffer].type = type;

	for (unsigned int instance = 0; instance < instancecount; ++instance) {
		run_pipeline(mode, offset, count, instance, baseinstance, GL_TRUE);
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
		;
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
void glDetachShader(GLuint program, GLuint shader) { }

GLuint glCreateProgram() { return 0; }
GLuint glCreateShader(GLenum shaderType) { return 0; }
GLint glGetUniformLocation(GLuint program, const GLchar* name) { return 0; }

// TODO
void glLogicOp(GLenum opcode) { }
void glLineWidth(GLfloat width) { }
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) { }
void glPolygonOffset(GLfloat factor, GLfloat units) { }

void glActiveTexture(GLenum texture) { }
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


