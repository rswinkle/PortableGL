#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define PORTABLEGL_IMPLEMENTATION
#define PGL_ARGB32
#include "portablegl.h"


pix_t* bbufpix;
glContext the_Context;


Display* display;
XImage* xWindowBuffer;
Window window;
GC defaultGC;
Atom WM_DELETE_WINDOW;
XIC xInputContext;
XVisualInfo visinfo;
int width, height;

typedef struct My_Uniforms {
	vec4 v_color;
} My_Uniforms;

void setSizeHint(Display* display, Window window, int minWidth, int minHeight, int maxWidth, int maxHeight);
Status toggleMaximize(Display* display, Window window);


void setup(void);
int handle_events(Display* display);

void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);


int main(int argc, char** args)
{
	width  = 800;
	height = 600;

	setup();

	float points[] =
	{
		-0.5, -0.5, 0,
		 0.5, -0.5, 0,
		 0,    0.5, 0
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
	vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	the_uniforms.v_color = Red;
	//the_uniforms.v_color = Blue;

	glClearColor(0, 0, 0, 1);

	while (1) {
		if (handle_events(display))
			break;

		// put all your GL stuff here
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);


		// blit to screen/window
		XPutImage(display, window, defaultGC, xWindowBuffer, 0, 0, 0, 0, width, height);
	}

	free_glContext(&the_Context);
	XDestroyImage(xWindowBuffer); // Free's the memory we malloced;

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


void setup(void)
{
	display = XOpenDisplay(0);

	if (!display) {
		printf("No display available\n");
		exit(1);
	}

	Window root       = DefaultRootWindow(display);
	int defaultScreen = DefaultScreen(display);

	int screenBitDepth  = 24;
	if (!XMatchVisualInfo(display, defaultScreen, screenBitDepth, TrueColor, &visinfo)) {
		printf("No matching visual info\n");
		exit(1);
	}

	XSetWindowAttributes windowAttr;
	windowAttr.bit_gravity      = StaticGravity;
	windowAttr.background_pixel = 0;
	windowAttr.colormap         = XCreateColormap(display, root, visinfo.visual, AllocNone);
	windowAttr.event_mask       = StructureNotifyMask | KeyPressMask | KeyReleaseMask;
	unsigned long attributeMask = CWBitGravity | CWBackPixel | CWColormap | CWEventMask;

	window = XCreateWindow(display, root, 0, 0, width, height, 0, visinfo.depth, InputOutput,
	                              visinfo.visual, attributeMask, &windowAttr);

	if (!window) {
		printf("Window wasn't created properly\n");
		exit(1);
	}

	XStoreName(display, window, "Hello, World!");
	setSizeHint(display, window, 400, 300, 0, 0);

	XIM xInputMethod = XOpenIM(display, 0, 0, 0);
	if (!xInputMethod) { printf("Input Method could not be opened\n"); }

	XIMStyles* styles = 0;
	if (XGetIMValues(xInputMethod, XNQueryInputStyle, &styles, NULL) || !styles) {
		printf("Input Styles could not be retrieved\n");
	}

	XIMStyle bestMatchStyle = 0;
	for (int i = 0; i < styles->count_styles; i++) {
		XIMStyle thisStyle = styles->supported_styles[i];
		if (thisStyle == (XIMPreeditNothing | XIMStatusNothing)) {
			bestMatchStyle = thisStyle;
			break;
		}
	}
	XFree(styles);

	if (!bestMatchStyle) { printf("No matching input style could be determined\n"); }

	xInputContext = XCreateIC(xInputMethod, XNInputStyle, bestMatchStyle, XNClientWindow,
	                              window, XNFocusWindow, window, NULL);
	if (!xInputContext) { printf("Input Context could not be created\n"); }

	XMapWindow(display, window);

	// toggleMaximize(display, window);
	XFlush(display);

	int pixelBits        = 32;
	int pixelBytes       = pixelBits / 8;
	int windowBufferSize = width * height * pixelBytes;
	bbufpix              = (pix_t*)malloc(windowBufferSize);

	if (!init_glContext(&the_Context, &bbufpix, width, height)) {
		puts("Failed to initialize glContext");
		exit(0);
	}

	xWindowBuffer = XCreateImage(display, visinfo.visual, visinfo.depth, ZPixmap, 0,
	                                     (char*)bbufpix, width, height, pixelBits, 0);
	defaultGC = DefaultGC(display, defaultScreen);

	WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
	if (!XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1)) {
		printf("Couldn't register WM_DELETE_WINDOW property\n");
	}
}

int handle_events(Display* display)
{
	int sizeChange = 0;
	XEvent ev = {};
	while (XPending(display) > 0) {
		XNextEvent(display, &ev);
		switch (ev.type) {
		case DestroyNotify: {
			XDestroyWindowEvent* e = (XDestroyWindowEvent*)&ev;
			if (e->window == window) return 1;
		} break;
		case ClientMessage: {
			XClientMessageEvent* e = (XClientMessageEvent*)&ev;
			if ((Atom)e->data.l[0] == WM_DELETE_WINDOW) {
				XDestroyWindow(display, window);
				return 1;
			}
		} break;
		case ConfigureNotify: {
			XConfigureEvent* e = (XConfigureEvent*)&ev;
			// TODO size change right here
			width              = e->width;
			height             = e->height;
			sizeChange         = 1;
		} break;
		case KeyPress: {
			XKeyPressedEvent* e = (XKeyPressedEvent*)&ev;

			if (e->keycode == XKeysymToKeycode(display, XK_Escape)) return 1;

			int symbol    = 0;
			Status status = 0;
			Xutf8LookupString(xInputContext, e, (char*)&symbol, 4, 0, &status);

			if (status == XBufferOverflow) {
				// Should not happen since there are no utf-8 characters larger than 24bits
				// But something to be aware of when used to directly write to a string buffer
				printf("Buffer overflow when trying to create keyboard symbol map\n");
			} else if (status == XLookupChars) {
				printf("%s\n", (char*)&symbol);
			}

			if (e->keycode == XKeysymToKeycode(display, XK_Left))
				printf("left arrow pressed\n");
			if (e->keycode == XKeysymToKeycode(display, XK_Right))
				printf("right arrow pressed\n");
			if (e->keycode == XKeysymToKeycode(display, XK_Up)) printf("up arrow pressed\n");
			if (e->keycode == XKeysymToKeycode(display, XK_Down))
				printf("down arrow pressed\n");
		} break;
		case KeyRelease: {
			XKeyPressedEvent* e = (XKeyPressedEvent*)&ev;

			if (e->keycode == XKeysymToKeycode(display, XK_Left))
				printf("left arrow released\n");
			if (e->keycode == XKeysymToKeycode(display, XK_Right))
				printf("right arrow released\n");
			if (e->keycode == XKeysymToKeycode(display, XK_Up)) printf("up arrow released\n");
			if (e->keycode == XKeysymToKeycode(display, XK_Down))
				printf("down arrow released\n");
		} break;
		}
	}

	if (sizeChange) {
		XDestroyImage(xWindowBuffer); // Free's the memory we malloced;
		bbufpix          = (pix_t*)malloc(width * height * sizeof(pix_t));

		pglResizeFramebuffer(width, height);
		pglSetBackBuffer(bbufpix, width, height);
		glViewport(0, 0, width, height)

		xWindowBuffer = XCreateImage(display, visinfo.visual, visinfo.depth, ZPixmap, 0,
		                             (char*)bbufpix, width, height, sizeof(pix_t)*8, 0);
	}

	return 0;
}



void setSizeHint(Display* display, Window window, int minWidth, int minHeight, int maxWidth, int maxHeight)
{
	XSizeHints hints = {};
	if (minWidth > 0 && minHeight > 0) hints.flags |= PMinSize;
	if (maxWidth > 0 && maxHeight > 0) hints.flags |= PMaxSize;

	hints.min_width  = minWidth;
	hints.min_height = minHeight;
	hints.max_width  = maxWidth;
	hints.max_height = maxHeight;

	XSetWMNormalHints(display, window, &hints);
}

Status toggleMaximize(Display* display, Window window)
{
	XClientMessageEvent ev = {};
	Atom wmState           = XInternAtom(display, "_NET_WM_STATE", False);
	Atom maxH              = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
	Atom maxV              = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

	if (wmState == None) return 0;

	ev.type         = ClientMessage;
	ev.format       = 32;
	ev.window       = window;
	ev.message_type = wmState;
	ev.data.l[0]    = 2; // _NET_WM_STATE_TOGGLE 2 according to spec; Not defined in my headers
	ev.data.l[1]    = maxH;
	ev.data.l[2]    = maxV;
	ev.data.l[3]    = 1;

	return XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, (XEvent*)&ev);
}
