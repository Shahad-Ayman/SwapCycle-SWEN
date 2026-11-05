import { act, renderHook, waitFor } from '@testing-library/react';
import { describe, expect, it } from 'vitest';

import { useAsync } from './useAsync';

function deferred<T>() {
  let resolve!: (value: T) => void;
  const promise = new Promise<T>((done) => {
    resolve = done;
  });
  return { promise, resolve };
}

describe('useAsync', () => {
  it('loads and exposes data', async () => {
    const { result } = renderHook(() => useAsync(() => Promise.resolve(42), []));
    expect(result.current.loading).toBe(true);
    await waitFor(() => expect(result.current.data).toBe(42));
    expect(result.current.loading).toBe(false);
  });

  it('ignores a slow older response once a newer request has started', async () => {
    const slow = deferred<string>();
    const fast = deferred<string>();
    const { result, rerender } = renderHook(({ query }) => useAsync(() => (query === 'a' ? slow.promise : fast.promise), [query]), {
      initialProps: { query: 'a' },
    });
    rerender({ query: 'b' });
    await act(async () => fast.resolve('results for b'));
    await act(async () => slow.resolve('results for a'));
    expect(result.current.data).toBe('results for b');
  });

  it('captures errors and can reload', async () => {
    let attempt = 0;
    const { result } = renderHook(() =>
      useAsync(() => (++attempt === 1 ? Promise.reject(new Error('down')) : Promise.resolve('ok')), []),
    );
    await waitFor(() => expect(result.current.error).toBeInstanceOf(Error));
    act(() => result.current.reload());
    await waitFor(() => expect(result.current.data).toBe('ok'));
    expect(result.current.error).toBeUndefined();
  });
});
