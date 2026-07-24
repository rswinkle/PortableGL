// Minimal PortableGL + Wayland example (static red triangle, resizable)
// Software path: wl_shm + xdg-shell (no EGL/Vulkan)
//
// Roughly the same role as backends/x11_xlib/xlib_pgl2.c
//
// (c) Robert Winkler under MIT License

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

// Wayland XRGB/ARGB8888 on LE is B,G,R,A in memory — same as Cairo ARGB32 / PGL_ARGB32
#define PGL_ARGB32
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define WIDTH  800
#define HEIGHT 600

typedef struct My_Uniforms {
	vec4 v_color;
} My_Uniforms;

// --- PGL -------------------------------------------------------------------

static pix_t* bbufpix;
static glContext the_Context;
static int width  = WIDTH;
static int height = HEIGHT;
static bool running = true;
static bool configured;
static bool need_resize;
static int pending_w = WIDTH;
static int pending_h = HEIGHT;

// --- Wayland globals -------------------------------------------------------

static struct wl_display* display;
static struct wl_registry* registry;
static struct wl_compositor* compositor;
static struct wl_shm* shm;
static struct xdg_wm_base* xdg_wm_base;
static struct wl_seat* seat;
static struct wl_keyboard* keyboard;

static struct wl_surface* surface;
static struct xdg_surface* xdg_surface;
static struct xdg_toplevel* xdg_toplevel;

// Double-buffered shm so we can draw while the compositor holds the last frame
typedef struct {
	struct wl_buffer* buffer;
	void* data;
	size_t size;
	int w, h;
	bool busy;
} shm_buffer;

static shm_buffer buffers[2];
static int front; // next free buffer index to draw into

// --- shm helpers -----------------------------------------------------------

static int create_shm_file(size_t size)
{
	int fd = -1;

#ifdef __linux__
	fd = memfd_create("portablegl-wl", MFD_CLOEXEC | MFD_ALLOW_SEALING);
	if (fd >= 0) {
		if (ftruncate(fd, (off_t)size) < 0) {
			close(fd);
			return -1;
		}
		return fd;
	}
#endif

	// Fallback: temporary file in the system temp dir
	char template[] = "/tmp/pgl-wl-XXXXXX";
	fd = mkstemp(template);
	if (fd < 0)
		return -1;
	unlink(template);
	if (ftruncate(fd, (off_t)size) < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

static void buffer_release(void* data, struct wl_buffer* wl_buffer)
{
	(void)wl_buffer;
	shm_buffer* b = data;
	b->busy = false;
}

static const struct wl_buffer_listener buffer_listener = {
	.release = buffer_release,
};

static void destroy_shm_buffer(shm_buffer* b)
{
	if (b->buffer) {
		wl_buffer_destroy(b->buffer);
		b->buffer = NULL;
	}
	if (b->data && b->size) {
		munmap(b->data, b->size);
		b->data = NULL;
		b->size = 0;
	}
	b->w = b->h = 0;
	b->busy = false;
}

static bool create_shm_buffer(shm_buffer* b, int w, int h)
{
	destroy_shm_buffer(b);

	int stride = w * 4;
	size_t size = (size_t)stride * (size_t)h;
	int fd = create_shm_file(size);
	if (fd < 0) {
		perror("create_shm_file");
		return false;
	}

	void* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return false;
	}

	struct wl_shm_pool* pool = wl_shm_create_pool(shm, fd, (int)size);
	close(fd);

	// XRGB8888: 0x00RRGGBB as word; LE memory matches PGL_ARGB32 with opaque alpha
	b->buffer = wl_shm_pool_create_buffer(pool, 0, w, h, stride, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);

	wl_buffer_add_listener(b->buffer, &buffer_listener, b);

	b->data = data;
	b->size = size;
	b->w = w;
	b->h = h;
	b->busy = false;
	return true;
}

static void ensure_buffers(int w, int h)
{
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;

	for (int i = 0; i < 2; i++) {
		if (buffers[i].w != w || buffers[i].h != h || !buffers[i].buffer)
			create_shm_buffer(&buffers[i], w, h);
	}
}

static shm_buffer* acquire_buffer(void)
{
	// Prefer a free buffer; if both busy, block on Wayland until one is released
	for (;;) {
		for (int i = 0; i < 2; i++) {
			int idx = (front + i) % 2;
			if (!buffers[idx].busy && buffers[idx].buffer)
				return &buffers[idx];
		}
		if (wl_display_dispatch(display) == -1)
			return NULL;
	}
}

// --- xdg / registry listeners ---------------------------------------------

static void xdg_wm_base_ping(void* data, struct xdg_wm_base* wm_base, uint32_t serial)
{
	(void)data;
	xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};

static void xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
{
	(void)data;
	xdg_surface_ack_configure(xdg_surface, serial);
	configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void* data, struct xdg_toplevel* toplevel,
                                   int32_t w, int32_t h, struct wl_array* states)
{
	(void)data;
	(void)toplevel;
	(void)states;
	// w/h of 0 means "client picks" — keep current
	if (w > 0 && h > 0) {
		if (w != width || h != height) {
			pending_w = w;
			pending_h = h;
			need_resize = true;
		}
	}
}

static void xdg_toplevel_close(void* data, struct xdg_toplevel* toplevel)
{
	(void)data;
	(void)toplevel;
	running = false;
}

static void xdg_toplevel_configure_bounds(void* data, struct xdg_toplevel* toplevel,
                                          int32_t width, int32_t height)
{
	(void)data;
	(void)toplevel;
	(void)width;
	(void)height;
}

static void xdg_toplevel_wm_capabilities(void* data, struct xdg_toplevel* toplevel,
                                         struct wl_array* capabilities)
{
	(void)data;
	(void)toplevel;
	(void)capabilities;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure        = xdg_toplevel_configure,
	.close            = xdg_toplevel_close,
	.configure_bounds = xdg_toplevel_configure_bounds,
	.wm_capabilities  = xdg_toplevel_wm_capabilities,
};

static void keyboard_keymap(void* data, struct wl_keyboard* keyboard, uint32_t format,
                            int32_t fd, uint32_t size)
{
	(void)data;
	(void)keyboard;
	(void)format;
	(void)size;
	close(fd);
}

static void keyboard_enter(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                           struct wl_surface* surface, struct wl_array* keys)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)surface;
	(void)keys;
}

static void keyboard_leave(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                           struct wl_surface* surface)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)surface;
}

static void keyboard_key(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                         uint32_t time, uint32_t key, uint32_t state)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)time;
	// Linux evdev keycodes: KEY_ESC = 1
	if (key == 1 && state == WL_KEYBOARD_KEY_STATE_PRESSED)
		running = false;
}

static void keyboard_modifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial,
                               uint32_t mods_depressed, uint32_t mods_latched,
                               uint32_t mods_locked, uint32_t group)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)mods_depressed;
	(void)mods_latched;
	(void)mods_locked;
	(void)group;
}

static void keyboard_repeat_info(void* data, struct wl_keyboard* keyboard,
                                 int32_t rate, int32_t delay)
{
	(void)data;
	(void)keyboard;
	(void)rate;
	(void)delay;
}

static const struct wl_keyboard_listener keyboard_listener = {
	.keymap      = keyboard_keymap,
	.enter       = keyboard_enter,
	.leave       = keyboard_leave,
	.key         = keyboard_key,
	.modifiers   = keyboard_modifiers,
	.repeat_info = keyboard_repeat_info,
};

static void seat_capabilities(void* data, struct wl_seat* seat, uint32_t caps)
{
	(void)data;
	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !keyboard) {
		keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
	} else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && keyboard) {
		wl_keyboard_release(keyboard);
		keyboard = NULL;
	}
}

static void seat_name(void* data, struct wl_seat* seat, const char* name)
{
	(void)data;
	(void)seat;
	(void)name;
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_capabilities,
	.name         = seat_name,
};

static void registry_global(void* data, struct wl_registry* registry, uint32_t name,
                            const char* interface, uint32_t version)
{
	(void)data;
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor = wl_registry_bind(registry, name, &wl_compositor_interface,
		                              version < 4 ? version : 4);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface,
		                               version < 2 ? version : 2);
		xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		seat = wl_registry_bind(registry, name, &wl_seat_interface,
		                        version < 7 ? version : 7);
		wl_seat_add_listener(seat, &seat_listener, NULL);
	}
}

static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
	(void)data;
	(void)registry;
	(void)name;
}

static const struct wl_registry_listener registry_listener = {
	.global        = registry_global,
	.global_remove = registry_global_remove,
};

// --- PGL shaders -----------------------------------------------------------

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

// --- setup / frame ---------------------------------------------------------

static void apply_resize(void)
{
	if (!need_resize)
		return;
	need_resize = false;

	width  = pending_w > 0 ? pending_w : 1;
	height = pending_h > 0 ? pending_h : 1;

	// PGL owns color buffer; grab pointer after resize
	pglResizeFramebuffer(width, height);
	bbufpix = (pix_t*)pglGetBackBuffer();
	glViewport(0, 0, width, height);

	ensure_buffers(width, height);
}

static void draw_frame(void)
{
	if (!configured)
		return;

	apply_resize();
	ensure_buffers(width, height);

	shm_buffer* buf = acquire_buffer();
	if (!buf)
		return;

	// put all your GL stuff here
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// blit PGL back buffer into the shm buffer the compositor will show
	memcpy(buf->data, bbufpix, (size_t)width * (size_t)height * sizeof(pix_t));

	buf->busy = true;
	front = (int)(buf - buffers); // remember which we used
	front = (front + 1) % 2;

	wl_surface_attach(surface, buf->buffer, 0, 0);
	wl_surface_damage_buffer(surface, 0, 0, width, height);
	wl_surface_commit(surface);
}

static void setup_wayland(void)
{
	display = wl_display_connect(NULL);
	if (!display) {
		fprintf(stderr, "Failed to connect to Wayland display.\n"
		                "Is WAYLAND_DISPLAY set? (X11-only sessions need a Wayland compositor.)\n");
		exit(1);
	}

	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display); // globals
	wl_display_roundtrip(display); // seat capabilities

	if (!compositor || !shm || !xdg_wm_base) {
		fprintf(stderr, "Missing compositor, wl_shm, or xdg_wm_base\n");
		exit(1);
	}

	surface = wl_compositor_create_surface(compositor);
	xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
	xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
	xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
	xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);
	xdg_toplevel_set_title(xdg_toplevel, "PortableGL Wayland");
	xdg_toplevel_set_app_id(xdg_toplevel, "com.portablegl.wayland");

	wl_surface_commit(surface);
	// Block until the first configure so we know a size before drawing
	while (!configured && wl_display_dispatch(display) != -1)
		;
}

static void cleanup_wayland(void)
{
	destroy_shm_buffer(&buffers[0]);
	destroy_shm_buffer(&buffers[1]);

	if (keyboard)
		wl_keyboard_release(keyboard);
	if (xdg_toplevel)
		xdg_toplevel_destroy(xdg_toplevel);
	if (xdg_surface)
		xdg_surface_destroy(xdg_surface);
	if (surface)
		wl_surface_destroy(surface);
	if (seat)
		wl_seat_release(seat);
	if (xdg_wm_base)
		xdg_wm_base_destroy(xdg_wm_base);
	if (shm)
		wl_shm_destroy(shm);
	if (compositor)
		wl_compositor_destroy(compositor);
	if (registry)
		wl_registry_destroy(registry);
	if (display)
		wl_display_disconnect(display);
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	setup_wayland();

	// Prefer compositor-suggested size if we already got one during configure
	if (pending_w > 0 && pending_h > 0) {
		width  = pending_w;
		height = pending_h;
		need_resize = false;
	}

	bbufpix = NULL;
	if (!init_glContext(&the_Context, &bbufpix, width, height)) {
		puts("Failed to initialize glContext");
		exit(1);
	}
	glViewport(0, 0, width, height);
	ensure_buffers(width, height);

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

	vec4 Red  = { 1.0f, 0.0f, 0.0f, 1.0f };
	//vec4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	the_uniforms.v_color = Red;

	glClearColor(0, 0, 0, 1);

	// Initial frame (required before many compositors will map the window)
	draw_frame();
	wl_display_flush(display);

	while (running) {
		// Non-blocking drain, then block for events, then draw (same spirit as xlib loop)
		while (wl_display_prepare_read(display) != 0)
			wl_display_dispatch_pending(display);
		wl_display_flush(display);

		struct pollfd pfd = { .fd = wl_display_get_fd(display), .events = POLLIN };
		int ret = poll(&pfd, 1, 16); // ~60 Hz cap when idle; still responsive
		if (ret < 0 && errno != EINTR) {
			wl_display_cancel_read(display);
			break;
		}
		if (ret > 0)
			wl_display_read_events(display);
		else
			wl_display_cancel_read(display);

		wl_display_dispatch_pending(display);

		if (!running)
			break;

		draw_frame();
		wl_display_flush(display);
	}

	free_glContext(&the_Context);
	cleanup_wayland();
	return 0;
}
