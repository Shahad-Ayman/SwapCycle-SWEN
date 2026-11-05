import { useParams } from 'react-router-dom';

import { getPublicProfile } from '../../api/auth';
import Card from '../../components/ui/Card';
import { useAsync } from '../../hooks/useAsync';
import { errorMessage, isNotFound } from '../../utils/errors';
import { formatDate, interestLabel } from '../../utils/format';
import { Alert, BackLink, EmptyState, LoadingState, PageHeading } from '../PageParts';

export default function PublicProfile() {
  const { userId = '' } = useParams();
  const { data: profile, error, loading } = useAsync(() => getPublicProfile(userId), [userId]);

  if (loading) return <LoadingState label="Loading profile…" />;
  if (!profile) {
    return isNotFound(error) ? (
      <EmptyState title="Reader not found" />
    ) : (
      <Alert tone="error">{errorMessage(error)}</Alert>
    );
  }

  return (
    <div className="mx-auto max-w-2xl space-y-6">
      <BackLink />
      <PageHeading title={profile.displayName} subtitle={`Member since ${formatDate(profile.memberSince)}`} />
      <Card>
        <dl className="space-y-4">
          <div>
            <dt className="text-sm text-slate">Meeting area</dt>
            <dd className="font-medium text-navy">{profile.meetingArea?.name ?? 'Not shared'}</dd>
          </div>
          <div>
            <dt className="text-sm text-slate">Reading interests</dt>
            <dd className="mt-1">
              {profile.interests === null ? (
                <span className="text-navy">Not shared</span>
              ) : profile.interests.length === 0 ? (
                <span className="text-navy">None chosen yet</span>
              ) : (
                <ul className="flex flex-wrap gap-2">
                  {profile.interests.map((interest) => (
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
    </div>
  );
}
