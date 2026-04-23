// libui supplement — upstream defines these as class statics in a separate
// .cpp; we follow the same pattern since inline-header definitions would
// trip ODR for sp<> globals.

#include <ui/Fence.h>
#include <ui/PictureProfileHandle.h>

namespace android {

const sp<Fence> Fence::NO_FENCE = sp<Fence>::make();

const PictureProfileHandle PictureProfileHandle::NONE{-1};

std::string toString(const PictureProfileHandle &handle) {
  return "PictureProfileHandle(" + std::to_string(handle.getId()) + ")";
}

} // namespace android
