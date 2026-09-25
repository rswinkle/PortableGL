// Guard NDC limits from init_glContext. Does not draw and does not call
// glViewport for the 640x480 numbers. The clipper still ignores these fields.

#include <math.h>

static int guard_near(float got, float expect)
{
	return fabsf(got - expect) < 1e-5f;
}

void guard_band_limits(int num, char** argv, void* data)
{
	PGL_UNUSED(num);
	PGL_UNUSED(argv);
	PGL_UNUSED(data);

	pix_t* pix = NULL;
	glContext ctx;
	glContext* saved = get_glContext();

	PGL_EXPECT(init_glContext(&ctx, &pix, 640, 480), "init 640x480");
	if (!get_glContext() || get_glContext() != &ctx) {
		PGL_EXPECT(0, "init did not install the context");
		set_glContext(saved);
		return;
	}

#if PGL_GUARD_BAND
	PGL_EXPECT(guard_near(ctx.guard_ndc_left,   -4.200050f), "ndc left");
	PGL_EXPECT(guard_near(ctx.guard_ndc_right,   4.200081f), "ndc right");
	PGL_EXPECT(guard_near(ctx.guard_ndc_bottom, -5.266756f), "ndc bottom");
	PGL_EXPECT(guard_near(ctx.guard_ndc_top,     5.266797f), "ndc top");
	PGL_EXPECT(ctx.guard_ndc_left != -ctx.guard_ndc_right, "0.01 bias, not symmetric");
#else
	PGL_EXPECT(ctx.guard_ndc_left == -1.0f && ctx.guard_ndc_right == 1.0f, "band off x");
	PGL_EXPECT(ctx.guard_ndc_bottom == -1.0f && ctx.guard_ndc_top == 1.0f, "band off y");
#endif

	glViewport(0, 0, 0, 0);
	PGL_EXPECT(ctx.guard_ndc_left == -1.0f && ctx.guard_ndc_right == 1.0f, "empty viewport x");
	PGL_EXPECT(ctx.guard_ndc_bottom == -1.0f && ctx.guard_ndc_top == 1.0f, "empty viewport y");

	free_glContext(&ctx);
	set_glContext(saved);
}
