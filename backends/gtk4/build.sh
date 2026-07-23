#!/bin/sh
# -lX11 is for optional startup centering under X11 (XQueryPointer / XMoveWindow).
# Harmless if you only ever run under Wayland; the code is #ifdef GDK_WINDOWING_X11.
${CC:=gcc} -g -I../../ gtk4_pgl.c -o gtk4_demo $(pkg-config --cflags --libs gtk4) -lX11 -lm
