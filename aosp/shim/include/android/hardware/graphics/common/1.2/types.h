// HIDL graphics/common@1.2 types shim. Adds HDR enum, extends Dataspace/
// ColorMode/PixelFormat with newer entries.
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
  BT2020_ITU = 281411584,
  BT2020_ITU_PQ = 298188800,
  BT2020_ITU_HLG = 302383104,
  BT2020_HLG = 168165376,
  DISPLAY_BT2020 = 142999552,
  // 1.2 additions:
  HEIF = 4098,
  JFIF = 146931712,
};

enum class Hdr : int32_t {
  DOLBY_VISION = 1,
  HDR10 = 2,
  HLG = 3,
  HDR10_PLUS = 4,
};

} // namespace android::hardware::graphics::common::V1_2
