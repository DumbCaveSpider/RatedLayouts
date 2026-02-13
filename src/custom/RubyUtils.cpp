#include "RubyUtils.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace rl {

int getTotalRubiesForDifficulty(int difficulty) noexcept {
  switch (difficulty) {
  case 1:
    return 0;
  case 2:
    return 50;
  case 3:
    return 100;
  case 4:
    return 175;
  case 5:
    return 175;
  case 6:
    return 250;
  case 7:
    return 250;
  case 8:
    return 350;
  case 9:
    return 350;
  case 10:
    return 500;
  case 15:
    return 625;
  case 20:
    return 750;
  case 25:
    return 875;
  case 30:
    return 1000;
  default:
    return 0;
  }
}

static int computeAtPercent(GJGameLevel *level, int totalRuby) noexcept {
  int percent = 0;
  if (level)
    percent = level->m_normalPercent;
  if (percent >= 100)
    return totalRuby;
  double calcD = static_cast<double>(totalRuby) * 0.8 *
                 (static_cast<double>(percent) / 100.0);
  int calcAtPercent = static_cast<int>(std::lround(calcD));
  if (calcAtPercent > totalRuby)
    calcAtPercent = totalRuby;
  return calcAtPercent;
}

RubyInfo computeRubyInfo(GJGameLevel *level, int difficulty,
                         int levelId) noexcept {
  RubyInfo info;
  info.total = getTotalRubiesForDifficulty(difficulty);

  // determine level id if not provided
  int id = levelId;
  if (id == 0 && level)
    id = level->m_levelID;

  // read save file
  auto savePath =
      dirs::getModsSaveDir() / Mod::get()->getID() / "rubies_collected.json";
  auto existing =
      utils::file::readString(utils::string::pathToString(savePath));
  matjson::Value root = matjson::Value::object();
  if (existing) {
    auto parsed = matjson::parse(existing.unwrap());
    if (parsed && parsed.unwrap().isObject())
      root = parsed.unwrap();
  }

  std::string levelKey = fmt::format("{}", id);
  matjson::Value entry = root[levelKey];
  if (entry.isObject()) {
    info.collected = entry["collectedRubies"].asInt().unwrapOr(0);
  }

  info.remaining = std::max(0, info.total - info.collected);
  info.calcAtPercent = computeAtPercent(level, info.total);
  return info;
}

bool persistCollectedRubies(int levelId, int totalRuby,
                            int collected) noexcept {
  auto savePath =
      dirs::getModsSaveDir() / Mod::get()->getID() / "rubies_collected.json";
  auto existing =
      utils::file::readString(utils::string::pathToString(savePath));
  matjson::Value root = matjson::Value::object();
  if (existing) {
    auto parsed = matjson::parse(existing.unwrap());
    if (parsed && parsed.unwrap().isObject())
      root = parsed.unwrap();
  }
  root[fmt::format("{}", levelId)] = matjson::Value::object();
  root[fmt::format("{}", levelId)]["totalRubies"] = totalRuby;
  root[fmt::format("{}", levelId)]["collectedRubies"] = collected;
  auto writeRes = utils::file::writeString(
      utils::string::pathToString(savePath), root.dump());
  return static_cast<bool>(writeRes);
  
  if (!writeRes) {
    log::warn("Failed to write rubies_collected.json: {}",
              writeRes.unwrapErr());
    return false;
  }
}

} // namespace rl