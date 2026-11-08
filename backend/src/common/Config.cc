#include "Config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <vector>

namespace swapcycle::common {

namespace {

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

// CORS needs an exact origin: scheme://host[:port], no path, no wildcard.
bool isValidOrigin(const std::string& origin) {
    std::string rest;
    if (origin.rfind("https://", 0) == 0) {
        rest = origin.substr(8);
    } else if (origin.rfind("http://", 0) == 0) {
        rest = origin.substr(7);
    } else {
        return false;
    }
    if (rest.empty()) return false;
    return std::all_of(rest.begin(), rest.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '.' || c == '-' || c == ':' || c == '[' || c == ']';
    });
}

// Runs one check and records its message instead of stopping, so startup
// reports every bad setting at once.
template <typename Fn>
void collect(std::vector<std::string>& problems, Fn&& check) {
    try {
        check();
    } catch (const ConfigError& error) {
        problems.emplace_back(error.what());
    }
}

}  // namespace

EnvLookup processEnvironment() {
    return [](const std::string& name) -> std::optional<std::string> {
        const char* value = std::getenv(name.c_str());
        if (value == nullptr || *value == '\0') return std::nullopt;
        return std::string(value);
    };
}

std::string envOr(const EnvLookup& env, const std::string& name, const std::string& fallback) {
    auto value = env(name);
    return value && !value->empty() ? *value : fallback;
}

std::string requireEnv(const EnvLookup& env, const std::string& name) {
    auto value = env(name);
    if (!value || value->empty()) throw ConfigError(name + " is required (see .env.example).");
    return *value;
}

int intEnv(const EnvLookup& env, const std::string& name, int fallback, int min, int max) {
    auto value = env(name);
    if (!value || value->empty()) return fallback;

    int parsed = 0;
    const char* begin = value->data();
    const char* end = begin + value->size();
    auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec != std::errc() || ptr != end || parsed < min || parsed > max) {
        throw ConfigError(name + " must be a whole number from " + std::to_string(min) + " to " +
                          std::to_string(max) + ".");
    }
    return parsed;
}

bool boolEnv(const EnvLookup& env, const std::string& name, bool fallback) {
    auto value = env(name);
    if (!value || value->empty()) return fallback;

    const auto normalised = lowercase(*value);
    if (normalised == "true" || normalised == "1" || normalised == "yes") return true;
    if (normalised == "false" || normalised == "0" || normalised == "no") return false;
    throw ConfigError(name + " must be true or false.");
}

AppConfig loadAppConfig(const EnvLookup& env) {
    AppConfig config;
    std::vector<std::string> problems;

    collect(problems, [&] { config.databaseUrl = requireEnv(env, "DATABASE_URL"); });
    collect(problems, [&] {
        config.sessionSecret = requireEnv(env, "SESSION_SECRET");
        if (config.sessionSecret.size() < kMinSessionSecretLength) {
            throw ConfigError("SESSION_SECRET must be at least " + std::to_string(kMinSessionSecretLength) +
                              " characters.");
        }
    });
    collect(problems, [&] {
        config.frontendOrigin = envOr(env, "FRONTEND_ORIGIN", "");
        if (!config.frontendOrigin.empty() && !isValidOrigin(config.frontendOrigin)) {
            throw ConfigError("FRONTEND_ORIGIN must be an exact origin such as https://swapcycle.example "
                              "(no path, no trailing slash, no wildcard).");
        }
    });
    collect(problems, [&] { config.cookieSecure = boolEnv(env, "COOKIE_SECURE", true); });
    collect(problems, [&] { config.port = intEnv(env, "PORT", 8080, 1, 65535); });
    collect(problems, [&] { config.dbPoolSize = intEnv(env, "DB_POOL_SIZE", 8, 1, 100); });
    config.uploadDir = envOr(env, "UPLOAD_DIR", "/var/lib/swapcycle/uploads");
    config.drogonConfig = envOr(env, "DROGON_CONFIG", "config.json");

    if (!problems.empty()) {
        std::string message = "Invalid configuration:";
        for (const auto& problem : problems) message += "\n  - " + problem;
        throw ConfigError(message);
    }
    return config;
}

}  // namespace swapcycle::common
