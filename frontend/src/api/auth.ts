// Accounts: sign up, log in/out, password reset, own profile, public profiles.
// Every function maps to one operation in docs/api/openapi.yaml.
import { api, seg } from './client';
import type {
  CurrentUser,
  LoginRequest,
  MeetingArea,
  PasswordResetConfirm,
  ProfileUpdate,
  PublicProfile,
  RegisterRequest,
} from './types';

/** Create an account and start a session (the server sets the cookie). */
export function register(input: RegisterRequest, signal?: AbortSignal): Promise<CurrentUser> {
  return api.post<CurrentUser>('/auth/register', { body: input, signal });
}

/**
 * Log in. A 401 here means wrong email or password, not an expired session,
 * so it must not trigger the global "session expired" handler.
 */
export function login(input: LoginRequest, signal?: AbortSignal): Promise<CurrentUser> {
  return api.post<CurrentUser>('/auth/login', {
    body: input,
    signal,
    skipUnauthorizedHandler: true,
  });
}

/** End the session. The server clears the cookie. */
export function logout(signal?: AbortSignal): Promise<void> {
  return api.post<void>('/auth/logout', { signal, skipUnauthorizedHandler: true });
}

/**
 * Who is logged in? Rejects with a 401 ApiError when nobody is. That is a
 * normal answer on first page load, so it skips the unauthorized handler.
 */
export function getCurrentUser(signal?: AbortSignal): Promise<CurrentUser> {
  return api.get<CurrentUser>('/auth/me', { signal, skipUnauthorizedHandler: true });
}

/** Ask for a reset link. Always succeeds (202) whether or not the email exists. */
export function requestPasswordReset(email: string, signal?: AbortSignal): Promise<void> {
  return api.post<void>('/auth/password-reset/request', { body: { email }, signal });
}

/** Set a new password with a single-use token from the reset link. */
export function confirmPasswordReset(
  input: PasswordResetConfirm,
  signal?: AbortSignal,
): Promise<void> {
  return api.post<void>('/auth/password-reset/confirm', { body: input, signal });
}

/** General campus areas a reader can choose from. */
export function listMeetingAreas(signal?: AbortSignal): Promise<MeetingArea[]> {
  return api.get<MeetingArea[]>('/meeting-areas', { signal });
}

/** Update your own profile. Only the fields you send are changed. */
export function updateProfile(input: ProfileUpdate, signal?: AbortSignal): Promise<CurrentUser> {
  return api.patch<CurrentUser>('/users/me', { body: input, signal });
}

/** Another reader's public profile. Hidden fields come back as null. */
export function getPublicProfile(userId: string, signal?: AbortSignal): Promise<PublicProfile> {
  return api.get<PublicProfile>(`/users/${seg(userId)}`, { signal });
}
