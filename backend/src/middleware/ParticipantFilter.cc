#include "ParticipantFilter.h"

#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <trantor/utils/Logger.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "AuthFilter.h"

namespace {

std::mutex checkerMutex;
std::shared_ptr<const ExchangeParticipantChecker> registeredChecker;

std::shared_ptr<const ExchangeParticipantChecker> currentChecker() {
  std::lock_guard<std::mutex> lock(checkerMutex);
  return registeredChecker;
}

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

void respondNotFound(const drogon::FilterCallback &reject) {
  respondError(reject, drogon::k404NotFound, "NOT_FOUND",
               "Exchange not found.");
}

// Pulls <id> out of /api/exchanges/<id>[/...]. Digits only, so nothing odd
// from the URL can sneak further into the app.
std::optional<std::int64_t> exchangeIdFromPath(const std::string &path) {
  const std::string prefix = "/api/exchanges/";
  if (path.rfind(prefix, 0) != 0) {
    return std::nullopt;
  }

  const auto end = path.find('/', prefix.size());
  const std::string idText = path.substr(
      prefix.size(), end == std::string::npos ? end : end - prefix.size());

  // 18 digits max so the number can never overflow an int64.
  if (idText.empty() || idText.size() > 18) {
    return std::nullopt;
  }
  for (char c : idText) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
  }

  const std::int64_t id = std::stoll(idText);
  if (id <= 0) {
    return std::nullopt;
  }
  return id;
}

}  // namespace

void ParticipantFilter::setChecker(
    std::shared_ptr<const ExchangeParticipantChecker> checker) {
  std::lock_guard<std::mutex> lock(checkerMutex);
  registeredChecker = std::move(checker);
}

void ParticipantFilter::doFilter(const drogon::HttpRequestPtr &req,
                                 drogon::FilterCallback &&reject,
                                 drogon::FilterChainCallback &&next) {
  auto attributes = req->attributes();

  // Means someone forgot to put AuthFilter in front of this one.
  if (!attributes->find(kAuthUserIdKey)) {
    respondError(reject, drogon::k401Unauthorized, "UNAUTHENTICATED",
                 "Please log in to continue.");
    return;
  }
  const std::int64_t userId = attributes->get<std::int64_t>(kAuthUserIdKey);

  const auto exchangeId = exchangeIdFromPath(req->path());
  if (!exchangeId) {
    respondNotFound(reject);
    return;
  }

  const auto checker = currentChecker();
  if (!checker) {
    LOG_ERROR << "ParticipantFilter used before a checker was registered";
    respondError(reject, drogon::k500InternalServerError, "INTERNAL_ERROR",
                 "Something went wrong. Please try again.");
    return;
  }

  // The answer may arrive later (async DB), so the callbacks move into it.
  checker->check(
      *exchangeId, userId,
      [reject = std::move(reject), next = std::move(next)](ParticipantCheck r) {
        switch (r) {
          case ParticipantCheck::kParticipant:
            next();
            return;
          case ParticipantCheck::kNotParticipant:
            respondNotFound(reject);
            return;
          case ParticipantCheck::kError:
            respondError(reject, drogon::k500InternalServerError,
                         "INTERNAL_ERROR",
                         "Something went wrong. Please try again.");
            return;
        }
      });
}
