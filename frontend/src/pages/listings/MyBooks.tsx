import { useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';

import BookCard from '../../components/books/BookCard';
import Badge from '../../components/ui/Badge';
import Button from '../../components/ui/Button';
import { useMyBooks } from '../../hooks/useBooks';
import { errorMessage } from '../../utils/errors';
import { statusLabel, statusTone } from '../../utils/format';
import { Alert, EmptyState, LoadingState, PageHeading, Pagination, linkClass } from '../PageParts';

export default function MyBooks() {
  const navigate = useNavigate();
  const [page, setPage] = useState(1);
  const { data, error, loading } = useMyBooks(page);

  return (
    <div>
      <PageHeading
        title="My books"
        subtitle="Books you are offering, including withdrawn ones."
        action={
          <Button variant="primary" onClick={() => navigate('/books/new')}>
            List a book
          </Button>
        }
      />
      {loading && <LoadingState label="Loading your books…" />}
      {!loading && error !== undefined && <Alert tone="error">{errorMessage(error)}</Alert>}
      {!loading && data && data.items.length === 0 && (
        <EmptyState title="You have not listed any books yet">
          <Link to="/books/new" className={linkClass}>
            List your first book
          </Link>{' '}
          to start swapping.
        </EmptyState>
      )}
      {!loading && data && data.items.length > 0 && (
        <>
          <ul className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
            {data.items.map((listing) => (
              <li key={listing.id}>
                <BookCard
                  listing={listing}
                  to={`/books/${listing.id}/edit`}
                  footer={<Badge tone={statusTone(listing.status)}>{statusLabel(listing.status)}</Badge>}
                />
              </li>
            ))}
          </ul>
          <Pagination page={data.page} pageSize={data.pageSize} total={data.total} onChange={setPage} />
        </>
      )}
    </div>
  );
}
