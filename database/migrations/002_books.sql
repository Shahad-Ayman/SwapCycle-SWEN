-- 002_books
-- A BookDescription (what the book is) is modelled separately from a
-- physical BookCopy (who owns it, its condition, photos and status).

CREATE EXTENSION IF NOT EXISTS pg_trgm;

CREATE TABLE book_descriptions (
    id           UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    title        TEXT NOT NULL CHECK (char_length(title) BETWEEN 1 AND 200),
    author       TEXT NOT NULL CHECK (char_length(author) BETWEEN 1 AND 120),
    edition      TEXT NOT NULL DEFAULT '' CHECK (char_length(edition) <= 60),
    language     TEXT NOT NULL CHECK (language ~ '^[a-z]{2}$'),
    genre        TEXT NOT NULL CHECK (genre IN (
        'FICTION', 'MYSTERY', 'SCIENCE_FICTION', 'FANTASY', 'ROMANCE', 'THRILLER',
        'BIOGRAPHY', 'HISTORY', 'TRAVEL', 'SCIENCE', 'TECHNOLOGY', 'ART_DESIGN',
        'SELF_HELP', 'BUSINESS', 'POETRY', 'OTHER')),
    -- Normalised copies used for de-duplication, search and matching.
    title_norm   TEXT GENERATED ALWAYS AS (lower(btrim(regexp_replace(title, '\s+', ' ', 'g')))) STORED,
    author_norm  TEXT GENERATED ALWAYS AS (lower(btrim(regexp_replace(author, '\s+', ' ', 'g')))) STORED,
    edition_norm TEXT GENERATED ALWAYS AS (lower(btrim(regexp_replace(edition, '\s+', ' ', 'g')))) STORED,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (title_norm, author_norm, edition_norm, language, genre)
);

CREATE INDEX idx_book_descriptions_title_trgm  ON book_descriptions USING gin (title_norm gin_trgm_ops);
CREATE INDEX idx_book_descriptions_author_trgm ON book_descriptions USING gin (author_norm gin_trgm_ops);
CREATE INDEX idx_book_descriptions_genre_lang  ON book_descriptions (genre, language);

CREATE TABLE book_copies (
    id               UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    description_id   UUID NOT NULL REFERENCES book_descriptions (id),
    owner_id         UUID NOT NULL REFERENCES users (id),
    condition        TEXT NOT NULL CHECK (condition IN ('NEW', 'LIKE_NEW', 'GOOD', 'FAIR', 'POOR')),
    -- Higher is better; lets "minimum condition" filters use a range comparison.
    condition_rank   SMALLINT GENERATED ALWAYS AS (CASE condition
                         WHEN 'NEW' THEN 5 WHEN 'LIKE_NEW' THEN 4 WHEN 'GOOD' THEN 3
                         WHEN 'FAIR' THEN 2 ELSE 1 END) STORED,
    condition_notes  TEXT NOT NULL DEFAULT '' CHECK (char_length(condition_notes) <= 500),
    exchange_area_id SMALLINT REFERENCES meeting_areas (id),
    status           TEXT NOT NULL DEFAULT 'AVAILABLE'
                         CHECK (status IN ('AVAILABLE', 'RESERVED', 'TRANSFERRED', 'WITHDRAWN')),
    -- Listings are soft-deleted so exchange history and condition snapshots
    -- that reference a copy are never lost.
    deleted_at       TIMESTAMPTZ,
    created_at       TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at       TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_book_copies_owner       ON book_copies (owner_id) WHERE deleted_at IS NULL;
CREATE INDEX idx_book_copies_description ON book_copies (description_id);
CREATE INDEX idx_book_copies_search      ON book_copies (status, exchange_area_id, condition_rank)
    WHERE deleted_at IS NULL;

CREATE TABLE book_photos (
    id           UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    copy_id      UUID NOT NULL REFERENCES book_copies (id),
    content_type TEXT NOT NULL CHECK (content_type IN ('image/jpeg', 'image/png')),
    byte_size    INTEGER NOT NULL CHECK (byte_size > 0),
    created_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
    deleted_at   TIMESTAMPTZ
);

CREATE INDEX idx_book_photos_copy ON book_photos (copy_id) WHERE deleted_at IS NULL;
