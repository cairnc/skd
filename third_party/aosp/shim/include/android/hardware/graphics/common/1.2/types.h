// HIDL graphics/common@1.2 types shim — comprehensive enough for SF/RE code.
// The real header has ~hundred Dataspace constants; we replicate the layout
// since SF does bit-manipulation (STANDARD / TRANSFER / RANGE fields).
#pragma once
#include <android/hardware/graphics/common/1.1/types.h>
#include <cstdint>

namespace android::hardware::graphics::common::V1_2 {

enum class PixelFormat : int32_t {
  UNSPECIFIED = 0,
  RGBA_8888 = 0x1,
  RGBX_8888 = 0x2,
  RGB_888 = 0x3,
  RGB_565 = 0x4,
  BGRA_8888 = 0x5,
  YCBCR_422_SP = 0x10,
  YCRCB_420_SP = 0x11,
  YCBCR_422_I = 0x14,
  RGBA_FP16 = 0x16,
  RAW16 = 0x20,
  BLOB = 0x21,
  IMPLEMENTATION_DEFINED = 0x22,
  YCBCR_420_888 = 0x23,
  RAW_OPAQUE = 0x24,
  RAW10 = 0x25,
  RAW12 = 0x26,
  RGBA_1010102 = 0x2b,
  Y8 = 0x20203859,
  Y16 = 0x20363159,
  YV12 = 0x32315659,
  DEPTH_16 = 0x30,
  DEPTH_24 = 0x31,
  DEPTH_24_STENCIL_8 = 0x32,
  DEPTH_32F = 0x33,
  DEPTH_32F_STENCIL_8 = 0x34,
  STENCIL_8 = 0x35,
  YCBCR_P010 = 0x36,
  HSV_888 = 0x37,
  JPEG = 0x100,
  DEPTH_POINT_CLOUD = 0x101,
};

enum class ColorMode : int32_t {
  NATIVE = 0,
  STANDARD_BT601_625 = 1,
  STANDARD_BT601_625_UNADJUSTED = 2,
  STANDARD_BT601_525 = 3,
  STANDARD_BT601_525_UNADJUSTED = 4,
  STANDARD_BT709 = 5,
  DCI_P3 = 6,
  SRGB = 7,
  ADOBE_RGB = 8,
  DISPLAY_P3 = 9,
  BT2020 = 10,
  BT2100_PQ = 11,
  BT2100_HLG = 12,
  DISPLAY_BT2020 = 13,
};

// Dataspace layout: STANDARD[16..21] | TRANSFER[22..26] | RANGE[27..29].
enum class Dataspace : int32_t {
  UNKNOWN = 0,
  ARBITRARY = 1,

  STANDARD_UNSPECIFIED = 0 << 16,
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
  STANDARD_MASK = 63 << 16,

  TRANSFER_UNSPECIFIED = 0 << 22,
  TRANSFER_LINEAR = 1 << 22,
  TRANSFER_SRGB = 2 << 22,
  TRANSFER_SMPTE_170M = 3 << 22,
  TRANSFER_GAMMA2_2 = 4 << 22,
  TRANSFER_GAMMA2_6 = 5 << 22,
  TRANSFER_GAMMA2_8 = 6 << 22,
  TRANSFER_ST2084 = 7 << 22,
  TRANSFER_HLG = 8 << 22,
  TRANSFER_MASK = 31 << 22,

  RANGE_UNSPECIFIED = 0 << 27,
  RANGE_FULL = 1 << 27,
  RANGE_LIMITED = 2 << 27,
  RANGE_EXTENDED = 3 << 27,
  RANGE_MASK = 7 << 27,

  // Composite named spaces used by SF.
  SRGB_LINEAR = 512,
  V0_SRGB_LINEAR = 512,
  V0_SCRGB_LINEAR = 406913024,
  V0_SRGB = 142671872,
  SRGB = 142671872,
  V0_SCRGB = 411107328,
  V0_JFIF = 146931712,
  JFIF = 146931712,
  V0_BT709 = 281083904,
  BT709 = 281083904,
  V0_BT601_625 = 281149440,
  BT601_625 = 281149440,
  V0_BT601_525 = 281280512,
  BT601_525 = 281280512,
  DISPLAY_P3_LINEAR = 139067392,
  DISPLAY_P3 = 143261696,
  DCI_P3_LINEAR = 139067392,
  DCI_P3 = 155844608,
  BT2020_LINEAR = 138018816,
  BT2020 = 147193856,
  BT2020_PQ = 163971072,
  BT2020_ITU = 281411584,
  BT2020_ITU_PQ = 298188800,
  BT2020_ITU_HLG = 302383104,
  BT2020_HLG = 168165376,
  DISPLAY_BT2020 = 142999552,
  DEPTH = 4096,
  SENSOR = 4097,
  HEIF = 4098,
};

enum class Hdr : int32_t {
  DOLBY_VISION = 1,
  HDR10 = 2,
  HLG = 3,
  HDR10_PLUS = 4,
};

// Bitwise operators — AOSP-generated HIDL enums support these so SF code can
// do `dataspace & STANDARD_MASK`.
inline constexpr int32_t operator&(Dataspace a, int32_t b) {
  return static_cast<int32_t>(a) & b;
}
inline constexpr int32_t operator&(int32_t a, Dataspace b) {
  return a & static_cast<int32_t>(b);
}
inline constexpr int32_t operator&(Dataspace a, Dataspace b) {
  return static_cast<int32_t>(a) & static_cast<int32_t>(b);
}
inline constexpr Dataspace operator|(Dataspace a, Dataspace b) {
  return static_cast<Dataspace>(static_cast<int32_t>(a) |
                                static_cast<int32_t>(b));
}
inline constexpr Dataspace operator|(int32_t a, Dataspace b) {
  return static_cast<Dataspace>(a | static_cast<int32_t>(b));
}
inline constexpr Dataspace operator|(Dataspace a, int32_t b) {
  return static_cast<Dataspace>(static_cast<int32_t>(a) | b);
}

} // namespace android::hardware::graphics::common::V1_2
