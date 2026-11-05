import { useState, type FormEvent } from 'react';
import { useNavigate } from 'react-router-dom';

import Button from '../../components/ui/Button';
import Chip from '../../components/ui/Chip';
import Input from '../../components/ui/Input';
import { useAuth } from '../../hooks/useAuth';
import { useMeetingAreas } from '../../hooks/useMeetingAreas';
import type { InterestCategory } from '../../types';
import { INTEREST_CATEGORIES, LIMITS } from '../../utils/constants';
import { errorMessage } from '../../utils/errors';
import { validateDisplayName } from '../../utils/validation';
import { Alert, BackLink, PageHeading, SelectField } from '../PageParts';

export default function EditProfile() {
  const { user, updateProfile } = useAuth();
  const { areas } = useMeetingAreas();
  const navigate = useNavigate();
  const [displayName, setDisplayName] = useState(user?.displayName ?? '');
  const [areaId, setAreaId] = useState<number | null>(user?.meetingArea?.id ?? null);
  const [interests, setInterests] = useState<InterestCategory[]>(user?.interests ?? []);
  const [nameError, setNameError] = useState<string>();
  const [formError, setFormError] = useState<string>();
  const [saving, setSaving] = useState(false);

  function toggle(category: InterestCategory) {
    setInterests((current) =>
      current.includes(category) ? current.filter((item) => item !== category) : [...current, category],
    );
  }

  async function handleSubmit(event: FormEvent) {
    event.preventDefault();
    const invalid = validateDisplayName(displayName);
    setNameError(invalid);
    setFormError(undefined);
    if (invalid) return;

    setSaving(true);
    try {
      await updateProfile({ displayName: displayName.trim(), meetingAreaId: areaId, interests });
      navigate('/profile', { replace: true });
    } catch (failure) {
      setFormError(errorMessage(failure, 'We could not save your profile. Please try again.'));
      setSaving(false);
    }
  }

  return (
    <div className="mx-auto max-w-2xl">
      <BackLink to="/profile" label="Back to profile" />
      <PageHeading title="Edit profile" />
      <form noValidate onSubmit={handleSubmit} className="space-y-6 rounded-card border border-soft bg-white p-5 sm:p-6">
        {formError && <Alert tone="error">{formError}</Alert>}
        <Input
          label="Display name"
          value={displayName}
          maxLength={LIMITS.displayNameMax}
          onChange={(event) => setDisplayName(event.target.value)}
          error={nameError}
        />
        <div>
          <SelectField
            label="General meeting area"
            value={areaId ?? ''}
            onChange={(event) => setAreaId(event.target.value ? Number(event.target.value) : null)}
          >
            <option value="">No preference</option>
            {areas.map((area) => (
              <option key={area.id} value={area.id}>
                {area.name}
              </option>
            ))}
          </SelectField>
          <p className="mt-1 text-sm text-slate">A general campus area only. Never enter a home address.</p>
        </div>
        <fieldset>
          <legend className="mb-2 font-medium text-navy">Reading interests</legend>
          <div className="grid grid-cols-2 gap-3">
            {INTEREST_CATEGORIES.map((category) => (
              <Chip
                key={category.value}
                tone="light"
                label={category.label}
                selected={interests.includes(category.value)}
                onToggle={() => toggle(category.value)}
              />
            ))}
          </div>
        </fieldset>
        <Button type="submit" variant="primary" fullWidth loading={saving} disabled={saving}>
          Save profile
        </Button>
      </form>
    </div>
  );
}
