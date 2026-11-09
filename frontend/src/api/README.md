# Frontend API layer (`frontend/src/api`)

Typed functions that call the SwapCycle backend. The contract is
`docs/api/openapi.yaml`. If it changes, change `types.ts` and the matching file here in the same commit.

## Files

| File | What it does |
| --- | --- |
| `client.ts` | The only code that uses `fetch`. Sends `credentials: 'include'`, unwraps `{ data }`, turns failures into `ApiError`, handles 401 and uploads. |
| `types.ts` | Types and enum lists for the Sprint 1 spec. |
| `auth.ts` | Register, log in/out, current user, password reset, profile, public profile, meeting areas. |
| `books.ts` | Create, edit, delete, withdraw, relist, own listings, search, photo upload/delete. |
| `wishlist.ts` | List, add, remove wish list entries. |
| `*.test.ts`, `testUtils.ts` | Vitest tests with `fetch` mocked. |

Not built yet because `openapi.yaml` has no endpoints for them: `chat.ts`, `disputes.ts`,
`handover.ts`, `matches.ts`, `notifications.ts`, `proposals.ts`, `reviews.ts`.

## Using it

```ts
import { login } from './api/auth';
import { isApiError, setUnauthorizedHandler } from './api/client';

// Once, e.g. in the auth context: what to do when a session expires.
setUnauthorizedHandler(() => { /* clear user, go to /login */ });

try {
  const user = await login({ email, password });
} catch (error) {
  if (isApiError(error)) show(error.message); // safe to display
}
```

- Every function throws `ApiError` (`status`, `code`, `message`, `retryAfterSeconds` on 429).
  `code` is `NETWORK_ERROR` when the server could not be reached.
- Pass an `AbortSignal` as the last argument to cancel a request (aborts are rethrown as-is).
- Photo `url` values come back as `/api/photos/<id>`. Wrap them: `<img src={assetUrl(photo.url)} />`.
- `login`, `logout` and `getCurrentUser` do not trigger the unauthorized handler, because a 401 is a normal answer for them.

## Configuration

`VITE_API_URL` (optional, in `frontend/.env`, never commit real secrets): the API base URL.
Defaults to `/api`, which works when Vite proxies `/api` to the backend in dev.

## Run and test

```
cd frontend
npm install
npm test            # needs the "test" script: "vitest run"
npx tsc --noEmit    # type check
npm audit
```

Needs `vitest` as a dev dependency (`npm i -D vitest`) if the repo does not have it yet.

## Security notes

- No tokens in `localStorage` or `sessionStorage`. The session is an httpOnly cookie that the browser handles.
- Request and response bodies are never logged.
- Non-JSON or unexpected error bodies are replaced by a generic message, so server output never reaches the UI.
- Client-side photo checks (JPEG/PNG, 5 MB) only save a round trip. The server does the real check.
- No CSRF token is sent. The spec has none and relies on `SameSite=Lax`. If the backend adds one, add it in `send()` in `client.ts`.
