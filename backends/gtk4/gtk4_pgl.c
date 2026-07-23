// PortableGL + GTK4 example with a simple control sidebar
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
#include <gdk/x11/gdkx.h>
#endif

// Cairo CAIRO_FORMAT_ARGB32 is native-endian 0xAARRGGBB (B,G,R,A bytes on LE)
#define PGL_ARGB32
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define WIDTH  640
#define HEIGHT 480
#define SIDEBAR_WIDTH 220

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

static void on_resize(GtkDrawingArea* area, int width, int height, gpointer user_data)
{
	PGL_UNUSED(area);
	PGL_UNUSED(user_data);

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

static void render_frame(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

static void on_draw(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer user_data)
{
	PGL_UNUSED(area);
	PGL_UNUSED(user_data);
	PGL_UNUSED(width);
	PGL_UNUSED(height);

	render_frame();

	// Blit PGL back buffer to the widget via Cairo
	cairo_surface_t* surface = cairo_image_surface_create_for_data(
	    (unsigned char*)bbufpix, CAIRO_FORMAT_ARGB32,
	    fb_width, fb_height, fb_width * (int)sizeof(pix_t));
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);
	cairo_surface_destroy(surface);
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

static void on_live_toggled(GtkCheckButton* button, gpointer user_data)
{
	PGL_UNUSED(user_data);
	set_live_update(gtk_check_button_get_active(button));
}

static void on_tri_color_notify(GtkColorDialogButton* button, GParamSpec* pspec, gpointer user_data)
{
	PGL_UNUSED(pspec);
	PGL_UNUSED(user_data);
	const GdkRGBA* rgba = gtk_color_dialog_button_get_rgba(button);
	the_uniforms.v_color = (vec4){ rgba->red, rgba->green, rgba->blue, rgba->alpha };
	if (tick_id == 0)
		gtk_widget_queue_draw(drawing_area);
}

static void on_bg_color_notify(GtkColorDialogButton* button, GParamSpec* pspec, gpointer user_data)
{
	PGL_UNUSED(pspec);
	PGL_UNUSED(user_data);
	const GdkRGBA* rgba = gtk_color_dialog_button_get_rgba(button);
	glClearColor(rgba->red, rgba->green, rgba->blue, rgba->alpha);
	if (tick_id == 0)
		gtk_widget_queue_draw(drawing_area);
}

static GtkWidget* make_color_button(const GdkRGBA* rgba, GCallback notify_cb)
{
	GtkColorDialog* dialog = gtk_color_dialog_new();
	gtk_color_dialog_set_with_alpha(dialog, TRUE);
	GtkWidget* button = gtk_color_dialog_button_new(dialog);
	gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(button), rgba);
	g_signal_connect(button, "notify::rgba", notify_cb, NULL);
	return button;
}

static gboolean on_key_pressed(GtkEventControllerKey* controller, guint keyval,
                               guint keycode, GdkModifierType state, gpointer user_data)
{
	PGL_UNUSED(controller);
	PGL_UNUSED(keycode);
	PGL_UNUSED(state);

	if (keyval == GDK_KEY_Escape) {
		gtk_window_close(GTK_WINDOW(user_data));
		return TRUE;
	}
	return FALSE;
}

// Pick a monitor rectangle to center on:
//   1) monitor under the pointer (best proxy for "where I launched from")
//   2) X11 primary monitor, if available
//   3) first monitor in the display list
// Returns FALSE if no geometry could be obtained.
static gboolean get_startup_monitor_geometry(GdkDisplay* display, GdkRectangle* out_geo)
{
	GListModel* monitors = gdk_display_get_monitors(display);
	guint n = g_list_model_get_n_items(monitors);
	if (n == 0)
		return FALSE;

#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(display)) {
		Display* xdpy = gdk_x11_display_get_xdisplay(display);
		Window root = gdk_x11_display_get_xrootwindow(display);
		Window root_ret, child;
		int root_x, root_y, win_x, win_y;
		unsigned int mask;

		// 1) Monitor under the pointer
		if (XQueryPointer(xdpy, root, &root_ret, &child, &root_x, &root_y, &win_x, &win_y, &mask)) {
			for (guint i = 0; i < n; i++) {
				GdkMonitor* m = g_list_model_get_item(monitors, i);
				GdkRectangle g;
				gdk_monitor_get_geometry(m, &g);
				g_object_unref(m);
				if (root_x >= g.x && root_x < g.x + g.width &&
				    root_y >= g.y && root_y < g.y + g.height) {
					*out_geo = g;
					return TRUE;
				}
			}
		}

		// 2) X11 primary monitor
		{
			GdkMonitor* primary = gdk_x11_display_get_primary_monitor(display);
			if (primary) {
				gdk_monitor_get_geometry(primary, out_geo);
				return TRUE;
			}
		}
	}
#endif

	// 3) First monitor (portable fallback for non-X11 or if above failed)
	{
		GdkMonitor* m = g_list_model_get_item(monitors, 0);
		gdk_monitor_get_geometry(m, out_geo);
		g_object_unref(m);
		return TRUE;
	}
}

// Center once after the window is mapped.  Real moves only work on X11.
// On Wayland (and other backends that don't allow client placement), log and leave it.
static void center_window_on_startup(GtkWindow* window)
{
	GtkWidget* widget = GTK_WIDGET(window);
	GdkDisplay* display = gtk_widget_get_display(widget);
	GdkSurface* surface = gtk_native_get_surface(GTK_NATIVE(window));
	if (!surface)
		return;

#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(display) && GDK_IS_X11_SURFACE(surface)) {
		GdkRectangle geo;
		if (!get_startup_monitor_geometry(display, &geo))
			return;

		int win_w = gtk_widget_get_width(widget);
		int win_h = gtk_widget_get_height(widget);
		if (win_w <= 1 || win_h <= 1)
			gtk_window_get_default_size(window, &win_w, &win_h);

		int x = geo.x + (geo.width - win_w) / 2;
		int y = geo.y + (geo.height - win_h) / 2;

		Display* xdpy = gdk_x11_display_get_xdisplay(display);
		XMoveWindow(xdpy, gdk_x11_surface_get_xid(surface), x, y);
		return;
	}
#endif

	// Wayland (and anything else): clients cannot position normal toplevels.
	// The compositor places the window (often near the active seat / pointer).
	g_message("PortableGL GTK4: window placement is compositor-controlled on this "
	          "backend (e.g. Wayland); centering is only applied under X11.");
}

static void on_window_map(GtkWidget* widget, gpointer user_data)
{
	PGL_UNUSED(user_data);
	// Run once: disconnect so later hide/show doesn't re-center.
	g_signal_handlers_disconnect_by_func(widget, (gpointer)on_window_map, NULL);
	center_window_on_startup(GTK_WINDOW(widget));
}

static GtkWidget* make_sidebar(void)
{
	GtkWidget* sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	gtk_widget_set_size_request(sidebar, SIDEBAR_WIDTH, -1);
	gtk_widget_set_margin_start(sidebar, 12);
	gtk_widget_set_margin_end(sidebar, 12);
	gtk_widget_set_margin_top(sidebar, 12);
	gtk_widget_set_margin_bottom(sidebar, 12);
	gtk_widget_set_hexpand(sidebar, FALSE);
	gtk_widget_set_vexpand(sidebar, TRUE);

	GtkWidget* title = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(title), "<b>Controls</b>");
	gtk_widget_set_halign(title, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(sidebar), title);

	GtkWidget* live = gtk_check_button_new_with_label("Live update");
	gtk_check_button_set_active(GTK_CHECK_BUTTON(live), TRUE);
	g_signal_connect(live, "toggled", G_CALLBACK(on_live_toggled), NULL);
	gtk_box_append(GTK_BOX(sidebar), live);

	fps_label = gtk_label_new("FPS: —");
	gtk_widget_set_halign(fps_label, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(sidebar), fps_label);

	gtk_box_append(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

	GtkWidget* tri_label = gtk_label_new("Triangle color");
	gtk_widget_set_halign(tri_label, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(sidebar), tri_label);

	GdkRGBA tri_rgba = { 1.0, 0.0, 0.0, 1.0 };
	gtk_box_append(GTK_BOX(sidebar),
	               make_color_button(&tri_rgba, G_CALLBACK(on_tri_color_notify)));

	GtkWidget* bg_label = gtk_label_new("Background color");
	gtk_widget_set_halign(bg_label, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(sidebar), bg_label);

	GdkRGBA bg_rgba = { 0.0, 0.0, 0.0, 1.0 };
	gtk_box_append(GTK_BOX(sidebar),
	               make_color_button(&bg_rgba, G_CALLBACK(on_bg_color_notify)));

	size_label = gtk_label_new(NULL);
	update_size_label();
	gtk_label_set_justify(GTK_LABEL(size_label), GTK_JUSTIFY_LEFT);
	gtk_widget_set_halign(size_label, GTK_ALIGN_START);
	gtk_widget_set_valign(size_label, GTK_ALIGN_END);
	gtk_widget_set_vexpand(size_label, TRUE);
	gtk_widget_add_css_class(size_label, "dim-label");
	gtk_box_append(GTK_BOX(sidebar), size_label);

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
	gtk_window_set_title(GTK_WINDOW(window), "PortableGL GTK4");
	// Default size: canvas + sidebar (+ margins/separator). Window is resizable;
	// the drawing area expands and resizes the PGL framebuffer.
	gtk_window_set_default_size(GTK_WINDOW(window), WIDTH + SIDEBAR_WIDTH + 24, HEIGHT);

	// Main layout: canvas | sidebar
	GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawing_area, 160, 120); // modest minimum
	gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing_area), WIDTH);
	gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing_area), HEIGHT);
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), on_draw, NULL, NULL);
	g_signal_connect(drawing_area, "resize", G_CALLBACK(on_resize), NULL);
	gtk_widget_set_hexpand(drawing_area, TRUE);
	gtk_widget_set_vexpand(drawing_area, TRUE);
	gtk_box_append(GTK_BOX(hbox), drawing_area);

	gtk_box_append(GTK_BOX(hbox), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
	gtk_box_append(GTK_BOX(hbox), make_sidebar());

	GtkEventController* keys = gtk_event_controller_key_new();
	g_signal_connect(keys, "key-pressed", G_CALLBACK(on_key_pressed), window);
	gtk_widget_add_controller(window, keys);

	gtk_window_set_child(GTK_WINDOW(window), hbox);

	// Live update on by default
	set_live_update(TRUE);

	// Center after map so the surface exists and size is known (X11 only; see helper)
	g_signal_connect(window, "map", G_CALLBACK(on_window_map), NULL);

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
	    gtk_application_new("com.portablegl.gtk4", G_APPLICATION_DEFAULT_FLAGS);

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
