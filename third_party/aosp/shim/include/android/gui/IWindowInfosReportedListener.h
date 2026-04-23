// Stub — LayerState.cpp reads sp<IWindowInfosReportedListener> from Parcel
// via Parcel::readStrongBinder(sp<IBinder>*), so it must derive from IBinder
// (not just RefBase) for the pointer to implicitly convert.
#pragma once
#include <binder/Binder.h>
namespace android::gui {
class IWindowInfosReportedListener : public IBinder {};
} // namespace android::gui
