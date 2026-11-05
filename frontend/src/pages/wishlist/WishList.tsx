import { useState } from 'react';
import { Link, useLocation, useNavigate } from 'react-router-dom';

import Button from '../../components/ui/Button';
import Card from '../../components/ui/Card';
import { useWishlist } from '../../hooks/useWishlist';
import type { WishItem } from '../../types';
import { errorMessage } from '../../utils/errors';
import { conditionLabel, genreLabel, languageName } from '../../utils/format';
import { paramsFromFilters } from '../../utils/searchParams';
import { Alert, EmptyState, LoadingState, PageHeading, linkClass } from '../PageParts';

function describe(item: WishItem): string {
  const parts = [item.language ? languageName(item.language) : 'Any language'];
  if (item.minCondition) parts.push(`${conditionLabel(item.minCondition)} or better`);
  return parts.join(' · ');
}

export default function WishList() {
  const navigate = useNavigate();
  const location = useLocation();
  const { items, error, loading, remove } = useWishlist();
  const [removing, setRemoving] = useState<string>();
  const [actionError, setActionError] = useState<string>();
  const added = (location.state as { added?: boolean } | null)?.added;

  async function handleRemove(item: WishItem) {
    setRemoving(item.id);
    setActionError(undefined);
    try {
      await remove(item.id);
    } catch (failure) {
      setActionError(errorMessage(failure));
    } finally {
      setRemoving(undefined);
    }
  }

  return (
    <div>
      <PageHeading
        title="My wish list"
        subtitle="Specific books or broader interests. Matching uses these to suggest swaps."
        action={
          <Button variant="primary" onClick={() => navigate('/wishlist/new')}>
            Add to wish list
          </Button>
        }
      />
      <div className="space-y-4">
        {added && <Alert tone="success">Added to your wish list.</Alert>}
        {actionError && <Alert tone="error">{actionError}</Alert>}
        {loading && <LoadingState label="Loading your wish list…" />}
        {!loading && error !== undefined && <Alert tone="error">{errorMessage(error)}</Alert>}
        {!loading && error === undefined && items.length === 0 && (
          <EmptyState title="Your wish list is empty">
            <Link to="/wishlist/new" className={linkClass}>
              Add a book or genre
            </Link>{' '}
            you would like to read.
          </EmptyState>
        )}
        {items.length > 0 && (
          <ul className="grid gap-4 md:grid-cols-2">
            {items.map((item) => (
              <li key={item.id}>
                <Card>
                  <div className="flex items-start justify-between gap-4">
                    <div className="min-w-0">
                      <p className="text-xs font-semibold uppercase tracking-wide text-slate">
                        {item.kind === 'TITLE' ? 'Specific book' : 'Interest'}
                      </p>
                      <h2 className="mt-1 break-words font-display text-xl font-semibold text-navy">
                        {item.kind === 'TITLE' ? item.title : genreLabel(item.genre)}
                      </h2>
                      {item.author && <p className="text-navy">by {item.author}</p>}
                      <p className="mt-1 text-sm text-slate">{describe(item)}</p>
                      {item.kind === 'TITLE' && item.title && (
                        <Link to={`/search?${paramsFromFilters({ q: item.title })}`} className={`${linkClass} text-sm`}>
                          Search listings
                        </Link>
                      )}
                    </div>
                    <Button
                      variant="ghost"
                      disabled={removing === item.id}
                      aria-label={`Remove ${item.kind === 'TITLE' ? item.title : genreLabel(item.genre)} from wish list`}
                      onClick={() => handleRemove(item)}
                    >
                      Remove
                    </Button>
                  </div>
                </Card>
              </li>
            ))}
          </ul>
        )}
      </div>
    </div>
  );
}
