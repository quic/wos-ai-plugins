// ---------------------------------------------------------------------
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

import { ExtensionContext, ExtensionMode, window } from 'vscode';
import { SidePanelViewProvider } from './side-panel-view-provider';
import { IExtensionComponent } from '../extension-component.interface';

class SidePanel implements IExtensionComponent {
  activate(context: ExtensionContext): void {
    const isProductionMode = context.extensionMode === ExtensionMode.Production;
    const sidePanelViewProvider = new SidePanelViewProvider(context.extensionUri, isProductionMode);

    context.subscriptions.push(window.registerWebviewViewProvider(SidePanelViewProvider.viewId, sidePanelViewProvider));
  }

  deactivate(): void {}
}

export const sidePanel = new SidePanel();
