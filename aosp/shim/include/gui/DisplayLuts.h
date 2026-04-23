// DisplayLuts shim. We don't do binder IPC, so drop the Parcelable-ness and
// keep just the data LayerSettings touches (a couple of optional 3D LUTs
// and sampling keys).
#pragma once

#include <android-base/unique_fd.h>
#include <cstdint>
#include <vector>

namespace android::gui {

struct DisplayLuts {
  struct Entry {
    Entry() = default;
    Entry(int32_t d, int32_t s, int32_t k)
        : dimension(d), size(s), samplingKey(k) {}
    int32_t dimension = 0;
    int32_t size = 0;
    int32_t samplingKey = 0;
  };
  base::unique_fd lutFileDescriptor;
  std::vector<int32_t> offsets;
  std::vector<Entry> lutProperties;

  const base::unique_fd &getLutFileDescriptor() const {
    return lutFileDescriptor;
  }
  const std::vector<int32_t> &getOffsets() const { return offsets; }
  const std::vector<Entry> &getLutProperties() const { return lutProperties; }
};

} // namespace android::gui
