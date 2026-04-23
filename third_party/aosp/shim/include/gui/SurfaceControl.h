#pragma once
#include <binder/Binder.h>
#include <binder/Parcelable.h>
#include <utils/RefBase.h>
namespace android {
class SurfaceControl : public RefBase {
public:
  sp<IBinder> getHandle() const { return nullptr; }
  // Static Parcel helpers — upstream LayerState.cpp uses these to flatten /
  // unflatten sp<SurfaceControl>. We never IPC so both are no-ops.
  static status_t writeNullableToParcel(Parcel &, const sp<SurfaceControl> &) {
    return OK;
  }
  static status_t readNullableFromParcel(const Parcel &,
                                         sp<SurfaceControl> * /*out*/) {
    return OK;
  }
  // layer_state_t::diff / merge use this to decide whether two states
  // reference the same underlying layer. Real impl compares the underlying
  // layer handle; during replay we never have real handles, so fall back to
  // pointer equality — which is what we get out of TransactionProtoParser.
  static bool isSameSurface(const sp<SurfaceControl> &lhs,
                            const sp<SurfaceControl> &rhs) {
    return lhs.get() == rhs.get();
  }
};
} // namespace android
