// SurfaceFlinger aconfig flag shim. All flags off.
#pragma once

namespace com::android::graphics::surfaceflinger::flags {
#define LV_SF_FLAG(name)                                                       \
  inline constexpr bool name() { return false; }
LV_SF_FLAG(add_sf_skipped_frames_to_trace)
LV_SF_FLAG(cache_if_source_crop_layer_only_moved)
LV_SF_FLAG(ce_fence_promise)
LV_SF_FLAG(commit_not_composited)
LV_SF_FLAG(connected_display)
LV_SF_FLAG(detached_mirror)
LV_SF_FLAG(dim_in_gamma_space_adjusted_for_display_size)
LV_SF_FLAG(display_protected)
LV_SF_FLAG(enable_fro_dependent_features)
LV_SF_FLAG(enable_layer_command_batching)
LV_SF_FLAG(enable_small_area_detection)
LV_SF_FLAG(fp16_client_target)
LV_SF_FLAG(force_compile_graphite_renderengine)
LV_SF_FLAG(frame_rate_category_mrr)
LV_SF_FLAG(game_default_frame_rate)
LV_SF_FLAG(graphite_renderengine)
LV_SF_FLAG(hdcp_level_hal)
LV_SF_FLAG(local_tonemap_screenshots)
LV_SF_FLAG(misc1)
LV_SF_FLAG(monitor_buffer_fences)
LV_SF_FLAG(multithreaded_present)
LV_SF_FLAG(protected_if_client)
LV_SF_FLAG(refresh_rate_overlay_on_external_display)
LV_SF_FLAG(renderable_buffer_usage)
LV_SF_FLAG(restore_blur_step)
LV_SF_FLAG(single_hop_screenshot)
LV_SF_FLAG(skip_invisible_windows_in_input)
LV_SF_FLAG(stable_edid_ids)
LV_SF_FLAG(true_hdr_screenshots)
LV_SF_FLAG(view_is_root_for_insets)
LV_SF_FLAG(vrr_config)
LV_SF_FLAG(vulkan_renderengine)
LV_SF_FLAG(vsync_predictor_recovery)
LV_SF_FLAG(use_known_refresh_rate_for_fps_consistency)
LV_SF_FLAG(latch_unsignaled_with_auto_refresh_changed)
LV_SF_FLAG(dont_skip_on_early_ro)
#undef LV_SF_FLAG
} // namespace com::android::graphics::surfaceflinger::flags
