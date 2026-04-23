// SurfaceFlinger top-class shim — LayerHandle takes sp<SurfaceFlinger> but
// replay never constructs one; a complete class is enough for sp<> to work.
#pragma once
#include <utils/RefBase.h>
namespace android {
class Layer;
class SurfaceFlinger : public RefBase {
public:
  void onHandleDestroyed(sp<Layer>, uint32_t /*layerId*/) {}
};
} // namespace android
