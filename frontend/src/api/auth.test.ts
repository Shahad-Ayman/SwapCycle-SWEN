import { afterEach, describe, expect, it, vi } from 'vitest';
import {
  confirmPasswordReset,
  getCurrentUser,
  getPublicProfile,
  listMeetingAreas,
  login,
  logout,
  register,
  requestPasswordReset,
  updateProfile,
} from './auth';
import { setUnauthorizedHandler } from './client';
import { callAt, errorBody, jsonResponse, mockFetch } from './testUtils';

const user = {
  id: 'u1',
  email: 'sara@example.com',
  displayName: 'Sara',
  role: 'READER',
  meetingArea: null,
  interests: [],
  privacy: { showMeetingArea: true, showInterests: true },
  onboardingCompleted: false,
};

afterEach(() => {
  vi.unstubAllGlobals();
  setUnauthorizedHandler(null);
});

describe('auth', () => {
  it('register posts the details and returns the user', async () => {
    const fetchMock = mockFetch(jsonResponse(201, { data: user }));
    const result = await register({ email: 'sara@example.com', password: 'pw123456', displayName: 'Sara' });
    const { url, init } = callAt(fetchMock);
    expect(url).toBe('/api/auth/register');
    expect(init.method).toBe('POST');
    expect(JSON.parse(init.body as string)).toEqual({
      email: 'sara@example.com',
      password: 'pw123456',
      displayName: 'Sara',
    });
    expect(result).toEqual(user);
  });

  it('login failure does not trigger the session-expired handler', async () => {
    const handler = vi.fn();
    setUnauthorizedHandler(handler);
    mockFetch(jsonResponse(401, errorBody('UNAUTHORIZED', 'Invalid email or password')));
    await expect(login({ email: 'a@b.co', password: 'wrong' })).rejects.toMatchObject({
      status: 401,
      message: 'Invalid email or password',
    });
    expect(handler).toHaveBeenCalledTimes(0);
  });

  it('getCurrentUser on a fresh visit rejects without the handler', async () => {
    const handler = vi.fn();
    setUnauthorizedHandler(handler);
    mockFetch(jsonResponse(401, errorBody('UNAUTHORIZED', 'Not logged in')));
    await expect(getCurrentUser()).rejects.toMatchObject({ code: 'UNAUTHORIZED' });
    expect(handler).toHaveBeenCalledTimes(0);
  });

  it('logout posts and resolves with nothing', async () => {
    const fetchMock = mockFetch(jsonResponse(204));
    await expect(logout()).resolves.toBeUndefined();
    expect(callAt(fetchMock).url).toBe('/api/auth/logout');
  });

  it('never reads or writes browser storage', async () => {
    const storage = { getItem: vi.fn(), setItem: vi.fn(), removeItem: vi.fn() };
    vi.stubGlobal('localStorage', storage);
    vi.stubGlobal('sessionStorage', storage);
    mockFetch(jsonResponse(200, { data: user }));
    await login({ email: 'sara@example.com', password: 'pw123456' });
    expect(storage.getItem).toHaveBeenCalledTimes(0);
    expect(storage.setItem).toHaveBeenCalledTimes(0);
  });

  it('password reset request resolves on 202', async () => {
    const fetchMock = mockFetch(jsonResponse(202));
    await expect(requestPasswordReset('sara@example.com')).resolves.toBeUndefined();
    expect(callAt(fetchMock).url).toBe('/api/auth/password-reset/request');
  });

  it('password reset confirm sends token and new password', async () => {
    const fetchMock = mockFetch(jsonResponse(204));
    await confirmPasswordReset({ token: 't'.repeat(24), newPassword: 'newpassword1' });
    const { url, init } = callAt(fetchMock);
    expect(url).toBe('/api/auth/password-reset/confirm');
    expect(JSON.parse(init.body as string)).toEqual({ token: 't'.repeat(24), newPassword: 'newpassword1' });
  });

  it('lists meeting areas', async () => {
    const fetchMock = mockFetch(jsonResponse(200, { data: [{ id: 1, name: 'Library' }] }));
    await expect(listMeetingAreas()).resolves.toEqual([{ id: 1, name: 'Library' }]);
    expect(callAt(fetchMock).url).toBe('/api/meeting-areas');
  });

  it('updateProfile patches /users/me with only the fields given', async () => {
    const fetchMock = mockFetch(jsonResponse(200, { data: user }));
    await updateProfile({ interests: ['FICTION', 'SCIENCE'], onboardingCompleted: true });
    const { url, init } = callAt(fetchMock);
    expect(url).toBe('/api/users/me');
    expect(init.method).toBe('PATCH');
    expect(JSON.parse(init.body as string)).toEqual({
      interests: ['FICTION', 'SCIENCE'],
      onboardingCompleted: true,
    });
  });

  it('getPublicProfile encodes the id into the path', async () => {
    const fetchMock = mockFetch(jsonResponse(200, { data: { id: 'x' } }));
    await getPublicProfile('a/b');
    expect(callAt(fetchMock).url).toBe('/api/users/a%2Fb');
  });
});
