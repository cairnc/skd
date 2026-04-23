// gui/FrameRateUtils shim — upstream validates vendor-specific frame-rate
// semantics. In the replayer every trace rate is already valid, so accept
// all of them.
#pragma once
#include <cstdint>
namespace android {
inline bool ValidateFrameRate(float /*frameRate*/, int8_t /*compatibility*/,
                              int8_t /*changeFrameRateStrategy*/,
                              const char * /*inFunctionName*/,
                              bool /*privileged*/ = false) {
  return true;
}
} // namespace android
