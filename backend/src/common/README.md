# backend/src/common

Shared helpers every module uses. No business rules and no database access live here.

| File | What it gives you |
|---|---|
| `ApiError.h` | `ErrorCode` (the spec's 9 codes), `ApiError`, `dataResponse` → `{ "data": ... }`, `errorResponse` → `{ "error": { code, message } }` with the right status. Any non-`ApiError` exception becomes a generic 500 and is only logged server-side. |
| `JsonUtils.h` | Server-side input checks that throw `VALIDATION_FAILED`: `requireObjectBody`, `requireString`/`optionalString` (trimmed, length in characters, valid UTF-8, no control characters), `requireInt`/`optionalInt`, `optionalBool`, `requireEnum`. Pagination: `parsePagination` (page ≥ 1, pageSize 1–50, default 20) and `pageJson` → `{ items, page, pageSize, total }`. |
| `Config.h` | `loadAppConfig()` reads and checks every variable in `.env.example` at startup and reports all problems at once, never printing secret values. Lower-level `envOr`, `requireEnv`, `intEnv`, `boolEnv`. |
| `Uuid.h` | `generateUuid()` (random v4 from libsodium) and `isValidUuid()` for path parameters. |

## Usage

```cpp
#include "common/ApiError.h"
#include "common/JsonUtils.h"

try {
    const auto& body = common::requireObjectBody(req);
    auto title = common::requireString(body, "title", {.maxChars = 200});
    auto condition = common::requireEnum(body, "condition", {"NEW", "LIKE_NEW", "GOOD", "FAIR", "POOR"});
    auto page = common::parsePagination(req);
    // ... call the service ...
    co_return common::dataResponse(common::pageJson(items, page, total));
} catch (const std::exception& error) {
    co_return common::errorResponse(error);
}
```

Never put SQL, file paths, tokens or handover codes in an `ApiError` message. Those messages go to the reader.

## Build and test

From the repository root:

```bash
docker build backend -f backend/DockerFile --target build --build-arg WARNINGS_AS_ERRORS=ON
```

This compiles everything with `-Werror` and runs every GoogleTest suite, including `tests/common/`.
To run only these tests inside the build image:

```bash
docker run --rm sc-build ./build/swapcycle_tests --gtest_filter='ApiErrorTest.*:JsonUtilsTest.*:ConfigTest.*:UuidTest.*'
```

(after `docker build ... -t sc-build`).
