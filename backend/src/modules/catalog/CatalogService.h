#pragma once

// Business rules for the catalog module: creating, editing, withdrawing and
// deleting listings, photo rules, and the public listing view used by search.

#include <drogon/utils/coroutine.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "CatalogRepository.h"
#include "CatalogTypes.h"
#include "modules/accounts/AccountService.h"

namespace swapcycle::catalog {

struct CatalogLimits {
    std::size_t maxPhotoBytes = 5 * 1024 * 1024;
    int maxPhotosPerCopy = 5;
};

class CatalogService {
public:
    CatalogService(std::shared_ptr<CatalogRepository> repository, std::shared_ptr<PhotoStore> photos,
                   std::shared_ptr<accounts::AccountService> accounts, CatalogLimits limits = {});

    drogon::Task<ListingView> createListing(std::string ownerId, BookInput input);
    // Public view: only AVAILABLE or RESERVED copies; anything else is NOT_FOUND.
    drogon::Task<ListingView> getPublicListing(std::string copyId);
    // The owner's view of their own copy in any status (e.g. WITHDRAWN).
    drogon::Task<ListingView> getOwnListing(std::string ownerId, std::string copyId);
    drogon::Task<ListingView> updateListing(std::string ownerId, std::string copyId, BookInput input);
    drogon::Task<ListingView> withdraw(std::string ownerId, std::string copyId);
    drogon::Task<ListingView> relist(std::string ownerId, std::string copyId);
    drogon::Task<void> deleteListing(std::string ownerId, std::string copyId);
    drogon::Task<Page<ListingView>> listMine(std::string ownerId, int page, int pageSize);

    // Validates the filters, then returns AVAILABLE listings. Used by wishlist_search.
    drogon::Task<Page<ListingView>> search(SearchFilters filters);

    drogon::Task<Photo> addPhoto(std::string ownerId, std::string copyId, std::string bytes);
    drogon::Task<void> deletePhoto(std::string ownerId, std::string copyId, std::string photoId);
    drogon::Task<PhotoFile> getPhotoFile(std::string photoId);

    // Content type from the file signature (magic bytes), or nullopt if it is
    // not a JPEG or PNG. The file name and declared Content-Type are ignored.
    static std::optional<std::string> detectImageType(std::string_view bytes);

    static void validatePaging(int page, int pageSize);

private:
    drogon::Task<BookInput> validated(BookInput input, std::optional<int> defaultAreaId);
    // NOT_FOUND unless the copy exists and belongs to `ownerId`, so other
    // readers cannot probe for listings they do not own.
    drogon::Task<BookCopy> ownedCopy(const std::string& ownerId, const std::string& copyId);
    drogon::Task<std::vector<ListingView>> toViews(std::vector<BookCopy> copies);
    drogon::Task<ListingView> toView(BookCopy copy);

    std::shared_ptr<CatalogRepository> repository_;
    std::shared_ptr<PhotoStore> photos_;
    std::shared_ptr<accounts::AccountService> accounts_;
    CatalogLimits limits_;
};

}  // namespace swapcycle::catalog
