#include <gtest/gtest.h>

#include "modules/accounts/AccountSecurity.h"
#include "modules/accounts/RateLimiter.h"

namespace swapcycle::accounts {
namespace {

const std::string kSecret = "test-secret-that-is-definitely-32-bytes-long";
constexpr std::int64_t kNow = 1'800'000'000;

class AccountSecurityTest : public ::testing::Test {
protected:
    void SetUp() override { ASSERT_TRUE(security::initialise()); }
};

TEST_F(AccountSecurityTest, HashesWithArgon2idAndVerifies) {
    const auto hash = security::hashPassword("correct horse");
    EXPECT_EQ(hash.rfind("$argon2id$", 0), 0u);
    EXPECT_EQ(hash.find("correct horse"), std::string::npos);
    EXPECT_TRUE(security::verifyPassword(hash, "correct horse"));
    EXPECT_FALSE(security::verifyPassword(hash, "wrong horse"));
}

TEST_F(AccountSecurityTest, SamePasswordGetsDifferentSalts) {
    EXPECT_NE(security::hashPassword("same password"), security::hashPassword("same password"));
}

TEST_F(AccountSecurityTest, SessionTokenRoundTrips) {
    const auto token = security::issueSessionToken({"user-1", Role::Moderator, kNow + 60}, kSecret);
    const auto claims = security::verifySessionToken(token, kSecret, kNow);
    ASSERT_TRUE(claims);
    EXPECT_EQ(claims->userId, "user-1");
    EXPECT_EQ(claims->role, Role::Moderator);
}

TEST_F(AccountSecurityTest, RejectsExpiredToken) {
    const auto token = security::issueSessionToken({"user-1", Role::Reader, kNow}, kSecret);
    EXPECT_FALSE(security::verifySessionToken(token, kSecret, kNow));
}

TEST_F(AccountSecurityTest, RejectsWrongSecret) {
    const auto token = security::issueSessionToken({"user-1", Role::Reader, kNow + 60}, kSecret);
    EXPECT_FALSE(security::verifySessionToken(token, kSecret + "x", kNow));
}

TEST_F(AccountSecurityTest, RejectsTamperedPayload) {
    // Forge a moderator payload but keep the reader token's signature.
    const auto reader = security::issueSessionToken({"user-1", Role::Reader, kNow + 60}, kSecret);
    const auto moderator = security::issueSessionToken({"user-1", Role::Moderator, kNow + 60}, kSecret);
    const auto forged = moderator.substr(0, moderator.find('.')) + reader.substr(reader.find('.'));
    EXPECT_FALSE(security::verifySessionToken(forged, kSecret, kNow));
}

TEST_F(AccountSecurityTest, RejectsMalformedTokens) {
    EXPECT_FALSE(security::verifySessionToken("", kSecret, kNow));
    EXPECT_FALSE(security::verifySessionToken("no-dot", kSecret, kNow));
    EXPECT_FALSE(security::verifySessionToken("a.b", kSecret, kNow));
}

TEST_F(AccountSecurityTest, RefusesShortSecret) {
    EXPECT_THROW(security::issueSessionToken({"user-1", Role::Reader, kNow + 60}, "short"), std::invalid_argument);
}

TEST_F(AccountSecurityTest, RandomTokensAreUniqueAndHashIsStable) {
    const auto a = security::randomToken();
    EXPECT_NE(a, security::randomToken());
    EXPECT_GE(a.size(), 40u);
    EXPECT_EQ(security::sha256Hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(RateLimiterTest, BlocksAfterLimitUntilWindowEnds) {
    RateLimiter limiter(2, std::chrono::seconds(60));
    const auto start = RateLimiter::Clock::now();
    EXPECT_EQ(limiter.hit("ip", start).count(), 0);
    EXPECT_EQ(limiter.hit("ip", start).count(), 0);
    EXPECT_EQ(limiter.hit("ip", start + std::chrono::seconds(10)).count(), 50);
    EXPECT_EQ(limiter.hit("other", start).count(), 0);
    EXPECT_EQ(limiter.hit("ip", start + std::chrono::seconds(60)).count(), 0);
}

TEST(RateLimiterTest, ResetClearsKey) {
    RateLimiter limiter(1, std::chrono::seconds(60));
    EXPECT_EQ(limiter.hit("email").count(), 0);
    EXPECT_GT(limiter.hit("email").count(), 0);
    limiter.reset("email");
    EXPECT_EQ(limiter.hit("email").count(), 0);
}

}  // namespace
}  // namespace swapcycle::accounts
