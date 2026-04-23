// replay_trace — stage 1 of the layerviewer pipeline.
//
// Reads a perfetto .pftrace that has both android.surfaceflinger.transactions
// and android.surfaceflinger.layers, drives the SF FrontEnd the same way the
// upstream layertracegenerator tool does (LayerLifecycleManager →
// LayerHierarchyBuilder → LayerSnapshotBuilder), and dumps a text summary of
// the reconstructed snapshots so we can eyeball whether the FE is recreating
// state correctly.

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <perfetto/trace/trace.pb.h>

#include <Client.h>
#include <FrontEnd/LayerCreationArgs.h>
#include <FrontEnd/LayerHierarchy.h>
#include <FrontEnd/LayerLifecycleManager.h>
#include <FrontEnd/LayerSnapshot.h>
#include <FrontEnd/LayerSnapshotBuilder.h>
#include <FrontEnd/RequestedLayerState.h>
#include <Tracing/TransactionProtoParser.h>
#include <TransactionState.h>
#include <gui/LayerState.h>
#include <gui/WindowInfo.h>

using android::BBinder;
using android::layer_state_t;
using android::TransactionState;
using android::surfaceflinger::LayerCreationArgs;
using android::surfaceflinger::TransactionProtoParser;
using android::surfaceflinger::frontend::DisplayInfos;
using android::surfaceflinger::frontend::LayerHierarchyBuilder;
using android::surfaceflinger::frontend::LayerLifecycleManager;
using android::surfaceflinger::frontend::LayerSnapshotBuilder;
using android::surfaceflinger::frontend::RequestedLayerState;

namespace {

bool readFile(const std::string &path, std::string &out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    fprintf(stderr, "failed to open %s\n", path.c_str());
    return false;
  }
  std::stringstream ss;
  ss << in.rdbuf();
  out = ss.str();
  return true;
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr,
            "usage: %s <trace.pftrace> [--dump-every N] [--dump-last]\n",
            argv[0]);
    return 1;
  }
  const std::string tracePath = argv[1];
  int dumpEvery = 0;
  bool dumpLast = true;
  for (int i = 2; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--dump-every" && i + 1 < argc)
      dumpEvery = std::atoi(argv[++i]);
    else if (a == "--no-dump-last")
      dumpLast = false;
  }

  std::string blob;
  if (!readFile(tracePath, blob))
    return 1;

  perfetto::protos::Trace trace;
  if (!trace.ParseFromString(blob)) {
    fprintf(stderr, "failed to parse perfetto trace (got %zu bytes)\n",
            blob.size());
    return 1;
  }

  std::vector<const perfetto::protos::TransactionTraceEntry *> entries;
  entries.reserve(trace.packet_size());
  // Also index SF's own snapshots by vsync_id so we can diff against them.
  std::unordered_map<int64_t, const perfetto::protos::LayersSnapshotProto *>
      sfSnapshotsByVsync;
  for (int i = 0; i < trace.packet_size(); i++) {
    const auto &pkt = trace.packet(i);
    if (pkt.has_surfaceflinger_transactions()) {
      entries.push_back(&pkt.surfaceflinger_transactions());
    }
    if (pkt.has_surfaceflinger_layers_snapshot()) {
      const auto &snap = pkt.surfaceflinger_layers_snapshot();
      if (snap.has_vsync_id())
        sfSnapshotsByVsync[snap.vsync_id()] = &snap;
    }
  }

  printf("parsed %d packets, %zu transaction entries\n", trace.packet_size(),
         entries.size());
  if (entries.empty()) {
    fprintf(stderr,
            "no android.surfaceflinger.transactions packets — recapture "
            "with that data source enabled on a userdebug build\n");
    return 1;
  }

  TransactionProtoParser parser(
      std::make_unique<TransactionProtoParser::FlingerDataMapper>());
  LayerLifecycleManager lifecycleManager;
  LayerHierarchyBuilder hierarchyBuilder;
  LayerSnapshotBuilder snapshotBuilder;
  DisplayInfos displayInfos;
  android::ShadowSettings globalShadowSettings{.ambientColor = {1, 1, 1, 1}};

  for (size_t i = 0; i < entries.size(); i++) {
    const auto &entry = *entries[i];

    std::vector<std::unique_ptr<RequestedLayerState>> addedLayers;
    addedLayers.reserve(entry.added_layers_size());
    for (int j = 0; j < entry.added_layers_size(); j++) {
      LayerCreationArgs args;
      parser.fromProto(entry.added_layers(j), args);
      addedLayers.emplace_back(std::make_unique<RequestedLayerState>(args));
    }

    std::vector<TransactionState> transactions;
    transactions.reserve(entry.transactions_size());
    for (int j = 0; j < entry.transactions_size(); j++) {
      TransactionState t = parser.fromProto(entry.transactions(j));
      for (auto &rcs : t.states) {
        if (rcs.state.what & layer_state_t::eInputInfoChanged) {
          if (!rcs.state.windowInfoHandle->getInfo()->inputConfig.test(
                  android::gui::WindowInfo::InputConfig::NO_INPUT_CHANNEL)) {
            rcs.state.windowInfoHandle->editInfo()->token =
                android::sp<BBinder>::make();
          }
        }
      }
      transactions.emplace_back(std::move(t));
    }

    std::vector<std::pair<uint32_t, std::string>> destroyedHandles;
    destroyedHandles.reserve(entry.destroyed_layer_handles_size());
    for (int j = 0; j < entry.destroyed_layer_handles_size(); j++) {
      destroyedHandles.push_back({entry.destroyed_layer_handles(j), ""});
    }

    bool displayChanged = entry.displays_changed();
    if (displayChanged) {
      parser.fromProto(entry.displays(), displayInfos);
    }

    lifecycleManager.addLayers(std::move(addedLayers));
    lifecycleManager.applyTransactions(transactions,
                                       /*ignoreUnknownHandles=*/true);
    lifecycleManager.onHandlesDestroyed(destroyedHandles,
                                        /*ignoreUnknownHandles=*/true);

    hierarchyBuilder.update(lifecycleManager);

    LayerSnapshotBuilder::Args args{
        .root = hierarchyBuilder.getHierarchy(),
        .layerLifecycleManager = lifecycleManager,
        .displays = displayInfos,
        .displayChanges = displayChanged,
        .globalShadowSettings = globalShadowSettings,
        .supportsBlur = true,
        .forceFullDamage = false,
        .supportedLayerGenericMetadata = {},
        .genericLayerMetadataKeyMap = {},
    };
    snapshotBuilder.update(args);
    lifecycleManager.commitChanges();

    bool last = (i == entries.size() - 1);
    bool dumpNow =
        (dumpEvery > 0 && (i % dumpEvery) == 0) || (last && dumpLast);
    if (dumpNow) {
      size_t visibleCount = 0;
      for (const auto &snap : snapshotBuilder.getSnapshots()) {
        if (snap && snap->isVisible)
          visibleCount++;
      }
      // See if SF has its own snapshot at this vsync — if so, compare
      // layer counts. Bounds / visibility would need deeper inspection.
      auto it = sfSnapshotsByVsync.find(entry.vsync_id());
      int sfLayers = -1;
      if (it != sfSnapshotsByVsync.end()) {
        sfLayers = it->second->layers().layers_size();
      }
      printf("[%4zu/%zu] vsync=%" PRId64 " ts=%" PRId64
             "ns layers=%zu snapshots=%zu visible=%zu  sf_layers=%s\n",
             i + 1, entries.size(), entry.vsync_id(),
             entry.elapsed_realtime_nanos(),
             lifecycleManager.getLayers().size(),
             snapshotBuilder.getSnapshots().size(), visibleCount,
             sfLayers < 0 ? "-" : std::to_string(sfLayers).c_str());
    }
  }

  if (dumpLast) {
    printf("\n=== final reconstructed snapshots (%zu) ===\n",
           snapshotBuilder.getSnapshots().size());
    int shown = 0;
    for (const auto &snap : snapshotBuilder.getSnapshots()) {
      if (!snap)
        continue;
      if (!snap->isVisible)
        continue;
      printf("  %s\n", snap->getDebugString().c_str());
      if (++shown >= 15) {
        printf("  ... (more)\n");
        break;
      }
    }
  }

  return 0;
}
