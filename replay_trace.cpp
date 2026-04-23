// replay_trace — stage 0 of the layerviewer pipeline.
//
// Reads a perfetto .pftrace captured with the
// android.surfaceflinger.transactions data source and dumps what's inside each
// TransactionTraceEntry: timestamps, vsync IDs, added/destroyed-layer counts,
// and transaction counts. This is the "can we even open the file" smoke test
// before we wire up the FE pipeline (LayerLifecycleManager →
// LayerHierarchyBuilder → LayerSnapshotBuilder).

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include <perfetto/trace/trace.pb.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <trace.pftrace> [--dump-layers]\n", argv[0]);
    return 1;
  }
  const std::string tracePath = argv[1];
  bool dumpLayers = false;
  for (int i = 2; i < argc; i++) {
    if (std::string(argv[i]) == "--dump-layers")
      dumpLayers = true;
  }

  std::ifstream in(tracePath, std::ios::binary);
  if (!in) {
    fprintf(stderr, "failed to open %s\n", tracePath.c_str());
    return 1;
  }
  std::stringstream ss;
  ss << in.rdbuf();
  std::string blob = ss.str();

  perfetto::protos::Trace trace;
  if (!trace.ParseFromString(blob)) {
    fprintf(stderr, "failed to parse perfetto trace (got %zu bytes)\n",
            blob.size());
    return 1;
  }

  int txnPackets = 0;
  int layerPackets = 0;
  int64_t totalTransactions = 0;
  int64_t totalAddedLayers = 0;
  int64_t totalDestroyedLayers = 0;
  int64_t totalDestroyedHandles = 0;
  int displayChangeEntries = 0;
  int firstLayerSnapshotIndex = -1;

  for (int i = 0; i < trace.packet_size(); i++) {
    const auto &pkt = trace.packet(i);
    if (pkt.has_surfaceflinger_layers_snapshot()) {
      layerPackets++;
      if (firstLayerSnapshotIndex < 0)
        firstLayerSnapshotIndex = i;
    }
    if (!pkt.has_surfaceflinger_transactions())
      continue;
    txnPackets++;
    const auto &entry = pkt.surfaceflinger_transactions();

    totalTransactions += entry.transactions_size();
    totalAddedLayers += entry.added_layers_size();
    totalDestroyedLayers += entry.destroyed_layers_size();
    totalDestroyedHandles += entry.destroyed_layer_handles_size();
    if (entry.displays_changed())
      displayChangeEntries++;

    printf("[%4d] ts=%" PRId64 "ns vsync=%" PRId64
           " +layers=%d -layers=%d -handles=%d txns=%d displays_changed=%d\n",
           txnPackets, entry.elapsed_realtime_nanos(), entry.vsync_id(),
           entry.added_layers_size(), entry.destroyed_layers_size(),
           entry.destroyed_layer_handles_size(), entry.transactions_size(),
           entry.displays_changed() ? 1 : 0);

    if (dumpLayers) {
      for (int j = 0; j < entry.added_layers_size(); j++) {
        const auto &la = entry.added_layers(j);
        printf("         +layer id=%u parentId=%u name=%s\n", la.layer_id(),
               la.parent_id(), la.name().c_str());
      }
      for (int j = 0; j < entry.destroyed_layers_size(); j++) {
        printf("         -layer id=%u\n", entry.destroyed_layers(j));
      }
    }
  }

  printf("\n=== summary ===\n");
  printf("total packets:                 %d\n", trace.packet_size());
  printf("transaction entries:           %d\n", txnPackets);
  printf("layer-snapshot packets:        %d\n", layerPackets);
  printf("total transactions applied:    %" PRId64 "\n", totalTransactions);
  printf("total layers added:            %" PRId64 "\n", totalAddedLayers);
  printf("total layers destroyed:        %" PRId64 "\n", totalDestroyedLayers);
  printf("total handles destroyed:       %" PRId64 "\n", totalDestroyedHandles);
  printf("entries with display changes:  %d\n", displayChangeEntries);

  // If there are no transactions but there ARE layer snapshots, the trace
  // was captured with the wrong data source set — but we can still show
  // what the snapshots look like as a sanity check.
  if (layerPackets > 0) {
    printf("\n=== sample layer snapshot (packet index %d) ===\n",
           firstLayerSnapshotIndex);
    const auto &snap =
        trace.packet(firstLayerSnapshotIndex).surfaceflinger_layers_snapshot();
    printf("vsync_id=%" PRId64 " elapsed=%" PRId64
           "ns where=%s layers=%d displays=%d\n",
           snap.vsync_id(), snap.elapsed_realtime_nanos(), snap.where().c_str(),
           snap.layers().layers_size(), snap.displays_size());
    int limit = std::min(10, snap.layers().layers_size());
    for (int i = 0; i < limit; i++) {
      const auto &l = snap.layers().layers(i);
      printf("  layer id=%d name=%s parent=%d z=%d w=%u h=%u\n", l.id(),
             l.name().c_str(), l.parent(), l.z(), l.size().w(), l.size().h());
    }
    if (snap.layers().layers_size() > limit) {
      printf("  ... %d more\n", snap.layers().layers_size() - limit);
    }
  }

  return 0;
}
