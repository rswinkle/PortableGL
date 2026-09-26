// Flat color on GL_LINES, GL_LINE_STRIP, and GL_LINE_LOOP.
// LAST and FIRST provoking vertices. Width 1 and width 16 share the geometry.
// A provoke index divided by sizeof(glVertex) paints these segments vertex 0's color.

static void flat_line_vs(float* vs_output, vec4* vertex_attribs, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(uniforms);
	builtins->gl_Position.x = vertex_attribs[PGL_ATTR_VERT].x;
	builtins->gl_Position.y = vertex_attribs[PGL_ATTR_VERT].y;
	builtins->gl_Position.z = 0.0f;
	builtins->gl_Position.w = 1.0f;
	((vec4*)vs_output)[0] = vertex_attribs[PGL_ATTR_COLOR];
}

static void flat_line_fs(float* fs_input, Shader_Builtins* builtins, void* uniforms)
{
	PGL_UNUSED(uniforms);
	builtins->gl_FragColor = ((vec4*)fs_input)[0];
}

static int flat_ndc_px(float ndc, int size)
{
	float A = ((float)size - 0.01f) * 0.5f;
	int p = (int)(ndc * A + A);
	if (p < 0)
		p = 0;
	if (p >= size)
		p = size - 1;
	return p;
}

// 0 red, 1 green, 2 blue, 3 yellow. Compares channels, so RGB565's 5-bit red still matches.
static int flat_is(int x, int y, int kind)
{
	pix_t p = ((pix_t*)the_Context.back_buffer.lastrow)[-y * the_Context.back_buffer.w + x];
	Color c = PIXEL_TO_COLOR(p);
	if (kind == 0)
		return c.r > c.g && c.r > c.b && c.r > 0;
	if (kind == 1)
		return c.g > c.r && c.g > c.b && c.g > 0;
	if (kind == 2)
		return c.b > c.r && c.b > c.g && c.b > 0;
	return c.r > 0 && c.g > 0 && c.b == 0;
}

// Pixels of kind in a box around the NDC point. The box covers a width-16 line.
static int flat_count(float ndc_x, float ndc_y, int kind)
{
	int cx = flat_ndc_px(ndc_x, WIDTH);
	int cy = flat_ndc_px(ndc_y, HEIGHT);
	int x, y, n = 0;
	for (y = cy - 14; y <= cy + 14; ++y) {
		if (y < 0 || y >= HEIGHT)
			continue;
		for (x = cx - 14; x <= cx + 14; ++x) {
			if (x < 0 || x >= WIDTH)
				continue;
			if (flat_is(x, y, kind))
				n++;
		}
	}
	return n;
}

static void flat_expect(const char* what, float x, float y, int kind, int min_hits)
{
	int n = flat_count(x, y, kind);
	PGL_EXPECT(n >= min_hits, what);
}

static void flat_draw(GLuint buf, GLenum mode, float* v, int nvert)
{
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(nvert * 6 * (int)sizeof(float)), v, GL_STATIC_DRAW);
	glDrawArrays(mode, 0, nvert);
}

void flat_line_provoke(int num, char** argv, void* data)
{
	float width = (num == 1) ? 16.0f : 1.0f;
	int min_hits = (num == 1) ? 16 : 1;
	GLenum interp[] = { PGL_FLAT4 };
	GLuint prog, buf;

	// x, y, r, g, b, a
	float lines[] = {
		-0.95f, 0.82f,  1, 0, 0, 1,
		-0.55f, 0.82f,  0, 1, 0, 1,
		-0.15f, 0.82f,  0, 0, 1, 1,
		 0.35f, 0.82f,  1, 1, 0, 1
	};
	float strip[] = {
		-0.95f, 0.55f,  1, 0, 0, 1,
		-0.15f, 0.55f,  0, 1, 0, 1,
		 0.65f, 0.55f,  0, 0, 1, 1
	};
	float loop[] = {
		-0.85f, 0.16f,  1, 0, 0, 1,
		-0.15f, 0.42f,  0, 1, 0, 1,
		 0.55f, 0.16f,  0, 0, 1, 1
	};
	float lines_lo[] = {
		-0.95f, -0.10f,  1, 0, 0, 1,
		-0.55f, -0.10f,  0, 1, 0, 1,
		-0.15f, -0.10f,  0, 0, 1, 1,
		 0.35f, -0.10f,  1, 1, 0, 1
	};
	float strip_lo[] = {
		-0.95f, -0.40f,  1, 0, 0, 1,
		-0.15f, -0.40f,  0, 1, 0, 1,
		 0.65f, -0.40f,  0, 0, 1, 1
	};
	float loop_lo[] = {
		-0.85f, -0.84f,  1, 0, 0, 1,
		-0.15f, -0.58f,  0, 1, 0, 1,
		 0.55f, -0.84f,  0, 0, 1, 1
	};

	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	prog = pglCreateProgram(flat_line_vs, flat_line_fs, 4, interp, GL_FALSE);
	glUseProgram(prog);
	glGenBuffers(1, &buf);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glEnableVertexAttribArray(PGL_ATTR_COLOR);
	pglVertexAttribPointer(PGL_ATTR_VERT, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);
	pglVertexAttribPointer(PGL_ATTR_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 2));

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glLineWidth(width);
	glProvokingVertex(GL_LAST_VERTEX_CONVENTION);

	flat_draw(buf, GL_LINES, lines, 4);
	flat_draw(buf, GL_LINE_STRIP, strip, 3);
	flat_draw(buf, GL_LINE_LOOP, loop, 3);

	// LAST: second vertex of each segment.
	flat_expect("LAST GL_LINES first segment", -0.75f, 0.82f, 1, min_hits);
	flat_expect("LAST GL_LINES second segment", 0.10f, 0.82f, 3, min_hits);
	flat_expect("LAST GL_LINE_STRIP second segment", 0.25f, 0.55f, 2, min_hits);
	flat_expect("LAST GL_LINE_LOOP v1-v2", 0.20f, 0.29f, 2, min_hits);

	glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
	flat_draw(buf, GL_LINES, lines_lo, 4);
	flat_draw(buf, GL_LINE_STRIP, strip_lo, 3);
	flat_draw(buf, GL_LINE_LOOP, loop_lo, 3);

	// FIRST: first vertex. The loop close is (last, first), so its provoke is the last vertex.
	flat_expect("FIRST GL_LINES second segment", 0.10f, -0.10f, 2, min_hits);
	flat_expect("FIRST GL_LINE_STRIP second segment", 0.25f, -0.40f, 1, min_hits);
	flat_expect("FIRST GL_LINE_LOOP close", -0.15f, -0.84f, 2, min_hits);

	glDeleteBuffers(1, &buf);
	glDeleteProgram(prog);
}
