#pragma once

// Verifies login token on protected routes.

#include <drogon/HttpFilter.h>

// Keys under which AuthFilter stores the logged-in user on the request.
// Later filters and controllers read them with req->attributes()->get<...>(key).
//   kAuthUserIdKey -> std::int64_t
//   kAuthRoleKey   -> std::string ("reader" or "moderator")
inline constexpr const char *kAuthUserIdKey = "auth.userId";
inline constexpr const char *kAuthRoleKey = "auth.role";

class AuthFilter : public drogon::HttpFilter<AuthFilter> {
 public:
  void doFilter(const drogon::HttpRequestPtr &req,
                drogon::FilterCallback &&reject,
                drogon::FilterChainCallback &&next) override;
};