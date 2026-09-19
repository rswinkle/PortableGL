
// Dense flat grid at ~1 triangle per pixel, rotated so edges are not
// 45°/axis-aligned (those make line_func exactly 0 and the old sentinel
// already works). Same density as Iceland CPU terrain from the article
// camera. PGL_EXPECT fails on clear-color holes in a conservative interior.
void fill_watertight(int argc, char** argv, void* data)
{
	PGL_UNUSED(argc);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	const int n = 256; /* quads on a side */
	const int nv = n + 1;
	const int nverts = nv * nv;
	const int nidx = n * n * 6;
	const float ang = 23.0f * 3.14159265f / 180.0f;
	const float ca = cosf(ang);
	const float sa = sinf(ang);

	float* verts = (float*)malloc((size_t)nverts * 3 * sizeof(float));
	GLuint* idx = (GLuint*)malloc((size_t)nidx * sizeof(GLuint));
	PGL_EXPECT(verts && idx, "grid alloc");
	if (!verts || !idx) {
		free(verts);
		free(idx);
		return;
	}

	/* NDC square [-0.5, 0.5], then rotate. ~1.25 px/quad at 640. */
	for (int j = 0; j < nv; ++j) {
		float v = (float)j / (float)n - 0.5f;
		for (int i = 0; i < nv; ++i) {
			float u = (float)i / (float)n - 0.5f;
			float* p = &verts[(j * nv + i) * 3];
			p[0] = ca * u - sa * v;
			p[1] = sa * u + ca * v;
			p[2] = 0.0f;
		}
	}

	int k = 0;
	for (int j = 0; j < n; ++j) {
		for (int i = 0; i < n; ++i) {
			GLuint a = (GLuint)(j * nv + i);
			GLuint b = a + 1;
			GLuint d = a + (GLuint)nv + 1;
			GLuint cidx = a + (GLuint)nv;
			idx[k++] = a;
			idx[k++] = b;
			idx[k++] = d;
			idx[k++] = a;
			idx[k++] = d;
			idx[k++] = cidx;
		}
	}

	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, verts);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, nidx, GL_UNSIGNED_INT, idx);

	/* Inscribed disk of the unrotated square survives the rotation. */
	int cx = WIDTH / 2;
	int cy = HEIGHT / 2;
	int r2 = 120 * 120;
	int holes = 0;
	for (int y = cy - 120; y <= cy + 120; ++y) {
		for (int x = cx - 120; x <= cx + 120; ++x) {
			int dx = x - cx, dy = y - cy;
			if (dx * dx + dy * dy >= r2)
				continue;
			pix_t pix = ((pix_t*)the_Context.back_buffer.lastrow)[-y * WIDTH + x];
			Color col = PIXEL_TO_COLOR(pix);
			if (col.r == 0 && col.g == 0 && col.b == 0)
				holes++;
		}
	}
	char msg[64];
	snprintf(msg, sizeof(msg), "interior clear-color holes=%d", holes);
	PGL_EXPECT(holes == 0, msg);

	free(verts);
	free(idx);
}
