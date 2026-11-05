import { useCallback, useEffect, useRef, useState, type DependencyList, type Dispatch, type SetStateAction } from 'react';

export interface AsyncState<T> {
  data: T | undefined;
  error: unknown;
  loading: boolean;
  reload: () => void;
  setData: Dispatch<SetStateAction<T | undefined>>;
}

/**
 * Runs `load` when `deps` change and tracks its result. Responses that arrive
 * after a newer request started (or after unmount) are ignored, so a slow
 * search can never overwrite a faster, newer one.
 */
export function useAsync<T>(load: () => Promise<T>, deps: DependencyList): AsyncState<T> {
  const [data, setData] = useState<T>();
  const [error, setError] = useState<unknown>();
  const [loading, setLoading] = useState(true);
  const [version, setVersion] = useState(0);
  const latestRequest = useRef(0);

  useEffect(() => {
    const request = ++latestRequest.current;
    setLoading(true);
    setError(undefined);
    load().then(
      (result) => {
        if (request !== latestRequest.current) return;
        setData(result);
        setLoading(false);
      },
      (failure: unknown) => {
        if (request !== latestRequest.current) return;
        setError(failure);
        setLoading(false);
      },
    );
    return () => {
      latestRequest.current++;
    };
    // `load` is intentionally not a dependency: callers list what it reads in `deps`.
  }, [...deps, version]);

  const reload = useCallback(() => setVersion((current) => current + 1), []);

  return { data, error, loading, reload, setData };
}
