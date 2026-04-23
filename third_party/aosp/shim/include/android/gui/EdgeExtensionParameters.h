// Stub — FE holds this type by value/pointer only. Real impl is AIDL-generated.
#pragma once
#include <binder/Parcelable.h>
#include <cstdint>
namespace android::gui {
struct EdgeExtensionParameters : public Parcelable {
  bool extendLeft = false;
  bool extendRight = false;
  bool extendTop = false;
  bool extendBottom = false;
  bool operator==(const EdgeExtensionParameters &o) const {
    return extendLeft == o.extendLeft && extendRight == o.extendRight &&
           extendTop == o.extendTop && extendBottom == o.extendBottom;
  }
};
} // namespace android::gui
