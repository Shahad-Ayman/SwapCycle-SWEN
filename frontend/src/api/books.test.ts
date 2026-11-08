import { afterEach, describe, expect, it, vi } from 'vitest';
import {
  MAX_PHOTO_BYTES,
  createBook,
  deleteBook,
  deleteBookPhoto,
  getBook,
  listMyBooks,
  relistBook,
  searchBooks,
  updateBook,
  uploadBookPhoto,
  withdrawBook,
} from './books';
import { callAt, errorBody, jsonResponse, mockFetch } from './testUtils';
import type { BookInput } from './types';

const input: BookInput = {
  title: 'Dune',
  author: 'Frank Herbert',
  language: 'en',
  genre: 'SCIENCE_FICTION',
  condition: 'GOOD',
};

afterEach(() => {
  vi.unstubAllGlobals();
});

describe('books', () => {
  it('createBook posts the listing', async () => {
    const fetchMock = mockFetch(jsonResponse(201, { data: { id: 'c1' } }));
    await expect(createBook(input)).resolves.toEqual({ id: 'c1' });
    const { url, init } = callAt(fetchMock);
    expect(url).toBe('/api/books');
    expect(init.method).toBe('POST');
    expect(JSON.parse(init.body as string)).toEqual(input);
  });

  it('getBook, updateBook and deleteBook use the copy path', async () => {
    const fetchMock = mockFetch(
      jsonResponse(200, { data: {} }),
      jsonResponse(200, { data: {} }),
      jsonResponse(204),
    );
    await getBook('c1');
    await updateBook('c1', input);
    await expect(deleteBook('c1')).resolves.toBeUndefined();
    expect(callAt(fetchMock, 0).url).toBe('/api/books/c1');
    expect(callAt(fetchMock, 1).init.method).toBe('PATCH');
    expect(callAt(fetchMock, 2).init.method).toBe('DELETE');
  });

  it('withdraw and relist post to their own paths', async () => {
    const fetchMock = mockFetch(jsonResponse(200, { data: {} }), jsonResponse(200, { data: {} }));
    await withdrawBook('c1');
    await relistBook('c1');
    expect(callAt(fetchMock, 0).url).toBe('/api/books/c1/withdraw');
    expect(callAt(fetchMock, 1).url).toBe('/api/books/c1/relist');
  });

  it('a CONFLICT (for example a reserved copy) reaches the caller', async () => {
    mockFetch(jsonResponse(409, errorBody('CONFLICT', 'This copy is reserved')));
    await expect(withdrawBook('c1')).rejects.toMatchObject({ status: 409, code: 'CONFLICT' });
  });

  it('listMyBooks passes pagination', async () => {
    const fetchMock = mockFetch(jsonResponse(200, { data: { items: [], page: 2, pageSize: 10, total: 0 } }));
    await listMyBooks({ page: 2, pageSize: 10 });
    expect(callAt(fetchMock).url).toBe('/api/users/me/books?page=2&pageSize=10');
  });

  it('searchBooks sends only the filters that are set', async () => {
    const fetchMock = mockFetch(jsonResponse(200, { data: { items: [], page: 1, pageSize: 20, total: 0 } }));
    await searchBooks({ q: 'dune', genre: 'SCIENCE_FICTION', minCondition: 'GOOD', areaId: 3, author: '' });
    expect(callAt(fetchMock).url).toBe(
      '/api/search/books?q=dune&genre=SCIENCE_FICTION&minCondition=GOOD&areaId=3',
    );
  });
});

describe('photos', () => {
  it('uploads the file as multipart field "photo"', async () => {
    const fetchMock = mockFetch(jsonResponse(201, { data: { id: 'p1', url: '/api/photos/p1' } }));
    const file = new File(['abc'], 'cover.png', { type: 'image/png' });
    await expect(uploadBookPhoto('c1', file)).resolves.toEqual({ id: 'p1', url: '/api/photos/p1' });
    const { url, init, headers } = callAt(fetchMock);
    expect(url).toBe('/api/books/c1/photos');
    expect(headers['Content-Type']).toBeUndefined();
    expect((init.body as FormData).get('photo')).toBeInstanceOf(File);
  });

  it('refuses non-JPEG/PNG files without calling the server', async () => {
    const fetchMock = mockFetch();
    const file = new File(['x'], 'evil.svg', { type: 'image/svg+xml' });
    await expect(uploadBookPhoto('c1', file)).rejects.toMatchObject({ code: 'VALIDATION_FAILED' });
    expect(fetchMock).toHaveBeenCalledTimes(0);
  });

  it('refuses files over 5 MB without calling the server', async () => {
    const fetchMock = mockFetch();
    const big = new File([new Uint8Array(MAX_PHOTO_BYTES + 1)], 'big.jpg', { type: 'image/jpeg' });
    await expect(uploadBookPhoto('c1', big)).rejects.toMatchObject({ code: 'VALIDATION_FAILED' });
    expect(fetchMock).toHaveBeenCalledTimes(0);
  });

  it('deleteBookPhoto targets the photo path', async () => {
    const fetchMock = mockFetch(jsonResponse(204));
    await deleteBookPhoto('c1', 'p1');
    const { url, init } = callAt(fetchMock);
    expect(url).toBe('/api/books/c1/photos/p1');
    expect(init.method).toBe('DELETE');
  });
});
