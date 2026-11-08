#pragma once

// Restricts routes to moderators. AuthFilter already makes sure every logged-in
// user is a reader or a moderator, so "moderator only" is the one extra rule a
// route ever needs. Always attach this AFTER AuthFilter.

#include <drogon/HttpFilter.h>

class RoleFilter : public drogon::HttpFilter<RoleFilter> {
 public:
  void doFilter(const drogon::HttpRequestPtr &req,
                drogon::FilterCallback &&reject,
                drogon::FilterChainCallback &&next) override;
};