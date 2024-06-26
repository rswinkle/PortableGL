
glViewport

glGetStringi
glGetString

glColorMask
glColorMaski

glGetError
glGetBooleanv
glGetDoublev
glGetFloatv
glGetIntegerv
glGetInteger64v
glIsEnabled
glIsProgram

glClearColor
glClearDepth
glClearDepthf
glDepthFunc
glDepthRange
glDepthRangef
glDepthMask
glBlendFunc
glBlendEquation
glBlendFuncSeparate
glBlendEquationSeparate
glBlendColor
glClear
glProvokingVertex
glEnable
glDisable
glCullFace
glFrontFace
glPolygonMode
glPointSize
glPointParameteri
glLineWidth
glLogicOp
glPolygonOffset
glScissor
glStencilFunc
glStencilFuncSeparate
glStencilOp
glStencilOpSeparate
glClearStencil
glStencilMask
glStencilMaskSeparate

//textures
glGenTextures
glDeleteTextures
glBindTexture

glActiveTexture
glTexParameteri
glTexParameterfv

glTexParameterf
glTexParameterfv
glTexParameteriv

glTexParameterliv
glTexParameterluiv


glPixelStorei
glTexImage1D
glTexImage2D
glTexImage3D

glTexSubImage1D
glTexSubImage2D
glTexSubImage3D


glGenVertexArrays
glDeleteVertexArrays
glBindVertexArray
glGenBuffers
glDeleteBuffers
glBindBuffer
glBufferData
glBufferSubData
glVertexAttribPointer
glVertexAttribDivisor
glEnableVertexAttribArray
glDisableVertexAttribArray
glDrawArrays
glMultiDrawArrays
glDrawElements
glMultiDrawElements
glDrawArraysInstanced
glDrawArraysInstancedBaseInstance
glDrawElementsInstanced
glDrawElementsInstancedBaseInstance

//DSA functions (from OpenGL 4.5+)
glCreateBuffers
glNamedBufferData
glNamedBufferSubData
glCreateTextures

glEnableVertexArrayAttrib
glDisableVertexArrayAttrib

glTextureParameteri
glTextureParameterfv

glTextureParameterf
glTextureParameterfv
glTextureParameteriv

glTextureParameterliv
glTextureParameterluiv

glCompressedTexImage1D
glCompressedTexImage2D
glCompressedTexImage3D

//shaders
pglCreateProgram
glDeleteProgram
glUseProgram

void pglSetUniform

//This isn't possible in regular OpenGL, changing the interpolation of vs output of
//an existing shader.  You'd have to switch between 2 almost identical shaders.
void pglSetInterp


// Stubs to let real OpenGL libs compile with minimal modifications/ifdefs
// add what you need
glGenerateMipmap

glDrawBuffers
glNamedFramebufferDrawBuffers

// Framebuffers/Renderbuffers
glGenFramebuffers
glBindFramebuffer
glDeleteFramebuffers
glFramebufferTexture
glFramebufferTexture1D
glFramebufferTexture2D
glFramebufferTexture3D
glIsFramebuffer

glFramebufferTextureLayer
glNamedFramebufferTextureLayer

glReadBuffer
glNamedFramebufferReadBuffer

glBlitFramebuffer
glBlitNamedFramebuffer


glGenRenderbuffers
glBindRenderbuffer
glDeleteRenderbuffers
glRenderbufferStorage
glIsRenderbuffer

glFramebufferRenderbuffer
glCheckFramebufferStatus

glRenderbufferStorageMultisample
glNamedRenderbufferStorageMultisample

glClearBufferiv
glClearBufferuiv
glClearBufferfv
glClearBufferi
glClearNamedFramebufferiv
glClearNamedFramebufferuiv
glClearNamedFramebufferfv
glClearNamedFramebufferi

glGetProgramiv
glGetProgramInfoLog
glAttachShader
glCompileShader
glGetShaderInfoLog

// use pglCreateProgram()
glCreateProgram

glLinkProgram
glShaderSource
glGetShaderiv
glCreateShader
glDeleteShader
glDetachShader

glGetUniformLocation
glGetAttribLocation

glMapBuffer
glMapNamedBuffer
glUnmapBuffer
glUnmapNamedBuffer

glUniform1f
glUniform2f
glUniform3f
glUniform4f

glUniform1i
glUniform2i
glUniform3i
glUniform4i

glUniform1ui
glUniform2ui
glUniform3ui
glUniform4ui

glUniform1fv
glUniform2fv
glUniform3fv
glUniform4fv

glUniform1iv
glUniform2iv
glUniform3iv
glUniform4iv

glUniform1uiv
glUniform2uiv
glUniform3uiv
glUniform4uiv

glUniformMatrix2fv
glUniformMatrix3fv
glUniformMatrix4fv
glUniformMatrix2x3fv
glUniformMatrix3x2fv
glUniformMatrix2x4fv
glUniformMatrix4x2fv
glUniformMatrix3x4fv
glUniformMatrix4x3fv



