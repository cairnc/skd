#pragma once
#include <cstdint>
namespace aidl::android::hardware::graphics::common {
enum class Interlaced : int32_t {
  NONE = 0,
  TOP_BOTTOM = 1,
  RIGHT_LEFT = 2,
};
}
