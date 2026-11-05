#pragma once

// Data types for the wishlist_search module.

#include <json/json.h>

#include <optional>
#include <string>
#include <string_view>

namespace swapcycle::wishlist_search {

namespace kind {
inline constexpr std::string_view kTitle = "TITLE";        // A specific book.
inline constexpr std::string_view kInterest = "INTEREST";  // A broader interest (genre).
}  // namespace kind

inline constexpr std::size_t kMaxWishItemsPerUser = 100;

struct WishItemInput {
    std::string kind;
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> genre;
    std::optional<std::string> language;
    std::optional<std::string> minCondition;
};

struct WishItem {
    std::string id;
    WishItemInput details;
    std::string createdAt;
};

Json::Value toJson(const WishItem& item);

// Parses a POST /wishlist body. Throws common::ApiError for wrong JSON types;
// value rules are checked by WishlistSearchService.
WishItemInput parseWishItemInput(const Json::Value& body);

}  // namespace swapcycle::wishlist_search
