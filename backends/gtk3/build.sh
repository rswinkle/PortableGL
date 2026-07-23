#!/bin/sh
# Optional -lX11 is not required for this demo (GTK3 uses gdk/gtk placement APIs),
# but kept harmless if you extend the X11 path later.
${CC:=gcc} -g -I../../ gtk3_pgl.c -o gtk3_demo $(pkg-config --cflags --libs gtk+-3.0) -lm
