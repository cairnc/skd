// ITransactionCompletedListener shim. Enough of the upstream type surface
// (CallbackId + ListenerCallbacks) for byte-identical gui/LayerState.cpp to
// build; we don't actually dispatch callbacks.
#pragma once
#include <android/gui/CachingHint.h>
#include <binder/Binder.h>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace android {

class ITransactionCompletedListener : public IBinder {
public:
  virtual void onTrustedPresentationChanged(int32_t /*windowId*/,
                                            bool /*entered*/) {}
};

struct CallbackId {
  int64_t id = 0;
  uint64_t sp = 0;
  enum class Type : int32_t { NORMAL = 0, ONCOMMIT = 1 };
  Type type = Type::NORMAL;
  bool operator==(const CallbackId &o) const {
    return id == o.id && sp == o.sp && type == o.type;
  }
};

struct CallbackIdHash {
  size_t operator()(const CallbackId &c) const {
    return std::hash<int64_t>{}(c.id) ^ std::hash<uint64_t>{}(c.sp);
  }
};

class ListenerCallbacks {
public:
  ListenerCallbacks() = default;
  ListenerCallbacks(
      const sp<IBinder> &listener,
      const std::unordered_set<CallbackId, CallbackIdHash> &callbacks)
      : transactionCompletedListener(listener),
        callbackIds(callbacks.begin(), callbacks.end()) {}
  ListenerCallbacks(const sp<IBinder> &listener,
                    const std::vector<CallbackId> &ids)
      : transactionCompletedListener(listener), callbackIds(ids) {}

  bool operator==(const ListenerCallbacks &rhs) const {
    return transactionCompletedListener == rhs.transactionCompletedListener &&
           callbackIds == rhs.callbackIds;
  }

  sp<IBinder> transactionCompletedListener;
  std::vector<CallbackId> callbackIds;
};

// Hash functor for sp<IBinder> keys (used by TransactionHandler).
struct IListenerHash {
  size_t operator()(const sp<IBinder> &ptr) const {
    return std::hash<IBinder *>{}(ptr.get());
  }
};

} // namespace android
