#include "CatalogRepository.h"

#include <coroutine>
#include <fstream>
#include <stdexcept>

namespace swapcycle::catalog {

namespace {

using drogon::orm::DbClient;
using drogon::orm::Row;

// Static SQL fragments only; every value is a bound parameter. Integer
// parameters are cast to ::int because Drogon sends them as binary int4 and
// Postgres would otherwise infer smallint/bigint from the column.
constexpr const char* kCopySelect = R"SQL(
    SELECT c.id::text AS id, c.description_id::text AS description_id, c.owner_id::text AS owner_id,
           d.title, d.author, d.edition, d.language, d.genre,
           c.condition, c.condition_notes, c.status, c.exchange_area_id,
           to_char(c.created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS created_at
    FROM book_copies c
    JOIN book_descriptions d ON d.id = c.description_id
)SQL";

constexpr const char* kSearchWhere = R"SQL(
    WHERE c.deleted_at IS NULL AND c.status = 'AVAILABLE'
      AND ($1 = '' OR d.title_norm LIKE lower($1) ESCAPE '\' OR d.author_norm LIKE lower($1) ESCAPE '\')
      AND ($2 = '' OR d.title_norm LIKE lower($2) ESCAPE '\')
      AND ($3 = '' OR d.author_norm LIKE lower($3) ESCAPE '\')
      AND ($4 = '' OR d.genre = $4)
      AND ($5 = '' OR d.language = $5)
      AND c.condition_rank >= $6::int
      AND ($7::int = 0 OR c.exchange_area_id = $7::int)
)SQL";

// Finds or creates the BookDescription ($1-$5) as a CTE, so creating or
// editing a listing is a single atomic statement. Drogon commits
// multi-statement transactions asynchronously, so a read that follows one on
// another connection could miss its changes.
constexpr const char* kDescriptionCte = R"SQL(
    WITH description AS (
        INSERT INTO book_descriptions (title, author, edition, language, genre)
        VALUES ($1, $2, $3, $4, $5)
        ON CONFLICT (title_norm, author_norm, edition_norm, language, genre)
            DO UPDATE SET title = book_descriptions.title
        RETURNING id
    )
)SQL";

// Awaits the COMMIT of a transaction (sent when its last reference is
// released), so later reads on any connection see its changes.
class CommitAwaiter {
public:
    explicit CommitAwaiter(std::shared_ptr<drogon::orm::Transaction> transaction)
        : transaction_(std::move(transaction)) {}

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        transaction_->setCommitCallback([this, handle](bool committed) {
            committed_ = committed;
            handle.resume();
        });
        transaction_.reset();
    }
    bool await_resume() const noexcept { return committed_; }

private:
    std::shared_ptr<drogon::orm::Transaction> transaction_;
    bool committed_ = false;
};

BookCopy copyFromRow(const Row& row) {
    BookCopy copy;
    copy.id = row["id"].as<std::string>();
    copy.descriptionId = row["description_id"].as<std::string>();
    copy.ownerId = row["owner_id"].as<std::string>();
    copy.title = row["title"].as<std::string>();
    copy.author = row["author"].as<std::string>();
    copy.edition = row["edition"].as<std::string>();
    copy.language = row["language"].as<std::string>();
    copy.genre = row["genre"].as<std::string>();
    copy.condition = row["condition"].as<std::string>();
    copy.conditionNotes = row["condition_notes"].as<std::string>();
    copy.status = row["status"].as<std::string>();
    if (!row["exchange_area_id"].isNull()) copy.exchangeAreaId = row["exchange_area_id"].as<int>();
    copy.createdAt = row["created_at"].as<std::string>();
    return copy;
}

Photo photoFromRow(const Row& row) {
    return Photo{row["id"].as<std::string>(), row["copy_id"].as<std::string>(), row["content_type"].as<std::string>(),
                 row["byte_size"].as<std::int64_t>()};
}

// Values are validated enums or UUIDs; always passed as a bound parameter.
std::string joinValues(const std::vector<std::string>& values, std::string_view open, std::string_view close) {
    std::string joined(open);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) joined += ',';
        joined += values[i];
    }
    return joined.append(close);
}

// Turns user text into a LIKE pattern that matches it literally (escapes \ % _),
// with whitespace collapsed the same way as the *_norm columns.
std::string containsPattern(const std::string& term) {
    std::string escaped;
    bool lastWasSpace = false;
    for (const char c : term) {
        const bool isSpace = c == ' ' || c == '\t' || c == '\n' || c == '\r';
        if (isSpace) {
            if (!lastWasSpace && !escaped.empty()) escaped += ' ';
            lastWasSpace = true;
            continue;
        }
        lastWasSpace = false;
        if (c == '\\' || c == '%' || c == '_') escaped += '\\';
        escaped += c;
    }
    while (!escaped.empty() && escaped.back() == ' ') escaped.pop_back();
    return escaped.empty() ? "" : "%" + escaped + "%";
}

drogon::Task<std::optional<BookCopy>> selectCopy(const std::shared_ptr<DbClient>& client, const std::string& copyId) {
    const auto result = co_await client->execSqlCoro(
        std::string(kCopySelect) + " WHERE c.id = $1::uuid AND c.deleted_at IS NULL", copyId);
    if (result.empty()) co_return std::nullopt;
    co_return copyFromRow(result[0]);
}

}  // namespace

PostgresCatalogRepository::PostgresCatalogRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

drogon::Task<BookCopy> PostgresCatalogRepository::createListing(std::string ownerId, BookInput input) {
    const auto inserted = co_await db_->execSqlCoro(
        std::string(kDescriptionCte) +
            "INSERT INTO book_copies (description_id, owner_id, condition, condition_notes, exchange_area_id) "
            "SELECT description.id, $6::uuid, $7, $8, $9::int FROM description RETURNING id::text",
        input.title, input.author, input.edition, input.language, input.genre, ownerId, input.condition,
        input.conditionNotes, input.exchangeAreaId);
    auto copy = co_await selectCopy(db_, inserted[0]["id"].as<std::string>());
    if (!copy) throw std::runtime_error("inserted book copy could not be read back");
    co_return std::move(*copy);
}

drogon::Task<std::optional<BookCopy>> PostgresCatalogRepository::findCopy(std::string copyId) {
    co_return co_await selectCopy(db_, copyId);
}

drogon::Task<bool> PostgresCatalogRepository::updateDetails(std::string copyId, BookInput input,
                                                            std::vector<std::string> allowedStatuses) {
    const auto updated = co_await db_->execSqlCoro(
        std::string(kDescriptionCte) +
            "UPDATE book_copies SET description_id = (SELECT id FROM description), condition = $7, "
            "condition_notes = $8, exchange_area_id = $9::int, updated_at = now() "
            "WHERE id = $6::uuid AND deleted_at IS NULL AND status = ANY(string_to_array($10, ','))",
        input.title, input.author, input.edition, input.language, input.genre, copyId, input.condition,
        input.conditionNotes, input.exchangeAreaId, joinValues(allowedStatuses, "", ""));
    co_return updated.affectedRows() == 1;
}

drogon::Task<bool> PostgresCatalogRepository::transitionStatus(std::string copyId, std::string from, std::string to) {
    const auto updated = co_await db_->execSqlCoro(
        "UPDATE book_copies SET status = $3, updated_at = now() "
        "WHERE id = $1::uuid AND status = $2 AND deleted_at IS NULL",
        copyId, from, to);
    co_return updated.affectedRows() == 1;
}

drogon::Task<bool> PostgresCatalogRepository::softDelete(std::string copyId,
                                                         std::vector<std::string> allowedStatuses) {
    const auto updated = co_await db_->execSqlCoro(
        "UPDATE book_copies SET deleted_at = now(), updated_at = now() "
        "WHERE id = $1::uuid AND deleted_at IS NULL AND status = ANY(string_to_array($2, ','))",
        copyId, joinValues(allowedStatuses, "", ""));
    co_return updated.affectedRows() == 1;
}

drogon::Task<std::pair<std::vector<BookCopy>, std::int64_t>> PostgresCatalogRepository::listByOwner(
    std::string ownerId, int page, int pageSize) {
    const auto counted = co_await db_->execSqlCoro(
        "SELECT count(*) AS total FROM book_copies WHERE owner_id = $1::uuid AND deleted_at IS NULL", ownerId);
    const auto result = co_await db_->execSqlCoro(
        std::string(kCopySelect) +
            " WHERE c.owner_id = $1::uuid AND c.deleted_at IS NULL ORDER BY c.created_at DESC, c.id LIMIT $2::int OFFSET $3::int",
        ownerId, pageSize, (page - 1) * pageSize);

    std::vector<BookCopy> copies;
    for (const auto& row : result) copies.push_back(copyFromRow(row));
    co_return std::make_pair(std::move(copies), counted[0]["total"].as<std::int64_t>());
}

drogon::Task<std::pair<std::vector<BookCopy>, std::int64_t>> PostgresCatalogRepository::search(
    SearchFilters filters) {
    // Patterns are lower-cased in SQL so non-ASCII letters match the *_norm columns.
    const auto q = containsPattern(filters.query);
    const auto t = containsPattern(filters.title);
    const auto a = containsPattern(filters.author);

    const std::string from = " FROM book_copies c JOIN book_descriptions d ON d.id = c.description_id ";
    const auto counted = co_await db_->execSqlCoro("SELECT count(*) AS total" + from + kSearchWhere, q, t, a,
                                                   filters.genre, filters.language, filters.minConditionRank,
                                                   filters.areaId);
    const auto result = co_await db_->execSqlCoro(
        std::string(kCopySelect) + kSearchWhere + " ORDER BY c.created_at DESC, c.id LIMIT $8::int OFFSET $9::int", q, t, a,
        filters.genre, filters.language, filters.minConditionRank, filters.areaId, filters.pageSize,
        (filters.page - 1) * filters.pageSize);

    std::vector<BookCopy> copies;
    for (const auto& row : result) copies.push_back(copyFromRow(row));
    co_return std::make_pair(std::move(copies), counted[0]["total"].as<std::int64_t>());
}

drogon::Task<std::vector<Photo>> PostgresCatalogRepository::listPhotos(std::vector<std::string> copyIds) {
    std::vector<Photo> photos;
    if (copyIds.empty()) co_return photos;
    const auto result = co_await db_->execSqlCoro(
        "SELECT id::text AS id, copy_id::text AS copy_id, content_type, byte_size FROM book_photos "
        "WHERE copy_id = ANY($1::uuid[]) AND deleted_at IS NULL ORDER BY created_at, id",
        joinValues(copyIds, "{", "}"));
    for (const auto& row : result) photos.push_back(photoFromRow(row));
    co_return photos;
}

drogon::Task<bool> PostgresCatalogRepository::insertPhoto(Photo photo, int maxPerCopy) {
    auto transaction = co_await db_->newTransactionCoro();
    // Lock the copy so two concurrent uploads cannot both pass the count check.
    co_await transaction->execSqlCoro("SELECT 1 FROM book_copies WHERE id = $1::uuid FOR UPDATE", photo.copyId);
    const auto inserted = co_await transaction->execSqlCoro(
        "INSERT INTO book_photos (id, copy_id, content_type, byte_size) "
        "SELECT $1::uuid, $2::uuid, $3, $4::int "
        "WHERE (SELECT count(*) FROM book_photos WHERE copy_id = $2::uuid AND deleted_at IS NULL) < $5::int",
        photo.id, photo.copyId, photo.contentType, static_cast<int>(photo.byteSize), maxPerCopy);
    const bool added = inserted.affectedRows() == 1;
    const bool committed = co_await CommitAwaiter(std::move(transaction));
    if (!committed) throw std::runtime_error("photo insert did not commit");
    co_return added;
}

drogon::Task<std::optional<Photo>> PostgresCatalogRepository::findPhoto(std::string photoId) {
    const auto result = co_await db_->execSqlCoro(
        "SELECT p.id::text AS id, p.copy_id::text AS copy_id, p.content_type, p.byte_size FROM book_photos p "
        "JOIN book_copies c ON c.id = p.copy_id "
        "WHERE p.id = $1::uuid AND p.deleted_at IS NULL AND c.deleted_at IS NULL",
        photoId);
    if (result.empty()) co_return std::nullopt;
    co_return photoFromRow(result[0]);
}

drogon::Task<bool> PostgresCatalogRepository::softDeletePhoto(std::string photoId, std::string copyId) {
    const auto updated = co_await db_->execSqlCoro(
        "UPDATE book_photos SET deleted_at = now() "
        "WHERE id = $1::uuid AND copy_id = $2::uuid AND deleted_at IS NULL",
        photoId, copyId);
    co_return updated.affectedRows() == 1;
}

FilePhotoStore::FilePhotoStore(std::filesystem::path directory) : directory_(std::move(directory)) {
    std::filesystem::create_directories(directory_);
}

void FilePhotoStore::save(const std::string& photoId, const std::string& bytes) {
    const auto finalPath = directory_ / photoId;
    const auto tempPath = directory_ / (photoId + ".part");
    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!out) throw std::runtime_error("could not write photo file");
    }
    std::filesystem::permissions(tempPath,
                                 std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
                                     std::filesystem::perms::group_read);
    std::filesystem::rename(tempPath, finalPath);
}

void FilePhotoStore::remove(const std::string& photoId) {
    std::error_code ignored;
    std::filesystem::remove(directory_ / photoId, ignored);
}

std::string FilePhotoStore::pathFor(const std::string& photoId) const {
    return (directory_ / photoId).string();
}

}  // namespace swapcycle::catalog
