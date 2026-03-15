overall=0

./run_tests || overall=1
./run_tests_d16 || overall=1
./run_tests_d16_no_stencil || overall=1
./run_tests_rgb565 || overall=1
./run_tests_clamp_border || overall=1
./run_tests_no_depth_no_stencil || overall=1

exit "$overall"
