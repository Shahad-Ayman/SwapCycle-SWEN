#pragma once

// Data types for the catalog module. A BookDescription (title, author,
// edition, language, genre) is separate from a physical BookCopy (owner,
// condition, photos, status).

#include <json/json.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "modules/accounts/AccountTypes.h"

namespace swapcycle::catalog {

inline constexpr std::array<std::string_view, 16> kGenres{
    "FICTION", "MYSTERY", "SCIENCE_FICTION", "FANTASY", "ROMANCE", "THRILLER", "BIOGRAPHY", "HISTORY",
    "TRAVEL", "SCIENCE", "TECHNOLOGY", "ART_DESIGN", "SELF_HELP", "BUSINESS", "POETRY", "OTHER"};

// Ordered best to worst.
inline constexpr std::array<std::string_view, 5> kConditions{"NEW", "LIKE_NEW", "GOOD", "FAIR", "POOR"};

namespace status {
inline constexpr std::string_view kAvailable = "AVAILABLE";
inline constexpr std::string_view kReserved = "RESERVED";
inline constexpr std::string_view kTransferred = "TRANSFERRED";
inline constexpr std::string_view kWithdrawn = "WITHDRAWN";
}  // namespace status

bool isGenre(std::string_view value);
// NEW = 5 ... POOR = 1; nullopt for unknown values.
std::optional<int> conditionRank(std::string_view condition);
bool isLanguageCode(std::string_view value);  // ISO 639-1, lower case.

// Shared text rules (also used by wishlist_search). Trims surrounding
// whitespace and rejects control characters and out-of-range lengths
// (counted in characters, not bytes). Throws common::ApiError.
std::string validText(std::string_view value, const char* field, std::size_t minLength, std::size_t maxLength);

struct BookInput {
    std::string title;
    std::string author;
    std::string edition;
    std::string language;
    std::string genre;
    std::string condition;
    std::string conditionNotes;
    std::optional<int> exchangeAreaId;
};

struct BookCopy {
    std::string id;
    std::string descriptionId;
    std::string ownerId;
    std::string title;
    std::string author;
    std::string edition;
    std::string language;
    std::string genre;
    std::string condition;
    std::string conditionNotes;
    std::string status;
    std::optional<int> exchangeAreaId;
    std::string createdAt;
};

struct Photo {
    std::string id;
    std::string copyId;
    std::string contentType;
    std::int64_t byteSize = 0;
};

struct PhotoFile {
    std::string path;
    std::string contentType;
};

// A copy as other readers see it: owner display name and exchange area only.
struct ListingView {
    BookCopy copy;
    accounts::OwnerSummary owner;
    std::optional<accounts::MeetingArea> exchangeArea;
    std::vector<Photo> photos;
};

struct SearchFilters {
    std::string query;  // Matches title or author.
    std::string title;
    std::string author;
    std::string genre;
    std::string language;
    int minConditionRank = 0;  // 0 = any.
    int areaId = 0;            // 0 = any.
    int page = 1;
    int pageSize = 20;
};

template <typename T>
struct Page {
    std::vector<T> items;
    int page = 1;
    int pageSize = 20;
    std::int64_t total = 0;
};

inline constexpr int kMaxPageSize = 50;

Json::Value toJson(const Photo& photo);
Json::Value toJson(const ListingView& listing);
Json::Value toJson(const Page<ListingView>& page);

// Parses an optional whole-number query parameter. Throws common::ApiError
// (VALIDATION_FAILED) if it is present but not a number.
int parseIntParam(std::string_view raw, int fallback, const char* name);

// Parses a POST/PATCH /books body. Throws common::ApiError for wrong JSON
// types; value rules are checked by CatalogService.
BookInput parseBookInput(const Json::Value& body);

}  // namespace swapcycle::catalog
