// PGL_CORE_PROFILE: incomplete mip textures return black; RECTANGLE wrap restricted
#define PGL_CORE_PROFILE
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

	Color red[16];
	for (int i = 0; i < 16; i++) {
		red[i].r = 255; red[i].g = 0; red[i].b = 0; red[i].a = 255;
	}

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, red);

	// Incomplete: mip min filter, only L0 → black under CORE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	vec4 c = texture2D(tex, 0.25f, 0.25f);
	EXPECT(near_color(c, 0.f, 0.f, 0.f, 0.02f), "incomplete mip filter samples black");
	c = texture2DLod(tex, 0.25f, 0.25f, 0.0f);
	EXPECT(near_color(c, 0.f, 0.f, 0.f, 0.02f), "incomplete texture2DLod is black");

	// Non-mip filter still samples L0 red
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	c = texture2D(tex, 0.25f, 0.25f);
	EXPECT(near_color(c, 1.f, 0.f, 0.f, 0.02f), "NEAREST still samples L0 red");

	// After GenerateMipmap, mip filter works (red throughout)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);
	EXPECT(glGetError() == GL_NO_ERROR, "GenerateMipmap ok");
	c = texture2DLod(tex, 0.25f, 0.25f, 1.0f);
	EXPECT(near_color(c, 1.f, 0.f, 0.f, 0.02f), "complete chain lod1 is red");

	// RECTANGLE: REPEAT is INVALID_ENUM under CORE
	GLuint rect;
	glGenTextures(1, &rect);
	glBindTexture(GL_TEXTURE_RECTANGLE, rect);
	glGetError();
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_REPEAT);
	EXPECT(glGetError() == GL_INVALID_ENUM, "RECTANGLE WRAP_S REPEAT is INVALID_ENUM");
	glGetError();
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	EXPECT(glGetError() == GL_INVALID_ENUM, "RECTANGLE WRAP_T MIRRORED_REPEAT is INVALID_ENUM");
	glGetError();
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	EXPECT(glGetError() == GL_NO_ERROR, "RECTANGLE CLAMP_TO_EDGE ok");

	glDeleteTextures(1, &tex);
	glDeleteTextures(1, &rect);
	free_glContext(&ctx);
	free(bbuf);

	if (failures) {
		fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}
	printf("test_core_profile_mips: all checks passed\n");
	return 0;
}
