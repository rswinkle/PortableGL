// PortableGL + Qt (5 or 6) example with a simple control sidebar
// Functionally matches the GTK3/GTK4 backend demos
// Blits the software framebuffer with QImage / QPainter
//
// (c) Robert Winkler under MIT License

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QCursor>
#include <QElapsedTimer>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>

// QImage::Format_ARGB32 is 0xAARRGGBB (same layout Cairo uses for ARGB32 on LE)
#define PGL_ARGB32
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#define WIDTH  640
#define HEIGHT 480
#define SIDEBAR_WIDTH  220
#define SIDEBAR_MARGIN 12
#define SEP_WIDTH 1
#define WINDOW_DEFAULT_W (WIDTH + SEP_WIDTH + SIDEBAR_WIDTH + 2 * SIDEBAR_MARGIN)
#define WINDOW_DEFAULT_H HEIGHT
#define CANVAS_MIN_W 160
#define CANVAS_MIN_H 120

// Rotation speed when live update is on (radians per second)
#define ROTATE_RAD_PER_SEC  DEG_TO_RAD(60.0f)

typedef struct My_Uniforms {
	mat4 mvp_mat;
	vec4 v_color;
} My_Uniforms;

static pix_t* bbufpix;
static glContext the_Context;
static My_Uniforms the_uniforms;
static int fb_width  = WIDTH;
static int fb_height = HEIGHT;
static float angle; // freezes when live update is off

static void rotate_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms);
static void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms);

static void update_mvp(void)
{
	vec3 z_axis = { 0.0f, 0.0f, 1.0f };
	load_rotation_m4(the_uniforms.mvp_mat, z_axis, angle);
}

static void resize_framebuffer(int width, int height)
{
	if (width <= 0 || height <= 0)
		return;
	if (width == fb_width && height == fb_height)
		return;

	fb_width  = width;
	fb_height = height;

	pglResizeFramebuffer(fb_width, fb_height);
	bbufpix = (pix_t*)pglGetBackBuffer();
	glViewport(0, 0, fb_width, fb_height);
}

static void render_frame(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

// ---------------------------------------------------------------------------
// Canvas widget: owns paint/resize for the PGL back buffer
// ---------------------------------------------------------------------------
class PGLCanvas : public QWidget {
public:
	explicit PGLCanvas(QWidget* parent = nullptr)
	    : QWidget(parent)
	{
		setMinimumSize(CANVAS_MIN_W, CANVAS_MIN_H);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}

	QSize sizeHint() const override { return QSize(WIDTH, HEIGHT); }

protected:
	void paintEvent(QPaintEvent*) override
	{
		render_frame();

		// Format_ARGB32 matches PGL_ARGB32 / Cairo ARGB32 memory layout
		QImage img(reinterpret_cast<const uchar*>(bbufpix), fb_width, fb_height,
		           fb_width * (int)sizeof(pix_t), QImage::Format_ARGB32);
		QPainter p(this);
		p.drawImage(0, 0, img);
	}

	void resizeEvent(QResizeEvent* e) override
	{
		QWidget::resizeEvent(e);
		resize_framebuffer(width(), height());
		if (size_label)
			update_size_label();
	}

public:
	QLabel* size_label = nullptr;

	void update_size_label()
	{
		if (!size_label)
			return;
		size_label->setText(
		    QString("Framebuffer: %1×%2\nEscape to quit").arg(fb_width).arg(fb_height));
	}
};

// ---------------------------------------------------------------------------
// Color button: QPushButton that opens QColorDialog (no moc / no Q_OBJECT)
// ---------------------------------------------------------------------------
static QPushButton* make_color_button(const QColor& initial, QWidget* parent,
                                      const std::function<void(const QColor&)>& on_color)
{
	QPushButton* btn = new QPushButton(parent);
	btn->setMinimumHeight(28);
	auto apply_style = [btn](const QColor& c) {
		btn->setProperty("pgl_color", c);
		btn->setStyleSheet(
		    QString("background-color: %1; border: 1px solid #666;").arg(c.name(QColor::HexArgb)));
	};
	apply_style(initial);
	QObject::connect(btn, &QPushButton::clicked, btn, [btn, on_color, apply_style, initial]() {
		QColor cur = btn->property("pgl_color").value<QColor>();
		if (!cur.isValid())
			cur = initial;
		QColor c = QColorDialog::getColor(cur, btn, QString(), QColorDialog::ShowAlphaChannel);
		if (!c.isValid())
			return;
		apply_style(c);
		on_color(c);
	});
	return btn;
}

// ---------------------------------------------------------------------------
// Main window
// ---------------------------------------------------------------------------
class MainWindow : public QWidget {
public:
	MainWindow()
	{
		setWindowTitle("PortableGL Qt");
		resize(WINDOW_DEFAULT_W, WINDOW_DEFAULT_H);

		canvas = new PGLCanvas(this);

		QWidget* sidebar = new QWidget(this);
		sidebar->setFixedWidth(SIDEBAR_WIDTH + 2 * SIDEBAR_MARGIN);
		sidebar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

		QVBoxLayout* side_layout = new QVBoxLayout(sidebar);
		side_layout->setContentsMargins(SIDEBAR_MARGIN, SIDEBAR_MARGIN,
		                                SIDEBAR_MARGIN, SIDEBAR_MARGIN);
		side_layout->setSpacing(12);

		QLabel* title = new QLabel("<b>Controls</b>", sidebar);
		side_layout->addWidget(title);

		live_check = new QCheckBox("Live update", sidebar);
		live_check->setChecked(true);
		side_layout->addWidget(live_check);

		fps_label = new QLabel("FPS: —", sidebar);
		side_layout->addWidget(fps_label);

		QFrame* sep = new QFrame(sidebar);
		sep->setFrameShape(QFrame::HLine);
		side_layout->addWidget(sep);

		side_layout->addWidget(new QLabel("Triangle color", sidebar));
		side_layout->addWidget(make_color_button(QColor::fromRgbF(1, 0, 0, 1), sidebar,
		    [this](const QColor& c) {
			    the_uniforms.v_color =
			        (vec4){ (float)c.redF(), (float)c.greenF(), (float)c.blueF(), (float)c.alphaF() };
			    if (!tick_timer->isActive())
				    canvas->update();
		    }));

		side_layout->addWidget(new QLabel("Background color", sidebar));
		side_layout->addWidget(make_color_button(QColor::fromRgbF(0, 0, 0, 1), sidebar,
		    [this](const QColor& c) {
			    glClearColor((float)c.redF(), (float)c.greenF(), (float)c.blueF(),
			                 (float)c.alphaF());
			    if (!tick_timer->isActive())
				    canvas->update();
		    }));

		size_label = new QLabel(sidebar);
		size_label->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
		canvas->size_label = size_label;
		canvas->update_size_label();
		side_layout->addStretch(1);
		side_layout->addWidget(size_label);

		QFrame* vsep = new QFrame(this);
		vsep->setFrameShape(QFrame::VLine);
		vsep->setFixedWidth(SEP_WIDTH);

		QHBoxLayout* root = new QHBoxLayout(this);
		root->setContentsMargins(0, 0, 0, 0);
		root->setSpacing(0);
		root->addWidget(canvas, 1);
		root->addWidget(vsep);
		root->addWidget(sidebar);

		tick_timer = new QTimer(this);
		// 0 ≈ "as fast as the event loop allows"; dt still comes from a wall clock.
		tick_timer->setTimerType(Qt::PreciseTimer);
		QObject::connect(tick_timer, &QTimer::timeout, this, [this]() { on_tick(); });

		QObject::connect(live_check, &QCheckBox::toggled, this, [this](bool on) {
			set_live_update(on);
		});

		elapsed.start();
		last_ns = elapsed.nsecsElapsed();
		set_live_update(true);
	}

protected:
	void keyPressEvent(QKeyEvent* e) override
	{
		if (e->key() == Qt::Key_Escape)
			close();
		else
			QWidget::keyPressEvent(e);
	}

	void showEvent(QShowEvent* e) override
	{
		QWidget::showEvent(e);
		if (!did_center) {
			did_center = true;
			center_on_startup();
		}
	}

private:
	void set_live_update(bool enabled)
	{
		if (enabled) {
			last_ns = elapsed.nsecsElapsed();
			tick_timer->start(0);
			fps_label->setVisible(true);
		} else {
			tick_timer->stop();
			fps_label->setText("FPS: —");
			fps_label->setVisible(false);
			canvas->update(); // one last frame at frozen angle
		}
	}

	void on_tick()
	{
		qint64 now = elapsed.nsecsElapsed();
		double dt = (now - last_ns) / 1e9;
		last_ns = now;

		if (dt > 0.0 && dt < 1.0) { // ignore multi-second stalls
			angle += (float)(ROTATE_RAD_PER_SEC * dt);
			if (angle > 2.0f * (float)M_PI)
				angle = fmodf(angle, 2.0f * (float)M_PI);
			update_mvp();

			fps_label->setText(QString("FPS: %1").arg((int)lround(1.0 / dt)));
		}
		canvas->update();
	}

	// Window placement: under X11 we can center on a monitor.  Wayland does not
	// allow clients to position normal toplevel windows; the compositor decides.
	void center_on_startup()
	{
		const QString platform = QGuiApplication::platformName();
		if (platform.startsWith(QLatin1String("wayland"), Qt::CaseInsensitive)) {
			// Same situation as SDL/GTK: no client-driven placement for normal windows.
			qInfo("PortableGL Qt: window placement is compositor-controlled on Wayland; "
			      "centering is only applied under X11 (and similar).");
			return;
		}

		// 1) screen under the pointer  2) primary  3) first
		QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
		if (!screen)
			screen = QGuiApplication::primaryScreen();
		if (!screen) {
			const auto screens = QGuiApplication::screens();
			if (!screens.isEmpty())
				screen = screens.first();
		}
		if (!screen)
			return;

		const QRect geo = screen->availableGeometry();
		const QRect frame = frameGeometry();
		move(geo.x() + (geo.width() - frame.width()) / 2,
		     geo.y() + (geo.height() - frame.height()) / 2);
	}

	PGLCanvas* canvas = nullptr;
	QCheckBox* live_check = nullptr;
	QLabel* fps_label = nullptr;
	QLabel* size_label = nullptr;
	QTimer* tick_timer = nullptr;
	QElapsedTimer elapsed;
	qint64 last_ns = 0;
	bool did_center = false;
};

static void setup_gl(void)
{
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
}

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	// High-DPI: keep QImage pixel size tied to widget size in device-independent
	// pixels by default; fine for a software framebuffer demo.

	setup_gl();

	MainWindow win;
	win.show();

	int status = app.exec();
	free_glContext(&the_Context);
	return status;
}

static void rotate_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	My_Uniforms* u = (My_Uniforms*)uniforms;
	builtins->gl_Position = mult_m4_v4(u->mvp_mat, vertex_attribs[0]);
}

static void uniform_color_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	builtins->gl_FragColor = ((My_Uniforms*)uniforms)->v_color;
}
