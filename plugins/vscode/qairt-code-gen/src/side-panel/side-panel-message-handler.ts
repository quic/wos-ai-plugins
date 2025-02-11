import { ISidePanelMessage, SidePanelMessageTypes } from '@shared/side-panel-message';
import { extensionState } from '../state';
import { Webview, commands } from 'vscode';
import * as vscode_ from 'vscode';
import { settingsService } from '../settings/settings.service';
import { COMMANDS } from '../constants';
import { ModelName } from '@shared/model';
import { ProfileName } from '@shared/profile';
import { ExamplePromptName, EXAMPLE_PROMPT_NAME_TO_PROMPT_MAP, EXAMPLE_PROMPT_NAME_TO_PROFILE } from '@shared/example-prompt';

type SidePanelMessageHandlerType = (webview: Webview, payload?: ISidePanelMessage['payload']) => void;

const sidePanelMessageHandlers: Record<SidePanelMessageTypes, SidePanelMessageHandlerType> = {
  [SidePanelMessageTypes.GET_EXTENSION_STATE]: (webview) => void webview.postMessage(extensionState.state),
  [SidePanelMessageTypes.SETTINGS_CLICK]: () => settingsService.openSettings(),
  [SidePanelMessageTypes.MODEL_CHANGE]: (_, payload) =>
    settingsService.updateSetting('model', (payload as { modelName: ModelName }).modelName),
  [SidePanelMessageTypes.PROFILE_CHANGE]: (_, payload) =>
    settingsService.updateSetting('profile', (payload as { profileName: ProfileName }).profileName),
  [SidePanelMessageTypes.CUSTOM_PROMPT_CHECKBOX_CHANGE]: (_, payload) =>
    settingsService.updateSetting('useCustomSystemPrompt', (payload as { isChecked: boolean }).isChecked),
  [SidePanelMessageTypes.CUSTOM_PROMPT_CHANGE]: (_, payload) =>
    settingsService.updateSetting('customSystemPrompt', (payload as { customSystemPrompt: string }).customSystemPrompt),
  [SidePanelMessageTypes.EXAMPLE_PROMPT_SELECT]: (_, payload) => {
    const examplePromptName = (payload as { examplePromptName: ExamplePromptName}).examplePromptName;
    const prompt : string = EXAMPLE_PROMPT_NAME_TO_PROMPT_MAP[examplePromptName];
    const profile = EXAMPLE_PROMPT_NAME_TO_PROFILE[examplePromptName];

    settingsService.updateSetting('profile', profile);
    
    var newFile : vscode_.Uri;
    const randomNumber = Math.floor(Math.random() * 10000); // Generate a random number
    const workspaceFolders = vscode_.workspace.workspaceFolders;
    if (workspaceFolders && workspaceFolders.length > 0) {
      const workspaceFolder = workspaceFolders[0].uri.fsPath;
      newFile = vscode_.Uri.parse('untitled:' + workspaceFolder + '/newfile' + randomNumber + '.py');
    } else {
      newFile = vscode_.Uri.parse('untitled:' + 'newfile' + randomNumber + '.py');
    }

    vscode_.workspace.openTextDocument(newFile).then(document => {
      vscode_.window.showTextDocument(document).then(editor => {
        const edit = new vscode_.WorkspaceEdit();
        const position = new vscode_.Position(0, 0); // Start at the beginning of the document

        edit.insert(newFile, position, prompt);

        vscode_.workspace.applyEdit(edit).then(success => {
          if (success) {
            vscode_.window.showTextDocument(document);
          } else {
            vscode_.window.showInformationMessage('Error!');
          }
        });
      });
    });
  },
  [SidePanelMessageTypes.START_SERVER_CLICK]: () => void commands.executeCommand(COMMANDS.START_SERVER_NATIVE),
  [SidePanelMessageTypes.STOP_SERVER_CLICK]: () => void commands.executeCommand(COMMANDS.STOP_SERVER_NATIVE),
  [SidePanelMessageTypes.SHOW_SERVER_LOG_CLICK]: () => void commands.executeCommand(COMMANDS.SHOW_SERVER_LOG),
  [SidePanelMessageTypes.SHOW_EXTENSION_LOG_CLICK]: () => void commands.executeCommand(COMMANDS.SHOW_EXTENSION_LOG),
  [SidePanelMessageTypes.CHECK_CONNECTION_CLICK]: () => void commands.executeCommand(COMMANDS.CHECK_CONNECTION),
  [SidePanelMessageTypes.GENERATE_COMPLETION_CLICK]: () =>
    void commands.executeCommand(COMMANDS.GENERATE_INLINE_COPMLETION),
};

export function handleSidePanelMessage<M extends ISidePanelMessage>(message: M, webview: Webview): void {
  const { type, payload } = message;
  const handler = sidePanelMessageHandlers[type];
  handler(webview, payload);
}
