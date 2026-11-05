#include "AccountTypes.h"

#include <algorithm>

#include "common/ApiError.h"

namespace swapcycle::accounts {

namespace {

[[noreturn]] void invalidField(const std::string& field) {
    throw common::ApiError(common::ErrorCode::ValidationFailed, "Field '" + field + "' has the wrong type.");
}

Json::Value toJson(const std::optional<MeetingArea>& area) {
    return area ? toJson(*area) : Json::Value(Json::nullValue);
}

Json::Value toJson(const std::vector<std::string>& values) {
    Json::Value array(Json::arrayValue);
    for (const auto& value : values) array.append(value);
    return array;
}

}  // namespace

bool isInterestCategory(std::string_view value) {
    return std::find(kInterestCategories.begin(), kInterestCategories.end(), value) != kInterestCategories.end();
}

std::string_view toString(Role role) {
    return role == Role::Moderator ? "MODERATOR" : "READER";
}

std::optional<Role> roleFromString(std::string_view value) {
    if (value == "READER") return Role::Reader;
    if (value == "MODERATOR") return Role::Moderator;
    return std::nullopt;
}

PublicProfile toPublicProfile(const User& user) {
    PublicProfile profile;
    profile.id = user.id;
    profile.displayName = user.displayName;
    if (user.privacy.showMeetingArea) profile.meetingArea = user.meetingArea;
    if (user.privacy.showInterests) profile.interests = user.interests;
    profile.memberSince = user.createdAt;
    return profile;
}

Json::Value toJson(const MeetingArea& area) {
    Json::Value json;
    json["id"] = area.id;
    json["name"] = area.name;
    return json;
}

Json::Value toJson(const PublicProfile& profile) {
    Json::Value json;
    json["id"] = profile.id;
    json["displayName"] = profile.displayName;
    json["meetingArea"] = toJson(profile.meetingArea);
    json["interests"] = profile.interests ? toJson(*profile.interests) : Json::Value(Json::nullValue);
    json["memberSince"] = profile.memberSince;
    return json;
}

Json::Value toCurrentUserJson(const User& user) {
    Json::Value json;
    json["id"] = user.id;
    json["email"] = user.email;
    json["displayName"] = user.displayName;
    json["role"] = std::string(toString(user.role));
    json["meetingArea"] = toJson(user.meetingArea);
    json["interests"] = toJson(user.interests);
    json["privacy"]["showMeetingArea"] = user.privacy.showMeetingArea;
    json["privacy"]["showInterests"] = user.privacy.showInterests;
    json["onboardingCompleted"] = user.onboardingCompleted;
    return json;
}

ProfileUpdate parseProfileUpdate(const Json::Value& body) {
    if (!body.isObject()) invalidField("body");
    ProfileUpdate update;

    if (body.isMember("displayName")) {
        if (!body["displayName"].isString()) invalidField("displayName");
        update.displayName = body["displayName"].asString();
    }
    if (body.isMember("meetingAreaId")) {
        const auto& area = body["meetingAreaId"];
        if (area.isNull()) {
            update.meetingAreaId = std::optional<int>{};
        } else if (area.isInt()) {
            update.meetingAreaId = std::optional<int>{area.asInt()};
        } else {
            invalidField("meetingAreaId");
        }
    }
    if (body.isMember("interests")) {
        const auto& interests = body["interests"];
        if (!interests.isArray()) invalidField("interests");
        std::vector<std::string> values;
        for (const auto& item : interests) {
            if (!item.isString()) invalidField("interests");
            values.push_back(item.asString());
        }
        update.interests = std::move(values);
    }
    if (body.isMember("privacy")) {
        const auto& privacy = body["privacy"];
        if (!privacy.isObject() || !privacy["showMeetingArea"].isBool() || !privacy["showInterests"].isBool()) {
            invalidField("privacy");
        }
        update.privacy = PrivacySettings{privacy["showMeetingArea"].asBool(), privacy["showInterests"].asBool()};
    }
    if (body.isMember("onboardingCompleted")) {
        if (!body["onboardingCompleted"].isBool()) invalidField("onboardingCompleted");
        update.onboardingCompleted = body["onboardingCompleted"].asBool();
    }
    return update;
}

}  // namespace swapcycle::accounts
