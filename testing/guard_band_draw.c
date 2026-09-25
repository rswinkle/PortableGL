// Guard-band behavior that a PNG golden does not lock: viewport line reject,
// a shared edge that crosses the viewport, a vertex far outside the guard,
// and a w < 0 corner that must not be perspective-divided.

static pix_t guard_at(int x, int y)
{
	return ((pix_t*)the_Context.back_buffer.lastrow)[-y * the_Context.back_buffer.w + x];
}

static int guard_drawn(int x, int y)
{
	Color col = PIXEL_TO_COLOR(guard_at(x, y));
	return col.r > 0;
}

static void guard_pos_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(vs_output);
	PGL_UNUSED(uniforms);
	builtins->gl_Position = vertex_attribs[0];
}

static void guard_red_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(fs_input);
	PGL_UNUSED(uniforms);
	SET_V4(builtins->gl_FragColor, 1.0f, 0.0f, 0.0f, 1.0f);
}

static void guard_draw(GLenum mode, float* verts, int ncomp, int nvert)
{
	GLuint buf;
	glGenBuffers(1, &buf);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(ncomp * nvert * (int)sizeof(float)), verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, ncomp, GL_FLOAT, GL_FALSE, 0, 0);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(mode, 0, nvert);
	glDeleteBuffers(1, &buf);
}

static int guard_px(float ndc, float origin, float size)
{
	float A = (size - 0.01f) * 0.5f;
	float X = ndc * A + (origin + A);
	int p = (int)X;
	if (p < 0)
		p = 0;
	return p;
}

void guard_band_line_inset(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int inset = 40;
	int x0 = inset;
	int y0 = inset;
	int x1 = WIDTH - inset;
	int y1 = HEIGHT - inset;
	float line[] = { -1.5f, 0.0f, 0.0f, 1.5f, 0.0f, 0.0f };
	int outside = 0;
	int inside = 0;
	int x, y;

	glViewport(x0, y0, x1 - x0, y1 - y0);
	glLineWidth(32);
	guard_draw(GL_LINES, line, 3, 2);

	for (y = 0; y < HEIGHT; ++y) {
		for (x = 0; x < WIDTH; ++x) {
			if (!guard_drawn(x, y))
				continue;
			if (x < x0 || x >= x1 || y < y0 || y >= y1)
				outside++;
			else
				inside++;
		}
	}
	PGL_EXPECT(outside == 0, "width-32 line painted outside the viewport");
	PGL_EXPECT(inside > 0, "width-32 line did not draw");
}

void guard_band_shared_edge(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	/* Shared edge from (-0.5,-0.2) to (1.2,0.4) crosses the viewport. */
	float verts[] = {
		-0.5f, -0.2f, 0.0f,
		 1.2f,  0.4f, 0.0f,
		-0.5f,  0.7f, 0.0f,

		-0.5f, -0.2f, 0.0f,
		-0.5f, -0.8f, 0.0f,
		 1.2f,  0.4f, 0.0f,
	};
	int holes = 0;
	int i;

	guard_draw(GL_TRIANGLES, verts, 3, 6);

	for (i = 0; i < 16; ++i) {
		float s = 0.30f + 0.02f * (float)i;
		float nx = -0.5f + s * 1.7f;
		float ny = -0.2f + s * 0.6f;
		int x = guard_px(nx, 0, (float)WIDTH);
		int y = guard_px(ny, 0, (float)HEIGHT);
		if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT && !guard_drawn(x, y))
			holes++;
	}
	PGL_EXPECT(holes == 0, "background pixel on a shared in-guard edge");
	PGL_EXPECT(guard_drawn(guard_px(-0.4f, 0, (float)WIDTH),
	                       guard_px(0.3f, 0, (float)HEIGHT)),
	           "upper triangle missing");
	PGL_EXPECT(guard_drawn(guard_px(-0.4f, 0, (float)WIDTH),
	                       guard_px(-0.45f, 0, (float)HEIGHT)),
	           "lower triangle missing");
}

void guard_band_far_vert(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	/* x = 8 is outside the 1024 px guard (~4.2 NDC) and still leaves area. */
	float verts[] = {
		-0.6f, -0.6f, 0.0f,
		 0.2f, -0.6f, 0.0f,
		 8.0f,  1.2f, 0.0f,
	};
	int x, y, covered = 0;

	guard_draw(GL_TRIANGLES, verts, 3, 3);

	for (y = 0; y < HEIGHT; ++y) {
		for (x = 0; x < WIDTH; ++x) {
			if (guard_drawn(x, y))
				covered++;
		}
	}
	PGL_EXPECT(guard_drawn(guard_px(-0.2f, 0, (float)WIDTH),
	                       guard_px(-0.55f, 0, (float)HEIGHT)),
	           "base of the far triangle missing");
	PGL_EXPECT(!guard_drawn(guard_px(-0.9f, 0, (float)WIDTH),
	                        guard_px(0.9f, 0, (float)HEIGHT)),
	           "far triangle covered the opposite corner");
	PGL_EXPECT(covered > 100 && covered < WIDTH * HEIGHT / 2, "far triangle coverage");
}

void guard_band_w_negative(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	/* Two in front, one behind the eye. Dividing the behind corner mirrors. */
	float verts[] = {
		-0.4f, -0.6f, 1.0f, 1.0f,
		 0.4f, -0.6f, 1.0f, 1.0f,
		 0.0f,  0.9f, -1.0f, -1.0f,
	};
	GLuint prog = pglCreateProgram(guard_pos_vs, guard_red_fs, 0, NULL, GL_FALSE);

	glUseProgram(prog);
	guard_draw(GL_TRIANGLES, verts, 4, 3);
	glUseProgram(0);

	/* Dividing w=-1 mirrors the corner below the base. Near clipping
	   keeps the wedge above the base and leaves that mirror black. */
	PGL_EXPECT(guard_drawn(guard_px(0.0f, 0, (float)WIDTH),
	                       guard_px(-0.5f, 0, (float)HEIGHT)),
	           "near-clipped triangle missed the front edge");
	PGL_EXPECT(guard_drawn(guard_px(0.0f, 0, (float)WIDTH),
	                       guard_px(0.2f, 0, (float)HEIGHT)),
	           "near-clipped wedge missing above the base");
	PGL_EXPECT(!guard_drawn(guard_px(0.0f, 0, (float)WIDTH),
	                        guard_px(-0.85f, 0, (float)HEIGHT)),
	           "w < 0 vertex was divided into the mirror below the base");
}
