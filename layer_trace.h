// Per-entry replay state. The SurfaceFlinger FrontEnd's LayerSnapshotBuilder
// maintains a stateful current-frame working set, so to let the UI scrub to
// any vsync we deep-copy its output after each commit. The UI reads real
// `LayerSnapshot` fields (no parallel struct, no drift when SF adds a new
// field) and the snapshot's sp<ExternalTexture> → sp<GraphicBuffer> refs
// travel with the copy so the per-buffer GL texture stays alive.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <FrontEnd/LayerSnapshot.h>
#include <ui/LayerStack.h>
#include <ui/Transform.h>

namespace layerviewer {

struct CapturedFrame {
  int64_t vsyncId = 0;
  int64_t tsNs = 0;
  int addedCount = 0;
  int destroyedHandleCount = 0;
  int txnCount = 0;
  bool displaysChanged = false;
  int32_t displayWidth = 0;
  int32_t displayHeight = 0;
  // Dominant display's traced layer stack + rotation. CompositionEngine's
  // Output needs these: `setLayerFilter(layerStack)` scopes which snapshots
  // land on this output, and `setProjection(orientation, ...)` feeds the
  // DisplaySettings.orientation that RenderEngine applies to the output.
  android::ui::LayerStack displayLayerStack{android::ui::INVALID_LAYER_STACK};
  android::ui::Rotation displayRotation = android::ui::ROTATION_0;

  // Reachable snapshots in paint order (globalZ ascending). Matches SF's
  // own android.surfaceflinger.layers emit set — offscreen snapshots are
  // dropped.
  std::vector<android::surfaceflinger::frontend::LayerSnapshot> snapshots;
  // layer id (snapshot.path.id) → index into `snapshots`.
  std::unordered_map<uint32_t, size_t> byLayerId;
  // layer id → parent id (0xffffffff / UNASSIGNED_LAYER_ID for roots).
  // LayerSnapshot doesn't carry parentId directly; it's on
  // RequestedLayerState in the lifecycle manager, so we capture it at
  // replay time alongside the snapshot copy.
  std::unordered_map<uint32_t, uint32_t> parentByLayerId;
  // layer id → child ids in paint order (ascending globalZ).
  std::unordered_map<uint32_t, std::vector<uint32_t>> childrenByLayerId;
  // top-level reachable layers, in paint order.
  std::vector<uint32_t> rootIds;

  // Convenience accessors so the UI doesn't have to rebuild iterators.
  const android::surfaceflinger::frontend::LayerSnapshot *
  snapshot(uint32_t layerId) const {
    auto it = byLayerId.find(layerId);
    return it == byLayerId.end() ? nullptr : &snapshots[it->second];
  }
  const std::vector<uint32_t> &children(uint32_t layerId) const {
    static const std::vector<uint32_t> kEmpty;
    auto it = childrenByLayerId.find(layerId);
    return it == childrenByLayerId.end() ? kEmpty : it->second;
  }
};

struct ReplayedTrace {
  std::string path;
  std::string error;
  std::vector<CapturedFrame> frames;
  int packetCount = 0;
  int layerSnapshotPacketCount = 0;
};

// Load + replay synchronously. `error` set and `frames` empty on failure.
std::unique_ptr<ReplayedTrace> LoadAndReplay(const std::string &path);

} // namespace layerviewer
