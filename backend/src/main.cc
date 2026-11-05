// SwapCycle backend entry point: reads configuration from the environment,
// wires repositories -> services -> controllers, and sets global HTTP policy
// (security headers, CORS, JSON 404s).

#include <drogon/drogon.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "common/ApiError.h"
#include "modules/accounts/AccountController.h"
#include "modules/accounts/AccountSecurity.h"
#include "modules/catalog/CatalogController.h"
#include "modules/wishlist_search/WishlistSearchController.h"

namespace {

using namespace swapcycle;

std::string env(const char* name, const std::string& fallback = "") {
    const char* value = std::getenv(name);
    return value && *value ? std::string(value) : fallback;
}

std::string requiredEnv(const char* name) {
    auto value = env(name);
    if (value.empty()) {
        std::cerr << "Missing required environment variable " << name << " (see .env.example)\n";
        std::exit(EXIT_FAILURE);
    }
    return value;
}

void addCorsHeaders(const drogon::HttpRequestPtr& req, const drogon::HttpResponsePtr& resp,
                    const std::string& allowedOrigin) {
    if (allowedOrigin.empty() || req->getHeader("Origin") != allowedOrigin) return;
    resp->addHeader("Access-Control-Allow-Origin", allowedOrigin);
    resp->addHeader("Access-Control-Allow-Credentials", "true");
    resp->addHeader("Vary", "Origin");
}

void installHttpPolicy(const std::string& allowedOrigin) {
    auto& app = drogon::app();

    // CORS pre-flight, only for the known frontend origin.
    app.registerSyncAdvice([allowedOrigin](const drogon::HttpRequestPtr& req) -> drogon::HttpResponsePtr {
        if (req->method() != drogon::Options) return nullptr;
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        addCorsHeaders(req, resp, allowedOrigin);
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
        return resp;
    });

    app.registerPostHandlingAdvice(
        [allowedOrigin](const drogon::HttpRequestPtr& req, const drogon::HttpResponsePtr& resp) {
            resp->addHeader("X-Content-Type-Options", "nosniff");
            resp->addHeader("X-Frame-Options", "DENY");
            resp->addHeader("Referrer-Policy", "strict-origin-when-cross-origin");
            resp->addHeader("Content-Security-Policy", "default-src 'none'; frame-ancestors 'none'");
            addCorsHeaders(req, resp, allowedOrigin);
        });

    app.setCustom404Page(common::errorResponse(common::ApiError(common::ErrorCode::NotFound, "Not found.")));
}

}  // namespace

int main() {
    if (!accounts::security::initialise()) {
        std::cerr << "libsodium failed to initialise\n";
        return EXIT_FAILURE;
    }

    accounts::AccountSettings accountSettings;
    accountSettings.sessionSecret = requiredEnv("SESSION_SECRET");
    if (accountSettings.sessionSecret.size() < 32) {
        std::cerr << "SESSION_SECRET must be at least 32 characters\n";
        return EXIT_FAILURE;
    }
    accounts::SessionCookieSettings cookie;
    cookie.secure = env("COOKIE_SECURE", "true") != "false";

    const auto db = drogon::orm::DbClient::newPgClient(requiredEnv("DATABASE_URL"),
                                                       std::stoi(env("DB_POOL_SIZE", "8")));

    auto accountService = std::make_shared<accounts::AccountService>(
        std::make_shared<accounts::PostgresAccountRepository>(db), accountSettings);
    auto catalogService = std::make_shared<catalog::CatalogService>(
        std::make_shared<catalog::PostgresCatalogRepository>(db),
        std::make_shared<catalog::FilePhotoStore>(env("UPLOAD_DIR", "/var/lib/swapcycle/uploads") + "/books"),
        accountService);
    auto wishlistService = std::make_shared<wishlist_search::WishlistSearchService>(
        std::make_shared<wishlist_search::PostgresWishlistSearchRepository>(db), catalogService);

    auto& app = drogon::app();
    app.registerController(std::make_shared<accounts::AccountController>(accountService, cookie));
    app.registerController(std::make_shared<catalog::CatalogController>(catalogService));
    app.registerController(std::make_shared<wishlist_search::WishlistSearchController>(wishlistService));

    installHttpPolicy(env("FRONTEND_ORIGIN"));

    constexpr std::size_t kMaxBodyBytes = 6 * 1024 * 1024;  // One 5 MB photo plus multipart overhead.
    app.setClientMaxBodySize(kMaxBodyBytes)
        .setClientMaxMemoryBodySize(kMaxBodyBytes)
        .setServerHeaderField("")
        .setLogLevel(trantor::Logger::kInfo)
        .addListener("0.0.0.0", static_cast<uint16_t>(std::stoi(env("PORT", "8080"))))
        .setThreadNum(0)
        .run();
    return EXIT_SUCCESS;
}
