// gui/DisplayInfo.h shim — fields match upstream so FE's DisplayInfo and
// TransactionProtoParser compile against the real ui::LogicalDisplayId type.
// Parcel writeToParcel/readFromParcel are no-ops since we never IPC.
#pragma once

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <ui/LogicalDisplayId.h>
#include <ui/Transform.h>

namespace android::gui {

struct DisplayInfo : public Parcelable {
  ui::LogicalDisplayId displayId = ui::LogicalDisplayId::INVALID;

  int32_t logicalWidth = 0;
  int32_t logicalHeight = 0;

  ui::Transform transform;

  status_t writeToParcel(android::Parcel *) const override { return 0; }
  status_t readFromParcel(const android::Parcel *) override { return 0; }
  void dump(std::string & /*result*/, const char * /*prefix*/ = "") const {}
};

} // namespace android::gui
