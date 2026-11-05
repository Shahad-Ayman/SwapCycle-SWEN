import { useCallback } from 'react';

import { addWishItem, deleteWishItem, getWishlist } from '../api/wishlist';
import type { WishItem, WishItemInput } from '../types';
import { useAsync } from './useAsync';

export function useWishlist() {
  const { data, error, loading, reload, setData } = useAsync(getWishlist, []);
  const items = data ?? [];

  const add = useCallback(
    async (input: WishItemInput): Promise<WishItem> => {
      const item = await addWishItem(input);
      setData((current) => [item, ...(current ?? [])]);
      return item;
    },
    [setData],
  );

  const remove = useCallback(
    async (id: string): Promise<void> => {
      await deleteWishItem(id);
      setData((current) => (current ?? []).filter((item) => item.id !== id));
    },
    [setData],
  );

  return { items, error, loading, reload, add, remove };
}
