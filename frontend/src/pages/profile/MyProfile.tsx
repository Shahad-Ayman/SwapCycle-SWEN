import { useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';

import Button from '../../components/ui/Button';
import Card from '../../components/ui/Card';
import { useAuth } from '../../hooks/useAuth';
import { interestLabel } from '../../utils/format';
import { PageHeading, linkClass } from '../PageParts';

export default function MyProfile() {
  const { user, logout } = useAuth();
  const navigate = useNavigate();
  const [loggingOut, setLoggingOut] = useState(false);

  if (!user) return null;

  async function handleLogout() {
    setLoggingOut(true);
    await logout();
    navigate('/', { replace: true });
  }

  return (
    <div className="mx-auto max-w-2xl space-y-6">
      <PageHeading
        title={user.displayName}
        subtitle="Your account"
        action={
          <Link to="/profile/edit" className={linkClass}>
            Edit profile
          </Link>
        }
      />

      <Card>
        <dl className="grid gap-4 sm:grid-cols-2">
          <div>
            <dt className="text-sm text-slate">Email (only you can see this)</dt>
            <dd className="break-all font-medium text-navy">{user.email}</dd>
          </div>
          <div>
            <dt className="text-sm text-slate">Meeting area</dt>
            <dd className="font-medium text-navy">{user.meetingArea?.name ?? 'Not set'}</dd>
          </div>
          <div className="sm:col-span-2">
            <dt className="text-sm text-slate">Reading interests</dt>
            <dd className="mt-1">
              {user.interests.length === 0 ? (
                <span className="text-navy">None chosen yet</span>
              ) : (
                <ul className="flex flex-wrap gap-2">
                  {user.interests.map((interest) => (
                    <li key={interest} className="rounded-full bg-pink-light px-3 py-1 text-sm text-navy">
                      {interestLabel(interest)}
                    </li>
                  ))}
                </ul>
              )}
            </dd>
          </div>
        </dl>
      </Card>

      <Card>
        <h2 className="font-display text-xl font-semibold text-navy">Privacy</h2>
        <ul className="mt-2 space-y-1 text-navy">
          <li>Meeting area is {user.privacy.showMeetingArea ? 'visible' : 'hidden'} on your profile.</li>
          <li>Reading interests are {user.privacy.showInterests ? 'visible' : 'hidden'} on your profile.</li>
        </ul>
        <Link to="/profile/privacy" className={`${linkClass} mt-3 inline-block`}>
          Change privacy settings
        </Link>
      </Card>

      <div className="flex flex-wrap gap-3">
        <Link to={`/users/${user.id}`} className={linkClass}>
          See your public profile
        </Link>
      </div>

      <Button variant="secondary" onClick={handleLogout} loading={loggingOut} disabled={loggingOut}>
        Log out
      </Button>
    </div>
  );
}
