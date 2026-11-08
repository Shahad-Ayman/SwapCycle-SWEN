#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "common/ApiError.h"

namespace swapcycle::common {
namespace {

Json::Value bodyOf(const drogon::HttpResponsePtr& resp) {
    const auto json = resp->jsonObject();
    return json ? *json : Json::Value();
}

TEST(ApiErrorTest, EveryCodeHasTheSpecNameAndStatus) {
    const std::vector<std::pair<ErrorCode, std::pair<std::string, drogon::HttpStatusCode>>> expected = {
        {ErrorCode::ValidationFailed, {"VALIDATION_FAILED", drogon::k400BadRequest}},
        {ErrorCode::Unauthorized, {"UNAUTHORIZED", drogon::k401Unauthorized}},
        {ErrorCode::Forbidden, {"FORBIDDEN", drogon::k403Forbidden}},
        {ErrorCode::NotFound, {"NOT_FOUND", drogon::k404NotFound}},
        {ErrorCode::Conflict, {"CONFLICT", drogon::k409Conflict}},
        {ErrorCode::RateLimited, {"RATE_LIMITED", drogon::k429TooManyRequests}},
        {ErrorCode::PayloadTooLarge, {"PAYLOAD_TOO_LARGE", drogon::k413RequestEntityTooLarge}},
        {ErrorCode::UnsupportedMediaType, {"UNSUPPORTED_MEDIA_TYPE", drogon::k415UnsupportedMediaType}},
        {ErrorCode::Internal, {"INTERNAL", drogon::k500InternalServerError}},
    };
    for (const auto& [code, nameAndStatus] : expected) {
        EXPECT_EQ(codeName(code), nameAndStatus.first);
        EXPECT_EQ(httpStatus(code), nameAndStatus.second);
    }
}

TEST(ApiErrorTest, KeepsCodeAndReaderMessage) {
    const ApiError error(ErrorCode::Conflict, "This book is reserved.");
    EXPECT_EQ(error.code(), ErrorCode::Conflict);
    EXPECT_STREQ(error.what(), "This book is reserved.");
}

TEST(ApiErrorTest, DataResponseWrapsInDataEnvelope) {
    Json::Value book;
    book["title"] = "Emma";
    const auto resp = dataResponse(book, drogon::k201Created);

    EXPECT_EQ(resp->statusCode(), drogon::k201Created);
    EXPECT_EQ(resp->contentType(), drogon::CT_APPLICATION_JSON);
    const auto body = bodyOf(resp);
    EXPECT_EQ(body["data"]["title"].asString(), "Emma");
    EXPECT_FALSE(body.isMember("error"));
}

TEST(ApiErrorTest, DataResponseDefaultsTo200) {
    EXPECT_EQ(dataResponse(Json::Value(Json::arrayValue))->statusCode(), drogon::k200OK);
}

TEST(ApiErrorTest, ApiErrorBecomesErrorEnvelope) {
    const auto resp = errorResponse(ApiError(ErrorCode::NotFound, "Book not found."));

    EXPECT_EQ(resp->statusCode(), drogon::k404NotFound);
    const auto body = bodyOf(resp);
    EXPECT_EQ(body["error"]["code"].asString(), "NOT_FOUND");
    EXPECT_EQ(body["error"]["message"].asString(), "Book not found.");
    EXPECT_FALSE(body.isMember("data"));
}

TEST(ApiErrorTest, UnknownExceptionsNeverLeakDetails) {
    const std::runtime_error leaky(
        "ERROR: duplicate key value violates unique constraint \"users_email_key\" at /src/db/Repo.cc:42");
    const auto resp = errorResponse(leaky);

    EXPECT_EQ(resp->statusCode(), drogon::k500InternalServerError);
    const auto body = bodyOf(resp);
    EXPECT_EQ(body["error"]["code"].asString(), "INTERNAL");
    EXPECT_EQ(body["error"]["message"].asString(), "Something went wrong.");
    const std::string raw(resp->body());
    EXPECT_EQ(raw.find("users_email_key"), std::string::npos);
    EXPECT_EQ(raw.find("Repo.cc"), std::string::npos);
}

TEST(ApiErrorTest, ErrorBodyMatchesTheSpecShape) {
    const auto body = errorBody(ErrorCode::RateLimited, "Too many attempts.");
    ASSERT_TRUE(body["error"].isObject());
    EXPECT_EQ(body["error"].size(), 2u);
    EXPECT_EQ(body["error"]["code"].asString(), "RATE_LIMITED");
}

}  // namespace
}  // namespace swapcycle::common
