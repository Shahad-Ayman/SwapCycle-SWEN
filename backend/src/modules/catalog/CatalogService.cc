#include "CatalogService.h"

#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <unordered_map>
#include <utility>

#include "common/ApiError.h"

namespace swapcycle::catalog {

namespace {

using common::ApiError;
using common::ErrorCode;

const std::vector<std::string> kEditableStatuses{std::string(status::kAvailable), std::string(status::kWithdrawn)};

constexpr const char* kLockedMessage =
    "This book is part of an exchange, so it cannot be changed right now.";

bool isEditable(const BookCopy& copy) {
    return std::find(kEditableStatuses.begin(), kEditableStatuses.end(), copy.status) != kEditableStatuses.end();
}

bool isPubliclyVisible(const BookCopy& copy) {
    return copy.status == status::kAvailable || copy.status == status::kReserved;
}

// Random (version 4) UUID; also the photo's file name.
std::string newUuid() {
    std::array<unsigned char, 16> bytes{};
    randombytes_buf(bytes.data(), bytes.size());
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);
    char text[37];
    std::snprintf(text, sizeof(text), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", bytes[0],
                  bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9], bytes[10],
                  bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return text;
}

}  // namespace

CatalogService::CatalogService(std::shared_ptr<CatalogRepository> repository, std::shared_ptr<PhotoStore> photos,
                               std::shared_ptr<accounts::AccountService> accounts, CatalogLimits limits)
    : repository_(std::move(repository)),
      photos_(std::move(photos)),
      accounts_(std::move(accounts)),
      limits_(limits) {}

std::optional<std::string> CatalogService::detectImageType(std::string_view bytes) {
    constexpr std::string_view kJpeg("\xFF\xD8\xFF", 3);
    constexpr std::string_view kPng("\x89PNG\r\n\x1A\n", 8);
    if (bytes.substr(0, kJpeg.size()) == kJpeg) return "image/jpeg";
    if (bytes.substr(0, kPng.size()) == kPng) return "image/png";
    return std::nullopt;
}

void CatalogService::validatePaging(int page, int pageSize) {
    if (page < 1 || page > 10'000 || pageSize < 1 || pageSize > kMaxPageSize) {
        throw ApiError(ErrorCode::ValidationFailed, "page must be 1 or more and pageSize 1 to 50.");
    }
}

drogon::Task<BookInput> CatalogService::validated(BookInput input, std::optional<int> defaultAreaId) {
    input.title = validText(input.title, "Title", 1, 200);
    input.author = validText(input.author, "Author", 1, 120);
    input.edition = validText(input.edition, "Edition", 0, 60);
    input.conditionNotes = validText(input.conditionNotes, "Condition notes", 0, 500);
    if (!isLanguageCode(input.language)) {
        throw ApiError(ErrorCode::ValidationFailed, "Language must be a two-letter code such as 'en'.");
    }
    if (!isGenre(input.genre)) throw ApiError(ErrorCode::ValidationFailed, "Unknown genre.");
    if (!conditionRank(input.condition)) throw ApiError(ErrorCode::ValidationFailed, "Unknown condition.");

    if (!input.exchangeAreaId) input.exchangeAreaId = defaultAreaId;
    if (input.exchangeAreaId) {
        const auto area = co_await accounts_->findMeetingArea(*input.exchangeAreaId);
        if (!area) throw ApiError(ErrorCode::ValidationFailed, "Unknown exchange area.");
    }
    co_return input;
}

drogon::Task<BookCopy> CatalogService::ownedCopy(const std::string& ownerId, const std::string& copyId) {
    auto copy = co_await repository_->findCopy(copyId);
    if (!copy || copy->ownerId != ownerId) throw ApiError(ErrorCode::NotFound, "Listing not found.");
    co_return std::move(*copy);
}

drogon::Task<std::vector<ListingView>> CatalogService::toViews(std::vector<BookCopy> copies) {
    std::vector<std::string> ownerIds;
    std::vector<std::string> copyIds;
    for (const auto& copy : copies) {
        ownerIds.push_back(copy.ownerId);
        copyIds.push_back(copy.id);
    }

    const auto owners = co_await accounts_->getOwnerSummaries(std::move(ownerIds));
    const auto areas = co_await accounts_->listMeetingAreas();
    const auto photos = co_await repository_->listPhotos(std::move(copyIds));

    std::unordered_map<std::string, std::vector<Photo>> photosByCopy;
    for (const auto& photo : photos) photosByCopy[photo.copyId].push_back(photo);

    std::vector<ListingView> views;
    views.reserve(copies.size());
    for (auto& copy : copies) {
        ListingView view;
        const auto owner = owners.find(copy.ownerId);
        view.owner = owner != owners.end() ? owner->second : accounts::OwnerSummary{copy.ownerId, "Reader"};
        if (copy.exchangeAreaId) {
            const auto area = std::find_if(areas.begin(), areas.end(),
                                           [&](const auto& candidate) { return candidate.id == *copy.exchangeAreaId; });
            if (area != areas.end()) view.exchangeArea = *area;
        }
        view.photos = std::move(photosByCopy[copy.id]);
        view.copy = std::move(copy);
        views.push_back(std::move(view));
    }
    co_return views;
}

drogon::Task<ListingView> CatalogService::toView(BookCopy copy) {
    std::vector<BookCopy> copies;
    copies.push_back(std::move(copy));
    auto views = co_await toViews(std::move(copies));
    co_return std::move(views.front());
}

drogon::Task<ListingView> CatalogService::createListing(std::string ownerId, BookInput input) {
    const auto owner = co_await accounts_->getCurrentUser(ownerId);
    const std::optional<int> ownerArea = owner.meetingArea ? std::optional<int>(owner.meetingArea->id) : std::nullopt;
    auto valid = co_await validated(std::move(input), ownerArea);
    auto copy = co_await repository_->createListing(std::move(ownerId), std::move(valid));
    co_return co_await toView(std::move(copy));
}

drogon::Task<ListingView> CatalogService::getPublicListing(std::string copyId) {
    auto copy = co_await repository_->findCopy(std::move(copyId));
    if (!copy || !isPubliclyVisible(*copy)) throw ApiError(ErrorCode::NotFound, "Listing not found.");
    co_return co_await toView(std::move(*copy));
}

drogon::Task<ListingView> CatalogService::updateListing(std::string ownerId, std::string copyId, BookInput input) {
    const auto existing = co_await ownedCopy(ownerId, copyId);
    if (!isEditable(existing)) throw ApiError(ErrorCode::Conflict, kLockedMessage);

    auto valid = co_await validated(std::move(input), existing.exchangeAreaId);
    const bool updated = co_await repository_->updateDetails(copyId, std::move(valid), kEditableStatuses);
    if (!updated) throw ApiError(ErrorCode::Conflict, kLockedMessage);
    auto refreshed = co_await ownedCopy(ownerId, copyId);
    co_return co_await toView(std::move(refreshed));
}

drogon::Task<ListingView> CatalogService::withdraw(std::string ownerId, std::string copyId) {
    co_await ownedCopy(ownerId, copyId);
    const bool changed = co_await repository_->transitionStatus(copyId, std::string(status::kAvailable),
                                                                std::string(status::kWithdrawn));
    if (!changed) throw ApiError(ErrorCode::Conflict, "Only an available listing can be withdrawn.");
    auto refreshed = co_await ownedCopy(ownerId, copyId);
    co_return co_await toView(std::move(refreshed));
}

drogon::Task<ListingView> CatalogService::relist(std::string ownerId, std::string copyId) {
    co_await ownedCopy(ownerId, copyId);
    const bool changed = co_await repository_->transitionStatus(copyId, std::string(status::kWithdrawn),
                                                                std::string(status::kAvailable));
    if (!changed) throw ApiError(ErrorCode::Conflict, "Only a withdrawn listing can be made available again.");
    auto refreshed = co_await ownedCopy(ownerId, copyId);
    co_return co_await toView(std::move(refreshed));
}

drogon::Task<void> CatalogService::deleteListing(std::string ownerId, std::string copyId) {
    co_await ownedCopy(ownerId, copyId);
    const bool deleted = co_await repository_->softDelete(copyId, kEditableStatuses);
    if (!deleted) throw ApiError(ErrorCode::Conflict, kLockedMessage);
}

drogon::Task<Page<ListingView>> CatalogService::listMine(std::string ownerId, int page, int pageSize) {
    validatePaging(page, pageSize);
    // No structured bindings across co_await: GCC 11 mis-compiles them in coroutines.
    auto result = co_await repository_->listByOwner(std::move(ownerId), page, pageSize);
    auto views = co_await toViews(std::move(result.first));
    co_return Page<ListingView>{std::move(views), page, pageSize, result.second};
}

drogon::Task<Page<ListingView>> CatalogService::search(SearchFilters filters) {
    filters.query = validText(filters.query, "Search text", 0, 100);
    filters.title = validText(filters.title, "Title", 0, 100);
    filters.author = validText(filters.author, "Author", 0, 100);
    if (!filters.genre.empty() && !isGenre(filters.genre)) {
        throw ApiError(ErrorCode::ValidationFailed, "Unknown genre.");
    }
    if (!filters.language.empty() && !isLanguageCode(filters.language)) {
        throw ApiError(ErrorCode::ValidationFailed, "Language must be a two-letter code such as 'en'.");
    }
    if (filters.minConditionRank < 0 || filters.minConditionRank > static_cast<int>(kConditions.size())) {
        throw ApiError(ErrorCode::ValidationFailed, "Unknown condition.");
    }
    if (filters.areaId < 0) throw ApiError(ErrorCode::ValidationFailed, "Unknown exchange area.");
    validatePaging(filters.page, filters.pageSize);

    const int page = filters.page;
    const int pageSize = filters.pageSize;
    auto result = co_await repository_->search(std::move(filters));
    auto views = co_await toViews(std::move(result.first));
    co_return Page<ListingView>{std::move(views), page, pageSize, result.second};
}

drogon::Task<Photo> CatalogService::addPhoto(std::string ownerId, std::string copyId, std::string bytes) {
    const auto copy = co_await ownedCopy(ownerId, copyId);
    if (!isEditable(copy)) throw ApiError(ErrorCode::Conflict, kLockedMessage);
    if (bytes.empty()) throw ApiError(ErrorCode::ValidationFailed, "Choose a photo to upload.");
    if (bytes.size() > limits_.maxPhotoBytes) throw ApiError(ErrorCode::PayloadTooLarge, "Photos must be 5 MB or smaller.");
    const auto contentType = detectImageType(bytes);
    if (!contentType) throw ApiError(ErrorCode::UnsupportedMediaType, "Photos must be JPEG or PNG images.");

    Photo photo{newUuid(), copyId, *contentType, static_cast<std::int64_t>(bytes.size())};
    photos_->save(photo.id, bytes);
    bool inserted = false;
    try {
        inserted = co_await repository_->insertPhoto(photo, limits_.maxPhotosPerCopy);
    } catch (...) {
        photos_->remove(photo.id);
        throw;
    }
    if (!inserted) {
        photos_->remove(photo.id);
        throw ApiError(ErrorCode::Conflict, "A listing can have at most 5 photos.");
    }
    co_return photo;
}

drogon::Task<void> CatalogService::deletePhoto(std::string ownerId, std::string copyId, std::string photoId) {
    const auto copy = co_await ownedCopy(ownerId, copyId);
    if (!isEditable(copy)) throw ApiError(ErrorCode::Conflict, kLockedMessage);
    // Soft delete only: the file is kept in case an exchange snapshot refers to it.
    const bool deleted = co_await repository_->softDeletePhoto(std::move(photoId), std::move(copyId));
    if (!deleted) throw ApiError(ErrorCode::NotFound, "Photo not found.");
}

drogon::Task<PhotoFile> CatalogService::getPhotoFile(std::string photoId) {
    const auto photo = co_await repository_->findPhoto(std::move(photoId));
    if (!photo) throw ApiError(ErrorCode::NotFound, "Photo not found.");
    co_return PhotoFile{photos_->pathFor(photo->id), photo->contentType};
}

}  // namespace swapcycle::catalog
