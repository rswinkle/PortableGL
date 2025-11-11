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

### Original Custom Examples

| Example | Description | Image |
| --- | --- | --- |
| ex1.c/pp and ex1_std_shaders.c | Hello Triangle      | ![ex1](https://raw.githubusercontent.com/rswinkle/PortableGL/master/media/screenshots/ex1.png) |
| ex2.c/pp and ex2_std_shaders.c | Hello Interpolation | ![ex2](https://raw.githubusercontent.com/rswinkle/PortableGL/master/media/screenshots/ex2.png) |
| ex3.c/pp | Hello 3D + rotation | ![ex3](https://raw.githubusercontent.com/rswinkle/PortableGL/master/media/screenshots/ex3.png) |

### Classic Ports

Ports of classic OpenGL demos

|  example  | image  | last version<br>updated | original<br>developer |
|-----------|--------|:-----------------------:|:---------------------:|
| [classic](core/gears.c) | <img src="core/core_basic_window.png" alt="core_basic_window" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |

### Learning WebGL ports

Ports of lessons from [learningwebgl.com](https://learningwebgl.com/blog/?page_id=1217) based
off of my own ports to OpenGL 3.3 [here](https://github.com/rswinkle/opengl_reference).

|  example  | image  | last version<br>updated | original<br>developer |
|-----------|--------|:-----------------------:|:----------------------|
| [lesson1](webgl_lessons/lesson1.cpp) | <img src="webgl_lessons/lesson1.png" alt="lesson1" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson2](webgl_lessons/lesson2.cpp) | <img src="webgl_lessons/lesson2.png" alt="lesson2" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson3](webgl_lessons/lesson3.cpp) | <img src="webgl_lessons/lesson3.png" alt="lesson3" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson4](webgl_lessons/lesson4.cpp) | <img src="webgl_lessons/lesson4.png" alt="lesson4" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson5](webgl_lessons/lesson5.cpp) | <img src="webgl_lessons/lesson5.png" alt="lesson5" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson6](webgl_lessons/lesson6.cpp) | <img src="webgl_lessons/lesson6.png" alt="lesson6" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson7](webgl_lessons/lesson7.cpp) | <img src="webgl_lessons/lesson7.png" alt="lesson7" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson8](webgl_lessons/lesson8.cpp) | <img src="webgl_lessons/lesson8.png" alt="lesson8" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson9](webgl_lessons/lesson9.cpp) | <img src="webgl_lessons/lesson9.png" alt="lesson9" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson10](webgl_lessons/lesson10.cpp) | <img src="webgl_lessons/lesson10.png" alt="lesson10" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson11](webgl_lessons/lesson11.cpp) | <img src="webgl_lessons/lesson11.png" alt="lesson11" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson12](webgl_lessons/lesson12.cpp) | <img src="webgl_lessons/lesson12.png" alt="lesson12" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson13](webgl_lessons/lesson13.cpp) | <img src="webgl_lessons/lesson13.png" alt="lesson13" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson14](webgl_lessons/lesson14.cpp) | <img src="webgl_lessons/lesson14.png" alt="lesson14" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson15](webgl_lessons/lesson15.cpp) | <img src="webgl_lessons/lesson15.png" alt="lesson15" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
| [lesson16](webgl_lessons/lesson16.cpp) | <img src="webgl_lessons/lesson16.png" alt="lesson16" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |


### Learn OpenGL ports

Ports of [learnopengl.com](https://learnopengl.com/) tutorial code [here](https://github.com/rswinkle/LearnPortableGL).
The project is too large to include here and works best as a separate repo anyway. I've currently ported the first 4 chapters
worth, or about 56 programs out out of a total of about 97 programs over 8 chapters. It's mostly stalled there until
PGL officially adds certain features like more texture formats, FBOs, etc. I've included a few below as a sampling.

|  example  | image  | last version<br>updated | original<br>developer |
|-----------|--------|:-----------------------:|:----------------------|
| [model_loading](https://raw.githubusercontent.com/rswinkle/LearnPortableGL/master/src/3.model_loading/model_loading.cpp) | <img src="../media/screenshots/backpack_model.png" alt="backpack_model" width="80"> | 0.100.0 | [Robert Winkler](https://github.com/rswinkle) |
