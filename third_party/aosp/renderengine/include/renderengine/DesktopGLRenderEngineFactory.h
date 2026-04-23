// Public entry point for layerviewer to construct the desktop-GL Skia
// RenderEngine without pulling in the SkiaRenderEngine / AutoBackendTexture
// headers (they live under src/ and depend on Skia's private include tree,
// which isn't part of the aosp_renderengine PUBLIC include surface). The
// returned object is a concrete SkiaDesktopGLRenderEngine downcast to the
// abstract RenderEngine — same instance upstream SF constructs via the
// RenderEngine::create() dispatcher, just with our desktop-GL backend
// plugged in.
#pragma once

#include <memory>

#include <include/gpu/ganesh/gl/GrGLInterface.h>

#include "RenderEngine.h"

namespace android::renderengine {

std::unique_ptr<RenderEngine>
createDesktopGLRenderEngine(const RenderEngineCreationArgs &args,
                            sk_sp<const GrGLInterface> glInterface);

} // namespace android::renderengine
