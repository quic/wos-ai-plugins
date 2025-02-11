export const EXTENSION_PACKAGE = {
  publisher: 'QUIC',
  name: 'qairt-code-completion',
  get fullName(): string {
    return `${this.publisher}.${this.name}`;
  },
};

export const EXTENSION_DISPLAY_NAME = 'QAIRT Code';
export const EXTENSION_SERVER_DISPLAY_NAME = 'QAIRT Code Server';

export const CONFIG_KEY = 'qairtCode';

export const SIDE_PANEL_VIEW_ID = 'qairt-code-side-panel';

export const COMMANDS = {
  STATUS_BAR: 'qairtCode.statusBar',
  FOCUS_SIDE_PANEL: `${SIDE_PANEL_VIEW_ID}.focus`,
  OPEN_SETTINGS: 'qairtCode.openSettings',
  GENERATE_INLINE_COPMLETION: 'qairtCode.generateInlineCompletion',
  GENERATE_INLINE_COPMLETION_TAB: 'qairtCode.generateInlineCompletionTab',
  ACCEPT_INLINE_COMPLETION: 'qairtCode.acceptInlineCompletion',
  GENERATE_DOC_STRING: 'qairtCode.generateDocstring',
  CHECK_CONNECTION: 'qairtCode.checkConnection',
  START_SERVER_NATIVE: 'qairtCode.startServerNative',
  STOP_SERVER_NATIVE: 'qairtCode.stopServerNative',
  SHOW_SERVER_LOG: 'qairtCode.showServerLog',
  SHOW_EXTENSION_LOG: 'qairtCode.showExtensionLog',
  STOP_GENERATION: 'qairtCode.stopGeneration',
};

export const EXTENSION_CONTEXT_STATE = {
  GENERATING: 'qairtCode.generating',
};
