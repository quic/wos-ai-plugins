import { ExamplePromptName } from '@shared/example-prompt';
import { Select, SelectOptionProps } from '../../../shared/Select/Select';
import { ServerStatus } from '@shared/server-state';

const options: SelectOptionProps<ExamplePromptName>[] = [
  { value: ExamplePromptName.EMAIL_VALIDATION },
  { value: ExamplePromptName.FIBONACCI_NUMBERS },
  { value: ExamplePromptName.COMMENTS_FOR_EMAIL_VALIDATION },
  { value: ExamplePromptName.DEBUG_RECURSION_CODE },
  { value: ExamplePromptName.DEBUG_LAMBDA_CODE },
];

interface ExamplePromptSelectProps {
  disabled: boolean;
  selectedExamplePromptName: ExamplePromptName;
  onChange: (examplePromptName: ExamplePromptName) => void;
}

export const ExamplePromptSelect = ({
  disabled,
  selectedExamplePromptName,
  onChange,
}: ExamplePromptSelectProps): JSX.Element => {
  return (
    <>
      <Select
        label="Example Prompt"
        options={options}
        selectedValue={selectedExamplePromptName}
        disabled={disabled}
        onChange={(value) => onChange(value)}
      ></Select>
    </>
  );
};
