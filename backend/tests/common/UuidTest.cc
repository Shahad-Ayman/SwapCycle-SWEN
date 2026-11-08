#include <gtest/gtest.h>

#include <sodium.h>

#include <set>
#include <string>

#include "common/Uuid.h"

namespace swapcycle::common {
namespace {

TEST(UuidTest, GeneratesCanonicalVersion4) {
    ASSERT_GE(sodium_init(), 0);
    const auto id = generateUuid();
    EXPECT_EQ(id.size(), 36u);
    EXPECT_TRUE(isValidUuid(id)) << id;
    EXPECT_EQ(id[14], '4');                                     // version
    EXPECT_NE(std::string("89ab").find(id[19]), std::string::npos);  // variant
    EXPECT_EQ(id.find_first_of("ABCDEF"), std::string::npos);   // lowercase
}

TEST(UuidTest, GeneratedIdsAreUnique) {
    std::set<std::string> seen;
    for (int i = 0; i < 1000; ++i) seen.insert(generateUuid());
    EXPECT_EQ(seen.size(), 1000u);
}

TEST(UuidTest, ValidatesCanonicalFormOnly) {
    EXPECT_TRUE(isValidUuid("3f2b8c1e-9a4d-4e6f-8b2a-1c3d5e7f9a0b"));
    EXPECT_TRUE(isValidUuid("3F2B8C1E-9A4D-4E6F-8B2A-1C3D5E7F9A0B"));

    EXPECT_FALSE(isValidUuid(""));
    EXPECT_FALSE(isValidUuid("3f2b8c1e9a4d4e6f8b2a1c3d5e7f9a0b"));       // no dashes
    EXPECT_FALSE(isValidUuid("{3f2b8c1e-9a4d-4e6f-8b2a-1c3d5e7f9a0b}"));  // braces
    EXPECT_FALSE(isValidUuid("3f2b8c1e-9a4d-4e6f-8b2a-1c3d5e7f9a0g"));   // not hex
    EXPECT_FALSE(isValidUuid("3f2b8c1e-9a4d-4e6f-8b2a-1c3d5e7f9a0b1"));  // too long
    EXPECT_FALSE(isValidUuid("1' OR '1'='1"));
}

}  // namespace
}  // namespace swapcycle::common
