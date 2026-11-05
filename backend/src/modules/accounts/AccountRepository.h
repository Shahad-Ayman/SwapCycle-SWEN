#pragma once

// Database access for the accounts module. The only code that touches the
// users, user_interests, meeting_areas and password_reset_tokens tables.

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "AccountTypes.h"

namespace swapcycle::accounts {

class AccountRepository {
public:
    virtual ~AccountRepository() = default;

    virtual drogon::Task<std::optional<User>> findByEmail(std::string email) = 0;
    virtual drogon::Task<std::optional<User>> findById(std::string userId) = 0;
    virtual drogon::Task<std::vector<User>> findByIds(std::vector<std::string> userIds) = 0;

    // Returns nullopt if the email is already registered.
    virtual drogon::Task<std::optional<User>> insertUser(NewUser user) = 0;

    // Applies the update (profile fields and interests) in one transaction.
    virtual drogon::Task<void> updateProfile(std::string userId, ProfileUpdate update) = 0;

    virtual drogon::Task<std::vector<MeetingArea>> listMeetingAreas() = 0;

    virtual drogon::Task<void> insertResetToken(std::string userId, std::string tokenHash,
                                                std::chrono::seconds ttl) = 0;

    // In one transaction: marks the token used if it is unused and unexpired,
    // invalidates the user's other reset tokens and stores the new hash.
    // Returns false if the token was not valid.
    virtual drogon::Task<bool> consumeResetTokenAndSetPassword(std::string tokenHash,
                                                               std::string newPasswordHash) = 0;
};

class PostgresAccountRepository final : public AccountRepository {
public:
    explicit PostgresAccountRepository(drogon::orm::DbClientPtr db);

    drogon::Task<std::optional<User>> findByEmail(std::string email) override;
    drogon::Task<std::optional<User>> findById(std::string userId) override;
    drogon::Task<std::vector<User>> findByIds(std::vector<std::string> userIds) override;
    drogon::Task<std::optional<User>> insertUser(NewUser user) override;
    drogon::Task<void> updateProfile(std::string userId, ProfileUpdate update) override;
    drogon::Task<std::vector<MeetingArea>> listMeetingAreas() override;
    drogon::Task<void> insertResetToken(std::string userId, std::string tokenHash,
                                        std::chrono::seconds ttl) override;
    drogon::Task<bool> consumeResetTokenAndSetPassword(std::string tokenHash,
                                                       std::string newPasswordHash) override;

private:
    drogon::orm::DbClientPtr db_;
};

}  // namespace swapcycle::accounts
