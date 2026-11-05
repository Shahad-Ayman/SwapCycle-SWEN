#pragma once

// Database and file-storage access for the catalog module. The only code that
// touches book_descriptions, book_copies, book_photos and stored photo files.

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "CatalogTypes.h"

namespace swapcycle::catalog {

class CatalogRepository {
public:
    virtual ~CatalogRepository() = default;

    // Finds or creates the BookDescription and inserts an AVAILABLE copy, in one transaction.
    virtual drogon::Task<BookCopy> createListing(std::string ownerId, BookInput input) = 0;

    // Excludes deleted copies.
    virtual drogon::Task<std::optional<BookCopy>> findCopy(std::string copyId) = 0;

    // The update only happens while the copy's status is one of `allowedStatuses`,
    // so a concurrent reservation cannot be overwritten. Returns false otherwise.
    virtual drogon::Task<bool> updateDetails(std::string copyId, BookInput input,
                                             std::vector<std::string> allowedStatuses) = 0;
    virtual drogon::Task<bool> transitionStatus(std::string copyId, std::string from, std::string to) = 0;
    virtual drogon::Task<bool> softDelete(std::string copyId, std::vector<std::string> allowedStatuses) = 0;

    // Returns (items, total).
    virtual drogon::Task<std::pair<std::vector<BookCopy>, std::int64_t>> listByOwner(std::string ownerId, int page,
                                                                                    int pageSize) = 0;
    // Only AVAILABLE, non-deleted copies.
    virtual drogon::Task<std::pair<std::vector<BookCopy>, std::int64_t>> search(SearchFilters filters) = 0;

    virtual drogon::Task<std::vector<Photo>> listPhotos(std::vector<std::string> copyIds) = 0;
    // Inserts only while the copy has fewer than `maxPerCopy` photos. Returns false otherwise.
    virtual drogon::Task<bool> insertPhoto(Photo photo, int maxPerCopy) = 0;
    // Excludes deleted photos and photos of deleted copies.
    virtual drogon::Task<std::optional<Photo>> findPhoto(std::string photoId) = 0;
    virtual drogon::Task<bool> softDeletePhoto(std::string photoId, std::string copyId) = 0;
};

class PostgresCatalogRepository final : public CatalogRepository {
public:
    explicit PostgresCatalogRepository(drogon::orm::DbClientPtr db);

    drogon::Task<BookCopy> createListing(std::string ownerId, BookInput input) override;
    drogon::Task<std::optional<BookCopy>> findCopy(std::string copyId) override;
    drogon::Task<bool> updateDetails(std::string copyId, BookInput input,
                                     std::vector<std::string> allowedStatuses) override;
    drogon::Task<bool> transitionStatus(std::string copyId, std::string from, std::string to) override;
    drogon::Task<bool> softDelete(std::string copyId, std::vector<std::string> allowedStatuses) override;
    drogon::Task<std::pair<std::vector<BookCopy>, std::int64_t>> listByOwner(std::string ownerId, int page,
                                                                            int pageSize) override;
    drogon::Task<std::pair<std::vector<BookCopy>, std::int64_t>> search(SearchFilters filters) override;
    drogon::Task<std::vector<Photo>> listPhotos(std::vector<std::string> copyIds) override;
    drogon::Task<bool> insertPhoto(Photo photo, int maxPerCopy) override;
    drogon::Task<std::optional<Photo>> findPhoto(std::string photoId) override;
    drogon::Task<bool> softDeletePhoto(std::string photoId, std::string copyId) override;

private:
    drogon::orm::DbClientPtr db_;
};

// Stores photo bytes under random UUID names, with no extension, in a
// directory outside any web root. Files are only ever served by the API.
class PhotoStore {
public:
    virtual ~PhotoStore() = default;
    virtual void save(const std::string& photoId, const std::string& bytes) = 0;
    virtual void remove(const std::string& photoId) = 0;
    virtual std::string pathFor(const std::string& photoId) const = 0;
};

class FilePhotoStore final : public PhotoStore {
public:
    explicit FilePhotoStore(std::filesystem::path directory);

    void save(const std::string& photoId, const std::string& bytes) override;
    void remove(const std::string& photoId) override;
    std::string pathFor(const std::string& photoId) const override;

private:
    std::filesystem::path directory_;
};

}  // namespace swapcycle::catalog
