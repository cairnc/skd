// AHardwareBuffer <-> GraphicBuffer bridge. In layerviewer, AHB and
// GraphicBuffer are the same object viewed through different types (see
// GraphicBuffer::toAHardwareBuffer / fromAHardwareBuffer). RE's Ganesh
// backend-texture path calls AHardwareBuffer_describe(), so we reconstitute
// the descriptor from the GraphicBuffer fields instead of leaving the
// AHardwareBuffer_Desc zero-initialised (which caused SkiaBackendTexture to
// LOG_ALWAYS_FATAL with a garbage format the first time drawLayers ran).
#include <android/hardware_buffer.h>

#include <ui/GraphicBuffer.h>

extern "C" void AHardwareBuffer_describe(const AHardwareBuffer *ahb,
                                         AHardwareBuffer_Desc *out) {
  if (!out)
    return;
  *out = AHardwareBuffer_Desc{};
  if (!ahb)
    return;
  const auto *gb = android::GraphicBuffer::fromAHardwareBuffer(ahb);
  if (!gb)
    return;
  out->width = gb->getWidth();
  out->height = gb->getHeight();
  out->layers = gb->getLayerCount();
  out->format = static_cast<uint32_t>(gb->getPixelFormat());
  out->usage = gb->getUsage();
  out->stride = gb->getStride();
}
