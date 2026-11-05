import type { ButtonHTMLAttributes, InputHTMLAttributes, ReactNode } from 'react';

import type { CurrentUser } from '../types';

// Simple stand-ins for components owned by other modules, so page tests check
// page behaviour only. Use with vi.mock(), e.g.
//   vi.mock('../../components/ui/Button', async () => ({ default: (await import('../../utils/testHelpers')).MockButton }));

export function MockButton({
  loading,
  fullWidth: _fullWidth,
  variant: _variant,
  children,
  type = 'button',
  ...props
}: ButtonHTMLAttributes<HTMLButtonElement> & { loading?: boolean; fullWidth?: boolean; variant?: string }) {
  return (
    <button type={type} aria-busy={loading || undefined} {...props}>
      {children}
    </button>
  );
}

export function MockInput({
  label,
  error,
  hint: _hint,
  hideLabel: _hideLabel,
  icon: _icon,
  trailing,
  id,
  ...props
}: InputHTMLAttributes<HTMLInputElement> & {
  label: string;
  error?: string;
  hint?: string;
  hideLabel?: boolean;
  icon?: ReactNode;
  trailing?: ReactNode;
}) {
  const inputId = id ?? `input-${label.replace(/\W+/g, '-').toLowerCase()}`;
  return (
    <div>
      <label htmlFor={inputId}>{label}</label>
      <input id={inputId} aria-invalid={error ? true : undefined} {...props} />
      {trailing}
      {error && <p>{error}</p>}
    </div>
  );
}

export function MockChip({ label, selected, onToggle }: { label: string; selected: boolean; onToggle: () => void }) {
  return (
    <button type="button" aria-pressed={selected} onClick={onToggle}>
      {label}
    </button>
  );
}

export function MockChildren({ children }: { children?: ReactNode }) {
  return <div>{children}</div>;
}

export function makeUser(overrides: Partial<CurrentUser> = {}): CurrentUser {
  return {
    id: '00000000-0000-4000-8000-000000000001',
    email: 'sara@example.com',
    displayName: 'Sara',
    role: 'READER',
    meetingArea: null,
    interests: [],
    privacy: { showMeetingArea: true, showInterests: true },
    onboardingCompleted: true,
    ...overrides,
  };
}

/** Mimics the API client's error shape: { status, code, message }. */
export function apiError(status: number, code: string, message: string): Error & { status: number; code: string } {
  return Object.assign(new Error(message), { status, code });
}
