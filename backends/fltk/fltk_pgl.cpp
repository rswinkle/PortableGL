// Minimal PortableGL + FLTK example (static red triangle, resizable)
// Roughly the same role as backends/x11_xlib/xlib_pgl2.c
//
// Blits via fl_draw_image (RGBA bytes). Uses default PGL pixel format
// (ABGR32 / RGBA memory order on little-endian), which matches FLTK.
//
// (c) Robert Winkler under MIT License

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>

#include <cstdio>
#include <cstdlib>

#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define WIDTH  800
#define HEIGHT 600

typedef struct My_Uniforms {
	vec4 v_color;
} My_Uniforms;

static pix_t* bbufpix;
static glContext the_Context;

static void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
static void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

static void resize_framebuffer(int w, int h)
{
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;

	pglResizeFramebuffer(w, h);
	bbufpix = (pix_t*)pglGetBackBuffer();
	glViewport(0, 0, w, h);
}

// Full-window canvas: draw PGL into our widget area each time FLTK asks.
class PGLWindow : public Fl_Double_Window {
public:
	PGLWindow(int w, int h, const char* title)
	    : Fl_Double_Window(w, h, title)
	{
		// Let the user shrink a bit; FLTK still enforces a sane floor
		size_range(160, 120);
		resizable(this);
		end();
	}

	void draw() override
	{
		// put all your GL stuff here
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// FLTK: D=4 → r,g,b,a per pixel (matches default PGL_ABGR32 on LE)
		if (bbufpix && w() > 0 && h() > 0) {
			fl_draw_image(reinterpret_cast<const unsigned char*>(bbufpix),
			              0, 0, w(), h(), 4, 0);
		}
	}

	void resize(int X, int Y, int W, int H) override
	{
		Fl_Double_Window::resize(X, Y, W, H);
		if (W > 0 && H > 0)
			resize_framebuffer(W, H);
	}

	int handle(int event) override
	{
		if (event == FL_KEYDOWN && Fl::event_key() == FL_Escape) {
			hide();
			return 1;
		}
		return Fl_Double_Window::handle(event);
	}
};

int main(int argc, char** argv)
{
	// Prefer true-color visuals on X11 when available
	Fl::visual(FL_RGB);

	bbufpix = NULL;
	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		return 1;
	}

	float points[] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		 0.0f,  0.5f, 0.0f
	};

	// Not actually needed for PGL but there's
	// no default vao in core profile ...
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint triangle;
	glGenBuffers(1, &triangle);
	glBindBuffer(GL_ARRAY_BUFFER, triangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint program = pglCreateProgram(identity_vs, uniform_color_fs, 0, NULL, GL_FALSE);
	glUseProgram(program);

	My_Uniforms the_uniforms;
	pglSetUniform(&the_uniforms);

	vec4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	//vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	the_uniforms.v_color = Red;

	glClearColor(0, 0, 0, 1);
	glViewport(0, 0, WIDTH, HEIGHT);

	PGLWindow* win = new PGLWindow(WIDTH, HEIGHT, "PortableGL FLTK");
	win->show(argc, argv);

	int status = Fl::run();

	free_glContext(&the_Context);
	return status;
}

static void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	PGL_UNUSED(uniforms);
	builtins->gl_Position = vertex_attribs[0];
}

static void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	builtins->gl_FragColor = ((My_Uniforms*)uniforms)->v_color;
}
