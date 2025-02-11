import { SidePanelMessageTypes } from '@shared/side-panel-message';
import { vscode } from '../../../utils/vscode';
import './CompletionSection.css';
import { VscodeIcon } from '../../shared/VscodeIcon/VscodeIcon';
import React, { useState } from 'react';
import { Checkbox } from '../../shared/Checkbox/Checkbox';
import { ExamplePromptSelect } from './ExamplePromptSelect/ExamplePromptSelect';
import { ExamplePromptName, EXAMPLE_PROMPT_NAME_TO_PROMPT_MAP } from '@shared/example-prompt';

interface CompletionSectionProps {
  isLoading: boolean;
  platform: NodeJS.Platform;
}

export function CompletionSection({ isLoading, platform }: CompletionSectionProps): JSX.Element {
  const handleGenerateClick = () => {
    vscode.postMessage({
      type: SidePanelMessageTypes.GENERATE_COMPLETION_CLICK,
    });
  };

  const platformKeyBinding = platform === 'darwin' ? 'Cmd+Alt+Space' : 'Ctrl+Alt+Space';
  const [checked, setchecked] = useState(false);
  const [examplePromptName, setExamplePromptName] = useState(ExamplePromptName.EMAIL_VALIDATION);
  
  const handleCheckbox = (isChecked : boolean) => {
    setchecked(isChecked)
  };

  const handlePromptSelectChange = (examplePromptName : ExamplePromptName) => {
    setExamplePromptName(examplePromptName)
    vscode.postMessage({
      type: SidePanelMessageTypes.EXAMPLE_PROMPT_SELECT,
      payload: {
        examplePromptName
      },
    });
  };

  return (
    <section className="completion-section">
      <h3>Code Completion</h3>
      <span>
        {/* TODO Consider getting keybinding from package.json */}
        To generate inline code completion use combination <kbd>{platformKeyBinding}</kbd> or press the button below.
      </span>
      <br />
      <button className="generate-button" onClick={handleGenerateClick} disabled={isLoading}>
        {isLoading && <VscodeIcon iconName="loading" spin></VscodeIcon>}
        <span>{isLoading ? 'Generating' : 'Generate'}</span>
      </button>
      <br />

      <div>
        <label>
          <Checkbox disabled={false} checked={checked} onChange={handleCheckbox} children='Use example prompts'/>
        </label>
          {checked && (
            <ExamplePromptSelect
              disabled={false}
              onChange={handlePromptSelectChange}
              selectedExamplePromptName={examplePromptName}
            ></ExamplePromptSelect>
          )}
        </div>
    </section>
  );
}
