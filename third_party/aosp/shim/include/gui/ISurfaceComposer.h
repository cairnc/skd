// ISurfaceComposer shim — FE references ISurfaceComposer::eAnimation and
// ISurfaceComposerClient::e* flags. Real upstream also surfaces gui::
// FrameTimelineInfo + utils::Vector from this header, so mirror those.
#pragma once
#include <android/gui/FrameTimelineInfo.h>
#include <binder/Binder.h>
#include <cstdint>
#include <utils/Vector.h>
namespace android {
class ISurfaceComposer : public IBinder {
public:
  enum : uint32_t {
    eAnimation = 0x1,
    eSynchronous = 0x2,
    eExplicitEarlyWakeupStart = 0x4,
    eExplicitEarlyWakeupEnd = 0x8,
    eOneWay = 0x10,
  };
};
} // namespace android

// LayerState.h references gui::ISurfaceComposerClient::e* constants; mirror
// the same flag set under the gui namespace.
namespace android::gui {
class ISurfaceComposerClient : public IBinder {
public:
  // Values sync'd to upstream aidl/android/gui/ISurfaceComposerClient.aidl.
  // Using the wrong bits corrupts layer-creation flags — for example
  // eNoColorFill being wrong makes all container layers "fill with black" and
  // register as contentOpaque in the reconstructed snapshot.
  enum : uint32_t {
    eHidden = 0x00000004,
    eDestroyBackbuffer = 0x00000020,
    eSkipScreenshot = 0x00000040,
    eSecure = 0x00000080,
    eNonPremultiplied = 0x00000100,
    eOpaque = 0x00000400,
    eProtectedByApp = 0x00000800,
    eProtectedByDRM = 0x00001000,
    eCursorWindow = 0x00002000,
    eNoColorFill = 0x00004000,
    eFXSurfaceBufferQueue = 0x00000000,
    eFXSurfaceEffect = 0x00020000,
    eFXSurfaceBufferState = 0x00040000,
    eFXSurfaceContainer = 0x00080000,
    eFXSurfaceMask = 0x000F0000,
  };
};
} // namespace android::gui
// Keep the android::ISurfaceComposerClient alias for code that still uses
// the unqualified form.
namespace android {
using ISurfaceComposerClient = ::android::gui::ISurfaceComposerClient;
using gui::FrameTimelineInfo;
} // namespace android
// Upstream LayerState.h uses SpHash<T> unqualified in android:: scope.
// Drag the gui:: one up so that works.
#include <gui/SpHash.h>
namespace android {
using gui::SpHash;
}
