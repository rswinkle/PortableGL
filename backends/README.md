Backends
========

Simple starter programs (same as ex1 in examples, just a red triangle) for
different backends.  They could still use some polish but they work fine.

## Win32

The `build.bat` script assumes you have
[Visual Studio 2022](https://visualstudio.microsoft.com/vs/compare/) installed
but you could probably edit it for a different version and theoretically the code
should compile with any windows toolchain (ie clang, mingw etc).

The `build.sh` is for cross compiling with
[quasi-msys2](https://github.com/HolyBlackCat/quasi-msys2).

There are two versions, one using BitBlt, one using stretchDIBits. The former
has to do a minor hack during initialization but it's not a big deal, see the code.

## X11 xlib

You will need the xlib development libraries installed (`sudo apt install libx11-dev`
on a debian based distro) but otherwise it should just work with `build.sh` to build.
The two programs are very similar, just based on two different tutorials/examples I found online.
The second one, xlib_demo2 is definitely better in my opinion. The first flickers a bit
during window resizing.

## GTK3

You will need the GTK3 development libraries (`sudo apt install libgtk-3-dev` on a
debian based distro).  Functionally the same as the GTK4 demo: resizable
`GtkDrawingArea` (default 640x480), Cairo blit with `PGL_ARGB32`, right-hand
sidebar with live update / integer FPS / triangle & background color pickers,
Escape to quit, and X11 startup centering (pointer monitor, else primary, else
first; Wayland placement is compositor-controlled).  Run `build.sh` then
`./gtk3_demo`.

## GTK4

You will need the GTK4 development libraries (`sudo apt install libgtk-4-dev` on a
debian based distro).  Uses a resizable `GtkDrawingArea` (default 640x480) and blits
the PortableGL back buffer with Cairo (`CAIRO_FORMAT_ARGB32`, so the example is
built with `PGL_ARGB32`).  A fixed-width right-hand sidebar has a live-update
checkbox (rotates the triangle and shows integer FPS), plus color pickers for
the triangle and clear/background colors.  Escape closes the window.  On X11 the
window is centered on the pointer's monitor (else primary / first); on Wayland
placement is left to the compositor.  Run `build.sh` then `./gtk4_demo`.

## Qt

You will need Qt5 or Qt6 Widgets development packages, e.g.
`sudo apt install qt6-base-dev` (or `qtbase5-dev` for Qt5).  `build.sh` picks
Qt6 if available, otherwise Qt5.  Functionally the same as the GTK demos: a
resizable canvas (default 640x480) blitted via `QImage`/`QPainter` with
`PGL_ARGB32`, right-hand sidebar with live update / integer FPS / triangle and
background color pickers, Escape to quit.  On X11 (and similar) the window is
centered on the screen under the pointer (else primary / first); on Wayland
placement is left to the compositor and a message is logged.  Run `build.sh`
then `./qt_demo`.

## Wayland

You will need `libwayland-dev` and `wayland-protocols` (`sudo apt install
libwayland-dev wayland-protocols` on a debian based distro).  `build.sh`
runs `wayland-scanner` to generate xdg-shell client stubs, then builds a
minimal resizable window (same idea as `xlib_pgl2.c`: static red triangle).
Pixels go through `wl_shm` (`WL_SHM_FORMAT_XRGB8888` + `PGL_ARGB32`); no
EGL/Vulkan.  Escape or the window close button quits.  You must run it under
a Wayland compositor (`WAYLAND_DISPLAY` set); an X11-only session will fail
to connect.  Run `build.sh` then `./wayland_demo`.

## FLTK

You will need FLTK development files (`sudo apt install libfltk1.3-dev` on a
debian based distro; FLTK 1.4 is fine if `fltk-config` or `pkg-config fltk`
works).  Minimal resizable window like `xlib_pgl2.c` / the Wayland demo: static
red triangle, blit with `fl_draw_image` (RGBA, so default PGL `ABGR32` / RGBA
memory on LE).  Escape closes the window.  Run `build.sh` then `./fltk_demo`.

## TODO

* wxWidgets
* Cocoa / AppKit (macOS)
* X11 xcb? Is it worth it?
* ???

