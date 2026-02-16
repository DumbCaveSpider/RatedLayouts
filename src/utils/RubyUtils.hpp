#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace ratedlayouts {
struct RubyInfo {
  int total = 0;         // total rubies for difficulty
  int collected = 0;     // already collected (from rubies_collected.json)
  int remaining = 0;     // total - collected (clamped >= 0)
  int calcAtPercent = 0; // value calculated at player's percent (for display)
};

// Returns total rubies for a given difficulty value
int getTotalRubiesForDifficulty(int difficulty) noexcept;

// Computes ruby info for a level and difficulty
RubyInfo computeRubyInfo(GJGameLevel *level, int difficulty,
                         int levelId = 0) noexcept;

// Persist updated collected rubies for a level. Returns true on success.
bool persistCollectedRubies(int levelId, int totalRuby, int collected) noexcept;

} // namespace rl