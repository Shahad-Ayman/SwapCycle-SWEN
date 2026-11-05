import { useId, useState, type FormEvent } from 'react';

import Button from '../../components/ui/Button';
import { useAuth } from '../../hooks/useAuth';
import type { PrivacySettings as Privacy } from '../../types';
import { errorMessage } from '../../utils/errors';
import { Alert, BackLink, PageHeading } from '../PageParts';

export default function PrivacySettings() {
  const { user, updateProfile } = useAuth();
  const [privacy, setPrivacy] = useState<Privacy>(user?.privacy ?? { showMeetingArea: true, showInterests: true });
  const [message, setMessage] = useState<{ tone: 'success' | 'error'; text: string }>();
  const [saving, setSaving] = useState(false);

  async function handleSubmit(event: FormEvent) {
    event.preventDefault();
    setSaving(true);
    setMessage(undefined);
    try {
      await updateProfile({ privacy });
      setMessage({ tone: 'success', text: 'Privacy settings saved.' });
    } catch (failure) {
      setMessage({ tone: 'error', text: errorMessage(failure) });
    } finally {
      setSaving(false);
    }
  }

  return (
    <div className="mx-auto max-w-2xl">
      <BackLink to="/profile" label="Back to profile" />
      <PageHeading
        title="Privacy settings"
        subtitle="Your display name is always visible. Your email and exact meeting details never appear on your profile."
      />
      <form onSubmit={handleSubmit} className="space-y-4">
        {message && <Alert tone={message.tone}>{message.text}</Alert>}
        <fieldset className="space-y-3">
          <legend className="sr-only">What other readers can see</legend>
          <Toggle
            label="Show my meeting area"
            description="Lets nearby readers know where you usually swap."
            checked={privacy.showMeetingArea}
            onChange={(checked) => setPrivacy({ ...privacy, showMeetingArea: checked })}
          />
          <Toggle
            label="Show my reading interests"
            description="Shows the categories you chose on your public profile."
            checked={privacy.showInterests}
            onChange={(checked) => setPrivacy({ ...privacy, showInterests: checked })}
          />
        </fieldset>
        <Button type="submit" variant="primary" loading={saving} disabled={saving}>
          Save settings
        </Button>
      </form>
    </div>
  );
}

function Toggle({
  label,
  description,
  checked,
  onChange,
}: {
  label: string;
  description: string;
  checked: boolean;
  onChange: (checked: boolean) => void;
}) {
  const id = useId();
  return (
    <div className="flex items-start gap-4 rounded-card border border-soft bg-white p-4 focus-within:ring-2 focus-within:ring-rose-deep">
      <input
        id={id}
        type="checkbox"
        aria-describedby={`${id}-description`}
        className="mt-1 h-5 w-5 cursor-pointer accent-rose-deep"
        checked={checked}
        onChange={(event) => onChange(event.target.checked)}
      />
      <div>
        <label htmlFor={id} className="block cursor-pointer font-medium text-navy">
          {label}
        </label>
        <p id={`${id}-description`} className="text-sm text-slate">
          {description}
        </p>
      </div>
    </div>
  );
}
