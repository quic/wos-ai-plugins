import { ModelName } from '@shared/model';
import { Select, SelectOptionProps } from '../../../shared/Select/Select';
import { ServerStatus } from '@shared/server-state';
import { Features } from '@shared/features';

const options: SelectOptionProps<ModelName>[] = [
  { value: ModelName.LLAMA_CHAT_V2_7B },
  { value: ModelName.LLAMA_CHAT_V3_1_8B },
];

interface ModelSelectProps {
  disabled: boolean;
  selectedModelName: ModelName;
  onChange: (modelName: ModelName) => void;
  supportedFeatures: Features[];
  serverStatus: ServerStatus;
}

export const ModelSelect = ({
  disabled,
  selectedModelName,
  onChange,
  supportedFeatures,
  serverStatus,
}: ModelSelectProps): JSX.Element => {
  const isServerStopped = serverStatus === ServerStatus.STOPPED;
  return (
    <>
      <Select
        label="Model"
        options={options}
        selectedValue={selectedModelName}
        disabled={disabled}
        onChange={(value) => onChange(value)}
      ></Select>
      {/* {isServerStopped && <span>Supported Features: {supportedFeatures.join(', ')}</span>} */}
    </>
  );
};
