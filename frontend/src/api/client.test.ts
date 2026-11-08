import { afterEach, describe, expect, it, vi } from 'vitest';
import { ApiError, api, assetUrl, isApiError, seg, setUnauthorizedHandler } from './client';
import { callAt, errorBody, jsonResponse, mockFetch } from './testUtils';

afterEach(() => {
  vi.unstubAllGlobals();
  vi.unstubAllEnvs();
  setUnauthorizedHandler(null);
});

describe('request basics', () => {
  it('always sends credentials and asks for JSON', async () => {
    const fetchMock = mockFetch(jsonResponse(200, { data: { ok: true } }));
    await api.get('/auth/me');
    const { url, init, headers } = callAt(fetchMock);
    expect(url).toBe('/api/auth/me');
    expect(init.method).toBe('GET');
    expect(init.credentials).toBe('include');
    expect(headers.Accept).toBe('application/json');
    expect(headers['Content-Type']).toBeUndefined();
  });

  it('returns the unwrapped data field', async () => {
    mockFetch(jsonResponse(200, { data: { id: 'abc' } }));
    await expect(api.get('/x')).resolves.toEqual({ id: 'abc' });
  });

  it('sends JSON bodies with a content type', async () => {
    const fetchMock = mockFetch(jsonResponse(201, { data: {} }));
    await api.post('/books', { body: { title: 'Dune' } });
    const { init, headers } = callAt(fetchMock);
    expect(init.method).toBe('POST');
    expect(headers['Content-Type']).toBe('application/json');
    expect(init.body).toBe('{"title":"Dune"}');
  });

  it('resolves to undefined for 204 and 202 answers', async () => {
    mockFetch(jsonResponse(204), jsonResponse(202));
    await expect(api.delete('/x')).resolves.toBeUndefined();
    await expect(api.post('/y')).resolves.toBeUndefined();
  });

  it('treats a 200 without a data field as an error', async () => {
    mockFetch(jsonResponse(200, { nope: 1 }));
    await expect(api.get('/x')).rejects.toMatchObject({ status: 200, code: 'UNKNOWN' });
  });

  it('does not set Content-Type for multipart bodies', async () => {
    const fetchMock = mockFetch(jsonResponse(201, { data: {} }));
    const formData = new FormData();
    formData.append('photo', new File(['x'], 'a.png', { type: 'image/png' }));
    await api.post('/books/1/photos', { formData });
    const { init, headers } = callAt(fetchMock);
    expect(init.body).toBe(formData);
    expect(headers['Content-Type']).toBeUndefined();
  });
});

describe('query strings', () => {
  it('leaves out empty, null and undefined values and encodes the rest', async () => {
    const fetchMock = mockFetch(jsonResponse(200, { data: {} }));
    await api.get('/search/books', {
      query: { q: 'war & peace', author: '', genre: undefined, language: null, page: 2, flag: false },
    });
    expect(callAt(fetchMock).url).toBe('/api/search/books?q=war+%26+peace&page=2&flag=false');
  });

  it('adds no question mark when there is nothing to send', async () => {
    const fetchMock = mockFetch(jsonResponse(200, { data: {} }));
    await api.get('/search/books', { query: { q: '' } });
    expect(callAt(fetchMock).url).toBe('/api/search/books');
  });
});

describe('errors', () => {
  it('maps a server error to ApiError', async () => {
    mockFetch(jsonResponse(404, errorBody('NOT_FOUND', 'No such book')));
    const error = await api.get('/books/1').catch((e: unknown) => e);
    expect(isApiError(error)).toBe(true);
    expect(error).toMatchObject({ status: 404, code: 'NOT_FOUND', message: 'No such book' });
  });

  it('keeps Retry-After on rate-limit errors', async () => {
    mockFetch(jsonResponse(429, errorBody('RATE_LIMITED', 'Slow down'), { 'Retry-After': '30' }));
    await expect(api.post('/auth/login')).rejects.toMatchObject({
      code: 'RATE_LIMITED',
      retryAfterSeconds: 30,
    });
  });

  it('never surfaces a non-JSON error page', async () => {
    mockFetch(new Response('<html>stack trace /srv/app.cpp</html>', { status: 502 }));
    const error = (await api.get('/x').catch((e: unknown) => e)) as ApiError;
    expect(error.code).toBe('INTERNAL');
    expect(error.message).toBe('Something went wrong. Please try again.');
  });

  it('falls back to the status when the code is unknown', async () => {
    mockFetch(jsonResponse(409, errorBody('SOMETHING_NEW', 'Clash')));
    await expect(api.post('/x')).rejects.toMatchObject({ code: 'CONFLICT', message: 'Clash' });
  });

  it('reports a network failure as NETWORK_ERROR', async () => {
    vi.stubGlobal('fetch', vi.fn().mockRejectedValue(new TypeError('Failed to fetch')));
    await expect(api.get('/x')).rejects.toMatchObject({ status: 0, code: 'NETWORK_ERROR' });
  });

  it('rethrows aborts untouched so callers can ignore them', async () => {
    const abort = new DOMException('Aborted', 'AbortError');
    vi.stubGlobal('fetch', vi.fn().mockRejectedValue(abort));
    await expect(api.get('/x')).rejects.toBe(abort);
  });
});

describe('unauthorized handler', () => {
  it('runs on 401 and still throws', async () => {
    const handler = vi.fn();
    setUnauthorizedHandler(handler);
    mockFetch(jsonResponse(401, errorBody('UNAUTHORIZED', 'Not logged in')));
    await expect(api.get('/users/me/books')).rejects.toMatchObject({ code: 'UNAUTHORIZED' });
    expect(handler).toHaveBeenCalledTimes(1);
  });

  it('is skipped when the caller expects a 401', async () => {
    const handler = vi.fn();
    setUnauthorizedHandler(handler);
    mockFetch(jsonResponse(401, errorBody('UNAUTHORIZED', 'Invalid email or password')));
    await expect(api.post('/auth/login', { skipUnauthorizedHandler: true })).rejects.toBeInstanceOf(
      ApiError,
    );
    expect(handler).toHaveBeenCalledTimes(0);
  });

  it('does not run for other statuses', async () => {
    const handler = vi.fn();
    setUnauthorizedHandler(handler);
    mockFetch(jsonResponse(403, errorBody('FORBIDDEN', 'No')));
    await expect(api.get('/x')).rejects.toBeInstanceOf(ApiError);
    expect(handler).toHaveBeenCalledTimes(0);
  });

  it('cannot hide the real error if the handler itself throws', async () => {
    setUnauthorizedHandler(() => {
      throw new Error('boom');
    });
    mockFetch(jsonResponse(401, errorBody('UNAUTHORIZED', 'Not logged in')));
    await expect(api.get('/x')).rejects.toMatchObject({ code: 'UNAUTHORIZED' });
  });
});

describe('base URL and helpers', () => {
  it('uses VITE_API_URL when set and trims trailing slashes', async () => {
    vi.stubEnv('VITE_API_URL', 'http://localhost:8080/api/');
    const fetchMock = mockFetch(jsonResponse(200, { data: {} }));
    await api.get('/auth/me');
    expect(callAt(fetchMock).url).toBe('http://localhost:8080/api/auth/me');
  });

  it('encodes path segments', () => {
    expect(seg('a/b c')).toBe('a%2Fb%20c');
  });

  it('assetUrl keeps same-origin paths and rejects anything else', () => {
    expect(assetUrl('/api/photos/1')).toBe('/api/photos/1');
    expect(assetUrl('//evil.example/x.png')).toBe('');
    expect(assetUrl('javascript:alert(1)')).toBe('');
    expect(assetUrl('https://evil.example/x.png')).toBe('');
  });

  it('assetUrl adds the API origin when the API is on another host', () => {
    vi.stubEnv('VITE_API_URL', 'http://localhost:8080/api');
    expect(assetUrl('/api/photos/1')).toBe('http://localhost:8080/api/photos/1');
  });
});
