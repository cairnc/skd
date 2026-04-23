// gui/DisplayInfo.h shim — FE's own DisplayInfo.h references this type
// only as an optional / pointer field.
#pragma once
#include <binder/Parcelable.h>
#include <ui/Transform.h>

namespace android::gui {
struct DisplayInfo : public Parcelable {
  int32_t displayId = 0;
  int32_t logicalWidth = 0;
  int32_t logicalHeight = 0;
  uint32_t rotation = 0;
  ::android::ui::Transform transform;
};
} // namespace android::gui
