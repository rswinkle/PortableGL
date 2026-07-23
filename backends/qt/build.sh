#!/bin/sh
# PortableGL Qt backend — builds against Qt6 or Qt5 Widgets.
# Debian/Ubuntu: sudo apt install qt6-base-dev
#            or: sudo apt install qtbase5-dev

set -e

if pkg-config --exists Qt6Widgets; then
	QT_PC=Qt6Widgets
	echo "Using Qt6 ($(pkg-config --modversion Qt6Widgets))"
elif pkg-config --exists Qt5Widgets; then
	QT_PC=Qt5Widgets
	echo "Using Qt5 ($(pkg-config --modversion Qt5Widgets))"
else
	echo "No Qt Widgets found. Install one of:" >&2
	echo "  sudo apt install qt6-base-dev" >&2
	echo "  sudo apt install qtbase5-dev" >&2
	exit 1
fi

# Qt headers need a modern C++ dialect; Qt6 wants C++17.
STD=-std=c++17
${CXX:=g++} -g $STD -I../../ qt_pgl.cpp -o qt_demo \
	$(pkg-config --cflags --libs "$QT_PC") -lm

echo "Built ./qt_demo"
