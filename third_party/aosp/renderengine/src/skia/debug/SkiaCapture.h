// layerviewer: drop-in SkiaCapture stub. The real AOSP one records SKP/MSKP
// traces of RenderEngine output; we don't need that in a trace-replay viewer
// (the whole app IS the trace replay). Shape matches what SkiaRenderEngine
// calls, so the base class compiles unchanged.
#pragma once

#include <include/core/SkSurface.h>

#include <cstdint>

class SkCanvas;
class SkNWayCanvas;

namespace android::renderengine::skia {

class SkiaCapture {
public:
  struct OffscreenState {
    SkCanvas *offscreenCanvas = nullptr;
    SkNWayCanvas *nwayCanvas = nullptr;
  };

  SkiaCapture() = default;
  ~SkiaCapture() = default;

  SkCanvas *tryCapture(SkSurface *surface) {
    return surface ? surface->getCanvas() : nullptr;
  }
  void endCapture() {}
  bool isCaptureRunning() { return false; }

  SkCanvas *tryOffscreenCapture(SkSurface *surface, OffscreenState *) {
    return surface ? surface->getCanvas() : nullptr;
  }
  uint64_t endOffscreenCapture(OffscreenState *) { return 0; }
};

} // namespace android::renderengine::skia
