-- 001_users
-- Reader accounts, general meeting areas, reading interests and password resets.
-- Only a general meeting area is stored: never a home address.

CREATE EXTENSION IF NOT EXISTS citext;

CREATE TABLE meeting_areas (
    id   SMALLSERIAL PRIMARY KEY,
    code TEXT NOT NULL UNIQUE CHECK (code ~ '^[A-Z_]{2,40}$'),
    name TEXT NOT NULL UNIQUE CHECK (char_length(name) BETWEEN 2 AND 80)
);

-- Reference data: general campus areas. Exact public meeting points are added
-- later by the handover module.
INSERT INTO meeting_areas (code, name) VALUES
    ('MAIN_LIBRARY',   'Main Library'),
    ('STUDENT_UNION',  'Student Union'),
    ('SCIENCE_QUARTER','Science Quarter'),
    ('ARTS_QUARTER',   'Arts Quarter'),
    ('SPORTS_CENTRE',  'Sports Centre'),
    ('CITY_CAMPUS',    'City Campus');

CREATE TABLE users (
    id                    UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email                 CITEXT NOT NULL UNIQUE CHECK (char_length(email) BETWEEN 3 AND 254),
    password_hash         TEXT NOT NULL,
    display_name          TEXT NOT NULL CHECK (char_length(display_name) BETWEEN 2 AND 40),
    meeting_area_id       SMALLINT REFERENCES meeting_areas (id),
    role                  TEXT NOT NULL DEFAULT 'READER' CHECK (role IN ('READER', 'MODERATOR')),
    show_meeting_area     BOOLEAN NOT NULL DEFAULT TRUE,
    show_interests        BOOLEAN NOT NULL DEFAULT TRUE,
    onboarding_completed  BOOLEAN NOT NULL DEFAULT FALSE,
    created_at            TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at            TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE user_interests (
    user_id  UUID NOT NULL REFERENCES users (id) ON DELETE CASCADE,
    category TEXT NOT NULL CHECK (category IN (
        'FICTION', 'NON_FICTION', 'SCIENCE', 'TECHNOLOGY',
        'ART_DESIGN', 'SELF_HELP', 'BUSINESS', 'OTHER')),
    PRIMARY KEY (user_id, category)
);

-- Only a SHA-256 hash of each reset token is stored. A token is single-use
-- (used_at) and expires (expires_at).
CREATE TABLE password_reset_tokens (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id     UUID NOT NULL REFERENCES users (id) ON DELETE CASCADE,
    token_hash  TEXT NOT NULL UNIQUE,
    expires_at  TIMESTAMPTZ NOT NULL,
    used_at     TIMESTAMPTZ,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_password_reset_tokens_user ON password_reset_tokens (user_id);
