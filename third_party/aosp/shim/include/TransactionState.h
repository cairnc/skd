// SF-internal TransactionState shim — FE + TransactionHandler thread it
// around as the "what happened in this transaction" bag of state. Real
// AOSP version is in frameworks/native/services/surfaceflinger/. We stub
// just the fields the replay path sets/reads.
#pragma once

#include <chrono>
#include <cstdint>
#include <gui/ITransactionCompletedListener.h>
#include <gui/LayerState.h>
#include <utils/StrongPointer.h>
#include <vector>

namespace android {

struct ResolvedComposerState : public ComposerState {
  uint32_t layerId = UNASSIGNED_LAYER_ID;
  uint32_t parentId = UNASSIGNED_LAYER_ID;
  uint32_t relativeParentId = UNASSIGNED_LAYER_ID;
  uint32_t touchCropId = UNASSIGNED_LAYER_ID;
  std::shared_ptr<renderengine::ExternalTexture> externalTexture;
};

struct TransactionState {
  TransactionState() = default;
  std::vector<ResolvedComposerState> states;
  std::vector<DisplayState> displays;
  uint32_t flags = 0;
  sp<IBinder> applyToken;
  InputWindowCommands inputWindowCommands;
  int64_t desiredPresentTime = 0;
  bool isAutoTimestamp = true;
  FrameTimelineInfo frameTimelineInfo;
  std::vector<ListenerCallbacks> listenerCallbacks;
  uint64_t id = 0;
  int32_t originPid = -1;
  int32_t originUid = -1;
  std::vector<uint64_t> mergedTransactionIds;
};

} // namespace android
