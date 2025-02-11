import { SidePanelMessageTypes } from '@shared/side-panel-message';
import { vscode } from '../../../utils/vscode';
import { IExtensionState } from '@shared/extension-state';
import { ServerStatus as ServerStatusEnum } from '@shared/server-state';
import { StartingStages } from './StartingStages/StartingStages';
import { ServerStatus } from './ServerStatus/ServerStatus';
import './ServerSection.css';
import { ModelSelect } from './ModelSelect/ModelSelect';
import { ModelName } from '@shared/model';
import { ProfileSelect } from './ProfileSelect/ProfileSelect';
import { ProfileName } from '@shared/profile';
import { useState , ChangeEvent } from 'react';
import { Checkbox } from '../../shared/Checkbox/Checkbox';

interface ServerSectionProps {
  state: IExtensionState | null;
}

export function ServerSection({ state }: ServerSectionProps): JSX.Element {
  const handleStartServerClick = () => {
    vscode.postMessage({
      type: SidePanelMessageTypes.START_SERVER_CLICK,
    });
  };

  const handleStopServerClick = () => {
    vscode.postMessage({
      type: SidePanelMessageTypes.STOP_SERVER_CLICK,
    });
  };

  const handleShowServerLogClick = () => {
    vscode.postMessage({
      type: SidePanelMessageTypes.SHOW_SERVER_LOG_CLICK,
    });
  };

  const handleCheckConnectionClick = () => {
    vscode.postMessage({
      type: SidePanelMessageTypes.CHECK_CONNECTION_CLICK,
    });
  };

  const handleModelChange = (modelName: ModelName) => {
    vscode.postMessage({
      type: SidePanelMessageTypes.MODEL_CHANGE,
      payload: {
        modelName,
      },
    });
  };

  const handleProfileChange = (profileName: ProfileName) => {
    vscode.postMessage({
      type: SidePanelMessageTypes.PROFILE_CHANGE,
      payload: {
        profileName,
      },
    });
  };

  const handleCheckbox = (isChecked : boolean) => {
    vscode.postMessage({
      type: SidePanelMessageTypes.CUSTOM_PROMPT_CHECKBOX_CHANGE,
      payload: {
        isChecked,
      },
    });
  };

  const handleSystemPromptChange = (event: ChangeEvent<HTMLInputElement>) => {
    const customSystemPrompt = event.target.value
    
    vscode.postMessage({
      type: SidePanelMessageTypes.CUSTOM_PROMPT_CHANGE,
      payload: {
        customSystemPrompt
      },
    });
  };

  
  if (!state) {
    return <>Extension state is not available</>;
  }

  const isServerStopped = state.server.status === ServerStatusEnum.STOPPED;
  const isServerStarting = state.server.status === ServerStatusEnum.STARTING;

  return (
    <section className="server-section">
      <h3>QAIRT Code</h3>
      <ServerStatus status={state.server.status} connectionStatus={state.connectionStatus}></ServerStatus>
      <ModelSelect
        disabled={!isServerStopped}
        onChange={handleModelChange}
        selectedModelName={state.config.model}
        supportedFeatures={state.features.supportedList}
        serverStatus={state.server.status}
      ></ModelSelect>
      <ProfileSelect
        disabled={false}
        onChange={handleProfileChange}
        selectedProfileName={state.config.profile}
        serverStatus={state.server.status}
      ></ProfileSelect>
      <div aria-disabled={false}>
        <label>
          <Checkbox disabled={false} checked={state.config.useCustomSystemPrompt} onChange={handleCheckbox} children='Use custom System prompt'/>
        </label>
        {state.config.useCustomSystemPrompt && (
          <div aria-disabled={false}>
            <input
              disabled={false}
              type="text"
              placeholder="Enter system prompt here"
              value={state.config.customSystemPrompt}
              onChange={handleSystemPromptChange}/>
          </div>
        )}
      </div>

      {isServerStarting && <StartingStages currentStage={state.server.stage}></StartingStages>}
      <div className="button-group">
        {isServerStopped && <button onClick={handleStartServerClick}>Start Model</button>}
        {!isServerStopped && (
          <button className="secondary" onClick={handleStopServerClick}>
            Stop Model
          </button>
        )}
      </div>
      <div className="button-group">
        <a href="#" onClick={handleShowServerLogClick}>
          Show Model Logs
        </a>
        <a href="#" onClick={handleCheckConnectionClick}>
          Check Connection
        </a>
      </div>
    </section>
  );
}
