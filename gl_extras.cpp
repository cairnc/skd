#ifndef __APPLE__

#include "gl_extras.h"
#include <SDL3/SDL_video.h>

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;

void LoadGLExtras()
{
    auto get = [](const char *n) { return SDL_GL_GetProcAddress(n); };
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)get("glGenFramebuffers");
    glDeleteFramebuffers =
        (PFNGLDELETEFRAMEBUFFERSPROC)get("glDeleteFramebuffers");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)get("glBindFramebuffer");
    glFramebufferTexture2D =
        (PFNGLFRAMEBUFFERTEXTURE2DPROC)get("glFramebufferTexture2D");
}

#endif
