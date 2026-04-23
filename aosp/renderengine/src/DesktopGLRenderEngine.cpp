#include "DesktopGLRenderEngine.h"

#include <renderengine/DisplaySettings.h>
#include <renderengine/ExternalTexture.h>
#include <renderengine/LayerSettings.h>
#include <ui/GraphicBuffer.h>

#include <include/core/SkBlendMode.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkRect.h>
#include <include/core/SkSurface.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>

#include <ftl/future.h>

namespace android::renderengine::skia {

// SkMatrix from a 4x4 affine-2d column-major mat4 (only the 2D submatrix
// matters for the projections SF typically uses).
static SkMatrix SkMatrixFromMat4(const mat4 &m) {
  return SkMatrix::MakeAll(m[0][0], m[1][0], m[3][0], m[0][1], m[1][1], m[3][1],
                           m[0][3], m[1][3], m[3][3]);
}

static SkRect ToSkRect(const FloatRect &r) {
  return SkRect::MakeLTRB(r.left, r.top, r.right, r.bottom);
}

std::unique_ptr<DesktopGLRenderEngine>
DesktopGLRenderEngine::create(sk_sp<GrDirectContext> grContext) {
  return std::unique_ptr<DesktopGLRenderEngine>(
      new DesktopGLRenderEngine(std::move(grContext)));
}

DesktopGLRenderEngine::DesktopGLRenderEngine(sk_sp<GrDirectContext> grContext)
    : mGrContext(std::move(grContext)) {}

DesktopGLRenderEngine::~DesktopGLRenderEngine() = default;

std::future<void> DesktopGLRenderEngine::primeCache(PrimeCacheConfig) {
  std::promise<void> p;
  p.set_value();
  return p.get_future();
}

void DesktopGLRenderEngine::dump(std::string &result) {
  result.append("DesktopGLRenderEngine (layerviewer, shared GrDirectContext)"
                "\n");
}

size_t DesktopGLRenderEngine::getMaxTextureSize() const {
  return mGrContext ? mGrContext->maxTextureSize() : 0;
}

size_t DesktopGLRenderEngine::getMaxViewportDims() const {
  return mGrContext ? mGrContext->maxRenderTargetSize() : 0;
}

void DesktopGLRenderEngine::drawLayersInternal(
    const std::shared_ptr<std::promise<FenceResult>> &&resultPromise,
    const DisplaySettings &display, const std::vector<LayerSettings> &layers,
    const std::shared_ptr<ExternalTexture> &buffer,
    base::unique_fd && /*bufferFence*/) {
  if (!buffer) {
    resultPromise->set_value(base::unexpected(BAD_VALUE));
    return;
  }

  // For now, rasterize into a CPU-side SkSurface whose pixels map back to
  // the ExternalTexture's GraphicBuffer-identity. A future revision will
  // create a GPU-backed SkSurface via SkSurfaces::RenderTarget keyed on the
  // buffer id.
  SkImageInfo info = SkImageInfo::Make(
      buffer->getBuffer()->getWidth(), buffer->getBuffer()->getHeight(),
      kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  sk_sp<SkSurface> surface =
      SkSurfaces::RenderTarget(mGrContext.get(), skgpu::Budgeted::kNo, info);
  if (!surface) {
    // Ganesh couldn't give us a render target — fall back to raster.
    surface = SkSurfaces::Raster(info);
  }
  if (!surface) {
    resultPromise->set_value(base::unexpected(NO_MEMORY));
    return;
  }

  SkCanvas *canvas = surface->getCanvas();

  // Clip to the display's clip Rect (in display pixels).
  if (!display.clip.isEmpty()) {
    canvas->clipRect(SkRect::MakeLTRB(display.clip.left, display.clip.top,
                                      display.clip.right, display.clip.bottom));
  }
  // Start fully transparent.
  canvas->clear(SK_ColorTRANSPARENT);

  for (const LayerSettings &layer : layers) {
    if (layer.skipContentDraw)
      continue;

    SkAutoCanvasRestore restore(canvas, true);
    canvas->concat(SkMatrixFromMat4(layer.geometry.positionTransform));

    SkRect bounds = ToSkRect(layer.geometry.boundaries);
    vec2 cr = layer.geometry.roundedCornersRadius;
    SkRRect rrect = (cr.x > 0 || cr.y > 0)
                        ? SkRRect::MakeRectXY(bounds, cr.x, cr.y)
                        : SkRRect::MakeRect(bounds);

    SkPaint paint;
    paint.setAntiAlias(true);
    if (!layer.disableBlending) {
      paint.setBlendMode(layer.usePremultipliedAlpha ? SkBlendMode::kSrcOver
                                                     : SkBlendMode::kSrc);
    } else {
      paint.setBlendMode(SkBlendMode::kSrc);
    }

    // Solid-color fill (the path that covers most ContainerLayer /
    // SolidColor layers in a trace).
    const vec3 &c = layer.source.solidColor;
    float a = static_cast<float>(layer.alpha);
    SkColor4f color{c.r, c.g, c.b, a};
    paint.setColor4f(color, nullptr);

    // Buffer-backed source paths (external textures) are stubbed: we
    // draw a solid placeholder tinted with the buffer id for now so the
    // layer is visible. A future revision hooks an SkImage cache here.
    if (layer.source.buffer.buffer) {
      uint64_t id = layer.source.buffer.buffer->getBuffer()->getId();
      SkColor4f tint{0.5f + 0.4f * ((id >> 0) & 0xff) / 255.f,
                     0.5f + 0.4f * ((id >> 8) & 0xff) / 255.f,
                     0.5f + 0.4f * ((id >> 16) & 0xff) / 255.f, a};
      paint.setColor4f(tint, nullptr);
    }

    canvas->drawRRect(rrect, paint);
  }

  mGrContext->flushAndSubmit();
  resultPromise->set_value(sp<Fence>(Fence::NO_FENCE));
}

void DesktopGLRenderEngine::drawGainmapInternal(
    const std::shared_ptr<std::promise<FenceResult>> &&resultPromise,
    const std::shared_ptr<ExternalTexture> &, base::borrowed_fd &&,
    const std::shared_ptr<ExternalTexture> &, base::borrowed_fd &&, float,
    ui::Dataspace, const std::shared_ptr<ExternalTexture> &) {
  // Not implemented — gainmap HDR compositing isn't meaningful in a
  // trace-replay viewer.
  resultPromise->set_value(sp<Fence>(Fence::NO_FENCE));
}

} // namespace android::renderengine::skia
