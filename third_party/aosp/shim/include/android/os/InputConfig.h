// AIDL-generated-equivalent stub. Mirrors
// frameworks/native/libs/input/android/os/InputConfig.aidl.
#pragma once
#include <cstdint>

namespace android::os {

enum class InputConfig : int32_t {
  DEFAULT = 0,
  NO_INPUT_CHANNEL = 1 << 0,
  NOT_VISIBLE = 1 << 1,
  NOT_FOCUSABLE = 1 << 2,
  NOT_TOUCHABLE = 1 << 3,
  PREVENT_SPLITTING = 1 << 4,
  DUPLICATE_TOUCH_TO_WALLPAPER = 1 << 5,
  IS_WALLPAPER = 1 << 6,
  PAUSE_DISPATCHING = 1 << 7,
  TRUSTED_OVERLAY = 1 << 8,
  WATCH_OUTSIDE_TOUCH = 1 << 9,
  SLIPPERY = 1 << 10,
  DISABLE_USER_ACTIVITY = 1 << 11,
  DROP_INPUT = 1 << 12,
  DROP_INPUT_IF_OBSCURED = 1 << 13,
  SPY = 1 << 14,
  INTERCEPTS_STYLUS = 1 << 15,
  CLONE = 1 << 16,
  GLOBAL_STYLUS_BLOCKS_TOUCH = 1 << 17,
  SENSITIVE_FOR_PRIVACY = 1 << 18,
};

} // namespace android::os
