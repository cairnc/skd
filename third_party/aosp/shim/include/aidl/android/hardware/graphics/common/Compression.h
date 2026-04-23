#pragma once
#include <cstdint>
namespace aidl::android::hardware::graphics::common {
enum class Compression : int32_t {
  NONE = 0,
  DISPLAY_STREAM_COMPRESSION = 1,
};
}
