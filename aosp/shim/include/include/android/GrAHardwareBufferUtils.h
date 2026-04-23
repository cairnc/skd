// GrAHardwareBufferUtils shim. Upstream Skia defines this only on Android
// builds (where it bridges AHardwareBuffer ↔ Ganesh textures). Outside
// Android we never have real AHBs, so expose only the callback typedefs and
// the format-→-color-type mapping that SF port code *declares* but doesn't
// exercise at runtime.
#pragma once

#include <include/core/SkColorType.h>
#include <include/core/SkImageInfo.h>
#include <include/gpu/ganesh/GrBackendSurface.h>

#include <android/hardware_buffer.h>
#include <cstdint>

class GrDirectContext;

namespace GrAHardwareBufferUtils {

using TexImageCtx = void *;
using DeleteImageProc = void (*)(void *);
using UpdateImageProc = void (*)(void *, GrDirectContext *);

inline SkColorType GetSkColorTypeFromBufferFormat(uint32_t format) {
  // Mirror the AOSP-side mapping, minus HDR formats we don't use.
  switch (format) {
  case 1: /* RGBA_8888 */
    return kRGBA_8888_SkColorType;
  case 2: /* RGBX_8888 */
    return kRGB_888x_SkColorType;
  case 3: /* RGB_888   */
    return kRGB_888x_SkColorType;
  case 4: /* RGB_565   */
    return kRGB_565_SkColorType;
  case 5: /* BGRA_8888 */
    return kBGRA_8888_SkColorType;
  case 0x16: /* RGBA_FP16 */
    return kRGBA_F16_SkColorType;
  case 0x2b: /* RGBA_1010102 */
    return kRGBA_1010102_SkColorType;
  default:
    return kRGBA_8888_SkColorType;
  }
}

inline GrBackendFormat GetGLBackendFormat(GrDirectContext *, uint32_t,
                                          bool /*requireKnownFormat*/) {
  // Real impl queries GL_TEXTURE_EXTERNAL / GL_TEXTURE_2D formats. We never
  // materialize real AHBs — return an invalid format; callers guard on
  // validity or skip in our paths.
  return {};
}

inline GrBackendTexture
MakeGLBackendTexture(GrDirectContext *, AHardwareBuffer *, int /*width*/,
                     int /*height*/, DeleteImageProc *outDelete,
                     UpdateImageProc *outUpdate, TexImageCtx *outCtx,
                     bool /*isProtectedContent*/, const GrBackendFormat &,
                     bool /*isRenderable*/) {
  if (outDelete)
    *outDelete = nullptr;
  if (outUpdate)
    *outUpdate = nullptr;
  if (outCtx)
    *outCtx = nullptr;
  return {};
}

// Vulkan variants declared to satisfy call sites in Vk branches (never taken
// in our desktop-GL port).
inline GrBackendFormat
GetVulkanBackendFormat(GrDirectContext *, AHardwareBuffer *, uint32_t, bool) {
  return {};
}
inline GrBackendTexture
MakeVulkanBackendTexture(GrDirectContext *, AHardwareBuffer *, int, int,
                         DeleteImageProc *, UpdateImageProc *, TexImageCtx *,
                         bool, const GrBackendFormat &, bool) {
  return {};
}

} // namespace GrAHardwareBufferUtils
