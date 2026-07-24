#!/bin/sh
# PortableGL minimal FLTK backend
# Debian/Ubuntu: sudo apt install libfltk1.3-dev
# (FLTK 1.4 packages work too if fltk-config or pkg-config fltk is available)

set -e
cd "$(dirname "$0")"

CXXFLAGS=
LDFLAGS=

if command -v fltk-config >/dev/null 2>&1; then
	echo "Using fltk-config ($(fltk-config --version))"
	CXXFLAGS=$(fltk-config --cxxflags)
	LDFLAGS=$(fltk-config --ldflags)
elif pkg-config --exists fltk; then
	echo "Using pkg-config fltk ($(pkg-config --modversion fltk))"
	CXXFLAGS=$(pkg-config --cflags fltk)
	LDFLAGS=$(pkg-config --libs fltk)
else
	echo "FLTK not found. Install development packages, e.g.:" >&2
	echo "  sudo apt install libfltk1.3-dev" >&2
	exit 1
fi

${CXX:=g++} -g -I../../ fltk_pgl.cpp -o fltk_demo $CXXFLAGS $LDFLAGS -lm
echo "Built ./fltk_demo"
