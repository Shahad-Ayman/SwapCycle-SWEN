#pragma once

// HTTP endpoints for the accounts module: /api/auth/*, /api/users/*, /api/meeting-areas.
// Parses requests, applies rate limits and sets the session cookie; the rules
// live in AccountService.

#include <drogon/HttpController.h>

#include <memory>
#include <string>

#include "AccountService.h"
#include "RateLimiter.h"

#ifndef SWAPCYCLE_UUID_RE
#define SWAPCYCLE_UUID_RE "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"
#endif

namespace swapcycle::accounts {

struct SessionCookieSettings {
    std::string name = "sc_session";
    bool secure = true;  // Turn off only for plain-http local development (COOKIE_SECURE=false).
};

class AccountController final : public drogon::HttpController<AccountController, false> {
public:
    AccountController(std::shared_ptr<AccountService> service, SessionCookieSettings cookie);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AccountController::registerUser, "/api/auth/register", drogon::Post);
    ADD_METHOD_TO(AccountController::login, "/api/auth/login", drogon::Post);
    ADD_METHOD_TO(AccountController::logout, "/api/auth/logout", drogon::Post);
    ADD_METHOD_TO(AccountController::me, "/api/auth/me", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(AccountController::requestPasswordReset, "/api/auth/password-reset/request", drogon::Post);
    ADD_METHOD_TO(AccountController::confirmPasswordReset, "/api/auth/password-reset/confirm", drogon::Post);
    ADD_METHOD_TO(AccountController::meetingAreas, "/api/meeting-areas", drogon::Get);
    ADD_METHOD_TO(AccountController::updateProfile, "/api/users/me", drogon::Patch, "AuthFilter");
    ADD_METHOD_VIA_REGEX(AccountController::publicProfile, "/api/users/(" SWAPCYCLE_UUID_RE ")", drogon::Get,
                         "AuthFilter");
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> registerUser(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> login(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> logout(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> me(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> requestPasswordReset(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> confirmPasswordReset(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> meetingAreas(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> updateProfile(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> publicProfile(drogon::HttpRequestPtr req, std::string userId);

private:
    void setSessionCookie(const drogon::HttpResponsePtr& resp, const Session& session) const;

    std::shared_ptr<AccountService> service_;
    SessionCookieSettings cookie_;
    RateLimiter registerPerIp_;
    RateLimiter loginPerIp_;
    RateLimiter loginPerEmail_;
    RateLimiter resetPerIp_;
};

}  // namespace swapcycle::accounts
