// Stubs for the ui/DebugUtils.h helpers CompositionEngine uses to format log
// strings. Upstream DebugUtils.cpp is 366 lines of HAL-enum string tables;
// every caller we hit is a debug log line (DisplayColorProfile::dump,
// Output::setColorProfile, CachedSet::dump, SkiaRenderEngine::dump). A
// decimal stringification is sufficient to keep link happy and keep
// diagnostic output readable enough when it fires.
#include <ui/DebugUtils.h>

#include <string>

std::string decodeColorMode(android::ui::ColorMode mode) {
  return "ColorMode(" + std::to_string(static_cast<int>(mode)) + ")";
}

std::string decodeRenderIntent(android::ui::RenderIntent intent) {
  return "RenderIntent(" + std::to_string(static_cast<int>(intent)) + ")";
}

std::string dataspaceDetails(android_dataspace_t dataspace) {
  return "Dataspace(0x" + [&] {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%x", static_cast<unsigned>(dataspace));
    return std::string(buf);
  }() + ")";
}

std::string decodePixelFormat(int format) {
  return "PixelFormat(" + std::to_string(format) + ")";
}
