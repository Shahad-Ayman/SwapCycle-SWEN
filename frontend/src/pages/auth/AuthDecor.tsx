import type { ReactNode } from 'react';

import type { InterestCategory } from '../../types';

// Decorative art and icons for the onboarding and auth screens. All
// decoration is aria-hidden; colours come from the Tailwind tokens.

export function Leaf({ className = '' }: { className?: string }) {
  return (
    <svg aria-hidden="true" viewBox="0 0 120 200" className={className} fill="none">
      <path d="M60 196 C58 140 60 80 62 8" className="stroke-navy" strokeWidth="3" strokeLinecap="round" />
      <path d="M61 150 C30 140 14 112 18 86 C40 92 58 118 61 150Z" className="fill-navy" />
      <path d="M61 118 C92 110 108 84 104 58 C82 64 64 90 61 118Z" className="fill-rose-deep" />
      <path d="M62 82 C36 72 26 46 32 24 C52 32 62 56 62 82Z" className="fill-navy" />
      <path d="M62 52 C82 44 92 26 90 8 C74 14 64 30 62 52Z" className="fill-rose" />
    </svg>
  );
}

export function Sparkle({ className = '' }: { className?: string }) {
  return (
    <svg aria-hidden="true" viewBox="0 0 24 24" className={className}>
      <path d="M12 0 L14 10 L24 12 L14 14 L12 24 L10 14 L0 12 L10 10Z" className="fill-rose-deep" />
    </svg>
  );
}

/** The illustrated book stack from the Welcome screen. */
export function BookStack({ className = '' }: { className?: string }) {
  const book = (y: number, x: number, cover: string, spine: string, lines: [string, string]) => (
    <g transform={`translate(${x} ${y})`}>
      <rect width="230" height="64" rx="8" className={cover} />
      <rect x="196" width="34" height="64" rx="6" className="fill-blush" />
      <path d="M200 10h26M200 22h26M200 34h26M200 46h26" className="stroke-soft" strokeWidth="2" />
      <rect x="164" width="8" height="64" className={spine} />
      <text x="44" y="28" className="fill-current font-display text-[17px]">
        {lines[0]}
      </text>
      <text x="44" y="50" className="fill-current font-display text-[17px]">
        {lines[1]}
      </text>
    </g>
  );
  return (
    <svg aria-hidden="true" viewBox="0 0 280 230" className={className}>
      <g className="text-navy">{book(8, 40, 'fill-rose', 'fill-rose-deep', ['Good', 'Ideas'])}</g>
      <g className="text-rose">{book(78, 20, 'fill-navy', 'fill-navy-deep', ['Better', 'People'])}</g>
      <g className="text-navy">{book(148, 0, 'fill-pink-light', 'fill-rose', ['New', 'Stories'])}</g>
    </svg>
  );
}

export function StepProgress({ step, total, label }: { step: number; total: number; label: string }) {
  return (
    <div
      role="progressbar"
      aria-label={label}
      aria-valuemin={1}
      aria-valuemax={total}
      aria-valuenow={step}
      aria-valuetext={`Step ${step} of ${total}`}
      className="flex items-center justify-center"
    >
      {Array.from({ length: total }, (_, index) => (
        <span key={index} className="flex items-center">
          {index > 0 && <span className="h-0.5 w-10 bg-slate sm:w-14" />}
          <span
            className={`h-3.5 w-3.5 rounded-full ${index + 1 <= step ? 'bg-rose-deep' : 'bg-slate'}`}
            aria-hidden="true"
          />
        </span>
      ))}
    </div>
  );
}

export function DotIndicator({ active, total }: { active: number; total: number }) {
  return (
    <div aria-hidden="true" className="flex justify-center gap-3">
      {Array.from({ length: total }, (_, index) => (
        <span key={index} className={`h-2.5 w-2.5 rounded-full ${index === active ? 'bg-rose-deep' : 'bg-soft'}`} />
      ))}
    </div>
  );
}

function Icon({ children, className = 'h-5 w-5' }: { children: ReactNode; className?: string }) {
  return (
    <svg
      aria-hidden="true"
      viewBox="0 0 24 24"
      className={className}
      fill="none"
      stroke="currentColor"
      strokeWidth="1.6"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      {children}
    </svg>
  );
}

export const MailIcon = () => (
  <Icon>
    <rect x="3" y="5" width="18" height="14" rx="2" />
    <path d="M3 7l9 6 9-6" />
  </Icon>
);

export const LockIcon = () => (
  <Icon>
    <rect x="5" y="11" width="14" height="10" rx="2" />
    <path d="M8 11V8a4 4 0 018 0v3M12 15v2" />
  </Icon>
);

export const UserIcon = () => (
  <Icon>
    <circle cx="12" cy="8" r="4" />
    <path d="M4 21c1.5-4 4.5-6 8-6s6.5 2 8 6" />
  </Icon>
);

export const EyeIcon = ({ crossed = false }: { crossed?: boolean }) => (
  <Icon>
    <path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12z" />
    <circle cx="12" cy="12" r="3" />
    {crossed && <path d="M4 4l16 16" />}
  </Icon>
);

export const ArrowRightIcon = () => (
  <Icon>
    <path d="M5 12h14M13 6l6 6-6 6" />
  </Icon>
);

export const PinIcon = () => (
  <Icon>
    <path d="M12 21s7-6.5 7-12a7 7 0 10-14 0c0 5.5 7 12 7 12z" />
    <circle cx="12" cy="9" r="2.5" />
  </Icon>
);

const categoryPaths: Record<InterestCategory, ReactNode> = {
  FICTION: <path d="M3 5c3-1 6-1 9 1 3-2 6-2 9-1v14c-3-1-6-1-9 1-3-2-6-2-9-1zM12 6v14" />,
  NON_FICTION: <path d="M6 3h9l4 4v14H6zM14 3v5h5M9 12h7M9 16h7" />,
  SCIENCE: <path d="M9 3h6M10 3v6l-5 10a1.5 1.5 0 001.3 2h11.4a1.5 1.5 0 001.3-2l-5-10V3M7.5 15h9" />,
  TECHNOLOGY: <path d="M4 5h16v11H4zM2 19h20" />,
  ART_DESIGN: (
    <path d="M12 3a9 9 0 100 18c1.4 0 2-1 2-2s-1-1.6-1-2.5S14 15 15 15h2a4 4 0 004-4c0-4.4-4-8-9-8zM7.5 11h.01M10 7.5h.01M14.5 7.5h.01" />
  ),
  SELF_HELP: <path d="M12 21c-5 0-8-3-8-7 3 0 6 1 8 4 2-3 5-4 8-4 0 4-3 7-8 7zM12 18c-2-3-2-8 0-13 2 5 2 10 0 13z" />,
  BUSINESS: <path d="M4 20V10h4v10M10 20V5h4v15M16 20v-7h4v7M2 20h20" />,
  OTHER: (
    <>
      <circle cx="12" cy="12" r="9" />
      <path d="M8 12h.01M12 12h.01M16 12h.01" />
    </>
  ),
};

export function CategoryIcon({ category }: { category: InterestCategory }) {
  return <Icon className="h-7 w-7">{categoryPaths[category]}</Icon>;
}
