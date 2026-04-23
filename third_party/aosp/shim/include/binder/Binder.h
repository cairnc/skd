// Binder.h shim — layerviewer has no IPC. Provide IBinder / BBinder as
// RefBase-derived identity types; FE uses them as opaque handles and for
// sp<>/wp<> plumbing only.
#pragma once

#include <utils/RefBase.h>
#include <utils/String16.h>

namespace android {

class Parcel;
class BBinder;

class IBinder : public virtual RefBase {
public:
  IBinder() = default;
  virtual ~IBinder() = default;
  virtual const String16 &getInterfaceDescriptor() const {
    static const String16 desc;
    return desc;
  }
  virtual BBinder *localBinder() { return nullptr; }
};

class BBinder : public IBinder {
public:
  BBinder() = default;
  ~BBinder() override = default;
  BBinder *localBinder() override { return this; }
};

} // namespace android
