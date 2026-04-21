// ---------------------------------------------------------------------
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

export function getFunctionName(functionDefinition: string): string {
  const pattern = /(?:def|class)\s+(\w+)\s*\(/;

  const match = pattern.exec(functionDefinition);

  if (match == undefined || match[1] == undefined) {
    return "";
  }

  return match[1];
}
