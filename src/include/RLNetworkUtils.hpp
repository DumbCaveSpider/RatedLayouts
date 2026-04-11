#pragma once

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>

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

    inline bool isGDPS() {
        return getBaseURL() != "https://www.boomlings.com/database";
    }
}  // namespace rl
