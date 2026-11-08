// The one place that talks to the backend. Every other file in this folder
// calls these helpers instead of using fetch directly.
//
// Security notes:
// - The session lives in an httpOnly cookie that the browser manages. We send
//   it with credentials: 'include' and never read, copy or store any token.
// - Nothing here logs request or response bodies (they can hold passwords,
//   reset tokens or handover codes).
import { ERROR_CODES } from './types';
import type { ApiErrorCode, ApiSuccess, ServerErrorCode } from './types';

const DEFAULT_BASE_URL = '/api';
const MAX_MESSAGE_LENGTH = 300;

// ---- Errors ----

export class ApiError extends Error {
  readonly status: number;
  readonly code: ApiErrorCode;
  /** Set on 429 when the server sent a Retry-After header. */
  readonly retryAfterSeconds?: number;

  constructor(status: number, code: ApiErrorCode, message: string, retryAfterSeconds?: number) {
    super(message);
    this.name = 'ApiError';
    this.status = status;
    this.code = code;
    this.retryAfterSeconds = retryAfterSeconds;
  }
}

export function isApiError(value: unknown): value is ApiError {
  return value instanceof ApiError;
}

const FALLBACK_CODE_BY_STATUS: Record<number, ServerErrorCode> = {
  400: 'VALIDATION_FAILED',
  401: 'UNAUTHORIZED',
  403: 'FORBIDDEN',
  404: 'NOT_FOUND',
  409: 'CONFLICT',
  413: 'PAYLOAD_TOO_LARGE',
  415: 'UNSUPPORTED_MEDIA_TYPE',
  429: 'RATE_LIMITED',
};

const GENERIC_MESSAGE = 'Something went wrong. Please try again.';

function isServerErrorCode(value: unknown): value is ServerErrorCode {
  return typeof value === 'string' && (ERROR_CODES as readonly string[]).includes(value);
}

function codeFromStatus(status: number): ApiErrorCode {
  if (FALLBACK_CODE_BY_STATUS[status]) return FALLBACK_CODE_BY_STATUS[status];
  return status >= 500 ? 'INTERNAL' : 'UNKNOWN';
}

function parseRetryAfter(res: Response): number | undefined {
  const raw = res.headers.get('Retry-After');
  if (raw === null) return undefined;
  const seconds = Number(raw);
  return Number.isFinite(seconds) && seconds >= 0 ? seconds : undefined;
}

/** Turn a failed response into an ApiError, whatever the body looks like. */
async function toApiError(res: Response): Promise<ApiError> {
  let code: ApiErrorCode = codeFromStatus(res.status);
  let message = GENERIC_MESSAGE;

  try {
    const body: unknown = await res.json();
    const err = (body as { error?: { code?: unknown; message?: unknown } } | null)?.error;
    if (isServerErrorCode(err?.code)) code = err.code;
    if (typeof err?.message === 'string' && err.message.trim() !== '') {
      message = err.message.trim().slice(0, MAX_MESSAGE_LENGTH);
    }
  } catch {
    // Not JSON (for example a proxy error page). Keep the generic message so
    // we never surface raw server output.
  }

  return new ApiError(res.status, code, message, parseRetryAfter(res));
}

// ---- Configuration ----

/** Called when a request that needs a session gets 401 (session expired). */
let unauthorizedHandler: (() => void) | null = null;

/**
 * Register what should happen when the session is no longer valid, for
 * example clearing the user in the auth context and going to /login.
 * Pass null to remove it.
 */
export function setUnauthorizedHandler(handler: (() => void) | null): void {
  unauthorizedHandler = handler;
}

function baseUrl(): string {
  const configured = (import.meta.env.VITE_API_URL as string | undefined)?.trim();
  const base = configured ? configured : DEFAULT_BASE_URL;
  return base.replace(/\/+$/, '');
}

/**
 * Turn a server-relative path such as /api/photos/<id> into something an
 * <img src> can load. Returns '' for anything that is not a plain absolute path.
 */
export function assetUrl(path: string): string {
  if (!path.startsWith('/') || path.startsWith('//')) return '';
  const base = baseUrl();
  return /^https?:\/\//i.test(base) ? new URL(base).origin + path : path;
}

/** Encode one value for use inside a URL path. */
export function seg(value: string): string {
  return encodeURIComponent(value);
}

// ---- Requests ----

type QueryValue = string | number | boolean | null | undefined;

export interface RequestOptions {
  query?: object;
  /** JSON body. */
  body?: unknown;
  /** Multipart body (file uploads). The browser sets the boundary header. */
  formData?: FormData;
  signal?: AbortSignal;
  /**
   * Skip the unauthorized handler. Use for calls where 401 is an expected
   * answer (wrong password, checking whether anyone is logged in).
   */
  skipUnauthorizedHandler?: boolean;
}

function buildQueryString(query: object | undefined): string {
  if (!query) return '';
  const params = new URLSearchParams();
  for (const [key, value] of Object.entries(query as Record<string, QueryValue>)) {
    if (value === undefined || value === null || value === '') continue;
    params.append(key, String(value));
  }
  const text = params.toString();
  return text ? `?${text}` : '';
}

function notifyUnauthorized(): void {
  try {
    unauthorizedHandler?.();
  } catch {
    // A faulty handler must not hide the original error from the caller.
  }
}

async function send(method: string, path: string, options: RequestOptions): Promise<Response> {
  const headers: Record<string, string> = { Accept: 'application/json' };
  let body: BodyInit | undefined;

  if (options.formData) {
    body = options.formData;
  } else if (options.body !== undefined) {
    headers['Content-Type'] = 'application/json';
    body = JSON.stringify(options.body);
  }

  try {
    return await fetch(`${baseUrl()}${path}${buildQueryString(options.query)}`, {
      method,
      headers,
      body,
      credentials: 'include',
      signal: options.signal,
    });
  } catch (error) {
    if ((error as { name?: string } | null)?.name === 'AbortError') throw error;
    throw new ApiError(0, 'NETWORK_ERROR', 'Could not reach the server. Check your connection.');
  }
}

/**
 * Send a request and return the unwrapped `data`. Endpoints that answer
 * with no body (204, 202) resolve to undefined. Any failure throws ApiError.
 */
export async function request<T>(
  method: 'GET' | 'POST' | 'PATCH' | 'PUT' | 'DELETE',
  path: string,
  options: RequestOptions = {},
): Promise<T> {
  const res = await send(method, path, options);

  if (!res.ok) {
    if (res.status === 401 && !options.skipUnauthorizedHandler) notifyUnauthorized();
    throw await toApiError(res);
  }

  if (res.status === 204 || res.status === 202 || res.status === 205) {
    return undefined as T;
  }

  let body: unknown;
  try {
    body = await res.json();
  } catch {
    throw new ApiError(res.status, 'UNKNOWN', GENERIC_MESSAGE);
  }
  if (typeof body !== 'object' || body === null || !('data' in body)) {
    throw new ApiError(res.status, 'UNKNOWN', GENERIC_MESSAGE);
  }
  return (body as ApiSuccess<T>).data;
}

export const api = {
  get: <T>(path: string, options?: RequestOptions) => request<T>('GET', path, options),
  post: <T>(path: string, options?: RequestOptions) => request<T>('POST', path, options),
  patch: <T>(path: string, options?: RequestOptions) => request<T>('PATCH', path, options),
  delete: <T>(path: string, options?: RequestOptions) => request<T>('DELETE', path, options),
};
