#!/bin/sh
# PortableGL minimal Wayland backend (wl_shm + xdg-shell)
# Debian/Ubuntu: sudo apt install libwayland-dev wayland-protocols
#
# Generates xdg-shell client stubs next to the source, then links the demo.
# Requires a Wayland compositor at runtime (WAYLAND_DISPLAY / suitable session).

set -e
cd "$(dirname "$0")"

PROTO_XML=
for p in \
	/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
	/usr/local/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml
do
	if [ -f "$p" ]; then
		PROTO_XML=$p
		break
	fi
done

if [ -z "$PROTO_XML" ]; then
	echo "xdg-shell.xml not found. Install wayland-protocols." >&2
	exit 1
fi

if ! command -v wayland-scanner >/dev/null 2>&1; then
	echo "wayland-scanner not found. Install libwayland-dev (or wayland-scanner)." >&2
	exit 1
fi

if ! pkg-config --exists wayland-client; then
	echo "wayland-client not found. Install libwayland-dev." >&2
	exit 1
fi

echo "Generating xdg-shell protocol stubs from $PROTO_XML"
wayland-scanner client-header "$PROTO_XML" xdg-shell-client-protocol.h
wayland-scanner private-code  "$PROTO_XML" xdg-shell-protocol.c

${CC:=gcc} -g -I../../ \
	wayland_pgl.c xdg-shell-protocol.c \
	-o wayland_demo \
	$(pkg-config --cflags --libs wayland-client) -lm

echo "Built ./wayland_demo"
echo "Note: run under a Wayland session (or with WAYLAND_DISPLAY set)."
