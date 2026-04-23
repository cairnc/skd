// FenceMonitor shim. Real one spins off a thread to wait on fences and
// records signal times; ours is a no-op because our fences are pre-signaled.
#pragma once
#include <ui/Fence.h>

namespace android::gui {
class FenceMonitor {
public:
  explicit FenceMonitor(const char * /*name*/) {}
  void queueFence(const sp<Fence> & /*fence*/) {}
};
} // namespace android::gui
