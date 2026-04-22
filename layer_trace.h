#pragma once

#include <memory>
#include <string>
#include <vector>

namespace perfetto::protos
{
class LayersTraceFileProto;
class LayersSnapshotProto;
class LayerProto;
} // namespace perfetto::protos

namespace layerviewer
{

// Thin wrapper owning a parsed LayersTraceFileProto; hides protobuf headers
// from the rest of the app.
struct LayerTrace
{
    std::unique_ptr<perfetto::protos::LayersTraceFileProto> proto;
    std::string path;
    std::string error;

    int snapshot_count() const;
    const perfetto::protos::LayersSnapshotProto &snapshot(int i) const;
};

std::unique_ptr<LayerTrace> LoadLayerTrace(const std::string &path);

} // namespace layerviewer
