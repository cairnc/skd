// layerproto/TransactionProto shim — upstream also pulls in pbzero headers
// for perfetto producer-side writes. The replayer only reads, so keep just
// the .pb.h.
#pragma once
#pragma GCC system_header
#include <perfetto/trace/android/surfaceflinger_transactions.pb.h>
