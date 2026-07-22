// Smoke test for mipmap phase 1: storage, GenerateMipmap, texelFetch/textureSize lod
#define PGL_EXCLUDE_STUBS
#define PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 32
#define HEIGHT 32

static int failures = 0;

#define EXPECT(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "FAIL: %s\n", msg); \
		failures++; \
	} \
} while (0)

int main(void)
{
	pix_t* bbuf = (pix_t*)calloc((size_t)WIDTH * HEIGHT, sizeof(pix_t));
	glContext ctx;
	if (!init_glContext(&ctx, &bbuf, WIDTH, HEIGHT)) {
		fprintf(stderr, "init_glContext failed\n");
		return 1;
	}
	set_glContext(&ctx);

	// 4x4 checker-like solid colors per texel for easy averaging checks
	// All red (255,0,0,255) so every mip stays red
	Color base[16];
	for (int i = 0; i < 16; i++) {
		base[i].r = 255;
		base[i].g = 0;
		base[i].b = 0;
		base[i].a = 255;
	}

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, base);

	const glTexture* t = pglGetTexture(tex);
	EXPECT(t && t->num_levels == 1, "after TexImage level0, num_levels==1");
	EXPECT(t->w == 4 && t->h == 4, "level0 size 4x4");
	EXPECT(t->levels[0].data == t->data, "levels[0] aliases data");
	EXPECT(t->levels[1].data == NULL, "no higher level views yet");

	ivec3 sz0 = textureSize(tex, 0);
	EXPECT(sz0.x == 4 && sz0.y == 4, "textureSize lod0 is 4x4");

	glGenerateMipmap(GL_TEXTURE_2D);
	EXPECT(glGetError() == GL_NO_ERROR, "GenerateMipmap no error");

	t = pglGetTexture(tex);
	// 4x4 -> 2x2 -> 1x1 = 3 levels; one block, ~4/3 of base
	EXPECT(t->num_levels == 3, "full chain has 3 levels");
	EXPECT(t->levels[0].data == t->data, "L0 still starts at data");
	EXPECT(t->levels[1].data == t->data + 4 * 4 * 4, "L1 packed after L0");
	EXPECT(t->levels[1].w == 2 && t->levels[1].h == 2, "level1 is 2x2");
	EXPECT(t->levels[2].w == 1 && t->levels[2].h == 1, "level2 is 1x1");
	EXPECT(t->data_alloc == (4*4 + 2*2 + 1*1) * 4, "single alloc is full chain size");
	// levels point inside the same block
	EXPECT(t->levels[2].data > t->data && t->levels[2].data < t->data + t->data_alloc,
	       "L2 pointer is inside the single allocation");

	ivec3 sz1 = textureSize(tex, 1);
	ivec3 sz2 = textureSize(tex, 2);
	EXPECT(sz1.x == 2 && sz1.y == 2, "textureSize lod1 is 2x2");
	EXPECT(sz2.x == 1 && sz2.y == 1, "textureSize lod2 is 1x1");

	vec4 c0 = texelFetch2D(tex, 0, 0, 0);
	vec4 c1 = texelFetch2D(tex, 0, 0, 1);
	vec4 c2 = texelFetch2D(tex, 0, 0, 2);
	EXPECT(c0.x > 0.99f && c0.y < 0.01f, "fetch L0 is red");
	EXPECT(c1.x > 0.99f && c1.y < 0.01f, "fetch L1 is red");
	EXPECT(c2.x > 0.99f && c2.y < 0.01f, "fetch L2 is red");

	// Replacing level 0 should replace the whole block (num_levels back to 1)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	t = pglGetTexture(tex);
	EXPECT(t->num_levels == 1, "TexImage L0 resets to single level");
	EXPECT(t->levels[1].data == NULL, "higher level views cleared");
	EXPECT(t->w == 8 && t->h == 8, "new base size 8x8");
	EXPECT(t->data_alloc == 8 * 8 * 4, "alloc is base only after L0 replace");

	// Manual upload of a higher level grows the single block
	Color lvl1[16]; // 4x4 for 8x8 base
	for (int i = 0; i < 16; i++) {
		lvl1[i].r = 0;
		lvl1[i].g = 255;
		lvl1[i].b = 0;
		lvl1[i].a = 255;
	}
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, lvl1);
	EXPECT(glGetError() == GL_NO_ERROR, "manual level1 upload ok");
	t = pglGetTexture(tex);
	EXPECT(t->num_levels == 2, "num_levels==2 after manual L1");
	EXPECT(t->levels[0].data == t->data, "still one block");
	EXPECT(t->levels[1].data == t->data + 8 * 8 * 4, "L1 after L0 in same block");
	vec4 g = texelFetch2D(tex, 0, 0, 1);
	EXPECT(g.y > 0.99f && g.x < 0.01f, "manual L1 is green");

	// Wrong size should error
	glGetError(); // clear
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, lvl1);
	EXPECT(glGetError() == GL_INVALID_VALUE, "wrong mip size is INVALID_VALUE");

	// 1D generate
	GLuint tex1d;
	glGenTextures(1, &tex1d);
	glBindTexture(GL_TEXTURE_1D, tex1d);
	Color row[8];
	for (int i = 0; i < 8; i++) {
		row[i].r = 0; row[i].g = 0; row[i].b = 255; row[i].a = 255;
	}
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, row);
	glGenerateMipmap(GL_TEXTURE_1D);
	t = pglGetTexture(tex1d);
	// 8->4->2->1 = 4 levels
	EXPECT(t->num_levels == 4, "1D chain has 4 levels");
	ivec3 s1 = textureSize(tex1d, 2);
	EXPECT(s1.x == 2, "1D lod2 width is 2");
	vec4 b = texelFetch1D(tex1d, 0, 3);
	EXPECT(b.z > 0.99f, "1D last mip is blue");

	// Cubemap rejects level > 0
	glGetError();
	GLuint cube;
	glGenTextures(1, &cube);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 1, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, base);
	EXPECT(glGetError() == GL_INVALID_VALUE, "cubemap level>0 rejected");

	glDeleteTextures(1, &tex);
	glDeleteTextures(1, &tex1d);
	glDeleteTextures(1, &cube);

	free_glContext(&ctx);
	free(bbuf);

	if (failures) {
		fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}
	printf("test_mipmap_phase1: all checks passed\n");
	return 0;
}
