#pragma once

// GL 3.0+ framebuffer functions not covered by imgui_impl_opengl3_loader.h.
// On macOS these come from <OpenGL/gl.h>. On Linux/Windows we load them
// at runtime via the platform's GetProcAddress equivalent.

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else

#include "imgui_impl_opengl3_loader.h"

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

typedef void(APIENTRYP PFNGLGENFRAMEBUFFERSPROC)(GLsizei n,
                                                 GLuint *framebuffers);
typedef void(APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n,
                                                    const GLuint *framebuffers);
typedef void(APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum target,
                                                 GLuint framebuffer);
typedef void(APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target,
                                                      GLenum attachment,
                                                      GLenum textarget,
                                                      GLuint texture,
                                                      GLint level);

extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;

void LoadGLExtras();

#endif
