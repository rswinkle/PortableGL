#${CC:=gcc}
${CC:=gcc} -I../../ xlib_pgl.c -o xlib_demo -lX11 -lm
