// ---------------------------------------------------------------------
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

import { window } from 'vscode';
import { AutoDocstring } from './generate-docstring';
import { extensionState } from '../state';

export function generateCommandHandler() {
  const { isSummarizationSupported } = extensionState.state.features;
  const editor = window.activeTextEditor;
  if (!isSummarizationSupported || !editor) {
    return;
  }

  try {
    const autoDocstring = new AutoDocstring(editor);
    return autoDocstring.generate();
  } catch (error) {
    // todo: add proper logger
    console.error(error);
  }
}
