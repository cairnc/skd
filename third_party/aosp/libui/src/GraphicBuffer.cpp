#include <ui/GraphicBuffer.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
// Minimal GL declarations — we only need glGenTextures / glDeleteTextures /
// glBindTexture / glTexImage2D. Avoids dragging in a full GL loader here.
#include <cstddef>
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;
#define GL_TEXTURE_2D 0x0DE1
#define GL_RGBA 0x1908
#define GL_RGBA8 0x8058
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_LINEAR 0x2601
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
extern "C" {
void glGenTextures(GLsizei, GLuint *);
void glDeleteTextures(GLsizei, const GLuint *);
void glBindTexture(GLenum, GLuint);
void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum,
                  const void *);
void glTexParameteri(GLenum, GLenum, GLint);
}
#endif

namespace android {

namespace {
GraphicBuffer::ContentPopulator &populator() {
  static GraphicBuffer::ContentPopulator sInstance;
  return sInstance;
}
} // namespace

void GraphicBuffer::setContentPopulator(ContentPopulator p) {
  populator() = std::move(p);
}

GraphicBuffer::GraphicBuffer(uint32_t w, uint32_t h, PixelFormat format,
                             uint32_t layerCount, uint64_t usage,
                             std::string requestorName)
    : mWidth(w), mHeight(h), mFormat(format), mLayerCount(layerCount),
      mUsage(usage), mRequestor(std::move(requestorName)) {
  mId = sNextId++;
  // Eagerly create the GL texture so its content is ready before any client
  // (RE, preview, or the wireframe) samples it. Skips if no GL context is
  // current — getOrCreateGLTexture() will no-op in that case.
  getOrCreateGLTexture();
}

GraphicBuffer::~GraphicBuffer() {
  if (mTextureId) {
    GLuint t = mTextureId;
    glDeleteTextures(1, &t);
    mTextureId = 0;
  }
}

unsigned int GraphicBuffer::getOrCreateGLTexture() {
  if (mTextureId)
    return mTextureId;
  GLuint t = 0;
  glGenTextures(1, &t);
  if (!t)
    return 0;
  glBindTexture(GL_TEXTURE_2D, t);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)mWidth, (GLsizei)mHeight, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
  mTextureId = t;
  // Hand off to the content populator (if installed) so the texture is not
  // just a blob of zeros. In layerviewer this draws a checkerboard plus the
  // buffer id; once CE/RE actually runs, this is where real composited
  // pixels could land.
  if (populator())
    populator()(mWidth, mHeight, mId, mTextureId);
  return t;
}

} // namespace android
