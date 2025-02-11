import { EventEmitter as NodeEventEmitter } from 'stream';
import {
  InlineCompletionItem,
  InlineCompletionItemProvider,
  Position,
  Range,
  TextDocument,
  commands,
  window,
} from 'vscode';
import { EXTENSION_DISPLAY_NAME } from '../constants';
import completionService from './completion.service';
import { ProfileName, PROFILE_NAME_TO_PROMPT_MAP } from '@shared/profile';
import { extensionState } from '../state';

interface ICommandInlineCompletionItemProvider extends InlineCompletionItemProvider {
  triggerCompletion(onceCompleted: () => void): void;
}

/**
 * Trigger {@link ICommandInlineCompletionItemProvider.provideInlineCompletionItems}.
 * Executing editor.action.inlineSuggest.trigger command doesn't trigger inline completion when inlineSuggestionVisible context key is set.
 * Executing editor.action.inlineSuggest.hide before editor.action.inlineSuggest.trigger will make inline completion text bliks.
 * Replacing previous character before trigger seems to do the job.
 */
async function triggerInlineCompletionProvider(): Promise<void> {
  const editor = window.activeTextEditor;
  if (!editor) {
    return;
  }

  const document = editor.document;
  const activePosition = editor.selection.active;
  const activeOffset = document.offsetAt(activePosition);

  if (activeOffset === 0) {
    return;
  }

  const prevCharPosition = document.positionAt(activeOffset - 1);
  const replaceRange = new Range(prevCharPosition, activePosition);
  const value = document.getText(replaceRange);

  await editor.edit((edit) => edit.replace(replaceRange, value));
  await commands.executeCommand('editor.action.inlineSuggest.trigger');
}

export class StreamingCommandInlineCompletionItemProvider implements ICommandInlineCompletionItemProvider {
  private _isCommandRunning = false;

  private readonly _emitter = new NodeEventEmitter().setMaxListeners(1);

  private _streamBuffer: string = '';
  private _codeBlocks: string[] = [];

  private readonly _commandCompletedEvent = 'CommandInlineCompletionItemProvider:completed';

  private _abortController = new AbortController();

  private _beforeComplete(): void {
    this._isCommandRunning = false;
    this._streamBuffer = '';
    this._codeBlocks = [];
    this._abortController.abort();
    this._abortController = new AbortController();
    this._emitter.emit(this._commandCompletedEvent);
  }

  async triggerCompletion(onceCompleted: () => void) {
    this._emitter.once(this._commandCompletedEvent, onceCompleted);

    if (!window.activeTextEditor) {
      void window.showInformationMessage(`Please open a file first to use ${EXTENSION_DISPLAY_NAME}.`);
      this._beforeComplete();
      return;
    }

    if (this._isCommandRunning) {
      return;
    }

    this._isCommandRunning = true;

    void commands.executeCommand('workbench.action.focusStatusBar');
    void window.showTextDocument(window.activeTextEditor.document);

    const profileName = extensionState.config.profile;
    const useCustomSystemPrompt = extensionState.config.useCustomSystemPrompt;
    const customSystemPrompt = extensionState.config.customSystemPrompt;

    var systemPrompt: string = PROFILE_NAME_TO_PROMPT_MAP[profileName];
    if (useCustomSystemPrompt === true && customSystemPrompt !== undefined) {
      systemPrompt = customSystemPrompt
    }
    if (systemPrompt === undefined || systemPrompt === '') {
      systemPrompt = PROFILE_NAME_TO_PROMPT_MAP[ProfileName.CODE_GENERATION];
    }
    // logger.info('system_prompt: ' + systemPrompt);

    await completionService.getCompletionStream(
      window.activeTextEditor.document,
      systemPrompt,
      window.activeTextEditor.selection.active,
      async (chunk) => {
        this._streamBuffer += chunk;
        this._codeBlocks = extractCodeBlocks(this._streamBuffer);
        await triggerInlineCompletionProvider();
      },
      this._abortController.signal
    );

    function extractCodeBlocks(text: string): string[] {
      const regex = /```([^`]+)```/g;
      let match;
      const matches = [];
      while ((match = regex.exec(text)) !== null) {
          matches.push(match[1].trim());
      }
      return matches;
    }
  

    this._isCommandRunning = false;
    await triggerInlineCompletionProvider();
  }

  stopGeneration() {
    this._abortController.abort();
  }

  cancelGeneration() {
    this._beforeComplete();
  }

  wrap(s: String, w: Number) {
    return s.replace(
      new RegExp(`(?![^\\n]{1,${w}}$)([^\\n]{1,${w}})\\s`, 'g'), '$1\n');
  }

  provideInlineCompletionItems(document: TextDocument, position: Position) {
    var buffer = this._streamBuffer;
    const codeBlocksBuffer = this._codeBlocks.join("\n\n");
    if (!this._isCommandRunning) {
      this._beforeComplete();
    }

    buffer = this.wrap(buffer, 100);
    let items = [new InlineCompletionItem(`${buffer}`, new Range(position, position.translate(0, 1)))];
    items.push(new InlineCompletionItem(`${codeBlocksBuffer}`, new Range(position, position.translate(0, 1))));

    return items;
  }
}
