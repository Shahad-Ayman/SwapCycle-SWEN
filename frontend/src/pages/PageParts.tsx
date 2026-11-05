import { useId, type ReactNode, type SelectHTMLAttributes } from 'react';
import { Link, useNavigate } from 'react-router-dom';

// Small building blocks shared by the pages. Anything that grows into a
// general-purpose component should move to components/ui.

export function PageHeading({ title, subtitle, action }: { title: string; subtitle?: string; action?: ReactNode }) {
  return (
    <header className="mb-6 flex flex-wrap items-end justify-between gap-4">
      <div>
        <h1 className="font-display text-3xl font-bold text-navy sm:text-4xl">{title}</h1>
        {subtitle && <p className="mt-1 text-slate">{subtitle}</p>}
      </div>
      {action}
    </header>
  );
}

export function Alert({ tone, children }: { tone: 'error' | 'success' | 'info'; children: ReactNode }) {
  const styles = {
    error: 'border-danger text-danger',
    success: 'border-success text-success',
    info: 'border-soft text-navy',
  }[tone];
  return (
    <div
      role={tone === 'error' ? 'alert' : 'status'}
      className={`rounded-card border bg-white px-4 py-3 text-sm font-medium ${styles}`}
    >
      {children}
    </div>
  );
}

export function LoadingState({ label = 'Loading…' }: { label?: string }) {
  return (
    <p role="status" className="py-12 text-center text-slate">
      {label}
    </p>
  );
}

export function EmptyState({ title, children }: { title: string; children?: ReactNode }) {
  return (
    <div className="rounded-card border border-dashed border-soft bg-white px-6 py-10 text-center">
      <h2 className="font-display text-xl font-semibold text-navy">{title}</h2>
      {children && <div className="mt-2 text-slate">{children}</div>}
    </div>
  );
}

export function BackLink({ to, label = 'Back' }: { to?: string; label?: string }) {
  const navigate = useNavigate();
  const className =
    'inline-flex h-11 w-11 items-center justify-center rounded-full text-current hover:bg-pink-light/40 ' +
    'focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-rose-deep';
  const arrow = (
    <svg aria-hidden="true" viewBox="0 0 24 24" className="h-6 w-6" fill="none" stroke="currentColor" strokeWidth="2">
      <path d="M15 5l-7 7 7 7" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
  if (to) {
    return (
      <Link to={to} aria-label={label} className={className}>
        {arrow}
      </Link>
    );
  }
  return (
    <button type="button" aria-label={label} onClick={() => navigate(-1)} className={className}>
      {arrow}
    </button>
  );
}

type SelectFieldProps = SelectHTMLAttributes<HTMLSelectElement> & {
  label: string;
  error?: string;
  hideLabel?: boolean;
  /** "dark" for the navy onboarding screen. */
  tone?: 'light' | 'dark';
  icon?: ReactNode;
};

/** Native select styled like the design's inputs (white, thin pink border). */
export function SelectField({
  label,
  error,
  hideLabel,
  tone = 'light',
  icon,
  className = '',
  children,
  ...props
}: SelectFieldProps) {
  const id = useId();
  const errorId = `${id}-error`;
  const dark = tone === 'dark';
  return (
    <div className={className}>
      <label
        htmlFor={id}
        className={hideLabel ? 'sr-only' : `mb-2 block font-medium ${dark ? 'text-white' : 'text-sm text-navy'}`}
      >
        {label}
      </label>
      <div className="relative">
        {icon && (
          <span className={`pointer-events-none absolute left-4 top-1/2 -translate-y-1/2 ${dark ? 'text-white' : 'text-navy'}`}>
            {icon}
          </span>
        )}
        <select
          id={id}
          aria-invalid={error ? true : undefined}
          aria-describedby={error ? errorId : undefined}
          className={
            `h-12 w-full rounded-card border px-4 ${icon ? 'pl-12' : ''} ` +
            'focus:outline-none focus-visible:ring-2 focus-visible:ring-rose ' +
            (dark ? 'border-slate bg-navy text-white ' : 'bg-white text-navy ') +
            (error ? 'border-danger' : dark ? '' : 'border-soft')
          }
          {...props}
        >
          {children}
        </select>
      </div>
      {error && (
        <p id={errorId} className={`mt-1 text-sm ${dark ? 'text-rose' : 'text-danger'}`}>
          {error}
        </p>
      )}
    </div>
  );
}

export function Pagination({
  page,
  pageSize,
  total,
  onChange,
}: {
  page: number;
  pageSize: number;
  total: number;
  onChange: (page: number) => void;
}) {
  const pages = Math.max(1, Math.ceil(total / pageSize));
  if (pages <= 1) return null;
  const button =
    'rounded-full border border-soft bg-white px-4 py-2 text-sm font-medium text-navy hover:bg-pink-light ' +
    'disabled:cursor-not-allowed disabled:opacity-50 focus-visible:outline focus-visible:outline-2 ' +
    'focus-visible:outline-offset-2 focus-visible:outline-rose-deep';
  return (
    <nav aria-label="Pages" className="mt-8 flex items-center justify-center gap-4">
      <button type="button" className={button} disabled={page <= 1} onClick={() => onChange(page - 1)}>
        Previous
      </button>
      <span className="text-sm text-slate">
        Page {page} of {pages}
      </span>
      <button type="button" className={button} disabled={page >= pages} onClick={() => onChange(page + 1)}>
        Next
      </button>
    </nav>
  );
}

export const linkClass =
  'font-semibold text-rose-deep underline-offset-4 hover:underline focus-visible:outline ' +
  'focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-rose-deep rounded';
