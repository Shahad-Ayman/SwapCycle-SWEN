import { useState } from 'react';
import { useLocation, useNavigate, useParams } from 'react-router-dom';

import { deleteBook, deletePhoto, getMyBook, relistBook, updateBook, uploadPhoto, withdrawBook } from '../../api/books';
import BookForm from '../../components/books/BookForm';
import PhotoUploader from '../../components/books/PhotoUploader';
import Badge from '../../components/ui/Badge';
import Button from '../../components/ui/Button';
import { useAsync } from '../../hooks/useAsync';
import { useMeetingAreas } from '../../hooks/useMeetingAreas';
import type { BookInput, BookListing } from '../../types';
import { LIMITS } from '../../utils/constants';
import { errorMessage, isNotFound } from '../../utils/errors';
import { statusLabel, statusTone } from '../../utils/format';
import { validatePhotoFile } from '../../utils/validation';
import { Alert, BackLink, EmptyState, LoadingState, PageHeading } from '../PageParts';

function toInput(listing: BookListing): BookInput {
  return {
    title: listing.title,
    author: listing.author,
    edition: listing.edition,
    language: listing.language,
    genre: listing.genre,
    condition: listing.condition,
    conditionNotes: listing.conditionNotes,
    exchangeAreaId: listing.exchangeArea?.id ?? null,
  };
}

export default function EditBook() {
  const { copyId = '' } = useParams();
  const location = useLocation();
  const navigate = useNavigate();
  const { areas } = useMeetingAreas();
  const { data: listing, error, loading, setData } = useAsync(() => getMyBook(copyId), [copyId]);
  const [message, setMessage] = useState<{ tone: 'success' | 'error'; text: string } | undefined>(
    (location.state as { created?: boolean } | null)?.created
      ? { tone: 'success', text: 'Your book is listed. Add a few photos so readers can see its condition.' }
      : undefined,
  );
  const [serverError, setServerError] = useState<string>();
  const [busy, setBusy] = useState(false);
  const [confirmingDelete, setConfirmingDelete] = useState(false);

  if (loading) return <LoadingState label="Loading your listing…" />;
  if (!listing) {
    return isNotFound(error) ? (
      <EmptyState title="Listing not found">It may have been deleted, or it belongs to someone else.</EmptyState>
    ) : (
      <Alert tone="error">{errorMessage(error)}</Alert>
    );
  }

  // Reserved or exchanged copies are locked: they are part of an exchange.
  const editable = listing.status === 'AVAILABLE' || listing.status === 'WITHDRAWN';

  async function run(action: () => Promise<void>, success?: string) {
    setBusy(true);
    setMessage(undefined);
    try {
      await action();
      if (success) setMessage({ tone: 'success', text: success });
    } catch (failure) {
      setMessage({ tone: 'error', text: errorMessage(failure) });
    } finally {
      setBusy(false);
    }
  }

  async function handleSave(input: BookInput) {
    setServerError(undefined);
    try {
      setData(await updateBook(copyId, input));
      setMessage({ tone: 'success', text: 'Changes saved.' });
    } catch (failure) {
      setServerError(errorMessage(failure, 'We could not save your changes. Please try again.'));
    }
  }

  async function handleUpload(file: File) {
    const invalid = validatePhotoFile(file);
    if (invalid) throw new Error(invalid);
    const photo = await uploadPhoto(copyId, file);
    setData((current) => (current ? { ...current, photos: [...current.photos, photo] } : current));
  }

  async function handleRemovePhoto(photoId: string) {
    await deletePhoto(copyId, photoId);
    setData((current) =>
      current ? { ...current, photos: current.photos.filter((photo) => photo.id !== photoId) } : current,
    );
  }

  return (
    <div className="mx-auto max-w-2xl space-y-8">
      <div>
        <BackLink to="/my-books" label="Back to my books" />
        <PageHeading
          title="Edit listing"
          subtitle={listing.title}
          action={<Badge tone={statusTone(listing.status)}>{statusLabel(listing.status)}</Badge>}
        />
        {message && <Alert tone={message.tone}>{message.text}</Alert>}
        {!editable && (
          <Alert tone="info">This book is part of an exchange, so it cannot be changed right now.</Alert>
        )}
      </div>

      <section aria-labelledby="photos-title" className="rounded-card border border-soft bg-white p-5">
        <h2 id="photos-title" className="font-display text-xl font-semibold text-navy">
          Photos
        </h2>
        <p className="mb-4 text-sm text-slate">
          JPEG or PNG, up to 5 MB each, at most {LIMITS.photosPerBook}. Show the cover, spine and any wear.
        </p>
        <PhotoUploader
          photos={listing.photos}
          max={LIMITS.photosPerBook}
          disabled={!editable}
          onUpload={handleUpload}
          onRemove={handleRemovePhoto}
        />
      </section>

      {editable && (
        <section aria-labelledby="details-title">
          <h2 id="details-title" className="mb-4 font-display text-xl font-semibold text-navy">
            Details
          </h2>
          <BookForm
            initial={toInput(listing)}
            areas={areas}
            submitLabel="Save changes"
            serverError={serverError}
            onSubmit={handleSave}
          />
        </section>
      )}

      {editable && (
        <section aria-labelledby="availability-title" className="rounded-card border border-soft bg-white p-5">
          <h2 id="availability-title" className="font-display text-xl font-semibold text-navy">
            Availability
          </h2>
          <div className="mt-4 flex flex-wrap gap-3">
            {listing.status === 'AVAILABLE' ? (
              <Button
                variant="secondary"
                disabled={busy}
                onClick={() =>
                  run(async () => setData(await withdrawBook(copyId)), 'Withdrawn. Readers can no longer find this book.')
                }
              >
                Withdraw from exchange
              </Button>
            ) : (
              <Button
                variant="secondary"
                disabled={busy}
                onClick={() => run(async () => setData(await relistBook(copyId)), 'Listed again. Readers can find it.')}
              >
                Make available again
              </Button>
            )}

            {confirmingDelete ? (
              <div role="group" aria-label="Confirm delete" className="flex flex-wrap items-center gap-3">
                <span className="text-navy">Delete this listing?</span>
                <Button
                  variant="danger"
                  disabled={busy}
                  onClick={() =>
                    run(async () => {
                      await deleteBook(copyId);
                      navigate('/my-books', { replace: true });
                    })
                  }
                >
                  Yes, delete
                </Button>
                <Button variant="ghost" onClick={() => setConfirmingDelete(false)}>
                  Cancel
                </Button>
              </div>
            ) : (
              <Button variant="danger" disabled={busy} onClick={() => setConfirmingDelete(true)}>
                Delete listing
              </Button>
            )}
          </div>
        </section>
      )}
    </div>
  );
}
