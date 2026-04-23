// LayerProtoHelper shim cpp — only the subset TransactionProtoParser uses.
// Method bodies are copy-paste byte-identical from upstream
// frameworks/native/services/surfaceflinger/LayerProtoHelper.cpp.

#include "LayerProtoHelper.h"

namespace android::surfaceflinger {

using gui::WindowInfo;

void LayerProtoHelper::writeToProto(
    const Region &region,
    std::function<perfetto::protos::RegionProto *()> getRegionProto) {
  if (region.isEmpty()) {
    return;
  }

  writeToProto(region, getRegionProto());
}

void LayerProtoHelper::writeToProto(
    const Region &region, perfetto::protos::RegionProto *regionProto) {
  if (region.isEmpty()) {
    return;
  }

  Region::const_iterator head = region.begin();
  Region::const_iterator const tail = region.end();
  while (head != tail) {
    writeToProto(*head, regionProto->add_rect());
    head++;
  }
}

void LayerProtoHelper::readFromProto(
    const perfetto::protos::RegionProto &regionProto, Region &outRegion) {
  for (int i = 0; i < regionProto.rect_size(); i++) {
    Rect rect;
    readFromProto(regionProto.rect(i), rect);
    outRegion.orSelf(rect);
  }
}

void LayerProtoHelper::writeToProto(
    const Rect &rect,
    std::function<perfetto::protos::RectProto *()> getRectProto) {
  if (rect.left != 0 || rect.right != 0 || rect.top != 0 || rect.bottom != 0) {
    writeToProto(rect, getRectProto());
  }
}

void LayerProtoHelper::writeToProto(const Rect &rect,
                                    perfetto::protos::RectProto *rectProto) {
  rectProto->set_left(rect.left);
  rectProto->set_top(rect.top);
  rectProto->set_bottom(rect.bottom);
  rectProto->set_right(rect.right);
}

void LayerProtoHelper::readFromProto(const perfetto::protos::RectProto &proto,
                                     Rect &outRect) {
  outRect.left = proto.left();
  outRect.top = proto.top();
  outRect.bottom = proto.bottom();
  outRect.right = proto.right();
}

void LayerProtoHelper::readFromProto(const perfetto::protos::RectProto &proto,
                                     FloatRect &outRect) {
  outRect.left = proto.left();
  outRect.top = proto.top();
  outRect.bottom = proto.bottom();
  outRect.right = proto.right();
}

void LayerProtoHelper::writeToProto(
    const mat4 matrix,
    perfetto::protos::ColorTransformProto *colorTransformProto) {
  for (int i = 0; i < mat4::ROW_SIZE; i++) {
    for (int j = 0; j < mat4::COL_SIZE; j++) {
      colorTransformProto->add_val(matrix[i][j]);
    }
  }
}

void LayerProtoHelper::readFromProto(
    const perfetto::protos::ColorTransformProto &colorTransformProto,
    mat4 &matrix) {
  for (int i = 0; i < mat4::ROW_SIZE; i++) {
    for (int j = 0; j < mat4::COL_SIZE; j++) {
      matrix[i][j] = colorTransformProto.val(i * mat4::COL_SIZE + j);
    }
  }
}

void LayerProtoHelper::writeToProto(const android::BlurRegion region,
                                    perfetto::protos::BlurRegion *proto) {
  proto->set_blur_radius(region.blurRadius);
  proto->set_corner_radius_tl(region.cornerRadiusTL);
  proto->set_corner_radius_tr(region.cornerRadiusTR);
  proto->set_corner_radius_bl(region.cornerRadiusBL);
  proto->set_corner_radius_br(region.cornerRadiusBR);
  proto->set_alpha(region.alpha);
  proto->set_left(region.left);
  proto->set_top(region.top);
  proto->set_right(region.right);
  proto->set_bottom(region.bottom);
}

void LayerProtoHelper::readFromProto(const perfetto::protos::BlurRegion &proto,
                                     android::BlurRegion &outRegion) {
  outRegion.blurRadius = proto.blur_radius();
  outRegion.cornerRadiusTL = proto.corner_radius_tl();
  outRegion.cornerRadiusTR = proto.corner_radius_tr();
  outRegion.cornerRadiusBL = proto.corner_radius_bl();
  outRegion.cornerRadiusBR = proto.corner_radius_br();
  outRegion.alpha = proto.alpha();
  outRegion.left = proto.left();
  outRegion.top = proto.top();
  outRegion.right = proto.right();
  outRegion.bottom = proto.bottom();
}

void LayerProtoHelper::writeToProto(
    const WindowInfo &inputInfo,
    std::function<perfetto::protos::InputWindowInfoProto *()>
        getInputWindowInfoProto) {
  perfetto::protos::InputWindowInfoProto *proto = getInputWindowInfoProto();
  proto->set_layout_params_flags(inputInfo.layoutParamsFlags.get());
  proto->set_input_config(inputInfo.inputConfig.get());
  using U = std::underlying_type_t<WindowInfo::Type>;
  static_assert(std::is_same_v<U, int32_t>);
  proto->set_layout_params_type(static_cast<U>(inputInfo.layoutParamsType));

  LayerProtoHelper::writeToProto({inputInfo.frame.left, inputInfo.frame.top,
                                  inputInfo.frame.right,
                                  inputInfo.frame.bottom},
                                 [&]() { return proto->mutable_frame(); });
  LayerProtoHelper::writeToProto(inputInfo.touchableRegion, [&]() {
    return proto->mutable_touchable_region();
  });

  proto->set_surface_inset(inputInfo.surfaceInset);
  using InputConfig = gui::WindowInfo::InputConfig;
  proto->set_visible(!inputInfo.inputConfig.test(InputConfig::NOT_VISIBLE));
  proto->set_focusable(!inputInfo.inputConfig.test(InputConfig::NOT_FOCUSABLE));
  proto->set_has_wallpaper(
      inputInfo.inputConfig.test(InputConfig::DUPLICATE_TOUCH_TO_WALLPAPER));

  proto->set_global_scale_factor(inputInfo.globalScaleFactor);
  proto->set_replace_touchable_region_with_crop(
      inputInfo.replaceTouchableRegionWithCrop);
}

} // namespace android::surfaceflinger
