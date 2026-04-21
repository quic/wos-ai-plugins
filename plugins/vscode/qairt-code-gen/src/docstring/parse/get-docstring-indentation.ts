// ---------------------------------------------------------------------
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

import { blankLine, getIndentation } from "./utilities";

export function getDocstringIndentation(
  document: string,
  linePosition: number,
  defaultIndentation: string
): string {
  const lines = document.split("\n");
  const definitionPattern = /\b(((async\s+)?\s*def)|\s*class)\b/g;

  let currentLineNum = linePosition;

  while (currentLineNum >= 0) {
    const currentLine = lines[currentLineNum];

    if (!blankLine(currentLine)) {
      if (definitionPattern.test(currentLine)) {
        return getIndentation(currentLine) + defaultIndentation;
      }
    }

    currentLineNum--;
  }

  return defaultIndentation;
}
