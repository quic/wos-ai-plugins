export enum ServerStatus {
  STOPPED = 'STOPPED',
  STARTING = 'STARTING',
  STARTED = 'STARTED',
}

export enum ServerStartingStage {
  START_SERVER,
}

export interface ServerState {
  status: ServerStatus;
  stage: ServerStartingStage | null;
}

export const INITIAL_SERVER_STATE: ServerState = {
  status: ServerStatus.STOPPED,
  stage: null,
};
