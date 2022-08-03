Testing
=======

This directory contains both regression/feature testing and performance testing.

### Regression Testing

Each test generates a single frame and writes it out to a png image.
The glContext is initialized before and freed after each test to make sure each
has a clean slate.  The tests are executed using `./run_tests`, which runs
all the tests and compares the output images against the expected output and
reports any failures.  The whole process looks like this (assuming you are
in /testing):

```
$ make run_tests
# build output
$ ./run_tests
hello_triangle
====================
freeing buffer 1

hello_indexing
====================
freeing buffer 1
freeing buffer 2

# more runtime output

instanceid
====================
freeing buffer 1

All tests passed
```

In addition to running all the tests, you can pass 1 or more arguments to select specific tests.
It will skip any tests it can't find:

```
$ ./run_tests blend_test test_edges stencal_test map_vbuffer
blend_test
====================
freeing buffer 1

test_edges
====================
freeing buffer 1

Error: could not find test 'stencal_test', skipping

map_vbuffer
====================
freeing buffer 1

All tests passed
```

If you add a new function or feature, you can add a test following the structure shown
in the existing tests.

### Performance Testing

The current tests are just basics.  If you make any significant changes to core pieces
that might affect performance, try to make sure the results are the same or
better afterward.

Any of the standalone demos could be used as performance test too and if you know one
of them specifically stress the area of your changes, definitely take advantage of that.
I may end up converting some to formal tests too (especially gears since it's so standard).

The actual tests do run in a visual window so you can see what they're actually doing
(and I thought it was a more realistic test than rendering offscreen).  Here's the process
and what the output looks like on my laptop:

```
$ make config=release perf_tests
# build output
$ ./perf_tests
points_perf: 471.642 FPS
freeing buffer 1
pointsize_perf: 648.193 FPS
freeing buffer 1
lines_perf: 206.676 FPS
freeing buffer 1
triangles_perf: 33.036 FPS
freeing buffer 1
tri_interp_perf: 42.029 FPS
freeing buffer 1
```

Keep in mind that's with OpenMP enabled which does boost the last two tests by 40-60% in my
experience though PortableGL works fine without compiling with OpenMP as well and it's
only even used for filling triangles, so the best case scenario is a scene with just
a few large triangles.





