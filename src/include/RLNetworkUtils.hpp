#pragma once

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>
#include <filesystem>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <ctime>

using namespace geode::prelude;

namespace rl {
    // Returns the server's response body as the error message when it is
    // non-empty, otherwise falls back to the provided fallback string.
    inline std::string getResponseFailMessage(web::WebResponse const& response,
        std::string const& fallback) {
        auto message = response.string().unwrapOrDefault();
        if (!message.empty())
            return message;
        return fallback;
    }

    // every gd mod has a little bit of betterinfo in it :) - cvolton
    inline std::string getBaseURL() {
        // if(Loader::get()->isModLoaded("km7dev.server_api")) {
        //     auto url = ServerAPIEvents::getCurrentServer().url;
        //     if(!url.empty() && url != "NONE_REGISTERED") {
        //         while(url.ends_with("/")) url.pop_back();
        //         return url;
        //     }
        // }

        // The addresses are pointing to "https://www.boomlings.com/database/getGJLevels21.php"
        // in the main game executable
        char* originalUrl = nullptr;
#ifdef GEODE_IS_WINDOWS
        static_assert(GEODE_COMP_GD_VERSION == 22081, "Unsupported GD version");
        originalUrl = (char*)(base::get() + 0x558b70);
#elif defined(GEODE_IS_ARM_MAC)
        static_assert(GEODE_COMP_GD_VERSION == 22081, "Unsupported GD version");
        originalUrl = (char*)(base::get() + 0x77d709);
#elif defined(GEODE_IS_INTEL_MAC)
        static_assert(GEODE_COMP_GD_VERSION == 22081, "Unsupported GD version");
        originalUrl = (char*)(base::get() + 0x868df0);
#elif defined(GEODE_IS_ANDROID64)
        static_assert(GEODE_COMP_GD_VERSION == 22081, "Unsupported GD version");
        originalUrl = (char*)(base::get() + 0xECCF90);
#elif defined(GEODE_IS_ANDROID32)
        static_assert(GEODE_COMP_GD_VERSION == 22081, "Unsupported GD version");
        originalUrl = (char*)(base::get() + 0x96C0DB);
#elif defined(GEODE_IS_IOS)
        static_assert(GEODE_COMP_GD_VERSION == 22081, "Unsupported GD version");
        originalUrl = (char*)(base::get() + 0x6b8cc2);
#else
        static_assert(false, "Unsupported platform");
#endif

        std::string ret = originalUrl;
        if (ret.size() > 34) ret = ret.substr(0, 34);

        log::debug("Base URL from game executable: '{}'", ret);
        return ret;
    }

    struct RequestCacheEntry {
        matjson::Value json;
        std::time_t timestamp = 0;
    };

    static inline std::mutex RequestCacheMutex;
    static inline std::unordered_map<int, RequestCacheEntry> CommentRoleCache;
    static inline std::unordered_map<int, RequestCacheEntry> LevelRatingCache;

    inline std::filesystem::path getRequestCachePath() {
        return dirs::getModsSaveDir() / Mod::get()->getID() / "request_cache.json";
    }

    inline bool requestCacheExists() {
        std::lock_guard lock(RequestCacheMutex);
        if (!CommentRoleCache.empty() || !LevelRatingCache.empty()) {
            return true;
        }
        return std::filesystem::exists(getRequestCachePath());
    }

    inline std::filesystem::path getNameplateCacheDir() {
        return dirs::getModsSaveDir() / Mod::get()->getID() / "nameplates";
    }

    inline std::filesystem::path getNameplateCachePath(int nameplateId) {
        return getNameplateCacheDir() / fmt::format("nameplate_{}.png", nameplateId);
    }

    inline bool saveNameplateCache(int nameplateId, std::string const& data) {
        auto path = getNameplateCachePath(nameplateId);
        std::filesystem::create_directories(path.parent_path());
        auto writeRes = utils::file::writeString(utils::string::pathToString(path), data);
        return static_cast<bool>(writeRes);
    }

    inline bool hasNameplateCache(int nameplateId) {
        return std::filesystem::exists(getNameplateCachePath(nameplateId));
    }

    inline int getRequestCacheLifetimeSeconds() {
        if (!Mod::get()) {
            return 300;
        }
        return Mod::get()->getSettingValue<int>("requestCacheLifetime");
    }

    inline int getRequestCacheMaxItems() {
        if (!Mod::get()) {
            return 100;
        }
        return Mod::get()->getSettingValue<int>("requestCacheMaxItems");
    }

    inline void pruneCacheMap(std::unordered_map<int, RequestCacheEntry>& cache) {
        int maxItems = getRequestCacheMaxItems();
        if (maxItems <= 0 || static_cast<int>(cache.size()) <= maxItems) {
            return;
        }

        std::vector<std::pair<int, std::time_t>> entries;
        entries.reserve(cache.size());
        for (auto const& kv : cache) {
            entries.emplace_back(kv.first, kv.second.timestamp);
        }
        std::sort(entries.begin(), entries.end(), [](auto const& a, auto const& b) {
            return a.second < b.second;
        });

        int removeCount = static_cast<int>(cache.size()) - maxItems;
        for (int i = 0; i < removeCount; ++i) {
            cache.erase(entries[i].first);
        }
    }

    inline bool isRequestCacheValid(std::time_t timestamp) {
        int lifetime = getRequestCacheLifetimeSeconds();
        if (lifetime <= 0) {
            return false;
        }
        return (std::time(nullptr) - timestamp) < lifetime;
    }

    inline matjson::Value loadRequestCacheRoot() {
        auto path = getRequestCachePath();
        auto existing = utils::file::readString(utils::string::pathToString(path));
        if (!existing) {
            return matjson::Value::object();
        }
        auto parsed = matjson::parse(existing.unwrap());
        if (!parsed || !parsed.unwrap().isObject()) {
            return matjson::Value::object();
        }
        return parsed.unwrap();
    }

    inline bool saveRequestCacheRoot(matjson::Value const& root) {
        auto path = getRequestCachePath();
        std::filesystem::create_directories(path.parent_path());
        auto writeRes = utils::file::writeString(utils::string::pathToString(path), root.dump());
        return static_cast<bool>(writeRes);
    }

    inline std::optional<RequestCacheEntry> loadRequestCacheEntry(std::string const& section, int id) {
        auto root = loadRequestCacheRoot();
        if (!root.isObject()) {
            return std::nullopt;
        }

        auto sectionValue = root[section];
        if (!sectionValue.isObject()) {
            return std::nullopt;
        }

        auto entry = sectionValue[fmt::format("{}", id)];
        if (!entry.isObject()) {
            return std::nullopt;
        }

        auto timestampValue = entry["timestamp"].asInt();
        if (!timestampValue) {
            return std::nullopt;
        }

        auto dataValue = entry["data"];
        if (!dataValue.isObject() && !dataValue.isArray()) {
            return std::nullopt;
        }

        return RequestCacheEntry{dataValue,
            static_cast<std::time_t>(timestampValue.unwrap())};
    }

    inline void storeRequestCacheEntry(std::string const& section, int id, matjson::Value const& data) {
        auto root = loadRequestCacheRoot();
        if (!root.isObject()) {
            root = matjson::Value::object();
        }
        auto sectionValue = root[section];
        if (!sectionValue.isObject()) {
            sectionValue = matjson::Value::object();
        }

        matjson::Value entry = matjson::Value::object();
        entry["timestamp"] = static_cast<int>(std::time(nullptr));
        entry["data"] = data;
        sectionValue[fmt::format("{}", id)] = entry;
        root[section] = sectionValue;

        saveRequestCacheRoot(root);
    }

    inline void removeRequestCacheEntry(std::string const& section, int id) {
        auto root = loadRequestCacheRoot();
        if (!root.isObject()) {
            return;
        }
        auto sectionValue = root[section];
        if (!sectionValue.isObject()) {
            return;
        }

        std::string key = fmt::format("{}", id);
        if (sectionValue.isObject() && sectionValue[std::string(key)].isObject()) {
            sectionValue.erase(key);
        }
        root[section] = sectionValue;
        saveRequestCacheRoot(root);
    }

    inline void clearNameplateCache() {
        auto path = getNameplateCacheDir();
        if (std::filesystem::exists(path)) {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    }

    inline void clearRequestCache() {
        std::lock_guard lock(RequestCacheMutex);
        CommentRoleCache.clear();
        LevelRatingCache.clear();
        auto requestPath = getRequestCachePath();
        if (std::filesystem::exists(requestPath)) {
            std::error_code ec;
            std::filesystem::remove(requestPath, ec);
        }
        clearNameplateCache();
    }

    inline std::optional<matjson::Value> getCachedCommentRole(int accountId) {
        std::lock_guard lock(RequestCacheMutex);
        auto it = CommentRoleCache.find(accountId);
        if (it != CommentRoleCache.end() && isRequestCacheValid(it->second.timestamp)) {
            return it->second.json;
        }
        auto entry = loadRequestCacheEntry("commentRoleCache", accountId);
        if (entry) {
            CommentRoleCache[accountId] = *entry;
            if (isRequestCacheValid(entry->timestamp)) {
                return entry->json;
            }
        }
        return std::nullopt;
    }

    inline std::optional<matjson::Value> getStaleCommentRole(int accountId) {
        std::lock_guard lock(RequestCacheMutex);
        auto it = CommentRoleCache.find(accountId);
        if (it != CommentRoleCache.end()) {
            return it->second.json;
        }
        auto entry = loadRequestCacheEntry("commentRoleCache", accountId);
        if (entry) {
            CommentRoleCache[accountId] = *entry;
            return entry->json;
        }
        return std::nullopt;
    }

    inline void setCachedCommentRole(int accountId, matjson::Value const& data) {
        std::lock_guard lock(RequestCacheMutex);
        CommentRoleCache[accountId] = RequestCacheEntry{data, std::time(nullptr)};
        pruneCacheMap(CommentRoleCache);
        storeRequestCacheEntry("commentRoleCache", accountId, data);
    }

    inline std::optional<matjson::Value> getCachedLevelRating(int levelId) {
        std::lock_guard lock(RequestCacheMutex);
        auto it = LevelRatingCache.find(levelId);
        if (it != LevelRatingCache.end() && isRequestCacheValid(it->second.timestamp)) {
            return it->second.json;
        }
        auto entry = loadRequestCacheEntry("levelRatingCache", levelId);
        if (entry) {
            LevelRatingCache[levelId] = *entry;
            if (isRequestCacheValid(entry->timestamp)) {
                return entry->json;
            }
        }
        return std::nullopt;
    }

    inline std::optional<matjson::Value> getStaleLevelRating(int levelId) {
        std::lock_guard lock(RequestCacheMutex);
        auto it = LevelRatingCache.find(levelId);
        if (it != LevelRatingCache.end()) {
            return it->second.json;
        }
        auto entry = loadRequestCacheEntry("levelRatingCache", levelId);
        if (entry) {
            LevelRatingCache[levelId] = *entry;
            return entry->json;
        }
        return std::nullopt;
    }

    inline void setCachedLevelRating(int levelId, matjson::Value const& data) {
        std::lock_guard lock(RequestCacheMutex);
        LevelRatingCache[levelId] = RequestCacheEntry{data, std::time(nullptr)};
        pruneCacheMap(LevelRatingCache);
        storeRequestCacheEntry("levelRatingCache", levelId, data);
    }

    inline void removeCachedLevelRating(int levelId) {
        std::lock_guard lock(RequestCacheMutex);
        LevelRatingCache.erase(levelId);
        removeRequestCacheEntry("levelRatingCache", levelId);
        log::debug("Removed cached level rating for level {}", levelId);
    }

    inline bool isGDPS() {
        return getBaseURL() != "https://www.boomlings.com/database";
    }
}  // namespace rl
