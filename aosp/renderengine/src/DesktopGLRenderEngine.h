// DesktopGLRenderEngine — a drop-in RenderEngine implementation for a host
// build of layerviewer.
//
// Real AOSP SkiaGLRenderEngine needs EGL + GLES and wires up AHB-backed
// textures via GrAHardwareBufferUtils. Neither works outside Android. Since
// the *interface* we care about is `renderengine::RenderEngine` (what SF's
// CompositionEngine drives), we subclass that directly and implement
// drawLayersInternal() with plain Skia/Ganesh primitives against a
// caller-owned GrDirectContext.
//
// Coverage is intentionally narrow: solid-colour layers, buffer-backed layers
// (rasterized into a managed SkImage), alpha, corner radius, basic
// transforms. Advanced filters (blur, linear effects, LUT shaders, gainmaps)
// are no-ops.
#pragma once

#include <renderengine/RenderEngine.h>

#include <include/gpu/ganesh/GrDirectContext.h>

#include <unordered_map>

namespace android::renderengine::skia {

class DesktopGLRenderEngine : public RenderEngine {
public:
  // Caller keeps ownership of the GrDirectContext (typically shared with
  // the app's own Skia compositor).
  static std::unique_ptr<DesktopGLRenderEngine>
  create(sk_sp<GrDirectContext> grContext);

  ~DesktopGLRenderEngine() override;

  // --- metadata ---
  std::future<void> primeCache(PrimeCacheConfig) override;
  void dump(std::string &result) override;
  size_t getMaxTextureSize() const override;
  size_t getMaxViewportDims() const override;
  bool supportsProtectedContent() const override { return false; }
  void useProtectedContext(bool) override {}
  void onActiveDisplaySizeChanged(ui::Size) override {}
  int getContextPriority() override { return 0; }
  bool supportsBackgroundBlur() override { return false; }
  void cleanupPostRender() override {}
  bool canSkipPostRenderCleanup() const override { return true; }

  // --- external textures: stubbed ---
  void mapExternalTextureBuffer(const sp<GraphicBuffer> &, bool) override {}
  void unmapExternalTextureBuffer(sp<GraphicBuffer> &&) override {}

protected:
  void drawLayersInternal(
      const std::shared_ptr<std::promise<FenceResult>> &&resultPromise,
      const DisplaySettings &display, const std::vector<LayerSettings> &layers,
      const std::shared_ptr<ExternalTexture> &buffer,
      base::unique_fd &&bufferFence) override;

  void drawGainmapInternal(
      const std::shared_ptr<std::promise<FenceResult>> &&resultPromise,
      const std::shared_ptr<ExternalTexture> &sdr, base::borrowed_fd &&sdrFence,
      const std::shared_ptr<ExternalTexture> &hdr, base::borrowed_fd &&hdrFence,
      float hdrSdrRatio, ui::Dataspace dataspace,
      const std::shared_ptr<ExternalTexture> &gainmap) override;

private:
  explicit DesktopGLRenderEngine(sk_sp<GrDirectContext> grContext);

  sk_sp<GrDirectContext> mGrContext;
};

} // namespace android::renderengine::skia
