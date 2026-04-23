// TransactionTracing shim — only the global TransactionTraceWriter singleton
// is referenced (enable/disable). FE's "fatal error trace" hook becomes a
// no-op in layerviewer.
#pragma once

namespace android {

class TransactionTraceWriter {
public:
  static TransactionTraceWriter &getInstance() {
    static TransactionTraceWriter w;
    return w;
  }
  void enable() {}
  void disable() {}
};

} // namespace android
