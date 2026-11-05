#pragma once

// Password hashing (argon2id), signed session tokens and reset tokens.
// Built on libsodium; call initialise() once at start-up.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "AccountTypes.h"

namespace swapcycle::accounts::security {

// Returns false if libsodium could not be initialised.
bool initialise();

std::string hashPassword(std::string_view password);
bool verifyPassword(std::string_view hash, std::string_view password);

// Token = base64url(payload) "." base64url(HMAC-SHA256(secret, payload)).
// The secret must be at least 32 bytes.
std::string issueSessionToken(const SessionClaims& claims, std::string_view secret);

// Checks the signature (constant time) and expiry. AuthFilter uses this.
std::optional<SessionClaims> verifySessionToken(std::string_view token, std::string_view secret,
                                                std::int64_t nowUnixSeconds);

// 32 random bytes, base64url encoded. Used for password-reset links.
std::string randomToken();

// Reset tokens are stored only as this hash.
std::string sha256Hex(std::string_view value);

}  // namespace swapcycle::accounts::security
