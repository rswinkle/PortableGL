/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Ported to GLES2.
 * Kristian Høgsberg <krh@bitplanet.net>
 * May 3, 2010
 * 
 * Improve GLES2 port:
 *   * Refactor gear drawing.
 *   * Use correct normals for surfaces.
 *   * Improve shader.
 *   * Use perspective projection transformation.
 *   * Add FPS count.
 *   * Add comments.
 * Alexandros Frantzis <alexandros.frantzis@linaro.org>
 * Jul 13, 2010
 *
 * Ported to SDL2 and OpenGL 3.3 core
 *   * Fix up shaders
 *   * Add a VAO
 *   * Remove all glut/egl code replace with SDL2
 *   * Add polygon_mode toggle for fun
 *   * TODO: refactor/clean up more
 *   * repo at:         https://github.com/rswinkle/opengl_reference
 *   * original source: https://cgit.freedesktop.org/mesa/demos/tree/src/egl/opengles2/es2gears.c
 * Robert Winkler
 * April 9, 2016
 *
 * Ported to PortableGL
 * Robert Winkler
 * September 5, 2016
 */



//#define _GNU_SOURCE
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define PORTABLEGL_IMPLEMENTATION
//#define PGL_ARGB32
//#define PGL_RGB565
//#define PGL_RGBA5551
#include "portablegl.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>



// NOTE(rswinkle): ran into this lovely bug when I was working on this
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=46926
// Naming it sin_cos is a work around.  Making it a static function worked
// too.
void sin_cos(double x, double* s, double* c)
{
	//printf("%f\n%p\n%p\n", x, s, c);
	*s = sin(x);
	*c = cos(x);
	return;
}

#define sincos sin_cos



#ifndef M_PI
#define M_PI 3.14159265
#endif


#define WIDTH 640
#define HEIGHT 480

void cleanup(void);
void setup_context(void);
int handle_events(void);

SDL_Window* window;
SDL_Renderer* ren;
SDL_Texture* tex;
pix_t* bbufpix;

glContext the_context;

int polygon_mode;

typedef struct My_Uniforms
{
	mat4 mvp_mat;
	mat4 normal_mat;
	vec3 material_color;

} My_Uniforms;

#define STRIPS_PER_TOOTH 7
#define VERTICES_PER_TOOTH 34
#define GEAR_VERTEX_STRIDE 6

/**
 * Struct describing the vertices in triangle strip
 */
struct vertex_strip {
	/** The first vertex in the strip */
	GLint first;
	/** The number of consecutive vertices in the strip after the first */
	GLint count;
};

/* Each vertex consist of GEAR_VERTEX_STRIDE GLfloat attributes */
typedef GLfloat GearVertex[GEAR_VERTEX_STRIDE];

/**
 * Struct representing a gear.
 */
struct gear {
	/** The array of vertices comprising the gear */
	GearVertex *vertices;
	/** The number of vertices comprising the gear */
	int nvertices;
	/** The array of triangle strips comprising the gear */
	struct vertex_strip *strips;
	/** The number of triangle strips comprising the gear */
	int nstrips;
	/** The Vertex Buffer Object holding the vertices in the graphics card */
	GLuint vbo;
};

/** The view rotation [x, y, z] */
static GLfloat view_rot[3] = { 20.0, 30.0, 0.0 };
/** The gears */
static struct gear *gear1, *gear2, *gear3;
/** The current gear rotation angle */
static GLfloat angle = 0.0;


/** The projection matrix */
static GLfloat ProjectionMatrix[16];

/** The direction of the directional light for the scene */
//static const GLfloat LightSourcePosition[4] = { 5.0, 5.0, 10.0, 1.0};


static My_Uniforms uniforms;




/** 
 * Fills a gear vertex.
 * 
 * @param v the vertex to fill
 * @param x the x coordinate
 * @param y the y coordinate
 * @param z the z coortinate
 * @param n pointer to the normal table 
 * 
 * @return the operation error code
 */
static GearVertex *
vert(GearVertex *v, GLfloat x, GLfloat y, GLfloat z, GLfloat n[3])
{
	v[0][0] = x;
	v[0][1] = y;
	v[0][2] = z;
	v[0][3] = n[0];
	v[0][4] = n[1];
	v[0][5] = n[2];

	return v + 1;
}

/**
 *  Create a gear wheel.
 * 
 *  @param inner_radius radius of hole at center
 *  @param outer_radius radius at center of teeth
 *  @param width width of gear
 *  @param teeth number of teeth
 *  @param tooth_depth depth of tooth
 *  
 *  @return pointer to the constructed struct gear
 */
static struct gear *
create_gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
		GLint teeth, GLfloat tooth_depth)
{
	GLfloat r0, r1, r2;
	GLfloat da;
	GearVertex *v;
	struct gear *gear;
	double s[5], c[5];
	GLfloat normal[3];
	int cur_strip = 0;
	int i;

	/* Allocate memory for the gear */
	gear = malloc(sizeof *gear);
	if (gear == NULL)
		return NULL;

	/* Calculate the radii used in the gear */
	r0 = inner_radius;
	r1 = outer_radius - tooth_depth / 2.0;
	r2 = outer_radius + tooth_depth / 2.0;

	da = 2.0 * M_PI / teeth / 4.0;

	/* Allocate memory for the triangle strip information */
	gear->nstrips = STRIPS_PER_TOOTH * teeth;
	gear->strips = calloc(gear->nstrips, sizeof (*gear->strips));

	/* Allocate memory for the vertices */
	gear->vertices = calloc(VERTICES_PER_TOOTH * teeth, sizeof(*gear->vertices));
	v = gear->vertices;

	double sc_val;
	for (i = 0; i < teeth; i++) {
		/* Calculate needed sin/cos for varius angles */

		sc_val = i * 2.0 * M_PI / teeth;
		sincos(sc_val, &s[0], &c[0]);
		sincos(sc_val + da, &s[1], &c[1]);
		sincos(sc_val + da * 2, &s[2], &c[2]);
		sincos(sc_val + da * 3, &s[3], &c[3]);
		sincos(sc_val + da * 4, &s[4], &c[4]);

		/* A set of macros for making the creation of the gears easier */
#define  GEAR_POINT(r, da) { (r) * c[(da)], (r) * s[(da)] }
#define  SET_NORMAL(x, y, z) do { \
	normal[0] = (x); normal[1] = (y); normal[2] = (z); \
} while(0)

#define  GEAR_VERT(v, point, sign) vert((v), p[(point)].x, p[(point)].y, (sign) * width * 0.5, normal)

#define START_STRIP do { \
	gear->strips[cur_strip].first = v - gear->vertices; \
} while(0);

#define END_STRIP do { \
	int _tmp = (v - gear->vertices); \
	gear->strips[cur_strip].count = _tmp - gear->strips[cur_strip].first; \
	cur_strip++; \
} while (0)

#define QUAD_WITH_NORMAL(p1, p2) do { \
	SET_NORMAL((p[(p1)].y - p[(p2)].y), -(p[(p1)].x - p[(p2)].x), 0); \
	v = GEAR_VERT(v, (p1), -1); \
	v = GEAR_VERT(v, (p1), 1); \
	v = GEAR_VERT(v, (p2), -1); \
	v = GEAR_VERT(v, (p2), 1); \
} while(0)

		struct point {
			GLfloat x;
			GLfloat y;
		};

		/* Create the 7 points (only x,y coords) used to draw a tooth */
		struct point p[7] = {
			GEAR_POINT(r2, 1), // 0
			GEAR_POINT(r2, 2), // 1
			GEAR_POINT(r1, 0), // 2
			GEAR_POINT(r1, 3), // 3
			GEAR_POINT(r0, 0), // 4
			GEAR_POINT(r1, 4), // 5
			GEAR_POINT(r0, 4), // 6
		};

		/* Front face */
		START_STRIP;
		SET_NORMAL(0, 0, 1.0);
		v = GEAR_VERT(v, 0, +1);
		v = GEAR_VERT(v, 1, +1);
		v = GEAR_VERT(v, 2, +1);
		v = GEAR_VERT(v, 3, +1);
		v = GEAR_VERT(v, 4, +1);
		v = GEAR_VERT(v, 5, +1);
		v = GEAR_VERT(v, 6, +1);
		END_STRIP;

		/* Inner face */
		START_STRIP;
		QUAD_WITH_NORMAL(4, 6);
		END_STRIP;

		/* Back face */
		START_STRIP;
		SET_NORMAL(0, 0, -1.0);
		v = GEAR_VERT(v, 6, -1);
		v = GEAR_VERT(v, 5, -1);
		v = GEAR_VERT(v, 4, -1);
		v = GEAR_VERT(v, 3, -1);
		v = GEAR_VERT(v, 2, -1);
		v = GEAR_VERT(v, 1, -1);
		v = GEAR_VERT(v, 0, -1);
		END_STRIP;

		/* Outer face */
		START_STRIP;
		QUAD_WITH_NORMAL(0, 2);
		END_STRIP;

		START_STRIP;
		QUAD_WITH_NORMAL(1, 0);
		END_STRIP;

		START_STRIP;
		QUAD_WITH_NORMAL(3, 1);
		END_STRIP;

		START_STRIP;
		QUAD_WITH_NORMAL(5, 3);
		END_STRIP;
	}

	gear->nvertices = (v - gear->vertices);

	/* Store the vertices in a vertex buffer object (VBO) */
	glGenBuffers(1, &gear->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);
	glBufferData(GL_ARRAY_BUFFER, gear->nvertices * sizeof(GearVertex),
			gear->vertices, GL_STATIC_DRAW);

	return gear;
}

/** 
 * Multiplies two 4x4 matrices.
 * 
 * The result is stored in matrix m.
 * 
 * @param m the first matrix to multiply
 * @param n the second matrix to multiply
 */
static void
multiply(GLfloat *m, const GLfloat *n)
{
	GLfloat tmp[16];
	const GLfloat *row, *column;
	div_t d;
	int i, j;

	for (i = 0; i < 16; i++) {
		tmp[i] = 0;
		d = div(i, 4);
		row = n + d.quot * 4;
		column = m + d.rem;
		for (j = 0; j < 4; j++)
			tmp[i] += row[j] * column[j * 4];
	}
	memcpy(m, &tmp, sizeof tmp);
}

/** 
 * Rotates a 4x4 matrix.
 * 
 * @param[in,out] m the matrix to rotate
 * @param angle the angle to rotate
 * @param x the x component of the direction to rotate to
 * @param y the y component of the direction to rotate to
 * @param z the z component of the direction to rotate to
 */
static void
rotate(GLfloat *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	double s, c;

	sincos(angle, &s, &c);
	GLfloat r[16] = {
		x * x * (1 - c) + c,     y * x * (1 - c) + z * s, x * z * (1 - c) - y * s, 0,
		x * y * (1 - c) - z * s, y * y * (1 - c) + c,     y * z * (1 - c) + x * s, 0, 
		x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, z * z * (1 - c) + c,     0,
		0, 0, 0, 1
	};

	multiply(m, r);
}


/** 
 * Translates a 4x4 matrix.
 * 
 * @param[in,out] m the matrix to translate
 * @param x the x component of the direction to translate to
 * @param y the y component of the direction to translate to
 * @param z the z component of the direction to translate to
 */
static void
translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z)
{
	GLfloat t[16] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  x, y, z, 1 };

	multiply(m, t);
}

/** 
 * Creates an identity 4x4 matrix.
 * 
 * @param m the matrix make an identity matrix
 */
static void
identity(GLfloat *m)
{
	GLfloat t[16] = {
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0,
	};

	memcpy(m, t, sizeof(t));
}

/** 
 * Transposes a 4x4 matrix.
 *
 * @param m the matrix to transpose
 */
static void 
transpose(GLfloat *m)
{
	GLfloat t[16] = {
		m[0], m[4], m[8],  m[12],
		m[1], m[5], m[9],  m[13],
		m[2], m[6], m[10], m[14],
		m[3], m[7], m[11], m[15]};

	memcpy(m, t, sizeof(t));
}

/**
 * Inverts a 4x4 matrix.
 *
 * This function can currently handle only pure translation-rotation matrices.
 * Read http://www.gamedev.net/community/forums/topic.asp?topic_id=425118
 * for an explanation.
 */
static void
invert(GLfloat *m)
{
	GLfloat t[16];
	identity(t);

	// Extract and invert the translation part 't'. The inverse of a
	// translation matrix can be calculated by negating the translation
	// coordinates.
	t[12] = -m[12]; t[13] = -m[13]; t[14] = -m[14];

	// Invert the rotation part 'r'. The inverse of a rotation matrix is
	// equal to its transpose.
	m[12] = m[13] = m[14] = 0;
	transpose(m);

	// inv(m) = inv(r) * inv(t)
	multiply(m, t);
}

/** 
 * Calculate a perspective projection transformation.
 * 
 * @param m the matrix to save the transformation in
 * @param fovy the field of view in the y direction
 * @param aspect the view aspect ratio
 * @param zNear the near clipping plane
 * @param zFar the far clipping plane
 */
void perspective(GLfloat *m, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
	GLfloat tmp[16];
	identity(tmp);

	double sine, cosine, cotangent, deltaZ;
	GLfloat radians = fovy / 2 * M_PI / 180;

	deltaZ = zFar - zNear;
	sincos(radians, &sine, &cosine);

	if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
		return;

	cotangent = cosine / sine;

	tmp[0] = cotangent / aspect;
	tmp[5] = cotangent;
	tmp[10] = -(zFar + zNear) / deltaZ;
	tmp[11] = -1;
	tmp[14] = -2 * zNear * zFar / deltaZ;
	tmp[15] = 0;

	memcpy(m, tmp, sizeof(tmp));
}

/**
 * Draws a gear.
 *
 * @param gear the gear to draw
 * @param transform the current transformation matrix
 * @param x the x position to draw the gear at
 * @param y the y position to draw the gear at
 * @param angle the rotation angle of the gear
 * @param color the color of the gear
 */
static void
draw_gear(struct gear *gear, GLfloat *transform,
		GLfloat x, GLfloat y, GLfloat angle, const GLfloat color[4])
{
	GLfloat model_view[16];
	GLfloat normal_matrix[16];
	GLfloat model_view_projection[16];

	/* Translate and rotate the gear */
	memcpy(model_view, transform, sizeof (model_view));
	translate(model_view, x, y, 0);
	rotate(model_view, 2 * M_PI * angle / 360.0, 0, 0, 1);

	/* Create and set the ModelViewProjectionMatrix */
	memcpy(model_view_projection, ProjectionMatrix, sizeof(model_view_projection));
	multiply(model_view_projection, model_view);

	memcpy(&uniforms.mvp_mat, model_view_projection, sizeof(mat4));

	/*
	 * Create and set the NormalMatrix. It's the inverse transpose of the
	 * ModelView matrix.
	 */
	memcpy(normal_matrix, model_view, sizeof (normal_matrix));
	invert(normal_matrix);
	transpose(normal_matrix);
	memcpy(&uniforms.normal_mat, normal_matrix, sizeof(mat4));

	/* Set the gear color, only copying rgb since alpha is 1 */
	memcpy(&uniforms.material_color, color, sizeof(vec3));

	/* Set the vertex buffer object to use */
	glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);

	/* Set up the position of the attributes in the vertex buffer object */
	/* Note, not using the macro pglVertexAttribPointer here because this is a demo
	 * of porting existing code with minimal changes.
	 *
	 * I will never understand why Khronos decided an offset parameter should be a pointer */
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(0 + 3*sizeof(GLfloat)));

	/* Enable the attributes */
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	/* Draw the triangle strips that comprise the gear */
	int n;
	for (n = 0; n < gear->nstrips; n++)
		glDrawArrays(GL_TRIANGLE_STRIP, gear->strips[n].first, gear->strips[n].count);

	/* Disable the attributes */
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
}

/**
 * Draws the gears.
 */
static void
gears_draw(void)
{
	//why bother with alpha if it's 1?
	static const GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
	static const GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
	static const GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };
	GLfloat transform[16];
	identity(transform);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Translate and rotate the view */
	translate(transform, 0, 0, -20);
	rotate(transform, 2 * M_PI * view_rot[0] / 360.0, 1, 0, 0);
	rotate(transform, 2 * M_PI * view_rot[1] / 360.0, 0, 1, 0);
	rotate(transform, 2 * M_PI * view_rot[2] / 360.0, 0, 0, 1);

	/* Draw the gears */
	draw_gear(gear1, transform, -3.0, -2.0, angle, red);
	draw_gear(gear2, transform, 3.1, -2.0, -2 * angle - 9.0, green);
	draw_gear(gear3, transform, -3.1, 4.2, -2 * angle - 25.0, blue);
}



static void
gears_idle(void)
{
	static int frames = 0;
	static double tRot0 = -1.0, tRate0 = -1.0;
	double dt, t = SDL_GetTicks() / 1000.0;

	if (tRot0 < 0.0)
		tRot0 = t;
	dt = t - tRot0;
	tRot0 = t;

	/* advance rotation for next frame */
	angle += 70.0 * dt;  /* 70 degrees per second */
	if (angle > 3600.0)
		angle -= 3600.0;

	frames++;

	if (tRate0 < 0.0)
		tRate0 = t;
	if (t - tRate0 >= 5.0) {
		GLfloat seconds = t - tRate0;
		GLfloat fps = frames / seconds;
		printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds,
				fps);
		tRate0 = t;
		frames = 0;
	}
}


void vertex_shader(float* vs_output, vec4* v_attrs, Shader_Builtins* builtins, void* uniforms)
{
	//convenience
	vec3* vs_out = (vec3*)vs_output;;
	My_Uniforms* u = uniforms;

	//print_vec4(v_attrs[1], "\n");
	vec4 v4 = mult_mat4_vec4(u->normal_mat, v_attrs[1]);
	//print_vec4(v4, "\n");
	vec3 v3 = { v4.x, v4.y, v4.z };
	vec3 N = norm_vec3(v3);

	const vec3 light_pos = { 5.0, 5.0, 10.0 };
	vec3 L = norm_vec3(light_pos);

	//prevent double dot calc using macro
	float tmp = dot_vec3s(N, L);
	float diff_intensity = MAX(tmp, 0.0);

	vs_out[0] = scale_vec3(u->material_color, diff_intensity);

	builtins->gl_Position = mult_mat4_vec4(u->mvp_mat, v_attrs[0]);
}

void fragment_shader(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	vec3 color = ((vec3*)fs_input)[0];

	builtins->gl_FragColor.x = color.x;
	builtins->gl_FragColor.y = color.y;
	builtins->gl_FragColor.z = color.z;
	builtins->gl_FragColor.w = 1;

}


/*
static const char vertex_shader_[] =
"#version 330 core\n"
"in vec3 position;\n"
"in vec3 normal;\n"
"\n"
"uniform mat4 ModelViewProjectionMatrix;\n"
"uniform mat4 NormalMatrix;\n"
"uniform vec4 LightSourcePosition;\n"
"uniform vec4 MaterialColor;\n"
"\n"
"out vec4 Color;\n"
"\n"
"void main(void)\n"
"{\n"
"    // Transform the normal to eye coordinates\n"
"    vec3 N = normalize(vec3(NormalMatrix * vec4(normal, 1.0)));\n"
"\n"
"    // The LightSourcePosition is actually its direction for directional light\n"
"    vec3 L = normalize(LightSourcePosition.xyz);\n"
"\n"
"    // Multiply the diffuse value by the vertex color (which is fixed in this case)\n"
"    // to get the actual color that we will use to draw this vertex with\n"
"    float diffuse = max(dot(N, L), 0.0);\n"
"    Color = diffuse * MaterialColor;\n"
"\n"
"    // Transform the position to clip coordinates\n"
"    gl_Position = ModelViewProjectionMatrix * vec4(position, 1.0);\n"
"    //gl_Position = vec4(position, 1.0);\n"
"}";

static const char fragment_shader_[] =
"#version 330 core\n"
"\n"
"in vec4 Color;\n"
"out vec4 frag_color;\n"
"\n"
"void main(void)\n"
"{\n"
"    frag_color = Color;\n"
"}";
*/

static void
gears_init(void)
{
	GLuint program;

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	GLenum smooth[3] = { PGL_SMOOTH3 };

	/* Create the shader program */
	program = pglCreateProgram(vertex_shader, fragment_shader, 3, smooth, GL_FALSE);

	/* Enable the shaders */
	glUseProgram(program);

	/* set uniform pointer */
	pglSetUniform(&uniforms);


	//These have to be set initially
	perspective(ProjectionMatrix, 60.0, WIDTH / (float)HEIGHT, 1.0, 1024.0);
	glViewport(0, 0, (GLint) WIDTH, (GLint) HEIGHT);

	/* make the gears */
	gear1 = create_gear(1.0, 4.0, 1.0, 20, 0.7);
	gear2 = create_gear(0.5, 2.0, 2.0, 10, 0.7);
	gear3 = create_gear(1.3, 2.0, 0.5, 10, 0.7);
}

void check_errors(int n, const char* str)
{
	GLenum error;
	int err = 0;
	while ((error = glGetError()) != GL_NO_ERROR) {
		switch (error)
		{
		case GL_INVALID_ENUM:
			fprintf(stderr, "invalid enum\n");
			break;
		case GL_INVALID_VALUE:
			fprintf(stderr, "invalid value\n");
			break;
		case GL_INVALID_OPERATION:
			fprintf(stderr, "invalid operation\n");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			fprintf(stderr, "invalid framebuffer operation\n");
			break;
		case GL_OUT_OF_MEMORY:
			fprintf(stderr, "out of memory\n");
			break;
		default:
			fprintf(stderr, "wtf?\n");
		}
		err = 1;
	}
	if (err)
		fprintf(stderr, "%d: %s\n\n", n, (!str)? "Errors cleared" : str);
}


void cleanup(void)
{
	free_glContext(&the_context);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

void setup_context(void)
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_init error: %s\n", SDL_GetError());
		exit(0);
	}

	window = SDL_CreateWindow("Gears", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Failed to create window\n");
		SDL_Quit();
		exit(0);
	}

	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
#ifdef PGL_ARGB32
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
#elif defined(PGL_RGB565)
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
#elif defined(PGL_RGBA5551)
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA5551, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
#elif defined(PGL_ABGR32)
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
#else
#error "No SDL texture defined, since no PGL pixel format defined"
#endif

	if (!init_glContext(&the_context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(0);
	}
}





int handle_events(void)
{
	SDL_Event e;
	int sc, width, height;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return 1;
		} else if (e.type == SDL_KEYDOWN) {
			sc = e.key.keysym.scancode;

			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				return 1;
				break;
			case SDL_SCANCODE_P:
				polygon_mode = (polygon_mode + 1) % 3;
				if (polygon_mode == 0)
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				else if (polygon_mode == 1)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;

			case SDL_SCANCODE_LEFT:
				view_rot[1] += 5.0;
				break;
			case SDL_SCANCODE_RIGHT:
				view_rot[1] -= 5.0;
				break;
			case SDL_SCANCODE_UP:
				view_rot[0] += 5.0;
				break;
			case SDL_SCANCODE_DOWN:
				view_rot[0] -= 5.0;
				break;
			}
		} else if (e.type == SDL_WINDOWEVENT) {
			switch (e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				width = e.window.data1;
				height = e.window.data2;

				/* Update the projection matrix */
				perspective(ProjectionMatrix, 60.0, width / (float)height, 1.0, 1024.0);

				/* Set the viewport */
				glViewport(0, 0, (GLint) width, (GLint) height);
				break;
			}
		}
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	/* Initialize the window */
	setup_context();
	polygon_mode = 2;

	//no default vao in core profile ...
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	/* Initialize the gears */
	gears_init();

	while (1) {
		if (handle_events())
			break;

		gears_idle();
		gears_draw();

		SDL_UpdateTexture(tex, NULL, bbufpix, WIDTH * sizeof(pix_t));
		//Render the scene
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
	}

	cleanup();

	return 0;
}


