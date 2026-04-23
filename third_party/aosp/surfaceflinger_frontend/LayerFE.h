// Copied from frameworks/native/services/surfaceflinger/LayerFE.h.
//
// Only change vs. upstream: upstream lives next to the SurfaceFlinger class
// and its LayerSnapshot include path is "FrontEnd/LayerSnapshot.h" relative
// to that directory; here we pull the snapshot from surfaceflinger_frontend
// via the same "FrontEnd/LayerSnapshot.h" path, which works because
// surfaceflinger_frontend's CMake adds its own directory as an include root.
// Runtime behaviour is identical — LayerFE wraps a LayerSnapshot, exposes it
// to CompositionEngine as a LayerFECompositionState*, and converts a
// snapshot into renderengine::LayerSettings on demand via
// prepareClientComposition(). Three pieces it depends on that are not real
// in our tree — SurfaceFlinger::getActiveDisplayRotationFlags() (stubbed to
// 0), FlagManager::display_protected() (stubbed to false), and
// GLConsumer::computeTransformMatrix (ported inline) — are all provided by
// the aosp_platform shims so this file compiles unchanged.

/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 */

#pragma once

#include "FrontEnd/LayerSnapshot.h"
#include "compositionengine/LayerFE.h"
#include "compositionengine/LayerFECompositionState.h"
#include "renderengine/LayerSettings.h"
#include <android/gui/CachingHint.h>
#include <ftl/future.h>
#include <gui/LayerMetadata.h>
#include <ui/LayerStack.h>
#include <ui/PictureProfileHandle.h>

namespace android {

struct CompositionResult {
  sp<Fence> lastClientCompositionFence = nullptr;
  bool wasPictureProfileCommitted = false;
  PictureProfileHandle pictureProfileHandle = PictureProfileHandle::NONE;
};

class LayerFE : public virtual RefBase,
                public virtual compositionengine::LayerFE {
public:
  LayerFE(const std::string &name);
  virtual ~LayerFE();

  const compositionengine::LayerFECompositionState *
  getCompositionState() const override;
  bool onPreComposition(bool updatingOutputGeometryThisFrame) override;
  const char *getDebugName() const override;
  int32_t getSequence() const override;
  bool hasRoundedCorners() const override;
  void setWasClientComposed(const sp<Fence> &) override;
  const gui::LayerMetadata *getMetadata() const override;
  const gui::LayerMetadata *getRelativeMetadata() const override;
  std::optional<compositionengine::LayerFE::LayerSettings>
  prepareClientComposition(
      compositionengine::LayerFE::ClientCompositionTargetSettings &) const;
  CompositionResult stealCompositionResult();
  ftl::Future<FenceResult> createReleaseFenceFuture() override;
  void setReleaseFence(const FenceResult &releaseFence) override;
  LayerFE::ReleaseFencePromiseStatus getReleaseFencePromiseStatus() override;
  void onPictureProfileCommitted() override;

  std::unique_ptr<surfaceflinger::frontend::LayerSnapshot> mSnapshot;

private:
  std::optional<compositionengine::LayerFE::LayerSettings>
  prepareClientCompositionInternal(
      compositionengine::LayerFE::ClientCompositionTargetSettings &) const;
  void prepareClearClientComposition(LayerFE::LayerSettings &,
                                     bool blackout) const;
  void prepareShadowClientComposition(LayerFE::LayerSettings &caster,
                                      const Rect &layerStackRect) const;
  void prepareBufferStateClientComposition(
      compositionengine::LayerFE::LayerSettings &,
      compositionengine::LayerFE::ClientCompositionTargetSettings &) const;
  void prepareEffectsClientComposition(
      compositionengine::LayerFE::LayerSettings &,
      compositionengine::LayerFE::ClientCompositionTargetSettings &) const;
  bool hasEffect() const { return fillsColor() || drawShadows() || hasBlur(); }
  bool hasBufferOrSidebandStream() const;
  bool fillsColor() const;
  bool hasBlur() const;
  bool drawShadows() const;
  const sp<GraphicBuffer> getBuffer() const;

  CompositionResult mCompositionResult;
  std::string mName;
  std::promise<FenceResult> mReleaseFence;
  ReleaseFencePromiseStatus mReleaseFencePromiseStatus =
      ReleaseFencePromiseStatus::UNINITIALIZED;
};

} // namespace android
