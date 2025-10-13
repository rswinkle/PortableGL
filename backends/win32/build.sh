#in quasi-msys2 cross compilation shell
cc -o bitblt_win32 -I../.. win32_bitblt_main.c -lgdi32
cc -o stretchdibits_win32 -I../.. win32_stretchdibits_main.c -lgdi32
