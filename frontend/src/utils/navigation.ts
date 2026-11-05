/**
 * Returns `candidate` only if it is a same-site path such as "/books/123".
 * Anything that could leave the site ("//evil.com", "/\\evil.com",
 * "https://…", "javascript:…") falls back to `fallback`, so a crafted
 * "return to" value can never become an open redirect.
 */
export function safeRedirectPath(candidate: unknown, fallback = '/home'): string {
  if (typeof candidate !== 'string' || candidate.length > 2048) return fallback;
  if (!candidate.startsWith('/') || candidate.startsWith('//')) return fallback;
  if (/[\\\u0000-\u001f\u007f]/.test(candidate)) return fallback;
  return candidate;
}

/** Where a reader goes after logging in or signing up. */
export function homeFor(user: { onboardingCompleted: boolean }): string {
  return user.onboardingCompleted ? '/home' : '/onboarding';
}
