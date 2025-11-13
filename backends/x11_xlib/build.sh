#${CC:=gcc}
${CC:=gcc} -Og -I../../ xlib_pgl.c -o xlib_demo -lX11 -lm
${CC:=gcc} -Og -I../../ xlib_pgl2.c -o xlib_demo2 -lX11 -lm
