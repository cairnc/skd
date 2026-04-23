// binder/IInterface shim — upstream LayerState.cpp only uses
// IInterface::asBinder, interface_cast, and checked_interface_cast to shuffle
// binder pointers during Parcel I/O. The replayer never IPCs, so asBinder
// returns nullptr and the cast helpers return nullptr.
#pragma once
#include <binder/Binder.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

namespace android {

class IInterface : public virtual RefBase {
public:
  IInterface() = default;
  ~IInterface() override = default;
  // Upstream signature: static sp<IBinder> asBinder(const sp<IInterface>&)
  // plus a const IInterface* overload. We template so any sp<X> works.
  template <typename T> static sp<IBinder> asBinder(const sp<T> & /*iface*/) {
    return nullptr;
  }
  template <typename T> static sp<IBinder> asBinder(const T * /*iface*/) {
    return nullptr;
  }
};

template <typename INTERFACE>
inline sp<INTERFACE> interface_cast(const sp<IBinder> & /*obj*/) {
  return nullptr;
}

template <typename INTERFACE>
inline sp<INTERFACE> checked_interface_cast(const sp<IBinder> & /*obj*/) {
  return nullptr;
}

} // namespace android
