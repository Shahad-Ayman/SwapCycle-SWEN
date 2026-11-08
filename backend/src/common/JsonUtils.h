#pragma once

// Helpers for building and reading JSON.
//
// Every reader validates on the server and throws common::ApiError
// (VALIDATION_FAILED) with a message that names the field, so controllers can
// pass any failure straight to common::errorResponse.

#include <drogon/HttpRequest.h>
#include <json/value.h>

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

namespace swapcycle::common {

// ---- text ------------------------------------------------------------------

// Number of characters (code points) in valid UTF-8, or nullopt if the bytes
// are not valid UTF-8 (overlong forms and surrogates count as invalid).
std::optional<std::size_t> utf8Length(std::string_view text);

// Removes leading and trailing ASCII whitespace.
std::string trim(std::string_view text);

// ---- request bodies --------------------------------------------------------

// The parsed JSON body. Throws unless it is a JSON object.
const Json::Value& requireObjectBody(const drogon::HttpRequestPtr& req);

struct TextRule {
    std::size_t maxChars = 255;
    std::size_t minChars = 1;  // checked after trimming
    bool trimWhitespace = true;
    bool allowNewlines = false;  // control characters are always rejected otherwise
};

// A required string field, trimmed and length-checked in characters.
std::string requireString(const Json::Value& object, const std::string& field, const TextRule& rule = {});

// Same as requireString, but absent or null gives nullopt.
std::optional<std::string> optionalString(const Json::Value& object, const std::string& field,
                                          const TextRule& rule = {});

// A required whole number in [min, max]. Rejects fractions and strings.
std::int64_t requireInt(const Json::Value& object, const std::string& field, std::int64_t min, std::int64_t max);
std::optional<std::int64_t> optionalInt(const Json::Value& object, const std::string& field, std::int64_t min,
                                        std::int64_t max);

std::optional<bool> optionalBool(const Json::Value& object, const std::string& field);

// A required string that must be one of `allowed` (exact match, e.g. "GOOD").
std::string requireEnum(const Json::Value& object, const std::string& field,
                        std::initializer_list<std::string_view> allowed);

// ---- pagination (docs/api/openapi.yaml: page >= 1, 1 <= pageSize <= 50) ------

constexpr int kDefaultPageSize = 20;
constexpr int kMaxPageSize = 50;

struct Pagination {
    int page = 1;
    int pageSize = kDefaultPageSize;

    std::int64_t offset() const { return static_cast<std::int64_t>(page - 1) * pageSize; }
};

// Reads ?page= and ?pageSize= from the query string. Missing values use the
// defaults; anything else that is not a whole number in range is rejected.
Pagination parsePagination(const drogon::HttpRequestPtr& req);
Pagination parsePagination(const std::string& page, const std::string& pageSize);

// { items, page, pageSize, total } as in the spec's paginated responses.
Json::Value pageJson(Json::Value items, const Pagination& pagination, std::int64_t total);

}  // namespace swapcycle::common
