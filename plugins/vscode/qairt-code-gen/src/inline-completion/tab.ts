// ---------------------------------------------------------------------
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

let isGeneralTabActiveInternal: boolean = false;

export function setIsGeneralTabActive(value: boolean): void {
  isGeneralTabActiveInternal = value;
}

export function getIsGeneralTabActive(): boolean {
  return isGeneralTabActiveInternal;
}