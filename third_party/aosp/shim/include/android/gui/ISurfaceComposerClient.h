// android/gui/ISurfaceComposerClient shim — upstream pulls this AIDL header
// into LayerState.cpp to get access to gui::ISurfaceComposerClient flag
// constants. The constants already live in our <gui/ISurfaceComposerClient.h>
// shim, so just delegate.
#pragma once
#include <gui/ISurfaceComposer.h>
