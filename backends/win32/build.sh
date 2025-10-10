#in quasi-msys2 cross compilation shell
cc -o bitblt_win32 -I../.. win32_bitblt_main.c -lgdi32
cc -o stretchdib_win32 -I../.. win32_stretchdib_main.c -lgdi32
