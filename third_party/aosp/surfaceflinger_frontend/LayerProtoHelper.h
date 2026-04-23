// LayerProtoHelper shim — upstream pulls in LayerTracing/TransactionTraceWriter
// writer paths + LayerSnapshotBuilder forward decls that don't exist here.
// TransactionProtoParser only needs the read + write-region/rect helpers, so
// this trimmed version keeps those byte-identical and drops the
// snapshot-to-proto generator.
#pragma once

#include <layerproto/LayerProtoHeader.h>

#include <cstdint>
#include <gui/WindowInfo.h>
#include <math/vec4.h>
#include <ui/BlurRegion.h>
#include <ui/Rect.h>
#include <ui/Region.h>

namespace android::surfaceflinger {
class LayerProtoHelper {
public:
  static void
  writeToProto(const Rect &rect,
               std::function<perfetto::protos::RectProto *()> getRectProto);
  static void writeToProto(const Rect &rect,
                           perfetto::protos::RectProto *rectProto);
  static void readFromProto(const perfetto::protos::RectProto &proto,
                            Rect &outRect);
  static void readFromProto(const perfetto::protos::RectProto &proto,
                            FloatRect &outRect);
  static void
  writeToProto(const Region &region,
               std::function<perfetto::protos::RegionProto *()> getRegionProto);
  static void writeToProto(const Region &region,
                           perfetto::protos::RegionProto *regionProto);
  static void readFromProto(const perfetto::protos::RegionProto &regionProto,
                            Region &outRegion);
  static void
  writeToProto(const mat4 matrix,
               perfetto::protos::ColorTransformProto *colorTransformProto);
  static void readFromProto(
      const perfetto::protos::ColorTransformProto &colorTransformProto,
      mat4 &matrix);
  static void writeToProto(const android::BlurRegion region,
                           perfetto::protos::BlurRegion *);
  static void readFromProto(const perfetto::protos::BlurRegion &proto,
                            android::BlurRegion &outRegion);
  static void
  writeToProto(const gui::WindowInfo &inputInfo,
               std::function<perfetto::protos::InputWindowInfoProto *()>
                   getInputWindowInfoProto);
};
} // namespace android::surfaceflinger
