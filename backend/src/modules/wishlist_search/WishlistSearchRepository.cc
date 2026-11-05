#include "WishlistSearchRepository.h"

#include <utility>

namespace swapcycle::wishlist_search {

namespace {

using drogon::orm::Row;

constexpr const char* kColumns =
    "id::text AS id, kind, title, author, genre, language, min_condition, "
    "to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD\"T\"HH24:MI:SS\"Z\"') AS created_at";

std::optional<std::string> nullableString(const Row& row, const char* column) {
    if (row[column].isNull()) return std::nullopt;
    return row[column].as<std::string>();
}

WishItem itemFromRow(const Row& row) {
    WishItem item;
    item.id = row["id"].as<std::string>();
    item.details.kind = row["kind"].as<std::string>();
    item.details.title = nullableString(row, "title");
    item.details.author = nullableString(row, "author");
    item.details.genre = nullableString(row, "genre");
    item.details.language = nullableString(row, "language");
    item.details.minCondition = nullableString(row, "min_condition");
    item.createdAt = row["created_at"].as<std::string>();
    return item;
}

}  // namespace

PostgresWishlistSearchRepository::PostgresWishlistSearchRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db)) {}

drogon::Task<std::vector<WishItem>> PostgresWishlistSearchRepository::listForUser(std::string userId) {
    const auto result = co_await db_->execSqlCoro(
        std::string("SELECT ") + kColumns + " FROM wish_items WHERE user_id = $1::uuid ORDER BY created_at DESC, id",
        userId);
    std::vector<WishItem> items;
    for (const auto& row : result) items.push_back(itemFromRow(row));
    co_return items;
}

drogon::Task<std::int64_t> PostgresWishlistSearchRepository::countForUser(std::string userId) {
    const auto result =
        co_await db_->execSqlCoro("SELECT count(*) AS total FROM wish_items WHERE user_id = $1::uuid", userId);
    co_return result[0]["total"].as<std::int64_t>();
}

drogon::Task<std::optional<WishItem>> PostgresWishlistSearchRepository::insert(std::string userId,
                                                                              WishItemInput input) {
    const auto result = co_await db_->execSqlCoro(
        std::string("INSERT INTO wish_items (user_id, kind, title, author, genre, language, min_condition) "
                    "VALUES ($1::uuid, $2, $3, $4, $5, $6, $7) ON CONFLICT DO NOTHING RETURNING ") +
            kColumns,
        userId, input.kind, input.title, input.author, input.genre, input.language, input.minCondition);
    if (result.empty()) co_return std::nullopt;
    co_return itemFromRow(result[0]);
}

drogon::Task<bool> PostgresWishlistSearchRepository::deleteForUser(std::string userId, std::string wishItemId) {
    const auto result = co_await db_->execSqlCoro(
        "DELETE FROM wish_items WHERE id = $1::uuid AND user_id = $2::uuid", wishItemId, userId);
    co_return result.affectedRows() == 1;
}

}  // namespace swapcycle::wishlist_search
