// PortableGL + GTK3 example with a simple control sidebar
// Functionally matches backends/gtk4/gtk4_pgl.c (API differences only)
// Uses GtkDrawingArea and Cairo to blit the software framebuffer
//
// (c) Robert Winkler under MIT License

#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Window placement: under X11 we can center on a monitor.  Wayland does not
// allow clients to position normal toplevel windows; the compositor decides.
// See center_window_on_startup() below.
#include <gdk/gdkconfig.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

// Cairo CAIRO_FORMAT_ARGB32 is native-endian 0xAARRGGBB (B,G,R,A bytes on LE)
#define PGL_ARGB32
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define WIDTH  640
#define HEIGHT 480
#define SIDEBAR_WIDTH  220
#define SIDEBAR_MARGIN 12
// Vertical separator is typically 1px; include it so the canvas starts at WIDTH.
#define SEP_WIDTH 1
// Client area so the expanding drawing area is allocated WIDTH x HEIGHT.
#define WINDOW_DEFAULT_W (WIDTH + SEP_WIDTH + SIDEBAR_WIDTH + 2 * SIDEBAR_MARGIN)
#define WINDOW_DEFAULT_H HEIGHT
// Drawing-area floor; window height min is usually larger because of the sidebar.
#define CANVAS_MIN_W 160
#define CANVAS_MIN_H 120

typedef struct My_Uniforms {
	mat4 mvp_mat;
	vec4 v_color;
} My_Uniforms;

// Rotation speed when live update is on (radians per second)
#define ROTATE_RAD_PER_SEC  DEG_TO_RAD(60.0f)

static pix_t* bbufpix;
static glContext the_Context;
static My_Uniforms the_uniforms;
static int fb_width  = WIDTH;
static int fb_height = HEIGHT;
static float angle; // current Z rotation; freezes when live update is off

static GtkWidget* drawing_area;
static GtkWidget* fps_label;
static GtkWidget* size_label;
static guint tick_id;
static gint64 last_frame_time;

void rotate_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

static void update_mvp(void)
{
	vec3 z_axis = { 0.0f, 0.0f, 1.0f };
	load_rotation_m4(the_uniforms.mvp_mat, z_axis, angle);
}

static void update_size_label(void)
{
	if (!size_label)
		return;
	char buf[64];
	snprintf(buf, sizeof(buf), "Framebuffer: %d×%d\nEscape to quit", fb_width, fb_height);
	gtk_label_set_text(GTK_LABEL(size_label), buf);
}

static void resize_framebuffer(int width, int height)
{
	if (width <= 0 || height <= 0)
		return;
	if (width == fb_width && height == fb_height)
		return;

	fb_width  = width;
	fb_height = height;

	// PGL owns the color buffer; reallocate and refresh our pointer
	pglResizeFramebuffer(fb_width, fb_height);
	bbufpix = (pix_t*)pglGetBackBuffer();
	glViewport(0, 0, fb_width, fb_height);
	update_size_label();
}

static void on_size_allocate(GtkWidget* widget, GtkAllocation* allocation, gpointer user_data)
{
	PGL_UNUSED(widget);
	PGL_UNUSED(user_data);
	resize_framebuffer(allocation->width, allocation->height);
}

static void render_frame(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

static gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer user_data)
{
	PGL_UNUSED(widget);
	PGL_UNUSED(user_data);

	render_frame();

	// Blit PGL back buffer to the widget via Cairo
	cairo_surface_t* surface = cairo_image_surface_create_for_data(
	    (unsigned char*)bbufpix, CAIRO_FORMAT_ARGB32,
	    fb_width, fb_height, fb_width * (int)sizeof(pix_t));
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);
	cairo_surface_destroy(surface);
	return FALSE;
}

static gboolean on_tick(GtkWidget* widget, GdkFrameClock* clock, gpointer user_data)
{
	PGL_UNUSED(user_data);

	gint64 now = gdk_frame_clock_get_frame_time(clock); // microseconds
	if (last_frame_time > 0) {
		double dt = (now - last_frame_time) / 1000000.0;
		if (dt > 0.0) {
			// Advance animation only while live update is running
			angle += (float)(ROTATE_RAD_PER_SEC * dt);
			// Keep angle in a reasonable range
			if (angle > 2.0f * (float)M_PI)
				angle = fmodf(angle, 2.0f * (float)M_PI);
			update_mvp();

			char buf[64];
			snprintf(buf, sizeof(buf), "FPS: %d", (int)lround(1.0 / dt));
			gtk_label_set_text(GTK_LABEL(fps_label), buf);
		}
	}
	last_frame_time = now;

	gtk_widget_queue_draw(widget);
	return G_SOURCE_CONTINUE;
}

static void set_live_update(gboolean enabled)
{
	if (enabled && tick_id == 0) {
		last_frame_time = 0;
		tick_id = gtk_widget_add_tick_callback(drawing_area, on_tick, NULL, NULL);
		gtk_widget_set_visible(fps_label, TRUE);
	} else if (!enabled && tick_id != 0) {
		gtk_widget_remove_tick_callback(drawing_area, tick_id);
		tick_id = 0;
		gtk_label_set_text(GTK_LABEL(fps_label), "FPS: —");
		gtk_widget_set_visible(fps_label, FALSE);
		// One last redraw so the canvas stays correct after stopping
		gtk_widget_queue_draw(drawing_area);
	}
}

static void on_live_toggled(GtkToggleButton* button, gpointer user_data)
{
	PGL_UNUSED(user_data);
	set_live_update(gtk_toggle_button_get_active(button));
}

static void on_tri_color_set(GtkColorButton* button, gpointer user_data)
{
	PGL_UNUSED(user_data);
	GdkRGBA rgba;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &rgba);
	the_uniforms.v_color = (vec4){ rgba.red, rgba.green, rgba.blue, rgba.alpha };
	if (tick_id == 0)
		gtk_widget_queue_draw(drawing_area);
}

static void on_bg_color_set(GtkColorButton* button, gpointer user_data)
{
	PGL_UNUSED(user_data);
	GdkRGBA rgba;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &rgba);
	glClearColor(rgba.red, rgba.green, rgba.blue, rgba.alpha);
	if (tick_id == 0)
		gtk_widget_queue_draw(drawing_area);
}

static GtkWidget* make_color_button(const GdkRGBA* rgba, GCallback color_set_cb)
{
	GtkWidget* button = gtk_color_button_new_with_rgba(rgba);
	gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(button), TRUE);
	g_signal_connect(button, "color-set", color_set_cb, NULL);
	return button;
}

static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
	PGL_UNUSED(widget);
	PGL_UNUSED(user_data);

	if (event->keyval == GDK_KEY_Escape) {
		gtk_window_close(GTK_WINDOW(widget));
		return TRUE;
	}
	return FALSE;
}

// Pick a monitor rectangle to center on:
//   1) monitor under the pointer (best proxy for "where I launched from")
//   2) primary monitor, if available
//   3) first monitor in the display list
// Returns FALSE if no geometry could be obtained.
static gboolean get_startup_monitor_geometry(GdkDisplay* display, GdkRectangle* out_geo)
{
	// 1) Monitor under the pointer
	GdkSeat* seat = gdk_display_get_default_seat(display);
	if (seat) {
		GdkDevice* pointer = gdk_seat_get_pointer(seat);
		if (pointer) {
			int px = 0, py = 0;
			gdk_device_get_position(pointer, NULL, &px, &py);
			GdkMonitor* mon = gdk_display_get_monitor_at_point(display, px, py);
			if (mon) {
				gdk_monitor_get_geometry(mon, out_geo);
				return TRUE;
			}
		}
	}

	// 2) Primary monitor
	{
		GdkMonitor* primary = gdk_display_get_primary_monitor(display);
		if (primary) {
			gdk_monitor_get_geometry(primary, out_geo);
			return TRUE;
		}
	}

	// 3) First monitor
	if (gdk_display_get_n_monitors(display) > 0) {
		GdkMonitor* m = gdk_display_get_monitor(display, 0);
		if (m) {
			gdk_monitor_get_geometry(m, out_geo);
			return TRUE;
		}
	}
	return FALSE;
}

// Center once after the window is mapped.  Real moves only work on X11.
// On Wayland (and other backends that don't allow client placement), log and leave it.
static void center_window_on_startup(GtkWindow* window)
{
	GtkWidget* widget = GTK_WIDGET(window);
	GdkDisplay* display = gtk_widget_get_display(widget);

#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(display)) {
		GdkRectangle geo;
		if (!get_startup_monitor_geometry(display, &geo))
			return;

		int win_w = 0, win_h = 0;
		gtk_window_get_size(window, &win_w, &win_h);
		if (win_w <= 1 || win_h <= 1)
			gtk_window_get_default_size(window, &win_w, &win_h);

		int x = geo.x + (geo.width - win_w) / 2;
		int y = geo.y + (geo.height - win_h) / 2;
		// gtk_window_move is honored under X11; ignored by most Wayland compositors.
		gtk_window_move(window, x, y);
		return;
	}
#endif

	// Wayland (and anything else): clients cannot position normal toplevels.
	// The compositor places the window (often near the active seat / pointer).
	g_message("PortableGL GTK3: window placement is compositor-controlled on this "
	          "backend (e.g. Wayland); centering is only applied under X11.");
}

static void on_window_map(GtkWidget* widget, gpointer user_data)
{
	PGL_UNUSED(user_data);
	// Run once: disconnect so later hide/show doesn't re-center.
	g_signal_handlers_disconnect_by_func(widget, (gpointer)on_window_map, NULL);
	center_window_on_startup(GTK_WINDOW(widget));
}

static void box_pack(GtkBox* box, GtkWidget* child, gboolean expand, gboolean fill)
{
	gtk_box_pack_start(box, child, expand, fill, 0);
}

static GtkWidget* make_sidebar(void)
{
	GtkWidget* sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_size_request(sidebar, SIDEBAR_WIDTH, -1);
	gtk_widget_set_margin_start(sidebar, SIDEBAR_MARGIN);
	gtk_widget_set_margin_end(sidebar, SIDEBAR_MARGIN);
	gtk_widget_set_margin_top(sidebar, SIDEBAR_MARGIN);
	gtk_widget_set_margin_bottom(sidebar, SIDEBAR_MARGIN);
	gtk_widget_set_hexpand(sidebar, FALSE);
	gtk_widget_set_vexpand(sidebar, TRUE);

	GtkWidget* title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<b>Controls</b>");
	gtk_widget_set_halign(title, GTK_ALIGN_START);
	box_pack(GTK_BOX(sidebar), title, FALSE, FALSE);

	GtkWidget* live = gtk_check_button_new_with_label("Live update");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(live), TRUE);
	g_signal_connect(live, "toggled", G_CALLBACK(on_live_toggled), NULL);
	box_pack(GTK_BOX(sidebar), live, FALSE, FALSE);

	fps_label = gtk_label_new("FPS: —");
	gtk_widget_set_halign(fps_label, GTK_ALIGN_START);
	box_pack(GTK_BOX(sidebar), fps_label, FALSE, FALSE);

	box_pack(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE);

	GtkWidget* tri_label = gtk_label_new("Triangle color");
	gtk_widget_set_halign(tri_label, GTK_ALIGN_START);
	box_pack(GTK_BOX(sidebar), tri_label, FALSE, FALSE);

	GdkRGBA tri_rgba = { 1.0, 0.0, 0.0, 1.0 };
	box_pack(GTK_BOX(sidebar),
	         make_color_button(&tri_rgba, G_CALLBACK(on_tri_color_set)), FALSE, FALSE);

	GtkWidget* bg_label = gtk_label_new("Background color");
	gtk_widget_set_halign(bg_label, GTK_ALIGN_START);
	box_pack(GTK_BOX(sidebar), bg_label, FALSE, FALSE);

	GdkRGBA bg_rgba = { 0.0, 0.0, 0.0, 1.0 };
	box_pack(GTK_BOX(sidebar),
	         make_color_button(&bg_rgba, G_CALLBACK(on_bg_color_set)), FALSE, FALSE);

	size_label = gtk_label_new(NULL);
	update_size_label();
	gtk_label_set_justify(GTK_LABEL(size_label), GTK_JUSTIFY_LEFT);
	gtk_widget_set_halign(size_label, GTK_ALIGN_START);
	gtk_widget_set_valign(size_label, GTK_ALIGN_END);
	gtk_widget_set_vexpand(size_label, TRUE);
	gtk_style_context_add_class(gtk_widget_get_style_context(size_label), "dim-label");
	box_pack(GTK_BOX(sidebar), size_label, TRUE, TRUE);

	return sidebar;
}

static void activate(GtkApplication* app, gpointer user_data)
{
	PGL_UNUSED(user_data);

	bbufpix = NULL;
	if (!init_glContext(&the_Context, &bbufpix, WIDTH, HEIGHT)) {
		puts("Failed to initialize glContext");
		exit(1);
	}

	float points[] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		 0.0f,  0.5f, 0.0f
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

	GLuint program = pglCreateProgram(rotate_vs, uniform_color_fs, 0, NULL, GL_FALSE);
	glUseProgram(program);

	angle = 0.0f;
	the_uniforms.v_color = (vec4){ 1.0f, 0.0f, 0.0f, 1.0f };
	update_mvp();
	pglSetUniform(&the_uniforms);

	glClearColor(0, 0, 0, 1);
	glViewport(0, 0, fb_width, fb_height);

	GtkWidget* window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "PortableGL GTK3");
	// Default client size is sized so the expanding canvas is WIDTH x HEIGHT
	// (sidebar width + margins + separator eat into a naive WIDTH+SIDEBAR sum).
	gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_DEFAULT_W, WINDOW_DEFAULT_H);

	// Main layout: canvas | sidebar
	GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	drawing_area = gtk_drawing_area_new();
	// Minimum for the canvas only; overall window min height is driven by the sidebar.
	gtk_widget_set_size_request(drawing_area, CANVAS_MIN_W, CANVAS_MIN_H);
	gtk_widget_set_hexpand(drawing_area, TRUE);
	gtk_widget_set_vexpand(drawing_area, TRUE);
	g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
	g_signal_connect(drawing_area, "size-allocate", G_CALLBACK(on_size_allocate), NULL);
	box_pack(GTK_BOX(hbox), drawing_area, TRUE, TRUE);

	box_pack(GTK_BOX(hbox), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE);
	box_pack(GTK_BOX(hbox), make_sidebar(), FALSE, TRUE);

	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

	gtk_container_add(GTK_CONTAINER(window), hbox);

	// Live update on by default
	set_live_update(TRUE);

	// Center after map so the window exists and size is known (X11 only; see helper)
	g_signal_connect(window, "map", G_CALLBACK(on_window_map), NULL);

	gtk_widget_show_all(window);
	gtk_window_present(GTK_WINDOW(window));
}

static void shutdown_app(GApplication* app, gpointer user_data)
{
	PGL_UNUSED(app);
	PGL_UNUSED(user_data);
	if (tick_id != 0 && drawing_area) {
		gtk_widget_remove_tick_callback(drawing_area, tick_id);
		tick_id = 0;
	}
	free_glContext(&the_Context);
}

int main(int argc, char** argv)
{
	GtkApplication* app =
	    gtk_application_new("com.portablegl.gtk3", G_APPLICATION_DEFAULT_FLAGS);

	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(shutdown_app), NULL);

	int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	return status;
}

void rotate_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	My_Uniforms* u = (My_Uniforms*)uniforms;
	builtins->gl_Position = mult_m4_v4(u->mvp_mat, vertex_attribs[0]);
}

void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	builtins->gl_FragColor = ((My_Uniforms*)uniforms)->v_color;
}
