import { ExtensionContext, commands } from 'vscode';

import { COMMANDS, EXTENSION_PACKAGE, EXTENSION_SERVER_DISPLAY_NAME } from '../constants';
import { IExtensionComponent } from '../extension-component.interface';
import { NativeCPlusPlusServerRunner } from './server-runner';
import { extensionState } from '../state';

class CPlusPlusServerService implements IExtensionComponent {
  private _cPlusPlusServer = new NativeCPlusPlusServerRunner();

  activate(context: ExtensionContext): void {
    const startCommandDisposable = commands.registerCommand(COMMANDS.START_SERVER_NATIVE, async () => {
      void commands.executeCommand(COMMANDS.SHOW_SERVER_LOG);
      await this._cPlusPlusServer.start();
    });

    const stopCommandDisposable = commands.registerCommand(COMMANDS.STOP_SERVER_NATIVE, () =>
      this._cPlusPlusServer.stop()
    );

    const showLogCommandDisposable = commands.registerCommand(COMMANDS.SHOW_SERVER_LOG, () => {
      void commands.executeCommand(
        `workbench.action.output.show.${EXTENSION_PACKAGE.fullName}.${EXTENSION_SERVER_DISPLAY_NAME}`
      );
    });

    const stateSubscriptionDisposable = this._cPlusPlusServer.stateReporter.subscribeToStateChange((serverState) => {
      extensionState.set('server', serverState);
    });

    context.subscriptions.push(
      startCommandDisposable,
      stopCommandDisposable,
      showLogCommandDisposable,
      stateSubscriptionDisposable
    );
  }

  deactivate() {
    this._cPlusPlusServer.stop();
  }
}

export const cPlusPlusServerService = new CPlusPlusServerService();
