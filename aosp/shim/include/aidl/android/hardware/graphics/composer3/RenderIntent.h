#pragma once
#include <cstdint>
namespace aidl::android::hardware::graphics::composer3 {
enum class RenderIntent : int32_t {
  COLORIMETRIC = 0,
  ENHANCE = 1,
  TONE_MAP_COLORIMETRIC = 2,
  TONE_MAP_ENHANCE = 3,
};
}
