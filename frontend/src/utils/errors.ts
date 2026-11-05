// Turns anything thrown by the API client into a message that is safe and
// useful to show. The client is expected to throw errors carrying the
// server's { code, message } and the HTTP status (see docs/api/openapi.yaml).

interface ApiErrorLike {
  status?: number;
  code?: string;
  message?: string;
}

function asApiError(error: unknown): ApiErrorLike | undefined {
  return typeof error === 'object' && error !== null ? (error as ApiErrorLike) : undefined;
}

export function errorStatus(error: unknown): number | undefined {
  const status = asApiError(error)?.status;
  return typeof status === 'number' ? status : undefined;
}

export function isUnauthorized(error: unknown): boolean {
  return errorStatus(error) === 401;
}

export function isNotFound(error: unknown): boolean {
  return errorStatus(error) === 404;
}

export function errorMessage(error: unknown, fallback = 'Something went wrong. Please try again.'): string {
  if (error instanceof TypeError) return "We couldn't reach SwapCycle. Check your connection and try again.";
  const apiError = asApiError(error);
  const status = errorStatus(error);
  if (status === 429) return 'Too many attempts. Please wait a few minutes and try again.';
  if (status !== undefined && status >= 500) return fallback;
  // Server messages for 4xx are written for readers; anything else uses the fallback.
  if (status !== undefined && apiError?.code && apiError.message) return apiError.message;
  return fallback;
}
