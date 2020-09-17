Demos
=====

This is where everything that doesn't fit into examples or testing goes.  Basically
a free for all of me experimenting and having fun.

ESC will exit all of them.

Flying controls = 6DOF flying controls ala Descent. Mouse + WASDQE + LShift + Space


### Gears

This is a straight conversion of the well knows [gears demo](https://cgit.freedesktop.org/mesa/demos/tree/src/egl/opengles2/es2gears.c),
via a conversion to OpenGL 3.3 in my [opengl_reference](https://github.com/rswinkle/opengl_reference/blob/master/src/gears.c)
repo.  Controls are the arrow keys to rotate the gears, and I added 'p' to toggle polygon mode.

### Grass

This is an incomplete port of the instanced rendering
[grass demo](https://github.com/rswinkle/oglsuperbible5/blob/1a92eb6b4eeb665582acd69bc41ba793ff974bd1/Src/Chapter12/Grass/Grass.cpp)
from Superbible 5.  Flying controls.

### Modelviewer

This demonstratesc gouraud and phong shading with a single directional light.  It will load one of the models in ./media/models if it's passed
as a command line argument, otherwise it generates a sphere.  The model then rotates counter clockwise.  's' and 'p' to switch between shaders
and polygon modes.  The program assimp_convert will, if you have libassimp installed and can compile it, convert other model formats to the plain
text format modelviewer reads.  Be aware you might have to scale and translate them to make them visible.  The stanford models provided are
already centered.

### Texturing

What it says on the tin.  Arrow keys to zoom/rotate, 1 to switch between 3 textures, 'f' to switch between GL_NEAREST and GL_LINEAR. Note,
PortableGL doesn't actually use min_filter.  You can set it, but only mag_filter is used in the texel access functions texture[1-3]D.

### Pointsprites

This draws 3 large pointsprites, making 2 targets using 1 shader, and creating a disolving textured point with a different shader that shows
how multitexturing works.  There is no glActiveTexture or texture units in PortableGL.  Shaders access textures by the handles
returned from glGenTextures.  It's much more convenient imo.

### Cubemap

This uses a cubemap texture to create the reflective sphere + skybox demo.  Partial flying controls (WASD + mouse).
Unfortunately there is a bug that I've never bothered to track down so when you look around, the skybox wobbles.  Just moving has no effect,
only turning with the mouse which makes sense since the skybox is "infinitely" distant iirc.

### Sphereworld

Another demo from [Superbible 5](https://github.com/rswinkle/oglsuperbible5/blob/1a92eb6b4eeb665582acd69bc41ba793ff974bd1/Src/Chapter05/Sphereworld/Sphereworld.cpp)
but with better controls and a slightly different shader and light direction.  The controls are shown in the commandline on startup (read from a user editable
controls.config file).  This is also, along with swrenderer, one of 2 current demos that show one way to resize the window, by calling resize_framebuffer.  The other way which I don't know if I show anywhere, lets SDL do the scale for you.

### Shadertoy

This is basically a standalone reimplementation of the graphical component of [shadertoy](https://www.shadertoy.com/).
Use '1' to cycle through 10 different shaders, roughly in order of increasing complexity.  I include links/attribution in the
comments above the shaders taken directly from shadertoy if you want to see them in their full glory on shadertoy.com

Originaly, it used the normal method of drawing 2 triangles that fill the screen (and you can still see that code commented out)
but making this in PortableGL meant I could add an extension, pglDrawFrame(), for this special use case that
bypasses the vertex shader entirely and just sets everything up the way shadertoy shaders need things.  Unfortunately, it doesn't
increase frame rate as much as I'd hoped so I ended up changing the resolution to 320x240 get "bearable" framerates on the harder shaders.
Even so, the last few shaders can hardly be called "realtime".  Also the tunnel light one has some graphical bug.

Since 320x240 is so small, especially on high-DPI monitors, I made this one resizable but unlike Sphereworld, where
I change the framebuffer (and the projection and glViewport) to match, here I just let SDL2 scale the texture; it's slower
than leaving the window at the small size but *much* faster than actually rendering at a higher resolution.

Also, if you want to play with a real standalone shadertoy (with live updating) that uses actual OpenGL and hardware acceleration, here are
[two](https://github.com/rswinkle/shadertoy) [options](https://github.com/githole/Live-Coder).

### Swrenderer

I need to think of a better name for this.  This dates back to the *very* beginning of this project, based off of a tutorial.  That's why it's main.cpp.
Basically, for the longest time this was where I tested each new feature I added.  So this has interpolation, textures, depth test toggle, and a pseudo-
render to texture that's really just an extra manual copy via TexSubImage2D after the first pass.  There are also multiple methods of doing things
commented out.  Same controls as Sphereworld and 1 to switch between textures (only seen when you're using the texture shader, switched to with 's').

TODO: I use dvorak, so I need to make a controls.config for QWERTY for others' convenience though it's not hard to edit the file by hand.

