// Trivial std::hash wrapper for sp<T>.
#pragma once
#include <functional>
#include <utils/StrongPointer.h>
namespace android {
template <typename T> struct SpHash {
  size_t operator()(const sp<T> &p) const { return std::hash<T *>{}(p.get()); }
};
} // namespace android
