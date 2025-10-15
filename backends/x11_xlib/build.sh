#${CC:=gcc}
${CC:=gcc} -Og -I../../ xlib_pgl.c -o xlib_demo -lX11 -lm
