// Copied from frameworks/native/services/surfaceflinger/LayerFE.cpp.
//
// Unchanged vs. upstream. The three not-really-there dependencies —
// SurfaceFlinger::getActiveDisplayRotationFlags(), FlagManager::
// display_protected(), GLConsumer::computeTransformMatrix — are provided by
// shim/include headers; each is a no-op / identity suitable for a single-
// display, non-protected-content, non-rotating viewer. See the header
// comment on LayerFE.h for rationale.

/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 */

#undef LOG_TAG
#define LOG_TAG "SurfaceFlinger"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <common/FlagManager.h>
#include <common/trace.h>
#include <gui/GLConsumer.h>
#include <math/vec3.h>
#include <system/window.h>
#include <ui/FenceResult.h>

#include "LayerFE.h"
#include "SurfaceFlinger.h"

namespace android {

namespace {

constexpr float defaultMaxLuminance = 1000.0;

constexpr mat4 inverseOrientation(uint32_t transform) {
  const mat4 flipH(-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1);
  const mat4 flipV(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1);
  const mat4 rot90(0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1);
  mat4 tr;
  if (transform & NATIVE_WINDOW_TRANSFORM_ROT_90)
    tr = tr * rot90;
  if (transform & NATIVE_WINDOW_TRANSFORM_FLIP_H)
    tr = tr * flipH;
  if (transform & NATIVE_WINDOW_TRANSFORM_FLIP_V)
    tr = tr * flipV;
  return inverse(tr);
}

FloatRect reduce(const FloatRect &win, const Region &exclude) {
  if (CC_LIKELY(exclude.isEmpty())) {
    return win;
  }
  // Convert through Rect (by rounding) for lack of FloatRegion
  return Region(Rect{win}).subtract(exclude).getBounds().toFloatRect();
}

void getDrawingTransformMatrix(
    const std::shared_ptr<renderengine::ExternalTexture> &buffer,
    Rect bufferCrop, uint32_t bufferTransform, bool filteringEnabled,
    float outMatrix[16]) {
  if (!buffer) {
    ALOGE("Buffer should not be null!");
    return;
  }
  GLConsumer::computeTransformMatrix(
      outMatrix, static_cast<float>(buffer->getWidth()),
      static_cast<float>(buffer->getHeight()), buffer->getPixelFormat(),
      bufferCrop, bufferTransform, filteringEnabled);
}

} // namespace

LayerFE::LayerFE(const std::string &name) : mName(name) {}

LayerFE::~LayerFE() {
  if (mReleaseFencePromiseStatus == ReleaseFencePromiseStatus::INITIALIZED) {
    setReleaseFence(Fence::NO_FENCE);
  }
}

const compositionengine::LayerFECompositionState *
LayerFE::getCompositionState() const {
  return mSnapshot.get();
}

bool LayerFE::onPreComposition(bool) { return mSnapshot->hasReadyFrame; }

std::optional<compositionengine::LayerFE::LayerSettings>
LayerFE::prepareClientComposition(
    compositionengine::LayerFE::ClientCompositionTargetSettings &targetSettings)
    const {
  std::optional<compositionengine::LayerFE::LayerSettings> layerSettings =
      prepareClientCompositionInternal(targetSettings);
  if (!layerSettings) {
    return {};
  }
  if (targetSettings.clearContent) {
    prepareClearClientComposition(*layerSettings, false /* blackout */);
    return layerSettings;
  }
  prepareShadowClientComposition(*layerSettings, targetSettings.viewport);
  return layerSettings;
}

std::optional<compositionengine::LayerFE::LayerSettings>
LayerFE::prepareClientCompositionInternal(
    compositionengine::LayerFE::ClientCompositionTargetSettings &targetSettings)
    const {
  SFTRACE_CALL();
  compositionengine::LayerFE::LayerSettings layerSettings;
  layerSettings.geometry.boundaries =
      reduce(mSnapshot->geomLayerBounds, mSnapshot->transparentRegionHint);
  layerSettings.geometry.positionTransform =
      mSnapshot->geomLayerTransform.asMatrix4();

  const bool drawContent =
      targetSettings.realContentIsVisible || targetSettings.clearContent;
  layerSettings.skipContentDraw = !drawContent;

  if (!mSnapshot->colorTransformIsIdentity) {
    layerSettings.colorTransform = mSnapshot->colorTransform;
  }

  const auto &roundedCornerState = mSnapshot->roundedCorner;
  layerSettings.geometry.roundedCornersRadius = roundedCornerState.radius;
  layerSettings.geometry.roundedCornersCrop = roundedCornerState.cropRect;

  layerSettings.alpha = mSnapshot->alpha;
  layerSettings.sourceDataspace = mSnapshot->dataspace;

  if (targetSettings.treat170mAsSrgb &&
      (layerSettings.sourceDataspace & HAL_DATASPACE_TRANSFER_MASK) ==
          HAL_DATASPACE_TRANSFER_SMPTE_170M) {
    layerSettings.sourceDataspace = static_cast<ui::Dataspace>(
        (layerSettings.sourceDataspace & HAL_DATASPACE_STANDARD_MASK) |
        (layerSettings.sourceDataspace & HAL_DATASPACE_RANGE_MASK) |
        HAL_DATASPACE_TRANSFER_SRGB);
  }

  layerSettings.whitePointNits = targetSettings.whitePointNits;

  switch (targetSettings.blurSetting) {
  case LayerFE::ClientCompositionTargetSettings::BlurSetting::Enabled:
    layerSettings.backgroundBlurRadius = mSnapshot->backgroundBlurRadius;
    layerSettings.blurRegions = mSnapshot->blurRegions;
    layerSettings.blurRegionTransform =
        mSnapshot->localTransformInverse.asMatrix4();
    break;
  case LayerFE::ClientCompositionTargetSettings::BlurSetting::
      BackgroundBlurOnly:
    layerSettings.backgroundBlurRadius = mSnapshot->backgroundBlurRadius;
    break;
  case LayerFE::ClientCompositionTargetSettings::BlurSetting::BlurRegionsOnly:
    layerSettings.blurRegions = mSnapshot->blurRegions;
    layerSettings.blurRegionTransform =
        mSnapshot->localTransformInverse.asMatrix4();
    break;
  case LayerFE::ClientCompositionTargetSettings::BlurSetting::Disabled:
  default:
    break;
  }

  layerSettings.stretchEffect = mSnapshot->stretchEffect;
  layerSettings.edgeExtensionEffect = mSnapshot->edgeExtensionEffect;
  layerSettings.name = mSnapshot->name;
  layerSettings.luts = mSnapshot->luts;

  if (hasEffect() && !hasBufferOrSidebandStream()) {
    prepareEffectsClientComposition(layerSettings, targetSettings);
    return layerSettings;
  }

  prepareBufferStateClientComposition(layerSettings, targetSettings);
  return layerSettings;
}

void LayerFE::prepareClearClientComposition(
    LayerFE::LayerSettings &layerSettings, bool blackout) const {
  layerSettings.source.buffer.buffer = nullptr;
  layerSettings.source.solidColor = half3(0.0f, 0.0f, 0.0f);
  layerSettings.disableBlending = true;
  layerSettings.bufferId = 0;
  layerSettings.frameNumber = 0;
  layerSettings.alpha = blackout ? 1.0f : 0.0f;
  layerSettings.name = mSnapshot->name;
}

void LayerFE::prepareEffectsClientComposition(
    compositionengine::LayerFE::LayerSettings &layerSettings,
    compositionengine::LayerFE::ClientCompositionTargetSettings &targetSettings)
    const {
  if (targetSettings.realContentIsVisible && fillsColor()) {
    layerSettings.source.solidColor = mSnapshot->color.rgb;
  } else if (hasBlur() || drawShadows()) {
    layerSettings.skipContentDraw = true;
  }
}

void LayerFE::prepareBufferStateClientComposition(
    compositionengine::LayerFE::LayerSettings &layerSettings,
    compositionengine::LayerFE::ClientCompositionTargetSettings &targetSettings)
    const {
  SFTRACE_CALL();
  if (CC_UNLIKELY(!mSnapshot->externalTexture)) {
    return;
  }

  bool blackOutLayer;
  if (FlagManager::getInstance().display_protected()) {
    blackOutLayer =
        (mSnapshot->hasProtectedContent && !targetSettings.isProtected) ||
        (mSnapshot->isSecure && !targetSettings.isSecure);
  } else {
    blackOutLayer =
        (mSnapshot->hasProtectedContent && !targetSettings.isProtected) ||
        ((mSnapshot->isSecure || mSnapshot->hasProtectedContent) &&
         !targetSettings.isSecure);
  }

  const bool bufferCanBeUsedAsHwTexture =
      mSnapshot->externalTexture->getUsage() & GraphicBuffer::USAGE_HW_TEXTURE;
  if (blackOutLayer || !bufferCanBeUsedAsHwTexture) {
    ALOGE_IF(!bufferCanBeUsedAsHwTexture,
             "%s is blacked out as buffer is not gpu readable",
             mSnapshot->name.c_str());
    prepareClearClientComposition(layerSettings, true /* blackout */);
    return;
  }

  layerSettings.source.buffer.buffer = mSnapshot->externalTexture;
  layerSettings.source.buffer.isOpaque = mSnapshot->contentOpaque;
  layerSettings.source.buffer.fence = mSnapshot->acquireFence;
  layerSettings.source.buffer.usePremultipliedAlpha =
      mSnapshot->premultipliedAlpha;

  bool hasSmpte2086 =
      mSnapshot->hdrMetadata.validTypes & HdrMetadata::SMPTE2086;
  bool hasCta861_3 = mSnapshot->hdrMetadata.validTypes & HdrMetadata::CTA861_3;
  float maxLuminance = 0.f;
  if (hasSmpte2086 && hasCta861_3) {
    maxLuminance =
        std::min(mSnapshot->hdrMetadata.smpte2086.maxLuminance,
                 mSnapshot->hdrMetadata.cta8613.maxContentLightLevel);
  } else if (hasSmpte2086) {
    maxLuminance = mSnapshot->hdrMetadata.smpte2086.maxLuminance;
  } else if (hasCta861_3) {
    maxLuminance = mSnapshot->hdrMetadata.cta8613.maxContentLightLevel;
  } else {
    switch (layerSettings.sourceDataspace & HAL_DATASPACE_TRANSFER_MASK) {
    case HAL_DATASPACE_TRANSFER_ST2084:
    case HAL_DATASPACE_TRANSFER_HLG:
      maxLuminance = defaultMaxLuminance;
      break;
    }
  }
  layerSettings.source.buffer.maxLuminanceNits = maxLuminance;
  layerSettings.frameNumber = mSnapshot->frameNumber;
  layerSettings.bufferId = mSnapshot->externalTexture->getId();

  const bool useFiltering =
      targetSettings.needsFiltering ||
      mSnapshot->geomLayerTransform.needsBilinearFiltering();

  float textureMatrix[16];
  getDrawingTransformMatrix(
      layerSettings.source.buffer.buffer, mSnapshot->geomContentCrop,
      mSnapshot->geomBufferTransform, useFiltering, textureMatrix);

  if (mSnapshot->geomBufferUsesDisplayInverseTransform) {
    uint32_t transform = SurfaceFlinger::getActiveDisplayRotationFlags();
    mat4 tr = inverseOrientation(transform);
    const auto parentTransform = mSnapshot->parentTransform;
    tr = tr * inverseOrientation(parentTransform.getOrientation());
    const mat4 texTransform(mat4(static_cast<const float *>(textureMatrix)) *
                            tr);
    std::memcpy(textureMatrix, texTransform.asArray(), sizeof(textureMatrix));
  }

  const Rect win{layerSettings.geometry.boundaries};
  float bufferWidth = static_cast<float>(mSnapshot->bufferSize.getWidth());
  float bufferHeight = static_cast<float>(mSnapshot->bufferSize.getHeight());
  if (!mSnapshot->bufferSize.isValid()) {
    bufferWidth = float(win.right) - float(win.left);
    bufferHeight = float(win.bottom) - float(win.top);
  }

  const float scaleHeight = (float(win.bottom) - float(win.top)) / bufferHeight;
  const float scaleWidth = (float(win.right) - float(win.left)) / bufferWidth;
  const float translateY = float(win.top) / bufferHeight;
  const float translateX = float(win.left) / bufferWidth;

  mat4 tr = mat4::translate(vec4(.5f, .5f, 0.f, 1.f)) *
            mat4::scale(vec4(1.f, -1.f, 1.f, 1.f)) *
            mat4::translate(vec4(-.5f, -.5f, 0.f, 1.f)) *
            mat4::translate(vec4(translateX, translateY, 0.f, 1.f)) *
            mat4::scale(vec4(scaleWidth, scaleHeight, 1.0f, 1.0f));

  layerSettings.source.buffer.useTextureFiltering = useFiltering;
  layerSettings.source.buffer.textureTransform =
      mat4(static_cast<const float *>(textureMatrix)) * tr;
}

void LayerFE::prepareShadowClientComposition(LayerFE::LayerSettings &caster,
                                             const Rect &layerStackRect) const {
  ShadowSettings state = mSnapshot->shadowSettings;
  if (state.length <= 0.f ||
      (state.ambientColor.a <= 0.f && state.spotColor.a <= 0.f)) {
    return;
  }
  state.lightPos.x = (static_cast<float>(layerStackRect.width()) / 2.f) -
                     mSnapshot->transformedBounds.left;
  state.lightPos.y -= mSnapshot->transformedBounds.top;
  caster.shadow = state;
}

void LayerFE::onPictureProfileCommitted() {
  mCompositionResult.wasPictureProfileCommitted = true;
  mCompositionResult.pictureProfileHandle = mSnapshot->pictureProfileHandle;
}

CompositionResult LayerFE::stealCompositionResult() {
  CompositionResult result;
  std::swap(mCompositionResult, result);
  return result;
}

const char *LayerFE::getDebugName() const { return mName.c_str(); }

const LayerMetadata *LayerFE::getMetadata() const {
  return &mSnapshot->layerMetadata;
}

const LayerMetadata *LayerFE::getRelativeMetadata() const {
  return &mSnapshot->relativeLayerMetadata;
}

int32_t LayerFE::getSequence() const {
  return static_cast<int32_t>(mSnapshot->uniqueSequence);
}

bool LayerFE::hasRoundedCorners() const {
  return mSnapshot->roundedCorner.hasRoundedCorners();
}

void LayerFE::setWasClientComposed(const sp<Fence> &fence) {
  mCompositionResult.lastClientCompositionFence = fence;
}

bool LayerFE::hasBufferOrSidebandStream() const {
  return mSnapshot->externalTexture || mSnapshot->sidebandStream;
}

bool LayerFE::fillsColor() const {
  return mSnapshot->color.r >= 0.0_hf && mSnapshot->color.g >= 0.0_hf &&
         mSnapshot->color.b >= 0.0_hf;
}

bool LayerFE::hasBlur() const {
  return mSnapshot->backgroundBlurRadius > 0 ||
         mSnapshot->blurRegions.size() > 0;
}

bool LayerFE::drawShadows() const {
  return mSnapshot->shadowSettings.length > 0.f &&
         (mSnapshot->shadowSettings.ambientColor.a > 0 ||
          mSnapshot->shadowSettings.spotColor.a > 0);
}

const sp<GraphicBuffer> LayerFE::getBuffer() const {
  return mSnapshot->externalTexture ? mSnapshot->externalTexture->getBuffer()
                                    : nullptr;
}

void LayerFE::setReleaseFence(const FenceResult &releaseFence) {
  if (mReleaseFencePromiseStatus == ReleaseFencePromiseStatus::FULFILLED) {
    return;
  }
  mReleaseFence.set_value(releaseFence);
  mReleaseFencePromiseStatus = ReleaseFencePromiseStatus::FULFILLED;
}

ftl::Future<FenceResult> LayerFE::createReleaseFenceFuture() {
  if (mReleaseFencePromiseStatus == ReleaseFencePromiseStatus::INITIALIZED) {
    LOG_ALWAYS_FATAL(
        "Attempting to create a new promise while one is still unfulfilled.");
  }
  mReleaseFence = std::promise<FenceResult>();
  mReleaseFencePromiseStatus = ReleaseFencePromiseStatus::INITIALIZED;
  return mReleaseFence.get_future();
}

LayerFE::ReleaseFencePromiseStatus LayerFE::getReleaseFencePromiseStatus() {
  return mReleaseFencePromiseStatus;
}

} // namespace android
