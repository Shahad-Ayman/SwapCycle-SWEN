-- 003_wishlist
-- Wish list entries: either a specific title, or a broader interest (genre).

CREATE TABLE wish_items (
    id            UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id       UUID NOT NULL REFERENCES users (id) ON DELETE CASCADE,
    kind          TEXT NOT NULL CHECK (kind IN ('TITLE', 'INTEREST')),
    title         TEXT CHECK (char_length(title) BETWEEN 1 AND 200),
    author        TEXT CHECK (char_length(author) BETWEEN 1 AND 120),
    genre         TEXT CHECK (genre IN (
        'FICTION', 'MYSTERY', 'SCIENCE_FICTION', 'FANTASY', 'ROMANCE', 'THRILLER',
        'BIOGRAPHY', 'HISTORY', 'TRAVEL', 'SCIENCE', 'TECHNOLOGY', 'ART_DESIGN',
        'SELF_HELP', 'BUSINESS', 'POETRY', 'OTHER')),
    language      TEXT CHECK (language ~ '^[a-z]{2}$'),
    min_condition TEXT CHECK (min_condition IN ('NEW', 'LIKE_NEW', 'GOOD', 'FAIR', 'POOR')),
    title_norm    TEXT GENERATED ALWAYS AS (lower(btrim(regexp_replace(title, '\s+', ' ', 'g')))) STORED,
    author_norm   TEXT GENERATED ALWAYS AS (lower(btrim(regexp_replace(author, '\s+', ' ', 'g')))) STORED,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    CHECK ((kind = 'TITLE' AND title IS NOT NULL) OR (kind = 'INTEREST' AND genre IS NOT NULL AND title IS NULL))
);

CREATE INDEX idx_wish_items_user  ON wish_items (user_id);
CREATE INDEX idx_wish_items_title ON wish_items (title_norm) WHERE kind = 'TITLE';
CREATE INDEX idx_wish_items_genre ON wish_items (genre) WHERE kind = 'INTEREST';

-- One entry per (user, title/author/genre/language) so duplicates are rejected.
CREATE UNIQUE INDEX uq_wish_items_entry ON wish_items (
    user_id, kind, COALESCE(title_norm, ''), COALESCE(author_norm, ''),
    COALESCE(genre, ''), COALESCE(language, ''));
