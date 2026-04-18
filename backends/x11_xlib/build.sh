#${CC:=gcc}
${CC:=gcc} -g -I../../ xlib_pgl.c -o xlib_demo -lX11 -lm
${CC:=gcc} -g -I../../ xlib_pgl2.c -o xlib_demo2 -lX11 -lm
