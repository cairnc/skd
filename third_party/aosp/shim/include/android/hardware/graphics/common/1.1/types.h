// HIDL graphics/common@1.1 types shim. Re-exports 1.0 and adds RenderIntent
// plus extended ColorMode/Dataspace/PixelFormat enums used by SF/RE.
#pragma once
#include <android/hardware/graphics/common/1.0/types.h>
#include <cstdint>

namespace android::hardware::graphics::common::V1_1 {

// V1.1 adds these values on top of V1.0 — the same type-name / same
// underlying int32 layout. Consumers use `using` aliases, so we redefine the
// enum with all values in scope.
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
  DEPTH_16 = 0x30,
  DEPTH_24 = 0x31,
  DEPTH_24_STENCIL_8 = 0x32,
  DEPTH_32F = 0x33,
  DEPTH_32F_STENCIL_8 = 0x34,
  STENCIL_8 = 0x35,
  YCBCR_P010 = 0x36,
  HSV_888 = 0x37,
  Y8 = 0x20203859,
  Y16 = 0x20363159,
  YV12 = 0x32315659,
  JPEG = 0x100,
  // 1.1 additions:
  DEPTH_POINT_CLOUD = 0x101,
  YCBCR_444_888 = 0x29,
  FLEX_RGB_888 = 0x29,
  FLEX_RGBA_8888 = 0x2A,
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
  // 1.1 additions:
  BT2020 = 10,
  BT2100_PQ = 11,
  BT2100_HLG = 12,
};

enum class Dataspace : int32_t {
  UNKNOWN = 0,
  ARBITRARY = 1,
  SRGB_LINEAR = 512,
  SRGB = 142671872,
  BT709 = 281083904,
  BT601_625 = 281149440,
  BT601_525 = 281280512,
  DISPLAY_P3_LINEAR = 139067392,
  DISPLAY_P3 = 143261696,
  DCI_P3_LINEAR = 139067392,
  DCI_P3 = 155844608,
  BT2020_LINEAR = 138018816,
  BT2020 = 147193856,
  BT2020_PQ = 163971072,
  DEPTH = 4096,
  SENSOR = 4097,
  // 1.1 additions:
  BT2020_ITU = 281411584,
  BT2020_ITU_PQ = 298188800,
  BT2020_ITU_HLG = 302383104,
  BT2020_HLG = 168165376,
  DISPLAY_BT2020 = 142999552,
};

enum class RenderIntent : int32_t {
  COLORIMETRIC = 0,
  ENHANCE = 1,
  TONE_MAP_COLORIMETRIC = 2,
  TONE_MAP_ENHANCE = 3,
};

} // namespace android::hardware::graphics::common::V1_1
