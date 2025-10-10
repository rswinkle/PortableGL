// Adapted from this tutorial, couldn't find a license
// https://croakingkero.com/tutorials/drawing_pixels_win32_gdi/

// Changes for PortableGL are in public domain if applicable
// otherwise (c) Robert Winkler under MIT License

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#define PGL_ARGB32
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

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

void draw_frame(void);

void identity_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

static bool quit = false;
static BITMAPINFO bitmap_info;

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CmdLine, int CmdShow)
{
	frame.w = 640;
	frame.h = 480;
	// Initialize PGL with the canvas you allocated for the window
	if (!init_glContext(&the_Context, (pix_t**)&frame.pixels, frame.w, frame.h)) {
		puts("Failed to initialize glContext");
		exit(0);
	}

	// NOTE: A negative biHeight indicates that the bitmap is topdown
	// like you normally think of memory. Ironically, PGL goes through some hoops
	// (accessing framebuffers through a pointer to the last row and negating y)
	// to address that mismatch between standard memory acces and OpenGL y
	// coordinates increasing upward
	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
	bitmap_info.bmiHeader.biWidth = frame.w;
	bitmap_info.bmiHeader.biHeight = -frame.h;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;

	WNDCLASS WindowClass = {};

	WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	WindowClass.lpfnWndProc = WindowProcessMessage;
	WindowClass.hInstance = Instance;
	WindowClass.hIcon;
	WindowClass.lpszClassName = "PGLWindowClass";

	if (!RegisterClassA(&WindowClass)) {
		// free/error message
		return 0;
	}

	// Calculate the required size of the window rectangle based on desired client area size
	DWORD win_style = WS_OVERLAPPED | WS_VISIBLE;
    RECT win_rect = { 0, 0, frame.w, frame.h };
    AdjustWindowRectEx(&win_rect, win_style, FALSE, 0);
	
	HWND Window =
		CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"PortableGL Win32",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			win_rect.right - win_rect.left,
			win_rect.bottom - win_rect.top,
			0,
			0,
			Instance,
			0);

	if (!Window) {
		return 0;
	}

    HDC device_context = GetDC(Window);

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


	int screen_w, screen_h;
	while (!quit) {
		static MSG message = {0};
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		draw_frame();

		RECT screen;
		GetClientRect(Window, &screen);
		screen_w = screen.right - screen.left;
		screen_h = screen.bottom - screen.top;

		StretchDIBits(device_context, 0, 0, screen_w, screen_h, 0, 0, frame.w, frame.h, frame.pixels, &bitmap_info, DIB_RGB_COLORS, SRCCOPY);
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
	case WM_CLOSE:
	case WM_DESTROY: {
		quit = true;
	} break;

	case WM_ACTIVATEAPP: {
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	} break;

	case WM_PAINT: {
		PAINTSTRUCT paint;
		HDC device_context;
		device_context = BeginPaint(window_handle, &paint);
		RECT screen;
		GetClientRect(window_handle, &screen);
		int screen_w = screen.right - screen.left;
		int screen_h = screen.bottom - screen.top;

		StretchDIBits(device_context, 0, 0, screen_w, screen_h, 0, 0, frame.w, frame.h, frame.pixels, &bitmap_info, DIB_RGB_COLORS, SRCCOPY);
		EndPaint(window_handle, &paint);
	} break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			quit = true;
		}
		break;

	case WM_SIZE: {
		frame.w  = LOWORD(lParam);
		frame.h = HIWORD(lParam);
		bitmap_info.bmiHeader.biWidth = frame.w;
		bitmap_info.bmiHeader.biHeight = -frame.h;

		pglResizeFramebuffer(frame.w, frame.h);
		frame.pixels = pglGetBackBuffer();
		glViewport(0, 0, frame.w, frame.h);

		// NOTE: Seems like this needs to be here unless you want the screen to be messed
		// up while you're resizing the window...there are probably other ways of doing
		// it, maybe better ways but this works well enough
		draw_frame();

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

