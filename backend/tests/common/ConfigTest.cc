#include <gtest/gtest.h>

#include <map>
#include <string>

#include "common/Config.h"

namespace swapcycle::common {
namespace {

const std::string kSecret = "test-secret-that-is-definitely-32-bytes-long";

EnvLookup fakeEnv(std::map<std::string, std::string> values) {
    return [values = std::move(values)](const std::string& name) -> std::optional<std::string> {
        auto it = values.find(name);
        if (it == values.end()) return std::nullopt;
        return it->second;
    };
}

std::map<std::string, std::string> minimalEnv() {
    return {{"DATABASE_URL", "postgresql://user:pw@db/swapcycle"}, {"SESSION_SECRET", kSecret}};
}

TEST(ConfigTest, LoadsDefaultsFromMinimalEnvironment) {
    const auto config = loadAppConfig(fakeEnv(minimalEnv()));
    EXPECT_EQ(config.databaseUrl, "postgresql://user:pw@db/swapcycle");
    EXPECT_EQ(config.sessionSecret, kSecret);
    EXPECT_TRUE(config.cookieSecure);
    EXPECT_EQ(config.port, 8080);
    EXPECT_EQ(config.dbPoolSize, 8);
    EXPECT_EQ(config.frontendOrigin, "");
    EXPECT_EQ(config.uploadDir, "/var/lib/swapcycle/uploads");
    EXPECT_EQ(config.drogonConfig, "config.json");
}

TEST(ConfigTest, ReadsEveryOverride) {
    auto env = minimalEnv();
    env["FRONTEND_ORIGIN"] = "http://localhost:5173";
    env["COOKIE_SECURE"] = "false";
    env["PORT"] = "9000";
    env["DB_POOL_SIZE"] = "16";
    env["UPLOAD_DIR"] = "/data/uploads";
    const auto config = loadAppConfig(fakeEnv(env));
    EXPECT_EQ(config.frontendOrigin, "http://localhost:5173");
    EXPECT_FALSE(config.cookieSecure);
    EXPECT_EQ(config.port, 9000);
    EXPECT_EQ(config.dbPoolSize, 16);
    EXPECT_EQ(config.uploadDir, "/data/uploads");
}

TEST(ConfigTest, ReportsEveryMissingRequiredSetting) {
    try {
        loadAppConfig(fakeEnv({}));
        FAIL() << "expected ConfigError";
    } catch (const ConfigError& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("DATABASE_URL"), std::string::npos);
        EXPECT_NE(message.find("SESSION_SECRET"), std::string::npos);
    }
}

TEST(ConfigTest, RejectsShortSecretWithoutPrintingIt) {
    auto env = minimalEnv();
    env["SESSION_SECRET"] = "short-secret-value";
    try {
        loadAppConfig(fakeEnv(env));
        FAIL() << "expected ConfigError";
    } catch (const ConfigError& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("at least 32"), std::string::npos);
        EXPECT_EQ(message.find("short-secret-value"), std::string::npos);
    }
}

TEST(ConfigTest, RejectsWildcardOrPathOrigins) {
    for (const char* origin : {"*", "localhost:5173", "http://localhost:5173/", "https://a.example/app",
                                     "ftp://a.example", "https://"}) {
        auto env = minimalEnv();
        env["FRONTEND_ORIGIN"] = origin;
        EXPECT_THROW(loadAppConfig(fakeEnv(env)), ConfigError) << origin;
    }
}

TEST(ConfigTest, RejectsOutOfRangeOrMalformedNumbers) {
    for (const char* port : {"0", "65536", "80a", "-1", " 80", "8.5"}) {
        auto env = minimalEnv();
        env["PORT"] = port;
        EXPECT_THROW(loadAppConfig(fakeEnv(env)), ConfigError) << port;
    }
}

TEST(ConfigTest, BoolAcceptsCommonSpellingsOnly) {
    EXPECT_TRUE(boolEnv(fakeEnv({{"X", "TRUE"}}), "X", false));
    EXPECT_TRUE(boolEnv(fakeEnv({{"X", "1"}}), "X", false));
    EXPECT_FALSE(boolEnv(fakeEnv({{"X", "No"}}), "X", true));
    EXPECT_TRUE(boolEnv(fakeEnv({}), "X", true));
    EXPECT_THROW(boolEnv(fakeEnv({{"X", "maybe"}}), "X", true), ConfigError);
}

TEST(ConfigTest, EmptyValuesCountAsUnset) {
    EXPECT_EQ(envOr(fakeEnv({{"X", ""}}), "X", "fallback"), "fallback");
    EXPECT_THROW(requireEnv(fakeEnv({{"X", ""}}), "X"), ConfigError);
    EXPECT_EQ(intEnv(fakeEnv({{"X", ""}}), "X", 7, 1, 10), 7);
}

}  // namespace
}  // namespace swapcycle::common
