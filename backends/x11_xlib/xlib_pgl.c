// Adapted from this tutorial, couldn't find a license
// https://croakingkero.com/tutorials/opening_a_window_with_xlib/index.html

// Changes for PortableGL are in public domain if applicable
// otherwise (c) Robert Winkler under MIT License

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/XKBlib.h>
#include <X11/keysymdef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PGL_ARGB32
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

typedef struct timespec timespec;
typedef struct My_Uniforms {
	vec4 v_color;
} My_Uniforms;

glContext the_Context;

Display* display;
int root_window;
int screen;
Window window;
Atom WM_DELETE_WINDOW;
uint8_t* canvas;
XImage* window_image;
GC graphics_context;
int window_width = 1280, window_height = 720;
bool keyboard[256] = {0};

void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

int main(int argc, char* argv[])
{
	// Create window
	XVisualInfo visual_info;
	{
		display     = XOpenDisplay(0);
		root_window = DefaultRootWindow(display);
		screen      = DefaultScreen(display);
		XMatchVisualInfo(display, screen, 24, TrueColor, &visual_info);
		XSetWindowAttributes window_attributes;
		window_attributes.background_pixel = 0;
		window_attributes.colormap = XCreateColormap(display, root_window, visual_info.visual, AllocNone);
		window_attributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | FocusChangeMask;
		window = XCreateWindow(display, root_window, 0, 0, window_width, window_height, 0,
		                       visual_info.depth, 0, visual_info.visual,
		                       CWBackPixel | CWColormap | CWEventMask, &window_attributes);
		XMapWindow(display, window);
		XFlush(display);
		WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);
		canvas           = (uint8_t*)malloc(window_width * window_height * 4);
		window_image     = XCreateImage(display, visual_info.visual, visual_info.depth, ZPixmap, 0,
		                                (char*)canvas, window_width, window_height, 32, 0);
		graphics_context = DefaultGC(display, screen);
		XkbSetDetectableAutoRepeat(display, True, 0);
	}

	// Initialize PGL with the canvas you allocated for the window
	if (!init_glContext(&the_Context, (pix_t**)&canvas, window_width, window_height)) {
		puts("Failed to initialize glContext");
		exit(0);
	}

	// Setup al your OpenGL data/buffers/shaders etc.
	float points[] =
	{
		-0.5, -0.5, 0,
		 0.5, -0.5, 0,
		 0,    0.5, 0
	};

	// Not actually needed for PGL but there's
	// no default vao in core profile so if you want it for form's sake...
	//GLuint vao;
	//glGenVertexArrays(1, &vao);
	//glBindVertexArray(vao);

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
	vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	the_uniforms.v_color = Red;
	//the_uniforms.v_color = Blue;

	glClearColor(0, 0, 0, 1);


	// game/loop control etc.
	bool game_running = true;
	int x = 0, y = 0;
	float delta = 0;
	timespec frame_start;
	timespec frame_finish;
	clock_gettime(CLOCK_REALTIME, &frame_start);
	long unsigned target_frame_time = 1000000000 / 60; // In nano seconds

	// Main loop
	while (game_running) {

		// Handle events
		XEvent e;
		while (XPending(display) > 0) {
			XNextEvent(display, &e);
			switch (e.type) {
			case KeyPress: {
				int symbol                = XLookupKeysym(&e.xkey, 0);
				keyboard[(uint8_t)symbol] = true;
				switch (symbol) {
				case XK_Escape: {
					game_running = false;
				} break;
				}
			} break;
			case KeyRelease: {
				int symbol                = XLookupKeysym(&e.xkey, 0);
				keyboard[(uint8_t)symbol] = false;
			} break;
			case ClientMessage: {
				XClientMessageEvent* ev = (XClientMessageEvent*)&e;
				if ((Atom)ev->data.l[0] == WM_DELETE_WINDOW) {
					game_running = false;
				}
			} break;
			case ConfigureNotify: {
				XConfigureEvent* ev = (XConfigureEvent*)&e;
				if (ev->width != window_width || ev->height != window_height) {
					window_width  = ev->width;
					window_height = ev->height;
					XDestroyImage(window_image);
					canvas = (uint8_t*)malloc(window_width * window_height * 4);

					pglResizeFramebuffer(window_width, window_height);
					pglSetBackBuffer(canvas, window_width, window_height);
					glViewport(0, 0, window_width, window_height)

					window_image = XCreateImage(display, visual_info.visual, visual_info.depth, ZPixmap,
					                            0, (char*)canvas, window_width, window_height, 32, 0);
					x = min(x, window_width - 1);
					y = min(y, window_height - 1);
				}
			} break;
			case DestroyNotify: {
				game_running = false;
			} break;
			case FocusOut: {
				memset(keyboard, false, sizeof(keyboard));
			} break;
			}
		}

		// Move our pixel
		if (keyboard[(uint8_t)XK_Up]) {
			if (--y < 0) {
				y = 0;
			}
		}
		if (keyboard[(uint8_t)XK_Down]) {
			if (++y >= window_height) {
				y = window_height - 1;
			}
		}
		if (keyboard[(uint8_t)XK_Left]) {
			if (--x < 0) {
				x = 0;
			}
		}
		if (keyboard[(uint8_t)XK_Right]) {
			if (++x >= window_width) {
				x = window_width - 1;
			}
		}


		// put all your GL stuff here
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// Still do the moving pixel thing from the original tutorial
		*((uint32_t*)(canvas) + y * window_width + x) = 0xffffffff;


		// Update window
		XPutImage(display, window, graphics_context, window_image, 0, 0, 0, 0, window_width, window_height);

		// Delay until we take up the full frame time
		clock_gettime(CLOCK_REALTIME, &frame_finish);
		long unsigned frame_time = (frame_finish.tv_sec - frame_start.tv_sec) * 1000000000 +
		                           (frame_finish.tv_nsec - frame_start.tv_nsec);
		timespec sleep_time;
		sleep_time.tv_sec  = 0;
		sleep_time.tv_nsec = target_frame_time - frame_time;
		nanosleep(&sleep_time, 0);
		clock_gettime(CLOCK_REALTIME, &frame_finish);
		delta = (frame_finish.tv_sec - frame_start.tv_sec) +
		        (frame_finish.tv_nsec - frame_start.tv_nsec) / 1000000000.f;
		frame_start = frame_finish;
		char window_title[30];
		sprintf(window_title, "PortableGL with Xlib FPS: %0.02f", 1.f / delta);
		XStoreName(display, window, window_title);
	}

	free_glContext(&the_Context);
	XDestroyImage(window_image); // Free's the memory we malloced;
	return 0;
}

void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	PGL_UNUSED(uniforms);
	builtins->gl_Position = vertex_attribs[0];
}

void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	builtins->gl_FragColor = ((My_Uniforms*)uniforms)->v_color;
}
