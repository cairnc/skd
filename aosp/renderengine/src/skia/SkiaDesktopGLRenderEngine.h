// SkiaDesktopGLRenderEngine
//
// Drop-in replacement for SkiaGLRenderEngine. SkiaRenderEngine (the base) is
// graphics-API-agnostic — it drives rendering through the SkiaGpuContext
// abstraction. SkiaGLRenderEngine binds that abstraction to EGL + GLES2; we
// bind it to a desktop GL context the app already owns (via GrGLInterface
// + GrDirectContext through a GaneshGpuContext), so no EGL, no GLES.
#pragma once

#include "SkiaRenderEngine.h"

#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>

#include <android-base/unique_fd.h>

namespace android::renderengine::skia {

class SkiaDesktopGLRenderEngine final : public SkiaRenderEngine {
public:
  // Caller owns the GL context; `glInterface` (typically
  // GrGLMakeNativeInterface()) is captured for SkiaGpuContext construction.
  static std::unique_ptr<SkiaDesktopGLRenderEngine>
  create(const RenderEngineCreationArgs &args,
         sk_sp<const GrGLInterface> glInterface);

  ~SkiaDesktopGLRenderEngine() override;
  int getContextPriority() override { return 0; }

protected:
  Contexts createContexts() override;
  bool supportsProtectedContentImpl() const override { return false; }
  bool useProtectedContextImpl(GrProtected) override { return true; }
  void waitFence(SkiaGpuContext *, base::borrowed_fd) override {}
  base::unique_fd flushAndSubmit(SkiaGpuContext *context,
                                 sk_sp<SkSurface> dstSurface) override;
  void appendBackendSpecificInfoToDump(std::string &result) override;

private:
  SkiaDesktopGLRenderEngine(const RenderEngineCreationArgs &args,
                            sk_sp<const GrGLInterface> glInterface);

  sk_sp<const GrGLInterface> mGLInterface;
};

} // namespace android::renderengine::skia
