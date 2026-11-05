#include <gtest/gtest.h>

#include "modules/accounts/AccountSecurity.h"
#include "modules/catalog/CatalogService.h"
#include "support/FakeRepositories.h"

namespace swapcycle::catalog {
namespace {

using common::ErrorCode;
using swapcycle::testing::expectApiError;

const std::string kJpeg = std::string("\xFF\xD8\xFF\xE0", 4) + "jpeg body";
const std::string kPng = std::string("\x89PNG\r\n\x1A\n", 8) + "png body";

BookInput mysteryNovel() {
    BookInput input;
    input.title = "  The Hound of the Baskervilles ";
    input.author = "Arthur Conan Doyle";
    input.language = "en";
    input.genre = "MYSTERY";
    input.condition = "GOOD";
    return input;
}

class CatalogServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(accounts::security::initialise());
        accountRepository = std::make_shared<swapcycle::testing::FakeAccountRepository>();
        accountService = std::make_shared<accounts::AccountService>(
            accountRepository, accounts::AccountSettings{"test-secret-that-is-definitely-32-bytes-long"});
        repository = std::make_shared<swapcycle::testing::FakeCatalogRepository>();
        photos = std::make_shared<swapcycle::testing::FakePhotoStore>();
        service = std::make_shared<CatalogService>(repository, photos, accountService);

        sara = drogon::sync_wait(accountService->registerUser("sara@example.com", "password123", "Sara")).user.id;
        ali = drogon::sync_wait(accountService->registerUser("ali@example.com", "password123", "Ali")).user.id;
        accounts::ProfileUpdate area;
        area.meetingAreaId = std::optional<int>{1};
        drogon::sync_wait(accountService->updateProfile(sara, area));
    }

    std::string createSaraListing() { return drogon::sync_wait(service->createListing(sara, mysteryNovel())).copy.id; }

    std::shared_ptr<swapcycle::testing::FakeAccountRepository> accountRepository;
    std::shared_ptr<accounts::AccountService> accountService;
    std::shared_ptr<swapcycle::testing::FakeCatalogRepository> repository;
    std::shared_ptr<swapcycle::testing::FakePhotoStore> photos;
    std::shared_ptr<CatalogService> service;
    std::string sara;
    std::string ali;
};

TEST_F(CatalogServiceTest, CreateTrimsInputAndDefaultsToOwnersArea) {
    const auto listing = drogon::sync_wait(service->createListing(sara, mysteryNovel()));
    EXPECT_EQ(listing.copy.title, "The Hound of the Baskervilles");
    EXPECT_EQ(listing.copy.status, "AVAILABLE");
    ASSERT_TRUE(listing.exchangeArea);
    EXPECT_EQ(listing.exchangeArea->name, "Main Library");
    EXPECT_EQ(listing.owner.displayName, "Sara");
    EXPECT_FALSE(toJson(listing)["owner"].isMember("email"));
}

TEST_F(CatalogServiceTest, CreateRejectsInvalidValues) {
    auto badGenre = mysteryNovel();
    badGenre.genre = "COOKING";
    expectApiError(service->createListing(sara, badGenre), ErrorCode::ValidationFailed);

    auto badLanguage = mysteryNovel();
    badLanguage.language = "English";
    expectApiError(service->createListing(sara, badLanguage), ErrorCode::ValidationFailed);

    auto badCondition = mysteryNovel();
    badCondition.condition = "MINT";
    expectApiError(service->createListing(sara, badCondition), ErrorCode::ValidationFailed);

    auto noTitle = mysteryNovel();
    noTitle.title = "   ";
    expectApiError(service->createListing(sara, noTitle), ErrorCode::ValidationFailed);

    auto badArea = mysteryNovel();
    badArea.exchangeAreaId = 42;
    expectApiError(service->createListing(sara, badArea), ErrorCode::ValidationFailed);

    auto controlChars = mysteryNovel();
    controlChars.author = std::string("Doyle\x07", 6);
    expectApiError(service->createListing(sara, controlChars), ErrorCode::ValidationFailed);
}

TEST_F(CatalogServiceTest, OnlyTheOwnerCanChangeAListing) {
    const auto id = createSaraListing();
    expectApiError(service->updateListing(ali, id, mysteryNovel()), ErrorCode::NotFound);
    expectApiError(service->withdraw(ali, id), ErrorCode::NotFound);
    expectApiError(service->deleteListing(ali, id), ErrorCode::NotFound);
    expectApiError(service->addPhoto(ali, id, kJpeg), ErrorCode::NotFound);
}

TEST_F(CatalogServiceTest, ReservedCopyIsLocked) {
    const auto id = createSaraListing();
    repository->copies.at(id).status = "RESERVED";
    expectApiError(service->updateListing(sara, id, mysteryNovel()), ErrorCode::Conflict);
    expectApiError(service->deleteListing(sara, id), ErrorCode::Conflict);
    expectApiError(service->withdraw(sara, id), ErrorCode::Conflict);
    expectApiError(service->addPhoto(sara, id, kJpeg), ErrorCode::Conflict);
}

TEST_F(CatalogServiceTest, WithdrawAndRelist) {
    const auto id = createSaraListing();
    EXPECT_EQ(drogon::sync_wait(service->withdraw(sara, id)).copy.status, "WITHDRAWN");
    expectApiError(service->getPublicListing(id), ErrorCode::NotFound);
    expectApiError(service->withdraw(sara, id), ErrorCode::Conflict);
    EXPECT_EQ(drogon::sync_wait(service->relist(sara, id)).copy.status, "AVAILABLE");
    EXPECT_EQ(drogon::sync_wait(service->getPublicListing(id)).copy.id, id);
}

TEST_F(CatalogServiceTest, DeletedListingDisappears) {
    const auto id = createSaraListing();
    drogon::sync_wait(service->deleteListing(sara, id));
    expectApiError(service->getPublicListing(id), ErrorCode::NotFound);
    EXPECT_EQ(drogon::sync_wait(service->listMine(sara, 1, 20)).total, 0);
}

TEST_F(CatalogServiceTest, DetectsImageTypeFromMagicBytes) {
    EXPECT_EQ(CatalogService::detectImageType(kJpeg), "image/jpeg");
    EXPECT_EQ(CatalogService::detectImageType(kPng), "image/png");
    EXPECT_FALSE(CatalogService::detectImageType("GIF89a..."));
    EXPECT_FALSE(CatalogService::detectImageType("<svg onload=alert(1)>"));
    EXPECT_FALSE(CatalogService::detectImageType(""));
}

TEST_F(CatalogServiceTest, PhotoRules) {
    const auto id = createSaraListing();
    expectApiError(service->addPhoto(sara, id, "<?php echo 1; ?>"), ErrorCode::UnsupportedMediaType);
    expectApiError(service->addPhoto(sara, id, kJpeg + std::string(5 * 1024 * 1024, 'x')),
                   ErrorCode::PayloadTooLarge);
    expectApiError(service->addPhoto(sara, id, ""), ErrorCode::ValidationFailed);

    for (int i = 0; i < 5; ++i) drogon::sync_wait(service->addPhoto(sara, id, i % 2 ? kPng : kJpeg));
    EXPECT_EQ(photos->files.size(), 5u);
    expectApiError(service->addPhoto(sara, id, kJpeg), ErrorCode::Conflict);
    EXPECT_EQ(photos->files.size(), 5u) << "rejected upload must not leave a file behind";

    const auto listing = drogon::sync_wait(service->getPublicListing(id));
    ASSERT_EQ(listing.photos.size(), 5u);
    EXPECT_EQ(toJson(listing)["photos"][0]["url"].asString(), "/api/photos/" + listing.photos[0].id);
}

TEST_F(CatalogServiceTest, PhotoIdsAreRandomUuids) {
    const auto id = createSaraListing();
    const auto a = drogon::sync_wait(service->addPhoto(sara, id, kJpeg));
    const auto b = drogon::sync_wait(service->addPhoto(sara, id, kJpeg));
    EXPECT_NE(a.id, b.id);
    EXPECT_EQ(a.id.size(), 36u);
    EXPECT_EQ(a.id[14], '4');
    EXPECT_EQ(drogon::sync_wait(service->getPhotoFile(a.id)).path, "/uploads/" + a.id);
}

TEST_F(CatalogServiceTest, DeletePhotoOnlyOnOwnCopy) {
    const auto id = createSaraListing();
    const auto photo = drogon::sync_wait(service->addPhoto(sara, id, kPng));
    expectApiError(service->deletePhoto(ali, id, photo.id), ErrorCode::NotFound);
    drogon::sync_wait(service->deletePhoto(sara, id, photo.id));
    expectApiError(service->getPhotoFile(photo.id), ErrorCode::NotFound);
}

TEST_F(CatalogServiceTest, SearchValidatesFiltersAndPaging) {
    SearchFilters badGenre;
    badGenre.genre = "COOKING";
    expectApiError(service->search(badGenre), ErrorCode::ValidationFailed);

    SearchFilters badPage;
    badPage.pageSize = 51;
    expectApiError(service->search(badPage), ErrorCode::ValidationFailed);

    SearchFilters longQuery;
    longQuery.query = std::string(101, 'a');
    expectApiError(service->search(longQuery), ErrorCode::ValidationFailed);

    createSaraListing();
    SearchFilters ok;
    ok.query = "  hound ";
    const auto page = drogon::sync_wait(service->search(ok));
    EXPECT_EQ(page.total, 1);
    EXPECT_EQ(repository->lastSearch.query, "hound");
}

}  // namespace
}  // namespace swapcycle::catalog
