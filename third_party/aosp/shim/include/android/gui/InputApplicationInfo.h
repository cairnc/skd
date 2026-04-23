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
};
} // namespace android::gui
