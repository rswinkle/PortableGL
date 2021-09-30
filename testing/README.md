Testing
=======

This directory contains both regression/feature testing and performance testing.

### Regression Testing

Each test generates a single frame and writes it out to a png image.
The glContext is initialized before and freed after each test to make sure each
has a clean slate.  After running `./run_test`, you run a python script
`check_tests.py` which compares the output images against the expected output and
reports any failures.  The whole process looks like this (assuming you are in /testing):

```
make run_tests
# build output
./run_tests
# runtime output
python3 ./check_tests.py
All tests passed
```

If any tests fail, the output of check_tests.py will give their names.

If you add a new function or feature, you can add a test following the structure shown
in the existing tests.

### Performance Testing

The current tests are just basics.  If you make any significant changes to core pieces
that might affect performance, try to make sure the results are the same or
better afterward.

Any of the standalone demos could be used as performance test too and if you know one
of them specifically stress the area of your changes, definitely take advantage of that.
I may end up converting some to formal tests too (especially gears since it's so standard).






