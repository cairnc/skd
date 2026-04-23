// android::view::LayerMetadataKey shim — upstream ships this as an AIDL
// generated header; mirror the int32_t values used in LayerMetadata.cpp.
#pragma once
#include <cstdint>
namespace android::view {
enum class LayerMetadataKey : int32_t {
  METADATA_OWNER_UID = 1,
  METADATA_WINDOW_TYPE = 2,
  METADATA_TASK_ID = 3,
  METADATA_MOUSE_CURSOR = 4,
  METADATA_ACCESSIBILITY_ID = 5,
  METADATA_OWNER_PID = 6,
  METADATA_DEQUEUE_TIME = 7,
  METADATA_GAME_MODE = 8,
  METADATA_CALLING_UID = 9,
};
} // namespace android::view
