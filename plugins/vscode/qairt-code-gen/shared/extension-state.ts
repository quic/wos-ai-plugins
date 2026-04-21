// ---------------------------------------------------------------------
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

import { ExtensionConfiguration } from '../src/configuration';
import { Features } from './features';
import { ServerState } from './server-state';

export enum ConnectionStatus {
  NOT_AVAILABLE = 'NOT_AVAILABLE',
  AVAILABLE = 'AVAILABLE',
  PENDING = 'PENDING',
}

interface IStateFeatures {
  get supportedList(): Features[];
  get isSummarizationSupported(): boolean;
}

export interface IExtensionState {
  isLoading: boolean;
  connectionStatus: ConnectionStatus;
  server: ServerState;
  get isServerAvailable(): boolean;
  get config(): ExtensionConfiguration;
  features: IStateFeatures;
  platform: NodeJS.Platform;
}
