#in quasi-msys2 cross compilation shell
cc -I../../.. win32_bitblit_main.c -lgdi32
cc -o stretchdib_win32 -I../../.. win32_stretchdib_main.c -lgdi32
