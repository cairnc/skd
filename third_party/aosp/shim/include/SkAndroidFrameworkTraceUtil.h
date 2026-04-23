// SkAndroidFrameworkTraceUtil shim. Real one is in Skia's Android build to
// cross-signal Perfetto/ATRACE; our stub is a no-op.
#pragma once

class SkAndroidFrameworkTraceUtil {
public:
  static void setEnableTracing(bool /*enable*/) {}
  static void setEnableMSAA(bool) {}
};
