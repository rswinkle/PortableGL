// Shared assertion helper for suite and standalone tests.
// Failed checks print to stderr and increment pgl_test_fails.
// run_tests resets the counter per test and treats a non-zero count as failure
// (in addition to the default-FB PNG compare).

#ifndef PGL_TEST_EXPECT_H
#define PGL_TEST_EXPECT_H

#include <stdio.h>

static int pgl_test_fails;

#define PGL_EXPECT(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "FAIL %s: %s\n", __func__, (msg)); \
		pgl_test_fails++; \
	} \
} while (0)

#endif
