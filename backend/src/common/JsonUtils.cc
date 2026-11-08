#include "JsonUtils.h"

#include <charconv>
#include <utility>

#include "ApiError.h"

namespace swapcycle::common {

namespace {

[[noreturn]] void invalid(const std::string& message) { throw ApiError(ErrorCode::ValidationFailed, message); }

std::string fieldMessage(const std::string& field, const std::string& problem) {
    return "Field '" + field + "' " + problem;
}

bool isAsciiSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

bool hasForbiddenControl(std::string_view text, bool allowNewlines) {
    for (unsigned char c : text) {
        if (allowNewlines && (c == '\n' || c == '\r' || c == '\t')) continue;
        if (c < 0x20 || c == 0x7F) return true;
    }
    return false;
}

std::string checkText(const std::string& field, const std::string& raw, const TextRule& rule) {
    std::string value = rule.trimWhitespace ? trim(raw) : raw;
    const auto length = utf8Length(value);
    if (!length) invalid(fieldMessage(field, "is not valid text."));
    if (hasForbiddenControl(value, rule.allowNewlines)) invalid(fieldMessage(field, "contains invalid characters."));
    if (*length < rule.minChars) {
        invalid(rule.minChars == 1 ? fieldMessage(field, "is required.")
                                   : fieldMessage(field, "must be at least " + std::to_string(rule.minChars) +
                                                             " characters."));
    }
    if (*length > rule.maxChars) {
        invalid(fieldMessage(field, "must be at most " + std::to_string(rule.maxChars) + " characters."));
    }
    return value;
}

// A whole number from the query string, or `fallback` if it is empty.
int queryInt(const std::string& name, const std::string& raw, int fallback, int min, int max) {
    if (raw.empty()) return fallback;
    int parsed = 0;
    const char* end = raw.data() + raw.size();
    auto [ptr, ec] = std::from_chars(raw.data(), end, parsed);
    if (ec != std::errc() || ptr != end || parsed < min || parsed > max) {
        invalid("Query parameter '" + name + "' must be a whole number from " + std::to_string(min) + " to " +
                std::to_string(max) + ".");
    }
    return parsed;
}

// object[field], treating a non-object (e.g. an array body) as a client error
// instead of letting jsoncpp throw its own exception.
const Json::Value& member(const Json::Value& object, const std::string& field) {
    if (!object.isObject()) invalid("Expected a JSON object.");
    return object[field];
}

bool isAbsent(const Json::Value& object, const std::string& field) {
    return member(object, field).isNull();
}

}  // namespace

std::optional<std::size_t> utf8Length(std::string_view text) {
    std::size_t count = 0;
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t extra = 0;
        std::uint32_t codePoint = 0;
        if (lead < 0x80) {
            codePoint = lead;
        } else if ((lead & 0xE0) == 0xC0) {
            extra = 1;
            codePoint = lead & 0x1F;
        } else if ((lead & 0xF0) == 0xE0) {
            extra = 2;
            codePoint = lead & 0x0F;
        } else if ((lead & 0xF8) == 0xF0) {
            extra = 3;
            codePoint = lead & 0x07;
        } else {
            return std::nullopt;
        }
        if (i + extra >= text.size()) return std::nullopt;  // truncated sequence
        for (std::size_t k = 1; k <= extra; ++k) {
            const auto next = static_cast<unsigned char>(text[i + k]);
            if ((next & 0xC0) != 0x80) return std::nullopt;
            codePoint = (codePoint << 6) | (next & 0x3F);
        }
        // Reject overlong encodings, UTF-16 surrogates and values past U+10FFFF.
        static constexpr std::uint32_t kMinForLength[] = {0, 0x80, 0x800, 0x10000};
        if (codePoint < kMinForLength[extra] || (codePoint >= 0xD800 && codePoint <= 0xDFFF) ||
            codePoint > 0x10FFFF) {
            return std::nullopt;
        }
        i += extra + 1;
        ++count;
    }
    return count;
}

std::string trim(std::string_view text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && isAsciiSpace(text[begin])) ++begin;
    while (end > begin && isAsciiSpace(text[end - 1])) --end;
    return std::string(text.substr(begin, end - begin));
}

const Json::Value& requireObjectBody(const drogon::HttpRequestPtr& req) {
    const auto& json = req->getJsonObject();
    if (!json || !json->isObject()) invalid("Expected a JSON object body.");
    return *json;
}

std::string requireString(const Json::Value& object, const std::string& field, const TextRule& rule) {
    const auto& value = member(object, field);
    if (value.isNull()) invalid(fieldMessage(field, "is required."));
    if (!value.isString()) invalid(fieldMessage(field, "must be a string."));
    return checkText(field, value.asString(), rule);
}

std::optional<std::string> optionalString(const Json::Value& object, const std::string& field,
                                          const TextRule& rule) {
    if (isAbsent(object, field)) return std::nullopt;
    return requireString(object, field, rule);
}

std::int64_t requireInt(const Json::Value& object, const std::string& field, std::int64_t min, std::int64_t max) {
    const auto& value = member(object, field);
    if (value.isNull()) invalid(fieldMessage(field, "is required."));
    if (value.isBool() || !value.isInt64()) invalid(fieldMessage(field, "must be a whole number."));
    const auto parsed = value.asInt64();
    if (parsed < min || parsed > max) {
        invalid(fieldMessage(field, "must be from " + std::to_string(min) + " to " + std::to_string(max) + "."));
    }
    return parsed;
}

std::optional<std::int64_t> optionalInt(const Json::Value& object, const std::string& field, std::int64_t min,
                                        std::int64_t max) {
    if (isAbsent(object, field)) return std::nullopt;
    return requireInt(object, field, min, max);
}

std::optional<bool> optionalBool(const Json::Value& object, const std::string& field) {
    if (isAbsent(object, field)) return std::nullopt;
    const auto& value = member(object, field);
    if (!value.isBool()) invalid(fieldMessage(field, "must be true or false."));
    return value.asBool();
}

std::string requireEnum(const Json::Value& object, const std::string& field,
                        std::initializer_list<std::string_view> allowed) {
    const auto& value = member(object, field);
    if (value.isNull()) invalid(fieldMessage(field, "is required."));
    if (!value.isString()) invalid(fieldMessage(field, "must be a string."));
    const auto text = value.asString();
    for (auto option : allowed) {
        if (text == option) return text;
    }
    std::string list;
    for (auto option : allowed) {
        if (!list.empty()) list += ", ";
        list += option;
    }
    invalid(fieldMessage(field, "must be one of: " + list + "."));
}

Pagination parsePagination(const std::string& page, const std::string& pageSize) {
    Pagination result;
    // Upper bound on page keeps offset() well inside int64 and the database's range.
    result.page = queryInt("page", page, 1, 1, 1'000'000);
    result.pageSize = queryInt("pageSize", pageSize, kDefaultPageSize, 1, kMaxPageSize);
    return result;
}

Pagination parsePagination(const drogon::HttpRequestPtr& req) {
    return parsePagination(req->getParameter("page"), req->getParameter("pageSize"));
}

Json::Value pageJson(Json::Value items, const Pagination& pagination, std::int64_t total) {
    Json::Value page(Json::objectValue);
    page["items"] = items.isArray() ? std::move(items) : Json::Value(Json::arrayValue);
    page["page"] = pagination.page;
    page["pageSize"] = pagination.pageSize;
    page["total"] = static_cast<Json::Int64>(total);
    return page;
}

}  // namespace swapcycle::common
