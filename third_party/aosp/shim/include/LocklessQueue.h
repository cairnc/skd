// LocklessQueue shim — TransactionHandler uses it for cross-thread queueing.
// For single-threaded offline replay, a plain std::deque with matching API
// is fine.
#pragma once
#include <deque>
#include <mutex>
#include <optional>

namespace android {

template <typename T> class LocklessQueue {
public:
  bool isEmpty() const {
    std::lock_guard lk(mMu);
    return mQ.empty();
  }
  void push(T item) {
    std::lock_guard lk(mMu);
    mQ.push_back(std::move(item));
  }
  std::optional<T> pop() {
    std::lock_guard lk(mMu);
    if (mQ.empty())
      return std::nullopt;
    T v = std::move(mQ.front());
    mQ.pop_front();
    return v;
  }

private:
  mutable std::mutex mMu;
  std::deque<T> mQ;
};

} // namespace android
