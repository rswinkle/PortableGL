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
has to do a minor hacky initialization but it's not a big deal, see the code.

## X11 xlib

You will need the xlib development libraries installed (`sudo apt install libx11-dev`
on a debian based distro) but otherwise it should just work with `build.sh` to build.
The two programs are very similar, just based on two different tutorials/examples I found online.

## TODO

* wayland
* X11 xcb? Is it worth it?
* I don't have a mac and but whatever the equivalent of win32 programs would be
* ???

