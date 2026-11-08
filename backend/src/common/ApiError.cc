#include "ApiError.h"

#include <trantor/utils/Logger.h>

#include <typeinfo>

namespace swapcycle::common {

namespace {

constexpr const char* kInternalMessage = "Something went wrong.";

}  // namespace

std::string_view codeName(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::ValidationFailed: return "VALIDATION_FAILED";
        case ErrorCode::Unauthorized: return "UNAUTHORIZED";
        case ErrorCode::Forbidden: return "FORBIDDEN";
        case ErrorCode::NotFound: return "NOT_FOUND";
        case ErrorCode::Conflict: return "CONFLICT";
        case ErrorCode::RateLimited: return "RATE_LIMITED";
        case ErrorCode::PayloadTooLarge: return "PAYLOAD_TOO_LARGE";
        case ErrorCode::UnsupportedMediaType: return "UNSUPPORTED_MEDIA_TYPE";
        case ErrorCode::Internal: return "INTERNAL";
    }
    return "INTERNAL";
}

drogon::HttpStatusCode httpStatus(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::ValidationFailed: return drogon::k400BadRequest;
        case ErrorCode::Unauthorized: return drogon::k401Unauthorized;
        case ErrorCode::Forbidden: return drogon::k403Forbidden;
        case ErrorCode::NotFound: return drogon::k404NotFound;
        case ErrorCode::Conflict: return drogon::k409Conflict;
        case ErrorCode::RateLimited: return drogon::k429TooManyRequests;
        case ErrorCode::PayloadTooLarge: return drogon::k413RequestEntityTooLarge;
        case ErrorCode::UnsupportedMediaType: return drogon::k415UnsupportedMediaType;
        case ErrorCode::Internal: return drogon::k500InternalServerError;
    }
    return drogon::k500InternalServerError;
}

ApiError::ApiError(ErrorCode code, const std::string& message) : std::runtime_error(message), code_(code) {}

Json::Value errorBody(ErrorCode code, const std::string& message) {
    Json::Value body;
    body["error"]["code"] = std::string(codeName(code));
    body["error"]["message"] = message;
    return body;
}

drogon::HttpResponsePtr dataResponse(const Json::Value& data, drogon::HttpStatusCode status) {
    Json::Value body;
    body["data"] = data;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(status);
    return resp;
}

drogon::HttpResponsePtr errorResponse(const std::exception& error) {
    ErrorCode code = ErrorCode::Internal;
    std::string message = kInternalMessage;

    if (const auto* apiError = dynamic_cast<const ApiError*>(&error)) {
        code = apiError->code();
        message = apiError->what();
        if (code == ErrorCode::Internal) LOG_ERROR << "Internal API error: " << message;
    } else {
        // Details stay in the server log only.
        LOG_ERROR << "Unhandled " << typeid(error).name() << ": " << error.what();
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(errorBody(code, message));
    resp->setStatusCode(httpStatus(code));
    return resp;
}

}  // namespace swapcycle::common
