#pragma once

// Standard error codes and the JSON response envelope shared by every module.
//
//   success: { "data": ... }
//   failure: { "error": { "code": "...", "message": "..." } }
//
// Codes match ErrorResponse.error.code in docs/api/openapi.yaml.

#include <drogon/HttpResponse.h>
#include <json/value.h>

#include <stdexcept>
#include <string>
#include <string_view>

namespace swapcycle::common {

enum class ErrorCode {
    ValidationFailed,
    Unauthorized,
    Forbidden,
    NotFound,
    Conflict,
    RateLimited,
    PayloadTooLarge,
    UnsupportedMediaType,
    Internal,
};

// The wire name, e.g. ErrorCode::NotFound -> "NOT_FOUND".
std::string_view codeName(ErrorCode code) noexcept;

// The HTTP status a code is sent with, e.g. ErrorCode::NotFound -> 404.
drogon::HttpStatusCode httpStatus(ErrorCode code) noexcept;

// An error that is safe to show the reader. what() is the reader-facing message,
// so never put SQL, file paths, tokens or other internals in it.
class ApiError : public std::runtime_error {
public:
    ApiError(ErrorCode code, const std::string& message);

    ErrorCode code() const noexcept { return code_; }

private:
    ErrorCode code_;
};

// { "data": data } with the given status (200 by default).
drogon::HttpResponsePtr dataResponse(const Json::Value& data, drogon::HttpStatusCode status = drogon::k200OK);

// { "error": { code, message } } with the matching status.
// An ApiError keeps its code and message. Any other exception is logged
// server-side and becomes 500 INTERNAL "Something went wrong.", so stack
// traces, SQL errors and file paths never reach the client.
drogon::HttpResponsePtr errorResponse(const std::exception& error);

// The JSON body errorResponse would send, without the HTTP wrapper.
Json::Value errorBody(ErrorCode code, const std::string& message);

}  // namespace swapcycle::common
