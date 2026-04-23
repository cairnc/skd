// HdrMetadata shim — LayerFECompositionState has an HdrMetadata member.
// Also pulls in utils/NativeHandle.h — LayerFECompositionState transitively
// expects NativeHandle through this include (keeping CE source unchanged).
#pragma once
#include <cstdint>
#include <utils/NativeHandle.h>

namespace android {

struct HdrMetadata {
  enum Type : uint32_t {
    SMPTE2086 = 1 << 0,
    CTA861_3 = 1 << 1,
    HDR10PLUS = 1 << 2,
  };
  uint32_t validTypes = 0;
  struct Smpte2086 {
    float displayPrimaryRed[2] = {0, 0};
    float displayPrimaryGreen[2] = {0, 0};
    float displayPrimaryBlue[2] = {0, 0};
    float whitePoint[2] = {0, 0};
    float maxLuminance = 0;
    float minLuminance = 0;
  } smpte2086;
  struct Cta861_3 {
    float maxContentLightLevel = 0;
    float maxFrameAverageLightLevel = 0;
  } cta8613;
  std::vector<uint8_t> hdr10plus;

  // Conservative equality — enough for layer_state_t::diff/merge to trigger
  // on hdr10plus payload changes; fine-grained Smpte2086/Cta861_3 comparison
  // is skipped since nothing the replayer reads mutates them at that
  // granularity.
  bool operator==(const HdrMetadata &o) const {
    return validTypes == o.validTypes && hdr10plus == o.hdr10plus;
  }
};

} // namespace android
