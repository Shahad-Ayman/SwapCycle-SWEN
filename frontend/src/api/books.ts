// Book listings: create, edit, withdraw, relist, delete, photos, and search.
import { ApiError, api, seg } from './client';
import type {
  BookInput,
  BookListing,
  BookListingPage,
  BookPhoto,
  BookSearchParams,
  PageParams,
} from './types';

/** Photo rules from the spec. The server is the real check; these save a round trip. */
export const MAX_PHOTO_BYTES = 5 * 1024 * 1024;
export const MAX_PHOTOS_PER_COPY = 5;
export const ALLOWED_PHOTO_TYPES: readonly string[] = ['image/jpeg', 'image/png'];

/** List a physical copy of a book. */
export function createBook(input: BookInput, signal?: AbortSignal): Promise<BookListing> {
  return api.post<BookListing>('/books', { body: input, signal });
}

/** A public listing. 404 for withdrawn, transferred or deleted copies (unless yours). */
export function getBook(copyId: string, signal?: AbortSignal): Promise<BookListing> {
  return api.get<BookListing>(`/books/${seg(copyId)}`, { signal });
}

/** Edit your own listing (only while AVAILABLE or WITHDRAWN). */
export function updateBook(
  copyId: string,
  input: BookInput,
  signal?: AbortSignal,
): Promise<BookListing> {
  return api.patch<BookListing>(`/books/${seg(copyId)}`, { body: input, signal });
}

/** Delete your own listing (soft delete on the server). */
export function deleteBook(copyId: string, signal?: AbortSignal): Promise<void> {
  return api.delete<void>(`/books/${seg(copyId)}`, { signal });
}

/** Take your AVAILABLE listing out of exchange. */
export function withdrawBook(copyId: string, signal?: AbortSignal): Promise<BookListing> {
  return api.post<BookListing>(`/books/${seg(copyId)}/withdraw`, { signal });
}

/** Make your WITHDRAWN listing AVAILABLE again. */
export function relistBook(copyId: string, signal?: AbortSignal): Promise<BookListing> {
  return api.post<BookListing>(`/books/${seg(copyId)}/relist`, { signal });
}

/** Your own listings in every status except deleted. */
export function listMyBooks(
  params: PageParams = {},
  signal?: AbortSignal,
): Promise<BookListingPage> {
  return api.get<BookListingPage>('/users/me/books', { query: params, signal });
}

/** One of your own listings in any status (for the edit form). */
export function getMyBook(copyId: string, signal?: AbortSignal): Promise<BookListing> {
  return api.get<BookListing>(`/users/me/books/${seg(copyId)}`, { signal });
}

/** Search AVAILABLE listings. Empty filters are left out of the request. */
export function searchBooks(
  params: BookSearchParams = {},
  signal?: AbortSignal,
): Promise<BookListingPage> {
  return api.get<BookListingPage>('/search/books', { query: params, signal });
}

/** Returns a message if the file breaks the photo rules, otherwise null. */
export function checkPhotoFile(file: File): string | null {
  if (!ALLOWED_PHOTO_TYPES.includes(file.type)) return 'Photos must be JPEG or PNG.';
  if (file.size > MAX_PHOTO_BYTES) return 'Photos must be 5 MB or smaller.';
  return null;
}

/** Add one photo to your listing. */
export function uploadBookPhoto(
  copyId: string,
  file: File,
  signal?: AbortSignal,
): Promise<BookPhoto> {
  const problem = checkPhotoFile(file);
  if (problem) return Promise.reject(new ApiError(400, 'VALIDATION_FAILED', problem));

  const formData = new FormData();
  formData.append('photo', file);
  return api.post<BookPhoto>(`/books/${seg(copyId)}/photos`, { formData, signal });
}

/** Remove one photo from your listing. */
export function deleteBookPhoto(
  copyId: string,
  photoId: string,
  signal?: AbortSignal,
): Promise<void> {
  return api.delete<void>(`/books/${seg(copyId)}/photos/${seg(photoId)}`, { signal });
}
