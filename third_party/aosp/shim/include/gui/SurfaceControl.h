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
  // layer handle; during replay we never have real handles — both sides
  // are always nullptr. Treating nullptr==nullptr as "same" would make
  // diff() skip eReparent, which in turn drops Changes::Hierarchy off the
  // global-changes mask so LayerHierarchyBuilder::doUpdate never runs for
  // the reparent — layers end up orphaned in the wrong subtree.
  //
  // To keep eReparent honest during replay, report nullptr-vs-nullptr as
  // NOT the same: the resolvedComposerState.parentId path still carries the
  // actual parent change, and this just ensures the hierarchy builder gets
  // signaled.
  static bool isSameSurface(const sp<SurfaceControl> &lhs,
                            const sp<SurfaceControl> &rhs) {
    if (!lhs && !rhs)
      return false;
    return lhs.get() == rhs.get();
  }
};
} // namespace android
