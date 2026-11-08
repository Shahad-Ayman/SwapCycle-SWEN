#pragma once

// Reads environment variables and app settings (see .env.example).
// Secrets only ever come from the environment; nothing here has a default secret.

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace swapcycle::common {

// A missing or invalid setting. The message names the variable but never
// includes its value, so it is safe to print at startup.
class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Looks a variable up by name. Empty values count as unset.
// The default reads the process environment; tests pass a fake.
using EnvLookup = std::function<std::optional<std::string>(const std::string& name)>;
EnvLookup processEnvironment();

std::string envOr(const EnvLookup& env, const std::string& name, const std::string& fallback);

// Throws ConfigError if the variable is unset.
std::string requireEnv(const EnvLookup& env, const std::string& name);

// A whole number in [min, max]. Throws ConfigError if it is not one.
int intEnv(const EnvLookup& env, const std::string& name, int fallback, int min, int max);

// Accepts true/false, 1/0, yes/no (any case). Throws ConfigError otherwise.
bool boolEnv(const EnvLookup& env, const std::string& name, bool fallback);

constexpr std::size_t kMinSessionSecretLength = 32;

struct AppConfig {
    std::string databaseUrl;     // DATABASE_URL (required)
    std::string sessionSecret;   // SESSION_SECRET (required, >= 32 chars)
    std::string frontendOrigin;  // FRONTEND_ORIGIN (optional; empty disables CORS)
    bool cookieSecure = true;    // COOKIE_SECURE
    int port = 8080;             // PORT
    int dbPoolSize = 8;          // DB_POOL_SIZE
    std::string uploadDir;       // UPLOAD_DIR
    std::string drogonConfig;    // DROGON_CONFIG
};

// Reads and checks every setting at once. Throws ConfigError listing every
// problem, so a misconfigured deployment fails at startup, not mid-request.
AppConfig loadAppConfig(const EnvLookup& env = processEnvironment());

}  // namespace swapcycle::common
