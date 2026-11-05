import { useState } from 'react';
import { useNavigate } from 'react-router-dom';

import { createBook } from '../../api/books';
import BookForm from '../../components/books/BookForm';
import { useAuth } from '../../hooks/useAuth';
import { useMeetingAreas } from '../../hooks/useMeetingAreas';
import type { BookInput } from '../../types';
import { errorMessage } from '../../utils/errors';
import { BackLink, PageHeading } from '../PageParts';

export default function AddBook() {
  const { user } = useAuth();
  const { areas } = useMeetingAreas();
  const navigate = useNavigate();
  const [serverError, setServerError] = useState<string>();

  const initial: BookInput = {
    title: '',
    author: '',
    edition: '',
    language: 'en',
    genre: 'FICTION',
    condition: 'GOOD',
    conditionNotes: '',
    exchangeAreaId: user?.meetingArea?.id ?? null,
  };

  async function handleSubmit(input: BookInput) {
    setServerError(undefined);
    try {
      const listing = await createBook(input);
      // Photos need the new copy's id, so they are added on the edit page.
      navigate(`/books/${listing.id}/edit`, { replace: true, state: { created: true } });
    } catch (error) {
      setServerError(errorMessage(error, 'We could not list your book. Please try again.'));
    }
  }

  return (
    <div className="mx-auto max-w-2xl">
      <BackLink to="/my-books" label="Back to my books" />
      <PageHeading title="List a book" subtitle="Describe the copy you are offering. You can add photos next." />
      <BookForm
        initial={initial}
        areas={areas}
        submitLabel="List book"
        serverError={serverError}
        onSubmit={handleSubmit}
      />
    </div>
  );
}
