#include <gtest/gtest.h>

#include <drogon/HttpRequest.h>

#include <string>

#include "common/ApiError.h"
#include "common/JsonUtils.h"

namespace swapcycle::common {
namespace {

// Runs `fn` and checks it throws ApiError VALIDATION_FAILED mentioning `needle`.
template <typename Fn>
void expectInvalid(Fn&& fn, const std::string& needle) {
    try {
        fn();
        FAIL() << "expected VALIDATION_FAILED";
    } catch (const ApiError& error) {
        EXPECT_EQ(error.code(), ErrorCode::ValidationFailed);
        EXPECT_NE(std::string(error.what()).find(needle), std::string::npos) << error.what();
    }
}

Json::Value object(const std::string& field, const Json::Value& value) {
    Json::Value json(Json::objectValue);
    json[field] = value;
    return json;
}

// ---- text ------------------------------------------------------------------

TEST(JsonUtilsTest, Utf8LengthCountsCharactersNotBytes) {
    EXPECT_EQ(utf8Length("Noor"), 4u);
    EXPECT_EQ(utf8Length("caf\xC3\xA9"), 4u);         // café
    EXPECT_EQ(utf8Length("\xD9\x86\xD9\x88\xD8\xB1"), 3u);  // نور
    EXPECT_EQ(utf8Length("\xF0\x9F\x93\x9A"), 1u);    // 📚
    EXPECT_EQ(utf8Length(""), 0u);
}

TEST(JsonUtilsTest, Utf8LengthRejectsMalformedBytes) {
    EXPECT_FALSE(utf8Length("\xC3"));              // truncated
    EXPECT_FALSE(utf8Length("\xC0\xAF"));          // overlong '/'
    EXPECT_FALSE(utf8Length("\xED\xA0\x80"));      // UTF-16 surrogate
    EXPECT_FALSE(utf8Length("\xF4\x90\x80\x80"));  // past U+10FFFF
    EXPECT_FALSE(utf8Length("\x80"));              // stray continuation byte
    EXPECT_FALSE(utf8Length("ok\xE2\x82"));        // truncated at the end
}

TEST(JsonUtilsTest, TrimRemovesOuterWhitespaceOnly) {
    EXPECT_EQ(trim("  The Hobbit \n"), "The Hobbit");
    EXPECT_EQ(trim("   "), "");
    EXPECT_EQ(trim("a  b"), "a  b");
}

// ---- strings ---------------------------------------------------------------

TEST(JsonUtilsTest, RequireStringTrimsAndReturns) {
    EXPECT_EQ(requireString(object("title", "  Emma "), "title"), "Emma");
}

TEST(JsonUtilsTest, RequireStringRejectsMissingWrongTypeAndBlank) {
    expectInvalid([] { requireString(Json::Value(Json::objectValue), "title"); }, "'title' is required");
    expectInvalid([] { requireString(object("title", 42), "title"); }, "must be a string");
    expectInvalid([] { requireString(object("title", "   "), "title"); }, "'title' is required");
}

TEST(JsonUtilsTest, RequireStringEnforcesLengthInCharacters) {
    TextRule rule;
    rule.maxChars = 4;
    EXPECT_EQ(requireString(object("name", "caf\xC3\xA9"), "name", rule), "caf\xC3\xA9");  // 5 bytes, 4 chars
    expectInvalid([&] { requireString(object("name", "cafes"), "name", rule); }, "at most 4");

    rule.minChars = 3;
    expectInvalid([&] { requireString(object("name", "ab"), "name", rule); }, "at least 3");
}

TEST(JsonUtilsTest, RequireStringRejectsControlCharactersUnlessNewlinesAllowed) {
    expectInvalid([] { requireString(object("title", "a\x01" "b"), "title"); }, "invalid characters");
    expectInvalid([] { requireString(object("title", "line1\nline2"), "title"); }, "invalid characters");

    TextRule multiline;
    multiline.allowNewlines = true;
    multiline.maxChars = 2000;
    EXPECT_EQ(requireString(object("message", "line1\nline2"), "message", multiline), "line1\nline2");
    expectInvalid([&] { requireString(object("message", "bell\x07"), "message", multiline); }, "invalid characters");
}

TEST(JsonUtilsTest, RequireStringRejectsInvalidUtf8) {
    expectInvalid([] { requireString(object("title", "\xC0\xAF"), "title"); }, "not valid text");
}

TEST(JsonUtilsTest, OptionalStringTreatsAbsentAndNullAsEmpty) {
    EXPECT_FALSE(optionalString(Json::Value(Json::objectValue), "edition"));
    EXPECT_FALSE(optionalString(object("edition", Json::Value()), "edition"));
    EXPECT_EQ(optionalString(object("edition", "2nd"), "edition"), "2nd");
    expectInvalid([] { optionalString(object("edition", 2), "edition"); }, "must be a string");
}

// ---- numbers, booleans, enums ----------------------------------------------

TEST(JsonUtilsTest, RequireIntChecksTypeAndRange) {
    EXPECT_EQ(requireInt(object("rating", 5), "rating", 1, 5), 5);
    expectInvalid([] { requireInt(object("rating", 6), "rating", 1, 5); }, "from 1 to 5");
    expectInvalid([] { requireInt(object("rating", 4.5), "rating", 1, 5); }, "whole number");
    expectInvalid([] { requireInt(object("rating", "5"), "rating", 1, 5); }, "whole number");
    expectInvalid([] { requireInt(object("rating", true), "rating", 1, 5); }, "whole number");
    expectInvalid([] { requireInt(Json::Value(Json::objectValue), "rating", 1, 5); }, "is required");
}

TEST(JsonUtilsTest, OptionalIntAndBool) {
    EXPECT_FALSE(optionalInt(Json::Value(Json::objectValue), "areaId", 1, 100));
    EXPECT_EQ(optionalInt(object("areaId", 3), "areaId", 1, 100), 3);
    EXPECT_EQ(optionalBool(object("showArea", false), "showArea"), false);
    EXPECT_FALSE(optionalBool(Json::Value(Json::objectValue), "showArea"));
    expectInvalid([] { optionalBool(object("showArea", "yes"), "showArea"); }, "true or false");
}

TEST(JsonUtilsTest, RequireEnumAcceptsOnlyListedValues) {
    EXPECT_EQ(requireEnum(object("condition", "GOOD"), "condition", {"NEW", "LIKE_NEW", "GOOD", "FAIR", "POOR"}),
              "GOOD");
    expectInvalid([] { requireEnum(object("condition", "good"), "condition", {"NEW", "GOOD"}); },
                  "one of: NEW, GOOD");
}

TEST(JsonUtilsTest, NonObjectInputIsAClientError) {
    expectInvalid([] { requireString(Json::Value(Json::arrayValue), "title"); }, "JSON object");
    expectInvalid([] { optionalBool(Json::Value("text"), "flag"); }, "JSON object");
}

// ---- request bodies --------------------------------------------------------

TEST(JsonUtilsTest, RequireObjectBodyAcceptsObjectsOnly) {
    Json::Value book(Json::objectValue);
    book["title"] = "Emma";
    EXPECT_EQ(requireObjectBody(drogon::HttpRequest::newHttpJsonRequest(book))["title"].asString(), "Emma");

    expectInvalid([] { requireObjectBody(drogon::HttpRequest::newHttpJsonRequest(Json::Value(Json::arrayValue))); },
                  "JSON object body");
    expectInvalid([] { requireObjectBody(drogon::HttpRequest::newHttpRequest()); }, "JSON object body");
}

// ---- pagination ------------------------------------------------------------

TEST(JsonUtilsTest, PaginationDefaultsMatchTheSpec) {
    const auto page = parsePagination("", "");
    EXPECT_EQ(page.page, 1);
    EXPECT_EQ(page.pageSize, 20);
    EXPECT_EQ(page.offset(), 0);
}

TEST(JsonUtilsTest, PaginationReadsQueryParameters) {
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setParameter("page", "3");
    req->setParameter("pageSize", "50");
    const auto page = parsePagination(req);
    EXPECT_EQ(page.page, 3);
    EXPECT_EQ(page.pageSize, 50);
    EXPECT_EQ(page.offset(), 100);
}

TEST(JsonUtilsTest, PaginationRejectsOutOfRangeOrMalformedValues) {
    for (const char* bad : {"0", "-1", "abc", "1.5", " 2", "2 ", "99999999999999999999"}) {
        expectInvalid([&] { parsePagination(bad, ""); }, "'page'");
    }
    for (const char* bad : {"0", "51", "x"}) {
        expectInvalid([&] { parsePagination("", bad); }, "'pageSize'");
    }
}

TEST(JsonUtilsTest, PageJsonHasTheSpecFields) {
    Json::Value items(Json::arrayValue);
    items.append("a");
    const auto json = pageJson(items, Pagination{2, 10}, 11);
    EXPECT_EQ(json["items"].size(), 1u);
    EXPECT_EQ(json["page"].asInt(), 2);
    EXPECT_EQ(json["pageSize"].asInt(), 10);
    EXPECT_EQ(json["total"].asInt64(), 11);

    EXPECT_TRUE(pageJson(Json::Value(), Pagination{}, 0)["items"].isArray());
}

}  // namespace
}  // namespace swapcycle::common
