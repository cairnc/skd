#pragma once
#include <cstdint>
namespace aidl::android::hardware::graphics::composer3 {
enum class DimmingStage : int32_t {
  NONE = 0,
  LINEAR = 1,
  GAMMA_OETF = 2,
};
}
