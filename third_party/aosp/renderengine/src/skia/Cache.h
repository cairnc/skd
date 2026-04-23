// layerviewer: no-op Cache. AOSP's Cache pre-warms Skia pipelines by drawing
// many variations of LayerSettings — we don't need this at viewer startup.
#pragma once

#include <renderengine/RenderEngine.h>

namespace android::renderengine::skia {
class SkiaRenderEngine;
class Cache {
public:
  static void primeShaderCache(SkiaRenderEngine *, PrimeCacheConfig) {}
};
} // namespace android::renderengine::skia
