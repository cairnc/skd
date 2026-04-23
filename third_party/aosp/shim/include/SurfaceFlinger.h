// SurfaceFlinger top-class shim — LayerHandle takes sp<SurfaceFlinger> but
// replay never constructs one; a complete class is enough for sp<> to work.
// The static rotation accessor is called by upstream LayerFE.cpp when a layer
// has `geomBufferUsesDisplayInverseTransform` set; the viewer never rotates
// its fake display, so returning 0 keeps the texture-transform math identity.
#pragma once
#include <cstdint>
#include <utils/RefBase.h>
namespace android {
class Layer;
class SurfaceFlinger : public RefBase {
public:
  void onHandleDestroyed(sp<Layer>, uint32_t /*layerId*/) {}
  static uint32_t getActiveDisplayRotationFlags() { return 0; }
};
} // namespace android
