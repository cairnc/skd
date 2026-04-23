// Minimal GraphicBuffer for layerviewer — not a real AOSP port.
//
// Real AOSP GraphicBuffer wraps an AHardwareBuffer + ANativeWindowBuffer and
// crosses binder via Flattenable. We replace it with a thin RefBase-derived
// object owning a GL texture (created lazily on first bind). Only the methods
// ported SurfaceFlinger/RenderEngine code actually calls are implemented.
//
// Buffers intended for RenderEngine to write into use USAGE_HW_RENDER and get
// an FBO attached for render-to-texture; sampled buffers use USAGE_HW_TEXTURE.

#pragma once

#include <cstdint>
#include <string>

#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <utils/RefBase.h>

// AHardwareBuffer is Android's opaque handle type. We forward-declare it as
// the same name so sp<GraphicBuffer>::toAHardwareBuffer() signatures match
// (we return nullptr in the stub, since we don't import real buffers).
extern "C" {
typedef struct AHardwareBuffer AHardwareBuffer;
}

namespace android {

class GraphicBuffer : public RefBase {
public:
  enum : uint64_t {
    USAGE_SW_READ_OFTEN = 0x03ULL,
    USAGE_SW_WRITE_OFTEN = 0x30ULL,
    USAGE_SOFTWARE_MASK = 0xFFULL,
    USAGE_PROTECTED = 1ULL << 14,
    USAGE_HW_TEXTURE = 1ULL << 8,
    USAGE_HW_RENDER = 1ULL << 9,
    USAGE_HW_COMPOSER = 1ULL << 11,
    USAGE_HW_VIDEO_ENCODER = 1ULL << 16,
    USAGE_HW_MASK = 0xFFFF00ULL,
    USAGE_CURSOR = 1ULL << 15,
  };

  GraphicBuffer() = default;
  GraphicBuffer(uint32_t w, uint32_t h, PixelFormat format, uint32_t layerCount,
                uint64_t usage, std::string requestorName = {})
      : mWidth(w), mHeight(h), mFormat(format), mLayerCount(layerCount),
        mUsage(usage), mRequestor(std::move(requestorName)) {
    mId = sNextId++;
  }

  uint32_t getWidth() const { return mWidth; }
  uint32_t getHeight() const { return mHeight; }
  uint32_t getStride() const { return mWidth; }
  PixelFormat getPixelFormat() const { return mFormat; }
  uint64_t getUsage() const { return mUsage; }
  uint32_t getLayerCount() const { return mLayerCount; }
  uint64_t getId() const { return mId; }
  uint64_t getUniqueId() const { return mId; }
  status_t initCheck() const { return OK; }
  const std::string &getRequestorName() const { return mRequestor; }

  // Gralloc handle accessors; stubbed out.
  const struct native_handle *handle = nullptr;

  // No real AHB import yet — RenderEngine only calls this for external
  // content which we don't have in a trace-driven context.
  AHardwareBuffer *toAHardwareBuffer() { return nullptr; }
  const AHardwareBuffer *toAHardwareBuffer() const { return nullptr; }
  static GraphicBuffer *fromAHardwareBuffer(AHardwareBuffer *) {
    return nullptr;
  }
  static const GraphicBuffer *fromAHardwareBuffer(const AHardwareBuffer *) {
    return nullptr;
  }

private:
  static inline uint64_t sNextId = 1;
  uint32_t mWidth = 0;
  uint32_t mHeight = 0;
  PixelFormat mFormat = 0;
  uint32_t mLayerCount = 1;
  uint64_t mUsage = 0;
  uint64_t mId = 0;
  std::string mRequestor;
};

} // namespace android
