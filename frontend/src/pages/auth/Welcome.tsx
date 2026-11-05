import { Link, Navigate, useNavigate } from 'react-router-dom';

import Button from '../../components/ui/Button';
import { useAuth } from '../../hooks/useAuth';
import { ArrowRightIcon, BookStack, DotIndicator, Leaf, Sparkle } from './AuthDecor';

export default function Welcome() {
  const { status } = useAuth();
  const navigate = useNavigate();

  if (status === 'authenticated') return <Navigate to="/home" replace />;

  return (
    <main className="relative flex min-h-screen flex-col overflow-hidden bg-blush px-6 pb-10 pt-6">
      {/* Soft background shapes from the prototype. */}
      <div aria-hidden="true" className="absolute -right-24 -top-24 h-80 w-80 rounded-full bg-pink-light" />
      <div aria-hidden="true" className="absolute -bottom-32 -left-24 h-80 w-80 rounded-full bg-pink-light" />
      <Leaf className="absolute right-2 top-40 h-48 w-auto rotate-12 sm:right-[12%]" />
      <Leaf className="absolute -left-6 bottom-56 h-40 w-auto -rotate-12 sm:left-[10%]" />

      <div className="relative mx-auto flex w-full max-w-md flex-1 flex-col">
        <div className="flex justify-end">
          <Link to="/home" className="rounded px-2 py-1 font-medium text-navy hover:underline focus-visible:outline focus-visible:outline-2 focus-visible:outline-rose-deep">
            Skip
          </Link>
        </div>

        <h1 className="mt-12 font-display text-6xl font-bold leading-[0.95] text-navy sm:text-7xl">
          Swap
          <br />
          Cycle
        </h1>
        <p className="mt-4 text-xl text-navy">
          Same books.
          <br />
          Brighter stories.
        </p>
        <Sparkle className="ml-auto mr-16 h-6 w-6" />

        <BookStack className="mx-auto mt-6 w-full max-w-xs" />

        <div className="mt-auto space-y-5 pt-10">
          <Button variant="primary" fullWidth onClick={() => navigate('/register')}>
            <span className="inline-flex items-center gap-2">
              Get Started <ArrowRightIcon />
            </span>
          </Button>
          <p className="text-center">
            <Link
              to="/login"
              className="rounded px-2 py-1 text-lg font-semibold text-navy underline-offset-4 hover:underline focus-visible:outline focus-visible:outline-2 focus-visible:outline-rose-deep"
            >
              Log In
            </Link>
          </p>
          <DotIndicator active={0} total={3} />
        </div>
      </div>
    </main>
  );
}
