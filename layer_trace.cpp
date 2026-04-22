#include "layer_trace.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>

#include "protos/perfetto/trace/android/surfaceflinger_common.pb.h"
#include "protos/perfetto/trace/android/surfaceflinger_layers.pb.h"

namespace layerviewer
{

int LayerTrace::snapshot_count() const
{
    return proto ? proto->entry_size() : 0;
}

const perfetto::protos::LayersSnapshotProto &LayerTrace::snapshot(int i) const
{
    return proto->entry(i);
}

std::unique_ptr<LayerTrace> LoadLayerTrace(const std::string &path)
{
    auto trace = std::make_unique<LayerTrace>();
    trace->path = path;

    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::ostringstream msg;
        msg << "open failed: " << std::strerror(errno);
        trace->error = msg.str();
        return trace;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    std::string data = buf.str();

    trace->proto = std::make_unique<perfetto::protos::LayersTraceFileProto>();
    if (!trace->proto->ParseFromString(data))
    {
        trace->proto.reset();
        trace->error = "ParseFromString failed; not a valid LayersTraceFile";
    }
    return trace;
}

} // namespace layerviewer
