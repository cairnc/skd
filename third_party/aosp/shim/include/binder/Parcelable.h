// Parcelable shim — FE never Parcels anything. Virtual writeToParcel /
// readFromParcel return OK so subclasses can override without binder.
#pragma once
#include <utils/Errors.h>

namespace android {

class Parcel;

class Parcelable {
public:
  virtual ~Parcelable() = default;
  virtual status_t writeToParcel(Parcel * /*parcel*/) const { return OK; }
  virtual status_t readFromParcel(const Parcel * /*parcel*/) { return OK; }
};

class Parcel {
public:
  Parcel() = default;
};

} // namespace android
