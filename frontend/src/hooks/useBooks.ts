import { getBook, getMyBooks, searchBooks } from '../api/books';
import type { SearchFilters } from '../types';
import { LIMITS } from '../utils/constants';
import { useAsync } from './useAsync';

export function useBookSearch(filters: SearchFilters) {
  const key = JSON.stringify(filters);
  return useAsync(() => searchBooks({ pageSize: LIMITS.pageSize, ...filters }), [key]);
}

export function useBook(copyId: string | undefined) {
  return useAsync(() => (copyId ? getBook(copyId) : Promise.reject(new Error('Missing book id'))), [copyId]);
}

export function useMyBooks(page: number) {
  return useAsync(() => getMyBooks(page, LIMITS.pageSize), [page]);
}
