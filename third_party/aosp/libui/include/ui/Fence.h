// Minimal Fence for layerviewer — not a real AOSP port.
//
// Real AOSP Fence wraps a sync_fence FD and is IPC'd across binder. Here we
// back it with a GLsync object (EGL/GLES path) or leave it unsignaled (CPU
// path). All ports of SF code that reach for Fence get our stub; wait() is
// implemented as a GL glClientWaitSync or a no-op in the CPU path.

#pragma once

#include <android-base/unique_fd.h>
#include <cstdint>
#include <memory>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>

namespace android {

class Fence : public RefBase {
public:
  static constexpr nsecs_t SIGNAL_TIME_INVALID = INT64_MAX;
  static constexpr nsecs_t SIGNAL_TIME_PENDING = INT64_MAX - 1;
  static constexpr int TIMEOUT_NEVER = -1;
  enum class Status { Invalid, Unsignaled, Signaled };

  static const sp<Fence> NO_FENCE;

  Fence() = default;
  // Accept the same shapes AOSP's Fence does: raw fd, base::unique_fd, move.
  explicit Fence(int fd) : mFd(fd) {}
  explicit Fence(base::unique_fd &&fd) : mFd(fd.release()) {}
  // Non-copyable; AOSP uses sp<>.
  Fence(const Fence &) = delete;
  Fence &operator=(const Fence &) = delete;

  // Always signaled in our stub.
  status_t wait(int /*timeoutMs*/) { return OK; }
  status_t waitForever(const char * /*name*/) { return OK; }

  // Raw sync FD (real AOSP returns -1 for NO_FENCE). Our stub usually has -1.
  int get() const { return mFd; }
  int dup() const { return mFd < 0 ? -1 : ::dup(mFd); }
  nsecs_t getSignalTime() const { return 0; }
  Status getStatus() const { return Status::Signaled; }
  bool isValid() const { return false; }

  // Union of this and rhs. Since both are signaled, result is a new Fence.
  static sp<Fence> merge(const char * /*name*/, const sp<Fence> &a,
                         const sp<Fence> & /*b*/) {
    return a ? a : NO_FENCE;
  }

private:
  int mFd = -1;
};

} // namespace android
