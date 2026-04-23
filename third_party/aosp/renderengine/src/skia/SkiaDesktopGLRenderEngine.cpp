#include "SkiaDesktopGLRenderEngine.h"

#include "compat/SkiaGpuContext.h"

#include <log/log.h>

#include <renderengine/DesktopGLRenderEngineFactory.h>

namespace android::renderengine {

// Public factory (declared in
// include/renderengine/DesktopGLRenderEngineFactory.h). Same construction
// skia::SkiaDesktopGLRenderEngine::create runs, returned as the abstract base
// so callers don't need the in-tree skia/ headers — those depend on Skia
// private includes that aren't part of aosp_renderengine's PUBLIC surface.
std::unique_ptr<RenderEngine>
createDesktopGLRenderEngine(const RenderEngineCreationArgs &args,
                            sk_sp<const GrGLInterface> glInterface) {
  return skia::SkiaDesktopGLRenderEngine::create(args, std::move(glInterface));
}

} // namespace android::renderengine

namespace android::renderengine::skia {

std::unique_ptr<SkiaDesktopGLRenderEngine>
SkiaDesktopGLRenderEngine::create(const RenderEngineCreationArgs &args,
                                  sk_sp<const GrGLInterface> glInterface) {
  if (!glInterface)
    return nullptr;
  auto engine = std::unique_ptr<SkiaDesktopGLRenderEngine>(
      new SkiaDesktopGLRenderEngine(args, std::move(glInterface)));
  engine->ensureContextsCreated();
  return engine;
}

SkiaDesktopGLRenderEngine::SkiaDesktopGLRenderEngine(
    const RenderEngineCreationArgs &args,
    sk_sp<const GrGLInterface> glInterface)
    : SkiaRenderEngine(Threaded::NO, static_cast<PixelFormat>(args.pixelFormat),
                       BlurAlgorithm::NONE),
      mGLInterface(std::move(glInterface)) {}

SkiaDesktopGLRenderEngine::~SkiaDesktopGLRenderEngine() {
  finishRenderingAndAbandonContexts();
}

SkiaRenderEngine::Contexts SkiaDesktopGLRenderEngine::createContexts() {
  Contexts contexts;
  contexts.first =
      SkiaGpuContext::MakeGL_Ganesh(mGLInterface, mSkSLCacheMonitor);
  contexts.second = nullptr; // no protected context
  return contexts;
}

base::unique_fd
SkiaDesktopGLRenderEngine::flushAndSubmit(SkiaGpuContext *context,
                                          sk_sp<SkSurface> /*dstSurface*/) {
  sk_sp<GrDirectContext> gr = context->grDirectContext();
  if (gr)
    gr->flushAndSubmit();
  return base::unique_fd(); // pre-signaled, no real sync FD
}

void SkiaDesktopGLRenderEngine::appendBackendSpecificInfoToDump(
    std::string &result) {
  result.append("SkiaDesktopGLRenderEngine (desktop GL, no EGL)\n");
}

} // namespace android::renderengine::skia
