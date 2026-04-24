// layerproto/LayerProtoHeader shim — upstream also pulls in pbzero headers
// for perfetto producer-side writes. The replayer only reads, so keep just
// the .pb.h.
#pragma once
#pragma GCC system_header
#include <perfetto/trace/perfetto_trace.pb.h>
