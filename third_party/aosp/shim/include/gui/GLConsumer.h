// GLConsumer shim — LayerFE.cpp only calls the static texture-matrix helper.
// Port of GLConsumer::computeTransformMatrix from
// frameworks/native/libs/gui/GLConsumerUtils.cpp: given a buffer's size,
// pixel format, crop rect and NATIVE_WINDOW_TRANSFORM_* flags, build the 4×4
// texture matrix that maps normalized UVs into the crop-cropped, optionally
// rotated region of the source buffer. The crop adjustment in `filtering`
// mode shrinks by half a texel on bilinear-friendly formats so samples stay
// inside the valid region, matching AOSP.
#pragma once

#include <cstring>

#include <math/mat4.h>
#include <system/window.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <utils/RefBase.h>

namespace android {

class GLConsumer {
public:
  static void computeTransformMatrix(float outTransform[16], float bufferWidth,
                                     float bufferHeight,
                                     PixelFormat pixelFormat,
                                     const Rect &cropRect, uint32_t transform,
                                     bool filtering) {
    static const mat4 mtxFlipH(-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1);
    static const mat4 mtxFlipV(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1);
    static const mat4 mtxRot90(0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1);

    mat4 xform;
    if (transform & NATIVE_WINDOW_TRANSFORM_FLIP_H)
      xform *= mtxFlipH;
    if (transform & NATIVE_WINDOW_TRANSFORM_FLIP_V)
      xform *= mtxFlipV;
    if (transform & NATIVE_WINDOW_TRANSFORM_ROT_90)
      xform *= mtxRot90;

    if (!cropRect.isEmpty()) {
      float tx = 0.0f, ty = 0.0f, sx = 1.0f, sy = 1.0f;
      float shrinkAmount = 0.0f;
      if (filtering) {
        switch (pixelFormat) {
        case PIXEL_FORMAT_RGBA_8888:
        case PIXEL_FORMAT_RGBX_8888:
        case PIXEL_FORMAT_RGBA_FP16:
        case PIXEL_FORMAT_RGBA_1010102:
        case PIXEL_FORMAT_RGB_888:
        case PIXEL_FORMAT_RGB_565:
        case PIXEL_FORMAT_BGRA_8888:
          shrinkAmount = 0.5f;
          break;
        default:
          shrinkAmount = 1.0f;
          break;
        }
      }
      if (cropRect.width() < bufferWidth) {
        tx = (float(cropRect.left) + shrinkAmount) / bufferWidth;
        sx = (float(cropRect.width()) - (2.0f * shrinkAmount)) / bufferWidth;
      }
      if (cropRect.height() < bufferHeight) {
        ty = (float(bufferHeight - cropRect.bottom) + shrinkAmount) /
             bufferHeight;
        sy = (float(cropRect.height()) - (2.0f * shrinkAmount)) / bufferHeight;
      }
      mat4 crop(sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, 1, 0, tx, ty, 0, 1);
      xform = crop * xform;
    }

    xform = mtxFlipV * xform;
    std::memcpy(outTransform, xform.asArray(), sizeof(float) * 16);
  }
};

} // namespace android
