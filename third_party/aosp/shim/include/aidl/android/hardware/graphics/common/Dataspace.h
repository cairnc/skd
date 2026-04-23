// AIDL Dataspace shim — mirrors the int32_t layout of the generated enum.
// Real one is in android::hardware::graphics::common AIDL; our port code
// refers to it through the fully-qualified AIDL namespace.
#pragma once
#include <cstdint>
#include <string>

namespace aidl::android::hardware::graphics::common {

enum class Dataspace : int32_t {
  UNKNOWN = 0,
  ARBITRARY = 1,
  STANDARD_UNSPECIFIED = 0,
  STANDARD_BT709 = 1 << 16,
  STANDARD_BT601_625 = 2 << 16,
  STANDARD_BT601_625_UNADJUSTED = 3 << 16,
  STANDARD_BT601_525 = 4 << 16,
  STANDARD_BT601_525_UNADJUSTED = 5 << 16,
  STANDARD_BT2020 = 6 << 16,
  STANDARD_BT2020_CONSTANT_LUMINANCE = 7 << 16,
  STANDARD_BT470M = 8 << 16,
  STANDARD_FILM = 9 << 16,
  STANDARD_DCI_P3 = 10 << 16,
  STANDARD_ADOBE_RGB = 11 << 16,

  TRANSFER_UNSPECIFIED = 0,
  TRANSFER_LINEAR = 1 << 22,
  TRANSFER_SRGB = 2 << 22,
  TRANSFER_SMPTE_170M = 3 << 22,
  TRANSFER_GAMMA2_2 = 4 << 22,
  TRANSFER_GAMMA2_6 = 5 << 22,
  TRANSFER_GAMMA2_8 = 6 << 22,
  TRANSFER_ST2084 = 7 << 22,
  TRANSFER_HLG = 8 << 22,

  RANGE_UNSPECIFIED = 0,
  RANGE_FULL = 1 << 27,
  RANGE_LIMITED = 2 << 27,
  RANGE_EXTENDED = 3 << 27,

  SRGB_LINEAR = 512,
  SCRGB_LINEAR = 406913024,
  SRGB = 142671872,
  SCRGB = 411107328,
  JFIF = 146931712,
  BT601_625 = 281149440,
  BT601_525 = 281280512,
  BT709 = 281083904,
  DCI_P3_LINEAR = 139067392,
  DCI_P3 = 155844608,
  DISPLAY_P3_LINEAR = 139067392,
  DISPLAY_P3 = 143261696,
  ADOBE_RGB = 151715840,
  BT2020_LINEAR = 138018816,
  BT2020 = 147193856,
  BT2020_PQ = 163971072,
  DEPTH = 4096,
  SENSOR = 4097,
  BT2020_ITU = 281411584,
  BT2020_ITU_PQ = 298188800,
  BT2020_ITU_HLG = 302383104,
  BT2020_HLG = 168165376,
  DISPLAY_BT2020 = 142999552,
  HEIF = 4098,

  // Bit-mask constants consumed by SF/tonemap/shaders code.
  STANDARD_MASK = 63 << 16,
  TRANSFER_MASK = 31 << 22,
  RANGE_MASK = 7 << 27,
};

inline constexpr int32_t operator&(Dataspace a, Dataspace b) {
  return static_cast<int32_t>(a) & static_cast<int32_t>(b);
}
inline constexpr Dataspace operator|(Dataspace a, Dataspace b) {
  return static_cast<Dataspace>(static_cast<int32_t>(a) |
                                static_cast<int32_t>(b));
}

inline std::string toString(Dataspace v) {
  return "Dataspace(" + std::to_string(static_cast<int32_t>(v)) + ")";
}

} // namespace aidl::android::hardware::graphics::common
