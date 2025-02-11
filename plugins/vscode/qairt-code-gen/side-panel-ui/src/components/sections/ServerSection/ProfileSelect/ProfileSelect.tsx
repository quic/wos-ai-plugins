import { ProfileName } from '@shared/profile';
import { Select, SelectOptionProps } from '../../../shared/Select/Select';
import { ServerStatus } from '@shared/server-state';
// import { Features } from '@shared/features';

const options: SelectOptionProps<ProfileName>[] = [
  { value: ProfileName.CODE_GENERATION },
  { value: ProfileName.CODE_REFACTOR },
  { value: ProfileName.TEST_EXPERT },
  { value: ProfileName.CODE_DEBUGGER },
];

interface ProfileSelectProps {
  disabled: boolean;
  selectedProfileName: ProfileName;
  onChange: (profileName: ProfileName) => void;
  serverStatus: ServerStatus;
}

export const ProfileSelect = ({
  disabled,
  selectedProfileName,
  onChange,
  serverStatus,
}: ProfileSelectProps): JSX.Element => {
  const isServerStopped = serverStatus === ServerStatus.STOPPED;
  return (
    <>
      <Select
        label="Profile"
        options={options}
        selectedValue={selectedProfileName}
        disabled={disabled}
        onChange={(value) => onChange(value)}
      ></Select>
    </>
  );
};
