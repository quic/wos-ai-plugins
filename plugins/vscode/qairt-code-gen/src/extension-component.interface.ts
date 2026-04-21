// ---------------------------------------------------------------------
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

import { ExtensionContext } from 'vscode';

export interface IExtensionComponent {
  activate(context: ExtensionContext): void;
  deactivate(): void;
}
