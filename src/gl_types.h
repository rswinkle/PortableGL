


#include <stdint.h>

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

//they use plain int not unsigned like you'd think
// TODO(rswinkle) just use uint32_t, remove all checks for < 0 and
// use for all offset/index type parameters (other than
// VertexAttribPointer because I already folded on that and have
// the pgl macro wrapper)
typedef int32_t   GLsizei;

typedef int32_t   GLenum;
typedef uint32_t  GLbitfield;

typedef intptr_t  GLintptr;
typedef uintptr_t GLsizeiptr;
typedef void      GLvoid;

typedef float     GLfloat;
typedef float     GLclampf;

// not used
typedef double    GLdouble;
typedef double    GLclampd;



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
	GL_REPEAT,
	GL_CLAMP_TO_EDGE,
	GL_CLAMP_TO_BORDER,  // not supported, alias to CLAMP_TO_EDGE
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
#define PGL_MAX_VERTICES 500000
#define GL_MAX_VERTEX_ATTRIBS 8
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS (4*GL_MAX_VERTEX_ATTRIBS)
#define GL_MAX_DRAW_BUFFERS 4
#define GL_MAX_COLOR_ATTACHMENTS 4

//TODO use prefix like GL_SMOOTH?  PGL_SMOOTH?
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
	vec4 gl_FragColor;
	//vec4 gl_FragData[GL_MAX_DRAW_BUFFERS];
	float gl_FragDepth;
	GLboolean discard;

} Shader_Builtins;

typedef void (*vert_func)(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
typedef void (*frag_func)(float* fs_input, Shader_Builtins* builtins, void* uniforms);

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

void init_glVertex_Attrib(glVertex_Attrib* v);
//void init_glVertex_Attrib(glVertex_Attrib* v, GLint size, GLenum type, GLsizei stride, GLsizei offset, GLboolean normalized, Buffer* buf);


typedef struct glVertex_Array
{
	glVertex_Attrib vertex_attribs[GL_MAX_VERTEX_ATTRIBS];

	//GLuint n_array_bufs;
	GLuint element_buffer;
	GLboolean deleted;

} glVertex_Array;

void init_glVertex_Array(glVertex_Array* v);


typedef struct glTexture
{
	GLsizei w;
	GLsizei h;
	GLsizei d;

	//GLint base_level;  // Not used
	//vec4 border_color; // I no longer support borders, not worth it
	GLenum mag_filter;
	GLenum min_filter;
	GLenum wrap_s;
	GLenum wrap_t;
	GLenum wrap_r;

	// TODO?
	//GLenum datatype; // only support GL_UNSIGNED_BYTE so not worth having yet
	GLenum format; // GL_RED, GL_RG, GL_RGB/BGR, GL_RGBA/BGRA
	
	GLenum type; // GL_TEXTURE_UNBOUND, GL_TEXTURE_2D etc.

	GLboolean deleted;

	// TODO same meaning as in glBuffer
	GLboolean user_owned;

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
	cvector_float output_buf;
} Vertex_Shader_output;


typedef void (*draw_triangle_func)(glVertex* v0, glVertex* v1, glVertex* v2, unsigned int provoke);


