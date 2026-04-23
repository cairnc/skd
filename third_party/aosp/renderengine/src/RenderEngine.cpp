// Subset of frameworks/native/libs/renderengine/RenderEngine.cpp: just the
// non-pure base-class methods that every concrete RE subclass needs linked.
// The upstream `RenderEngine::create()` dispatcher (GL vs Vulkan, Ganesh vs
// Graphite, threaded vs not) is not ported — layerviewer instantiates
// SkiaDesktopGLRenderEngine directly via createDesktopGLRenderEngine().

/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 */

#include <renderengine/RenderEngine.h>

#include <algorithm>

#include <hardware/gralloc.h>
#include <log/log.h>
#include <ui/GraphicTypes.h>

#include "renderengine/ExternalTexture.h"

namespace android {
namespace renderengine {

RenderEngine::~RenderEngine() = default;

void RenderEngine::validateInputBufferUsage(const sp<GraphicBuffer> &buffer) {
  LOG_ALWAYS_FATAL_IF(!(buffer->getUsage() & GraphicBuffer::USAGE_HW_TEXTURE),
                      "input buffer not gpu readable");
}

void RenderEngine::validateOutputBufferUsage(const sp<GraphicBuffer> &buffer) {
  LOG_ALWAYS_FATAL_IF(!(buffer->getUsage() & GraphicBuffer::USAGE_HW_RENDER),
                      "output buffer not gpu writeable");
}

ftl::Future<FenceResult>
RenderEngine::drawLayers(const DisplaySettings &display,
                         const std::vector<LayerSettings> &layers,
                         const std::shared_ptr<ExternalTexture> &buffer,
                         base::unique_fd &&bufferFence) {
  const auto resultPromise = std::make_shared<std::promise<FenceResult>>();
  std::future<FenceResult> resultFuture = resultPromise->get_future();

  updateProtectedContext(layers, {buffer.get()});
  drawLayersInternal(std::move(resultPromise), display, layers, buffer,
                     std::move(bufferFence));

  return resultFuture;
}

ftl::Future<FenceResult> RenderEngine::drawGainmap(
    const std::shared_ptr<ExternalTexture> &sdr, base::borrowed_fd &&sdrFence,
    const std::shared_ptr<ExternalTexture> &hdr, base::borrowed_fd &&hdrFence,
    float hdrSdrRatio, ui::Dataspace dataspace,
    const std::shared_ptr<ExternalTexture> &gainmap) {
  const auto resultPromise = std::make_shared<std::promise<FenceResult>>();
  std::future<FenceResult> resultFuture = resultPromise->get_future();

  updateProtectedContext({}, {sdr.get(), hdr.get(), gainmap.get()});
  drawGainmapInternal(std::move(resultPromise), sdr, std::move(sdrFence), hdr,
                      std::move(hdrFence), hdrSdrRatio, dataspace, gainmap);

  return resultFuture;
}

void RenderEngine::updateProtectedContext(
    const std::vector<LayerSettings> &layers,
    std::vector<const ExternalTexture *> buffers) {
  const bool needsProtectedContext =
      std::any_of(layers.begin(), layers.end(),
                  [](const LayerSettings &layer) {
                    const std::shared_ptr<ExternalTexture> &buffer =
                        layer.source.buffer.buffer;
                    return buffer &&
                           (buffer->getUsage() & GRALLOC_USAGE_PROTECTED);
                  }) ||
      std::any_of(
          buffers.begin(), buffers.end(), [](const ExternalTexture *buffer) {
            return buffer && (buffer->getUsage() & GRALLOC_USAGE_PROTECTED);
          });
  useProtectedContext(needsProtectedContext);
}

} // namespace renderengine
} // namespace android
