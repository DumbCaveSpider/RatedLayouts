#pragma once

#include <string>
#include <optional>
#include <functional>

struct ModInfo {
    std::string message;
    std::string status;
    std::string serverVersion;
    std::string modVersion;
};

void fetchModInfo(std::function<void(std::optional<ModInfo>)> cb);
