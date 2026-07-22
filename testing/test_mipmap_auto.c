// Phase 2B: per-triangle automatic LOD for texture2D with *MIPMAP* min filters
#define PGL_EXCLUDE_STUBS
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WIDTH 64
#define HEIGHT 64

static int failures = 0;

#define EXPECT(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "FAIL: %s\n", msg); \
		failures++; \
	} \
} while (0)

// Read pixel with OpenGL bottom-left origin
static void read_pixel(glContext* ctx, int x, int y, u8* r, u8* g, u8* b, u8* a)
{
	pix_t* p = &((pix_t*)ctx->back_buffer.lastrow)[-y * ctx->back_buffer.w + x];
	Color c = PIXEL_TO_COLOR(*p);
	*r = c.r; *g = c.g; *b = c.b; *a = c.a;
}

static void draw_textured_quad(float x0, float y0, float x1, float y1, GLuint tex)
{
	float points[] = {
		x0, y1, -0.1f,
		x0, y0, -0.1f,
		x1, y1, -0.1f,
		x1, y0, -0.1f,
	};
	float uvs[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};

	GLuint vbo, tbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_VERT);
	glVertexAttribPointer(PGL_ATTR_VERT, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &tbo);
	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
	glEnableVertexAttribArray(PGL_ATTR_TEXCOORD0);
	glVertexAttribPointer(PGL_ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLuint shaders[PGL_NUM_SHADERS];
	pgl_init_std_shaders(shaders);
	glUseProgram(shaders[PGL_SHADER_TEX_REPLACE]);

	pgl_uniforms u;
	pglSetUniform(&u);
	SET_IDENTITY_M4(u.mvp_mat);
	u.tex0 = tex;

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &tbo);
}

static void fill_solid(Color* px, int n, u8 r, u8 g, u8 b)
{
	for (int i = 0; i < n; i++) {
		px[i].r = r; px[i].g = g; px[i].b = b; px[i].a = 255;
	}
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

	// Distinct colors per level: L0 red, L1 green, L2 blue, L3 white
	Color l0[64]; // 8x8
	Color l1[16]; // 4x4
	Color l2[4];  // 2x2
	Color l3[1];  // 1x1
	fill_solid(l0, 64, 255, 0, 0);
	fill_solid(l1, 16, 0, 255, 0);
	fill_solid(l2, 4, 0, 0, 255);
	fill_solid(l3, 1, 255, 255, 255);

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, l0);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, l1);
	glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, l2);
	glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, l3);

	// --- Large quad: UV 0..1 over ~64px => uv/px ~ 1/64, ρ ~ 8/64 = 0.125 => λ < 0 => L0 red
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	draw_textured_quad(-1.0f, -1.0f, 1.0f, 1.0f, tex);

	u8 r, g, b, a;
	read_pixel(&ctx, WIDTH / 2, HEIGHT / 2, &r, &g, &b, &a);
	EXPECT(r > 200 && g < 30 && b < 30, "large on-screen quad uses L0 (red)");

	// --- Tiny quad: ~2px wide in center, UV 0..1 => uv/px ~ 0.5, ρ ~ 4 => λ ~ 2 => L2 blue
	glClear(GL_COLOR_BUFFER_BIT);
	// 2/64 of NDC width ≈ 2 pixels
	float half = 2.0f / (float)WIDTH;
	draw_textured_quad(-half, -half, half, half, tex);
	read_pixel(&ctx, WIDTH / 2, HEIGHT / 2, &r, &g, &b, &a);
	EXPECT(b > 200 && r < 30 && g < 30, "tiny on-screen quad uses higher mip (blue L2)");

	// --- Non-mip min filter still always L0 even when tiny
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glClear(GL_COLOR_BUFFER_BIT);
	draw_textured_quad(-half, -half, half, half, tex);
	read_pixel(&ctx, WIDTH / 2, HEIGHT / 2, &r, &g, &b, &a);
	EXPECT(r > 200 && g < 30 && b < 30, "NEAREST min filter ignores auto LOD (red L0)");

	// Restore mip filter; no chain of 1 level after re-upload L0 only would clear mips
	// Explicit Lod still works independently of triangle size
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	// Re-upload chain (L0 replace wiped mips)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, l0);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, l1);
	glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, l2);
	glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, l3);

	vec4 c = texture2DLod(tex, 0.5f, 0.5f, 1.0f);
	EXPECT(c.y > 0.9f && c.x < 0.1f, "texture2DLod still selects L1 green");

	glDeleteTextures(1, &tex);
	free_glContext(&ctx);
	free(bbuf);

	if (failures) {
		fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}
	printf("test_mipmap_auto: all checks passed\n");
	return 0;
}
