# Sprint 1 handoff: accounts, catalog, wish list & search

Branch: `feature/sprint1-accounts-catalog-wishlist`

## What is built

**Backend** (`backend/src/modules/{accounts,catalog,wishlist_search}`):
argon2id passwords (libsodium), HMAC-signed session token in an httpOnly,
SameSite=Lax cookie `sc_session`, generic login errors with a dummy hash for
unknown emails, rate limits (register, login per IP and per email, password
reset), single-use hashed reset tokens (30 min), profile, interests and privacy,
BookDescription vs BookCopy, owner-only edits, status guards (RESERVED/TRANSFERRED
copies are locked), soft delete, photo uploads (JPEG/PNG by magic bytes, 5 MB,
5 per copy, UUID file names outside the web root), wish list (title or interest,
max 100, duplicates rejected) and paginated, LIKE-escaped search.

**Frontend** (`frontend/src/{context,hooks,pages,utils}`): AuthContext (no
tokens in JS storage), hooks, auth/home/listings/wishlist/profile pages, and
utils (validation mirroring the server, labels, safe redirects, error mapping,
URL-synced search filters).

Contract: `docs/api/openapi.yaml`. Schema: `database/migrations/001-003`.

## Interfaces other teammates need to provide

My code is written against these. Nothing builds or runs fully until they exist.

### `backend/src/common/ApiError.h`

```cpp
namespace swapcycle::common {
enum class ErrorCode { ValidationFailed, Unauthorized, Forbidden, NotFound, Conflict,
                       RateLimited, PayloadTooLarge, UnsupportedMediaType, Internal };
class ApiError : public std::runtime_error {   // what() is the reader-facing message
 public:
  ApiError(ErrorCode code, const std::string& message);
  ErrorCode code() const noexcept;
};
// { "data": data }
drogon::HttpResponsePtr dataResponse(const Json::Value& data, drogon::HttpStatusCode status = drogon::k200OK);
// ApiError -> its status + { "error": { code, message } } (codes as in openapi.yaml).
// Any other exception -> log server-side, 500 INTERNAL "Something went wrong."
drogon::HttpResponsePtr errorResponse(const std::exception& error);
}
```

### `backend/src/middleware/AuthFilter`

A Drogon `HttpFilter` registered as `AuthFilter` (global namespace). Read the
`sc_session` cookie, verify it with
`swapcycle::accounts::security::verifySessionToken(token, SESSION_SECRET, time(nullptr))`,
and on success set `req->attributes()` `"userId"` (std::string) and `"role"`
(`"READER"`/`"MODERATOR"`). Otherwise respond 401 `UNAUTHORIZED`. Controllers
also reject requests without `userId`, so a missing filter fails closed.

### Frontend `types/`, `api/`, `components/`

- **types**: as in `openapi.yaml` schemas: `CurrentUser`, `PublicProfile`, `ProfileUpdate`,
  `MeetingArea`, `PrivacySettings`, `InterestCategory`, `Genre`, `BookCondition`, `CopyStatus`,
  `BookListing`, `BookInput`, `BookPhoto`, `Page<T>`, `SearchFilters`
  (`q,title,author,genre,language,minCondition,areaId,page,pageSize`), `WishItem`,
  `WishItemInput`, `WishItemKind`, all exported from `types/index.ts`.
- **api/client.ts** must send `credentials: 'include'`, never read `localStorage`, unwrap
  `{data}`, and throw an error with `status`, `code`, `message` (the current file reads a
  token from `localStorage`, which breaks the security rules).
- **api/auth.ts**: `login(email, password)`, `register({email,password,displayName})`,
  `logout()`, `getMe()`, `requestPasswordReset(email)`, `confirmPasswordReset(token, newPassword)`,
  `updateProfile(update)`, `getPublicProfile(userId)`, `getMeetingAreas()`.
- **api/books.ts**: `createBook`, `getBook`, `getMyBook`, `getMyBooks(page, pageSize)`, `updateBook`,
  `deleteBook`, `withdrawBook`, `relistBook`, `uploadPhoto(copyId, file)` (multipart field `photo`),
  `deletePhoto(copyId, photoId)`, `searchBooks(filters)`.
- **api/wishlist.ts**: `getWishlist`, `addWishItem`, `deleteWishItem`.
- **components/ui**: `Button` (`variant: primary|secondary|ghost|danger`, `fullWidth`, `loading`),
  `Input` (`label`, `hideLabel`, `icon`, `trailing`, `error`, `hint` + input props),
  `Chip` (`label`, `icon`, `selected`, `onToggle`, `tone: light|dark`), `Card`, `Badge`
  (`tone: neutral|success|warning|danger`).
- **components/books**: `BookCard` (`listing`, `to`, `footer`), `BookForm` (`initial`, `areas`,
  `submitLabel`, `serverError`, `onSubmit(input)`; can reuse `utils/validation.validateBookInput`),
  `PhotoUploader` (`photos`, `max`, `disabled`, `onUpload(file)`, `onRemove(photoId)`).
- **components/layout/PageShell**: wraps `children` with the nav.

## How to run

```bash
cp .env.example .env        # then set real values
docker compose up --build   # postgres:16 + migrations + backend on :8080
cd frontend && npm install && npm run dev   # http://localhost:5173, /api proxied
```

## How to test

```bash
docker build backend -f backend/DockerFile --target build   # compiles and runs GoogleTest
cd frontend && npm test                                      # Vitest
cd frontend && npm run check                                 # typecheck + tests + build
cd frontend && npm audit                                     # 0 vulnerabilities at time of writing
```

The frontend tests mock the teammate-owned modules, so `npm test` already passes.
`npm run typecheck`/`build` and the backend build need the interfaces above.

## CI (`.github/workflows/ci.yml`)

Runs on every PR and on pushes to `main`:

- **Repository hygiene:** fails if an `.env` file, a private key or uploaded files are committed.
- **Migrations:** applies every migration (and the dev seed) to PostgreSQL 16.
- **Backend:** builds the Docker image with `-Werror` and runs GoogleTest.
- **Frontend:** `npm ci`, `npm audit --audit-level=high`, Vitest, typecheck and production build.

The backend build and the frontend typecheck/build are skipped with a warning
until `common/ApiError`, `middleware/AuthFilter`, `api/`, `types/` and
`components/ui` are implemented; they start running automatically after that.

## Backend runtime

`backend/config.json` holds the non-secret Drogon settings (threads, 6 MB body
limit, logging, no sessions, no server header, and no static files served from
disk). Secrets and per-environment values come from the environment. The image
is multi-stage: build + tests in the pinned Drogon image, then only the binary on
Ubuntu 22.04, running as an unprivileged user.

## Notes for the team

- **Drogon commits transactions asynchronously** when the last reference is dropped.
  A read on another connection right after can miss the change. Multi-statement
  writes here are single CTE statements, or await `CommitAwaiter`
  (`CatalogRepository.cc`). The exchange/reservation code will need the same.
- Integer SQL parameters need `::int` casts (Drogon binds binary int4).
- Avoid structured bindings across `co_await` (GCC 11 miscompiles them).
- Password-reset e-mail is not sent yet: `LoggingResetSender` only logs that a
  reset was requested. A mail sender is needed (notifications work).
- Sessions are stateless: changing a password does not end existing sessions
  (they expire within 2 hours).
- Contrast: white on deep rose (#D9708F) is 3.2:1 and deep-rose links on blush
  are 3.0:1. That's fine for large/bold text (buttons should be at least 19px bold) but
  below WCAG AA for small text such as "Forgot Password?" (design decision pending).
