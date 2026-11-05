#include "AccountSecurity.h"

#include <sodium.h>

#include <charconv>
#include <stdexcept>
#include <vector>

namespace swapcycle::accounts::security {

namespace {

constexpr std::string_view kTokenVersion = "v1";
constexpr std::size_t kMinSecretBytes = 32;

std::string toBase64Url(const unsigned char* data, std::size_t length) {
    const int variant = sodium_base64_VARIANT_URLSAFE_NO_PADDING;
    std::string out(sodium_base64_encoded_len(length, variant), '\0');
    sodium_bin2base64(out.data(), out.size(), data, length, variant);
    out.resize(std::char_traits<char>::length(out.c_str()));
    return out;
}

std::optional<std::string> fromBase64Url(std::string_view encoded) {
    std::string out(encoded.size(), '\0');
    std::size_t length = 0;
    if (sodium_base642bin(reinterpret_cast<unsigned char*>(out.data()), out.size(), encoded.data(), encoded.size(),
                          nullptr, &length, nullptr, sodium_base64_VARIANT_URLSAFE_NO_PADDING) != 0) {
        return std::nullopt;
    }
    out.resize(length);
    return out;
}

std::array<unsigned char, crypto_auth_hmacsha256_BYTES> hmac(std::string_view secret, std::string_view message) {
    std::array<unsigned char, crypto_auth_hmacsha256_BYTES> mac{};
    crypto_auth_hmacsha256_state state;
    crypto_auth_hmacsha256_init(&state, reinterpret_cast<const unsigned char*>(secret.data()), secret.size());
    crypto_auth_hmacsha256_update(&state, reinterpret_cast<const unsigned char*>(message.data()), message.size());
    crypto_auth_hmacsha256_final(&state, mac.data());
    return mac;
}

std::vector<std::string_view> split(std::string_view value, char separator) {
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (true) {
        const auto end = value.find(separator, start);
        parts.push_back(value.substr(start, end - start));
        if (end == std::string_view::npos) return parts;
        start = end + 1;
    }
}

}  // namespace

bool initialise() {
    return sodium_init() >= 0;
}

std::string hashPassword(std::string_view password) {
    char hash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str_alg(hash, password.data(), password.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
                              crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        throw std::runtime_error("password hashing ran out of memory");
    }
    return hash;
}

bool verifyPassword(std::string_view hash, std::string_view password) {
    const std::string storedHash(hash);  // libsodium needs a NUL-terminated string.
    return crypto_pwhash_str_verify(storedHash.c_str(), password.data(), password.size()) == 0;
}

std::string issueSessionToken(const SessionClaims& claims, std::string_view secret) {
    if (secret.size() < kMinSecretBytes) throw std::invalid_argument("session secret is too short");
    const std::string payload = std::string(kTokenVersion) + "|" + claims.userId + "|" +
                                std::string(toString(claims.role)) + "|" + std::to_string(claims.expiresAt);
    const auto mac = hmac(secret, payload);
    return toBase64Url(reinterpret_cast<const unsigned char*>(payload.data()), payload.size()) + "." +
           toBase64Url(mac.data(), mac.size());
}

std::optional<SessionClaims> verifySessionToken(std::string_view token, std::string_view secret,
                                                std::int64_t nowUnixSeconds) {
    if (secret.size() < kMinSecretBytes || token.size() > 512) return std::nullopt;
    const auto dot = token.find('.');
    if (dot == std::string_view::npos) return std::nullopt;

    const auto payload = fromBase64Url(token.substr(0, dot));
    const auto signature = fromBase64Url(token.substr(dot + 1));
    if (!payload || !signature || signature->size() != crypto_auth_hmacsha256_BYTES) return std::nullopt;

    const auto expected = hmac(secret, *payload);
    if (sodium_memcmp(expected.data(), signature->data(), expected.size()) != 0) return std::nullopt;

    const auto fields = split(*payload, '|');
    if (fields.size() != 4 || fields[0] != kTokenVersion) return std::nullopt;
    const auto role = roleFromString(fields[2]);
    std::int64_t expiresAt = 0;
    const auto [end, error] = std::from_chars(fields[3].data(), fields[3].data() + fields[3].size(), expiresAt);
    if (!role || error != std::errc{} || end != fields[3].data() + fields[3].size()) return std::nullopt;
    if (expiresAt <= nowUnixSeconds) return std::nullopt;

    return SessionClaims{std::string(fields[1]), *role, expiresAt};
}

std::string randomToken() {
    std::array<unsigned char, 32> bytes{};
    randombytes_buf(bytes.data(), bytes.size());
    return toBase64Url(bytes.data(), bytes.size());
}

std::string sha256Hex(std::string_view value) {
    std::array<unsigned char, crypto_hash_sha256_BYTES> digest{};
    crypto_hash_sha256(digest.data(), reinterpret_cast<const unsigned char*>(value.data()), value.size());
    std::string hex(digest.size() * 2 + 1, '\0');
    sodium_bin2hex(hex.data(), hex.size(), digest.data(), digest.size());
    hex.pop_back();
    return hex;
}

}  // namespace swapcycle::accounts::security
