#pragma once
#include <binder/Binder.h>
#include <utils/RefBase.h>
namespace android {
class SurfaceControl : public RefBase {
public:
  sp<IBinder> getHandle() const { return nullptr; }
};
} // namespace android
