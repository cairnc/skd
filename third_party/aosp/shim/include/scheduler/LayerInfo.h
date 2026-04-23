// LayerInfo shim — trace replay doesn't exercise SF's frame-rate scheduler;
// stub just the nested types FE pulls in (FrameRate, FrameRateCategory,
// FrameRateCompatibility, Seamlessness, FrameRateSelectionStrategy).
#pragma once

#include <cstdint>
#include <string>

namespace android {

// Minimal Fps wrapper — real Fps.h is extensive; FE only constructs Fps
// and reads it back into FrameRateVote.
class Fps {
public:
  constexpr Fps() = default;
  explicit constexpr Fps(float v) : mValue(v) {}
  static constexpr Fps fromValue(float v) { return Fps(v); }
  float getValue() const { return mValue; }
  bool operator==(const Fps &o) const { return mValue == o.mValue; }

private:
  float mValue = 0.f;
};

namespace scheduler {

enum class Seamlessness : int32_t { Default, OnlySeamless, SeamedAndSeamless };
enum class FrameRateCompatibility : int32_t {
  Default,
  Min,
  Exact,
  ExactOrMultiple,
  NoVote,
  Max,
  Gte,
  ftl_first = Default,
  ftl_last = Gte
};
enum class FrameRateCategory : int32_t {
  Default,
  NoPreference,
  Low,
  Normal,
  High,
  HighHint
};

class LayerInfo {
public:
  enum class FrameRateSelectionStrategy : int32_t {
    Propagate,
    OverrideChildren,
    Self,
    ftl_last = Self,
  };

  struct FrameRate {
    struct FrameRateVote {
      Fps rate;
      FrameRateCompatibility type = FrameRateCompatibility::Default;
      Seamlessness seamlessness = Seamlessness::Default;

      bool operator==(const FrameRateVote &o) const {
        return rate == o.rate && type == o.type &&
               seamlessness == o.seamlessness;
      }
      FrameRateVote() = default;
      FrameRateVote(Fps r, FrameRateCompatibility t,
                    Seamlessness s = Seamlessness::Default)
          : rate(r), type(t), seamlessness(s) {}
    };

    FrameRateVote vote;
    FrameRateCategory category = FrameRateCategory::Default;
    bool categorySmoothSwitchOnly = false;

    FrameRate() = default;
    FrameRate(Fps r, FrameRateCompatibility t,
              Seamlessness s = Seamlessness::Default,
              FrameRateCategory c = FrameRateCategory::Default)
        : vote(r, t, s), category(c) {}

    bool operator==(const FrameRate &o) const {
      return vote == o.vote && category == o.category;
    }
    bool isValid() const { return true; }

    // HAL-int → enum converters. The real Layer has these branching on
    // named HAL constants; layerviewer collapses them to Default since
    // we don't drive SF's frame-rate scheduler.
    static FrameRateCompatibility convertCompatibility(int8_t) {
      return FrameRateCompatibility::Default;
    }
    static Seamlessness convertChangeFrameRateStrategy(int8_t) {
      return Seamlessness::Default;
    }
    static FrameRateCategory convertCategory(int8_t) {
      return FrameRateCategory::Default;
    }
  };
};

} // namespace scheduler
} // namespace android
