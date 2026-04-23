// AIDL-generated-equivalent stub.
#pragma once
#include <binder/Binder.h>
#include <binder/Parcelable.h>
#include <cstdint>
#include <string>
namespace android::gui {
struct InputApplicationInfo : public Parcelable {
  sp<IBinder> token;
  std::string name;
  int64_t dispatchingTimeoutMillis = 0;
  bool operator==(const InputApplicationInfo &other) const {
    return token == other.token && name == other.name &&
           dispatchingTimeoutMillis == other.dispatchingTimeoutMillis;
  }
};
} // namespace android::gui
