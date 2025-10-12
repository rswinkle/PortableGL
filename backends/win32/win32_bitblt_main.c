// Adapted from this tutorial, couldn't find a license
// https://croakingkero.com/tutorials/drawing_pixels_win32_gdi/

// Changes for PortableGL are in public domain if applicable
// otherwise (c) Robert Winkler under MIT License

#define UNICODE
#define _UNICODE
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#define PGL_ARGB32
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define WIDTH 640
#define HEIGHT 480

struct {
	int w;
	int h;
	// TODO use pix_t* and try a 16 bit pixel format? Can windows support that?
	uint32_t* pixels;
} frame = {0};

typedef struct My_Uniforms {
	vec4 v_color;
} My_Uniforms;

glContext the_Context;

LRESULT CALLBACK WindowProcessMessage(HWND, UINT, WPARAM, LPARAM);

void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);
void draw_frame(void);

static bool quit = false;
static BITMAPINFO frame_bitmap_info;
static HBITMAP frame_bitmap     = 0;
static HDC frame_device_context = 0;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	frame.w = 640;
	frame.h = 480;

	// Need to do this first because we end up calling pgl functions in WM_SIZE
	// event triggered on window creation before we call init_glContext()
	//
	// Other alternative would be calling init_glContext first with arbitrary non-null
	// value for backbuf pointer and 0,0 for dimensions to get the proper state
	set_glContext(&the_Context);

	const wchar_t window_class_name[] = L"My Window Class";
	static WNDCLASS window_class      = {0};
	window_class.lpfnWndProc          = WindowProcessMessage;
	window_class.hInstance            = hInstance;
	window_class.lpszClassName        = window_class_name;
	RegisterClass(&window_class);

	frame_bitmap_info.bmiHeader.biSize        = sizeof(frame_bitmap_info.bmiHeader);
	frame_bitmap_info.bmiHeader.biPlanes      = 1;
	frame_bitmap_info.bmiHeader.biBitCount    = 32;
	frame_bitmap_info.bmiHeader.biCompression = BI_RGB;
	frame_device_context                      = CreateCompatibleDC(0);

	// Calculate the required size of the window rectangle based on desired client area size
	DWORD win_style = WS_OVERLAPPED | WS_VISIBLE;
	RECT win_rect = { 0, 0, frame.w, frame.h };
	AdjustWindowRectEx(&win_rect, win_style, FALSE, 0);

	static HWND window_handle;
	window_handle = CreateWindow(window_class_name, L"PortableGL Win32 BitBlit", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                             640, 300, win_rect.right - win_rect.left, win_rect.bottom - win_rect.top, NULL, NULL, hInstance, NULL);
	if (window_handle == NULL) {
		return -1;
	}

	// NOTE(rswinkle): wait for WM_SIZE event triggered on window creation where we get the
	// dimensions and allocate pixels indirectly; not sure if this is really
	// needed, or if it's guaranteed to happen before CreateWindow() returns but
	// just in case
	while (!frame.pixels) {}

	// Initialize PGL with the canvas you allocated for the window
	if (!init_glContext(&the_Context, (pix_t**)&frame.pixels, frame.w, frame.h)) {
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



	while (!quit) {
		static MSG message = {0};
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			DispatchMessage(&message);
		}

		draw_frame();

		InvalidateRect(window_handle, NULL, FALSE);
		UpdateWindow(window_handle);
	}

	free_glContext(&the_Context);
	return 0;
}

void draw_frame(void)
{
	// put all your GL stuff here
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

LRESULT CALLBACK WindowProcessMessage(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_QUIT:
	case WM_DESTROY: {
		quit = true;
	} break;

	case WM_PAINT: {
		static PAINTSTRUCT paint;
		static HDC device_context;
		device_context = BeginPaint(window_handle, &paint);
		BitBlt(device_context, paint.rcPaint.left, paint.rcPaint.top,
		       paint.rcPaint.right - paint.rcPaint.left, paint.rcPaint.bottom - paint.rcPaint.top,
		       frame_device_context, paint.rcPaint.left, paint.rcPaint.top, SRCCOPY);
		EndPaint(window_handle, &paint);
	} break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			quit = true;
		}
		break;

	case WM_SIZE: {
		// NOTE(rswinkle): A negative biHeight indicates that the bitmap is topdown
		// like you normally think of memory. Ironically, PGL goes through some hoops
		// (accessing framebuffers through a pointer to the last row and negating y)
		// to address that mismatch between standard memory acces and OpenGL y
		// coordinates increasing upward
		frame_bitmap_info.bmiHeader.biWidth  = LOWORD(lParam);
		frame_bitmap_info.bmiHeader.biHeight = -HIWORD(lParam);

		if (frame_bitmap) DeleteObject(frame_bitmap);
		frame_bitmap =
		    CreateDIBSection(NULL, &frame_bitmap_info, DIB_RGB_COLORS, (void**)&frame.pixels, 0, 0);
		SelectObject(frame_device_context, frame_bitmap);

		frame.w  = LOWORD(lParam);
		frame.h = HIWORD(lParam);

		// Resize depth/stencil and set backbuf to new pixel array
		//
		// Normally the order of these two wouldn't matter but because WM_SIZE
		// occurs on window creation and we can't call init_glContext() until
		// after we have the pixels, calling SetBackBuffer first tells PGL
		// not to allocate its own color buffer. There are other hacky solutions
		// I could use to work around this stupid windows API but probably
		// better to just use the stretchDIBits demo anyway
		pglSetBackBuffer(frame.pixels, frame.w, frame.h);
		pglResizeFramebuffer(frame.w, frame.h);

		glViewport(0, 0, frame.w, frame.h);

	} break;

	case WM_CREATE: {
		// happens before WM_SIZE when CreateWindow*() is called
	} break;

	default: {
		return DefWindowProc(window_handle, message, wParam, lParam);
	}
	}
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
