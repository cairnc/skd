#pragma once
#include <binder/Parcelable.h>
#include <memory>
#include <utils/RefBase.h>
namespace android::gui {
class BufferReleaseChannel {
public:
  // ProducerEndpoint is a Parcelable upstream — LayerState.cpp reads/writes
  // a shared_ptr<ProducerEndpoint> via Parcel::writeParcelable on layer
  // transactions. Expose the inheritance so those sites compile.
  class ProducerEndpoint : public Parcelable {};
};
} // namespace android::gui
