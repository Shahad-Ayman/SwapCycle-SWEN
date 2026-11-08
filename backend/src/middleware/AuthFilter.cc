#include "AuthFilter.h"

#include <drogon/HttpResponse.h>
#include <drogon/utils/Utilities.h>
#include <json/json.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <trantor/utils/Logger.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

namespace {

constexpr const char *kCookieName = "swapcycle_session";
constexpr size_t kMinSecretLength = 32;

struct Session {
  std::int64_t userId;
  std::string role;
};

std::string readSecret() {
  const char *value = std::getenv("SESSION_SECRET");
  return value ? value : "";
}

// Base64url HMAC-SHA256 of the data, or "" if signing failed.
std::string sign(const std::string &data, const std::string &secret) {
  unsigned char mac[EVP_MAX_MD_SIZE];
  unsigned int macLength = 0;
  const unsigned char *result =
      HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
           reinterpret_cast<const unsigned char *>(data.data()), data.size(),
           mac, &macLength);
  if (result == nullptr) {
    return "";
  }
  return drogon::utils::base64Encode(mac, macLength, true, false);
}

// Constant-time compare so response timing doesn't leak the signature.
bool sameBytes(const std::string &a, const std::string &b) {
  return a.size() == b.size() &&
         CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

bool isKnownRole(const std::string &role) {
  return role == "reader" || role == "moderator";
}

std::int64_t nowSeconds() {
  using namespace std::chrono;
  return duration_cast<seconds>(system_clock::now().time_since_epoch())
      .count();
}

std::optional<Session> readPayload(const std::string &payloadB64) {
  const std::string json = drogon::utils::base64Decode(payloadB64);

  Json::CharReaderBuilder builder;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value root;
  std::string errors;
  if (!reader->parse(json.data(), json.data() + json.size(), &root, &errors)) {
    return std::nullopt;
  }

  if (!root.isObject() || !root["sub"].isInt64() || !root["role"].isString() ||
      !root["exp"].isInt64()) {
    return std::nullopt;
  }
  if (root["exp"].asInt64() <= nowSeconds()) {
    return std::nullopt;
  }

  Session session{root["sub"].asInt64(), root["role"].asString()};
  if (!isKnownRole(session.role)) {
    return std::nullopt;
  }
  return session;
}

// Token format: base64url(payload JSON) + "." + base64url(HMAC-SHA256 of the
// first part). Payload: {"sub": <user id>, "role": "...", "exp": <unix secs>}.
std::optional<Session> verifyToken(const std::string &token,
                                   const std::string &secret) {
  const auto dot = token.find('.');
  if (dot == std::string::npos || dot == 0 || dot + 1 >= token.size()) {
    return std::nullopt;
  }

  const std::string payloadB64 = token.substr(0, dot);
  const std::string signature = token.substr(dot + 1);

  const std::string expected = sign(payloadB64, secret);
  if (expected.empty() || !sameBytes(signature, expected)) {
    return std::nullopt;
  }
  return readPayload(payloadB64);
}

std::optional<Session> readSession(const drogon::HttpRequest &req) {
  const std::string secret = readSecret();
  if (secret.size() < kMinSecretLength) {
    LOG_ERROR << "SESSION_SECRET is missing or too short; rejecting all logins";
    return std::nullopt;
  }

  const std::string token = req.getCookie(kCookieName);
  if (token.empty()) {
    return std::nullopt;
  }
  return verifyToken(token, secret);
}

// Same generic message for every failure so nobody can tell why it failed.
void respondUnauthenticated(const drogon::FilterCallback &reject) {
  Json::Value body;
  body["error"]["code"] = "UNAUTHENTICATED";
  body["error"]["message"] = "Please log in to continue.";
  auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
  resp->setStatusCode(drogon::k401Unauthorized);
  reject(resp);
}

}  // namespace

void AuthFilter::doFilter(const drogon::HttpRequestPtr &req,
                          drogon::FilterCallback &&reject,
                          drogon::FilterChainCallback &&next) {
  const auto session = readSession(*req);
  if (!session) {
    respondUnauthenticated(reject);
    return;
  }

  std::int64_t userId = session->userId;
  std::string role = session->role;
  req->attributes()->insert(kAuthUserIdKey, userId);
  req->attributes()->insert(kAuthRoleKey, role);
  next();
}
