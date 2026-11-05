#include "CatalogTypes.h"

#include <algorithm>
#include <cctype>
#include <charconv>

#include "common/ApiError.h"

namespace swapcycle::catalog {

namespace {

using common::ApiError;
using common::ErrorCode;

std::string optionalString(const Json::Value& body, const char* field) {
    if (!body.isMember(field) || body[field].isNull()) return {};
    if (!body[field].isString()) {
        throw ApiError(ErrorCode::ValidationFailed, std::string("Field '") + field + "' must be text.");
    }
    return body[field].asString();
}

std::size_t utf8Length(std::string_view value) {
    return static_cast<std::size_t>(
        std::count_if(value.begin(), value.end(), [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

}  // namespace

bool isGenre(std::string_view value) {
    return std::find(kGenres.begin(), kGenres.end(), value) != kGenres.end();
}

std::optional<int> conditionRank(std::string_view condition) {
    const auto it = std::find(kConditions.begin(), kConditions.end(), condition);
    if (it == kConditions.end()) return std::nullopt;
    return static_cast<int>(kConditions.size() - (it - kConditions.begin()));
}

bool isLanguageCode(std::string_view value) {
    return value.size() == 2 && std::islower(static_cast<unsigned char>(value[0])) &&
           std::islower(static_cast<unsigned char>(value[1]));
}

std::string validText(std::string_view value, const char* field, std::size_t minLength, std::size_t maxLength) {
    const auto first = value.find_first_not_of(" \t\r\n");
    const auto last = value.find_last_not_of(" \t\r\n");
    std::string trimmed = first == std::string_view::npos ? "" : std::string(value.substr(first, last - first + 1));

    // Newlines are allowed inside free text (e.g. condition notes); other control characters are not.
    const bool hasControl = std::any_of(trimmed.begin(), trimmed.end(), [](char c) {
        return std::iscntrl(static_cast<unsigned char>(c)) && c != '\n';
    });
    const auto length = utf8Length(trimmed);
    if (hasControl || length < minLength || length > maxLength) {
        throw ApiError(ErrorCode::ValidationFailed, std::string(field) + " must be " + std::to_string(minLength) +
                                                        " to " + std::to_string(maxLength) + " characters.");
    }
    return trimmed;
}

Json::Value toJson(const Photo& photo) {
    Json::Value json;
    json["id"] = photo.id;
    json["url"] = "/api/photos/" + photo.id;
    return json;
}

Json::Value toJson(const ListingView& listing) {
    const auto& copy = listing.copy;
    Json::Value json;
    json["id"] = copy.id;
    json["title"] = copy.title;
    json["author"] = copy.author;
    json["edition"] = copy.edition;
    json["language"] = copy.language;
    json["genre"] = copy.genre;
    json["condition"] = copy.condition;
    json["conditionNotes"] = copy.conditionNotes;
    json["status"] = copy.status;
    json["exchangeArea"] = listing.exchangeArea ? accounts::toJson(*listing.exchangeArea) : Json::Value(Json::nullValue);
    json["owner"]["id"] = listing.owner.id;
    json["owner"]["displayName"] = listing.owner.displayName;
    json["photos"] = Json::Value(Json::arrayValue);
    for (const auto& photo : listing.photos) json["photos"].append(toJson(photo));
    json["createdAt"] = copy.createdAt;
    return json;
}

Json::Value toJson(const Page<ListingView>& page) {
    Json::Value json;
    json["items"] = Json::Value(Json::arrayValue);
    for (const auto& item : page.items) json["items"].append(toJson(item));
    json["page"] = page.page;
    json["pageSize"] = page.pageSize;
    json["total"] = static_cast<Json::Int64>(page.total);
    return json;
}

int parseIntParam(std::string_view raw, int fallback, const char* name) {
    if (raw.empty()) return fallback;
    int value = 0;
    const auto [end, error] = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    if (error != std::errc{} || end != raw.data() + raw.size()) {
        throw ApiError(ErrorCode::ValidationFailed, std::string(name) + " must be a whole number.");
    }
    return value;
}

BookInput parseBookInput(const Json::Value& body) {
    if (!body.isObject()) throw ApiError(ErrorCode::ValidationFailed, "Expected a JSON object body.");
    BookInput input;
    input.title = optionalString(body, "title");
    input.author = optionalString(body, "author");
    input.edition = optionalString(body, "edition");
    input.language = optionalString(body, "language");
    input.genre = optionalString(body, "genre");
    input.condition = optionalString(body, "condition");
    input.conditionNotes = optionalString(body, "conditionNotes");
    if (body.isMember("exchangeAreaId") && !body["exchangeAreaId"].isNull()) {
        if (!body["exchangeAreaId"].isInt()) {
            throw ApiError(ErrorCode::ValidationFailed, "Field 'exchangeAreaId' must be a whole number.");
        }
        input.exchangeAreaId = body["exchangeAreaId"].asInt();
    }
    return input;
}

}  // namespace swapcycle::catalog
