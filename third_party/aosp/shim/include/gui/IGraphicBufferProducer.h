// IGraphicBufferProducer shim. Also drags in GraphicBuffer/Fence/HdrMetadata/
// NativeHandle/ReleaseCallbackId/<unordered_set> since LayerState.h reaches
// for all of them transitively through this header in real AOSP.
#pragma once
#include <binder/Binder.h>
#include <cstdint>
#include <gui/HdrMetadata.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <unordered_set>
#include <utils/NativeHandle.h>
#include <utils/Timers.h> // nsecs_t

namespace android {
class IGraphicBufferProducer : public IBinder {};
class IProducerListener : public IBinder {};

struct ReleaseCallbackId {
  uint64_t id = 0;
  uint64_t framenumber = 0;
  bool operator==(const ReleaseCallbackId &o) const {
    return id == o.id && framenumber == o.framenumber;
  }
};
} // namespace android
