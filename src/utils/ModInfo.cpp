#include "ModInfo.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

void fetchModInfo(geode::Function<void(std::optional<ModInfo>)> cb) {
    log::debug("Fetching mod info from API");
    async::spawn([cb = std::move(cb)]() mutable -> arc::Future<> {
        auto req = web::WebRequest();
        auto response = co_await req.get("https://gdrate.arcticwoof.xyz/v1/");
        if (!cb) co_return;
        if (!response.ok()) {
            log::warn("Failed to fetch mod info from server");
            cb(std::nullopt);
            co_return;
        }

        auto jsonRes = response.json();
        if (!jsonRes) {
            log::warn("Failed to parse mod info JSON");
            cb(std::nullopt);
            co_return;
        }

        auto json = jsonRes.unwrap();
        ModInfo info;

        if (json.contains("message")) {
            if (auto m = json["message"].asString(); m) info.message = m.unwrap();
        }
        if (json.contains("status")) {
            if (auto s = json["status"].asString(); s) info.status = s.unwrap();
        }
        if (json.contains("serverVersion")) {
            if (auto sv = json["serverVersion"].asString(); sv) info.serverVersion = sv.unwrap();
        }
        if (json.contains("modVersion")) {
            if (auto mv = json["modVersion"].asString(); mv) info.modVersion = mv.unwrap();
        }

        log::debug("ModInfo fetched: status={}, serverVersion={}, modVersion={}, message={}", info.status, info.serverVersion, info.modVersion, info.message);
        cb(info);
        co_return;
    });
}
