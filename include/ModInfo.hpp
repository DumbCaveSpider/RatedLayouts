#pragma once

#include <string>
#include <optional>
#include <Geode/Geode.hpp>

struct ModInfo {
    std::string message;
    std::string status;
    std::string serverVersion;
    std::string modVersion;
};

void fetchModInfo(geode::Function<void(std::optional<ModInfo>)> cb);
