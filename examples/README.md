Examples
========

These were originally going to be where NeHe style example programs go though obviously I haven't
gotten very far.  In general, programs that go here will cleaner and more polished code than the
free-for-all that is demos.

So far we have 3 programs in both C and C++, and the first 2 again in C but demoing using
the included standard shaders for a total of 8 separate programs.

There are also a couple misc. programs that probably shouldn't be
here since I mostly use(d) them for testing things, like various line
drawing methods and how much memory PGL uses in different configurations.

## Building

They all require SDL2 to be installed to build.

On Debian/Ubuntu based distributions you can install SDL2 using the following command:

`sudo apt install libsdl2-dev`

On Mac you can download the DMG file from their [releases page](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10) or install it through
a package manager like [Homebrew](https://brew.sh/), [MacPorts](https://ports.macports.org/), or [Fink](https://www.finkproject.org/).  Note, I do
not own a mac and have never tested PortableGL on one.  Worst case, you can always just compile SDL2 from source but one of the above options should work.

On Windows you can grab the zip you want from the same releases page linked above.

I use premake generated makefiles that I include in the repo which I use on Linux. I have used these same Makefiles
to build under [MSYS2](https://www.msys2.org/) on Windows. However, at least for now, even though PortableGL and all the
examples and demos are cross platform, I don't officially support building them on other platforms. I've thought about
removing the premake scripts from the repo entirely and just leaving the Makefiles to make that clearer but decided not to
for the benefit of those who want to modify it for themselves to handle different platforms and build systems. For now
the win32 backend examples will have to suffice.

Once you have SDL2 installed you should be able to cd into examples, demos, or testing, and just run `make` or `make config=release` for optimized builds.
`make verbose=1` will let you see all the build steps. You can run `make help` to see all the individual targets.

### Original Custom

| Program | Description | Image |
| --- | --- | --- |
| ex1.c/pp and ex1_std_shaders.c | Hello Triangle      | ![ex1](https://raw.githubusercontent.com/rswinkle/PortableGL/master/media/screenshots/ex1.png) |
| ex2.c/pp and ex2_std_shaders.c | Hello Interpolation | ![ex2](https://raw.githubusercontent.com/rswinkle/PortableGL/master/media/screenshots/ex2.png) |
| ex3.c/pp | Hello 3D + rotation | ![ex3](https://raw.githubusercontent.com/rswinkle/PortableGL/master/media/screenshots/ex3.png) |
| lines.c | line testing |  |
| minimal_pgl.c | Does nothing but initializes PGL and waits to exit, no visual |  |

### Learn WebGL ports


