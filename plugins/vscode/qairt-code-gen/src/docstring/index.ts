// ---------------------------------------------------------------------
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

import { ExtensionContext, commands, languages } from 'vscode';
import { IExtensionComponent } from '../extension-component.interface';
import { COMMANDS } from '../constants';
import { generateCommandHandler } from './generate-command';
import { completionItemProvider } from './completion-item-provider';

class DocString implements IExtensionComponent {
  activate(context: ExtensionContext): void {
    const commandDisposable = commands.registerCommand(COMMANDS.GENERATE_DOC_STRING, generateCommandHandler);

    const providerDisposable = languages.registerCompletionItemProvider(
      'python',
      completionItemProvider,
      '"',
      "'",
      '#'
    );

    context.subscriptions.push(commandDisposable, providerDisposable);
  }
  deactivate(): void {}
}

export const docString = new DocString();
