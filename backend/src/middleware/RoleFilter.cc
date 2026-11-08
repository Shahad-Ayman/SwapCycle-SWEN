#include "RoleFilter.h"

#include <drogon/HttpResponse.h>
#include <json/json.h>

#include <string>

#include "AuthFilter.h"

namespace {

void respondError(const drogon::FilterCallback &reject,
                  drogon::HttpStatusCode status, const std::string &code,
                  const std::string &message) {
  Json::Value body;
  body["error"]["code"] = code;
  body["error"]["message"] = message;
  auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
  resp->setStatusCode(status);
  reject(resp);
}

}  // namespace

void RoleFilter::doFilter(const drogon::HttpRequestPtr &req,
                          drogon::FilterCallback &&reject,
                          drogon::FilterChainCallback &&next) {
  auto attributes = req->attributes();

  // Means someone forgot to put AuthFilter in front of this one.
  if (!attributes->find(kAuthRoleKey)) {
    respondError(reject, drogon::k401Unauthorized, "UNAUTHENTICATED",
                 "Please log in to continue.");
    return;
  }

  if (attributes->get<std::string>(kAuthRoleKey) != "moderator") {
    respondError(reject, drogon::k403Forbidden, "FORBIDDEN",
                 "You do not have permission to do that.");
    return;
  }
  next();
}
