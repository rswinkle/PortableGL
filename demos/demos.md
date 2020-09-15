Demos
=====

This is where everything that doesn't fit into examples or testing goes.  Basically
a free for all of me experimenting and having fun.

FYI: Flying controls = 6DOF flying controls ala Descent. Mouse + WASDQE + LShift + Space


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
only turning with the mouse.

### Sphereworld

