// Phase 2A: explicit texture2DLod / texture1DLod
#define PGL_EXCLUDE_STUBS
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WIDTH 16
#define HEIGHT 16

static int failures = 0;

#define EXPECT(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "FAIL: %s\n", msg); \
		failures++; \
	} \
} while (0)

static int near_color(vec4 c, float r, float g, float b, float eps)
{
	return fabsf(c.x - r) < eps && fabsf(c.y - g) < eps && fabsf(c.z - b) < eps;
}

int main(void)
{
	pix_t* bbuf = (pix_t*)calloc((size_t)WIDTH * HEIGHT, sizeof(pix_t));
	glContext ctx;
	if (!init_glContext(&ctx, &bbuf, WIDTH, HEIGHT)) {
		fprintf(stderr, "init_glContext failed\n");
		return 1;
	}
	set_glContext(&ctx);

	// Level 0: red 4x4, level 1 will be green (manual), Generate would average red
	Color red[16];
	for (int i = 0; i < 16; i++) {
		red[i].r = 255; red[i].g = 0; red[i].b = 0; red[i].a = 255;
	}
	Color green[4];
	for (int i = 0; i < 4; i++) {
		green[i].r = 0; green[i].g = 255; green[i].b = 0; green[i].a = 255;
	}
	Color blue[1];
	blue[0].r = 0; blue[0].g = 0; blue[0].b = 255; blue[0].a = 255;

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

	GLint minf;
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minf);
	EXPECT(minf == GL_NEAREST_MIPMAP_NEAREST, "min_filter stores full mipmap enum");

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, red);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, green);
	glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blue);

	// texture2D ignores lod — always L0 + mag
	vec4 c = texture2D(tex, 0.25f, 0.25f);
	EXPECT(near_color(c, 1.f, 0.f, 0.f, 0.02f), "texture2D samples L0 red");

	// Explicit lod
	c = texture2DLod(tex, 0.25f, 0.25f, 0.0f);
	EXPECT(near_color(c, 1.f, 0.f, 0.f, 0.02f), "Lod 0 is red");

	c = texture2DLod(tex, 0.25f, 0.25f, 1.0f);
	EXPECT(near_color(c, 0.f, 1.f, 0.f, 0.02f), "Lod 1 is green");

	c = texture2DLod(tex, 0.25f, 0.25f, 2.0f);
	EXPECT(near_color(c, 0.f, 0.f, 1.f, 0.02f), "Lod 2 is blue");

	// NEAREST_MIPMAP_NEAREST rounds: 0.6 -> 1, 1.4 -> 1
	c = texture2DLod(tex, 0.25f, 0.25f, 0.6f);
	EXPECT(near_color(c, 0.f, 1.f, 0.f, 0.02f), "Lod 0.6 rounds to L1");

	c = texture2DLod(tex, 0.25f, 0.25f, 99.0f);
	EXPECT(near_color(c, 0.f, 0.f, 1.f, 0.02f), "Lod clamps to max level");

	c = texture2DLod(tex, 0.25f, 0.25f, -5.0f);
	EXPECT(near_color(c, 1.f, 0.f, 0.f, 0.02f), "negative Lod uses mag on L0");

	// λ=0 is magnification: MAG_FILTER, not within-level(MIN)
	// Solid red L0 → NEAREST vs LINEAR both still red, so just confirm L0
	c = texture2DLod(tex, 0.25f, 0.25f, 0.0f);
	EXPECT(near_color(c, 1.f, 0.f, 0.f, 0.02f), "Lod 0 is mag path L0 red");
	// λ=0.49999 still L0 for *MIPMAP_NEAREST (round) but minify path
	c = texture2DLod(tex, 0.25f, 0.25f, 0.49999f);
	EXPECT(near_color(c, 1.f, 0.f, 0.f, 0.02f), "Lod 0.49999 stays L0 red (minify)");

	// LINEAR_MIPMAP_LINEAR blends floor(lod) and floor(lod)+1 (trilinear)
	// lod 0.9 => 0.1*L0(red) + 0.9*L1(green)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	c = texture2DLod(tex, 0.25f, 0.25f, 0.9f);
	EXPECT(near_color(c, 0.1f, 0.9f, 0.f, 0.05f), "LINEAR_MIPMAP_LINEAR blends L0/L1 at lod 0.9");
	// λ ≤ 0 with trilinear: still mag, not blend toward L1
	c = texture2DLod(tex, 0.25f, 0.25f, 0.0f);
	EXPECT(near_color(c, 1.f, 0.f, 0.f, 0.02f), "Lod 0 with trilinear min still pure L0 mag");

	// 1D lod
	GLuint tex1d;
	glGenTextures(1, &tex1d);
	glBindTexture(GL_TEXTURE_1D, tex1d);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	Color row0[4] = {
		{255,0,0,255},{255,0,0,255},{255,0,0,255},{255,0,0,255}
	};
	Color row1[2] = { {0,255,0,255}, {0,255,0,255} };
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, row0);
	glTexImage1D(GL_TEXTURE_1D, 1, GL_RGBA, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, row1);
	c = texture1D(tex1d, 0.25f);
	EXPECT(near_color(c, 1.f, 0.f, 0.f, 0.02f), "texture1D is L0");
	c = texture1DLod(tex1d, 0.25f, 1.0f);
	EXPECT(near_color(c, 0.f, 1.f, 0.f, 0.02f), "texture1DLod 1 is green");

	glDeleteTextures(1, &tex);
	glDeleteTextures(1, &tex1d);
	free_glContext(&ctx);
	free(bbuf);

	if (failures) {
		fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}
	printf("test_mipmap_lod: all checks passed\n");
	return 0;
}
