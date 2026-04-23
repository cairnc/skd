#pragma once
namespace aidl::android::hardware::graphics::common {
struct XyColor {
  float x = 0.f, y = 0.f;
};
struct Smpte2086 {
  XyColor primaryRed, primaryGreen, primaryBlue, whitePoint;
  float maxLuminance = 0.f;
  float minLuminance = 0.f;
};
} // namespace aidl::android::hardware::graphics::common
