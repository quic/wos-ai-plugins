import { isAbortError, spawnCommand } from './commands-runner';
import { OS, detectOs } from './detect-os';
import { ServerStateController } from './server-state-controller';
import { ServerStatus, ServerStartingStage } from '@shared/server-state';
import { EXTENSION_SERVER_DISPLAY_NAME } from '../constants';
import { LogOutputChannel, window } from 'vscode';
import { join } from 'path';
import { MODEL_NAME_TO_ID_MAP, ModelName } from '@shared/model';
import { ProfileName, PROFILE_NAME_TO_PROMPT_MAP } from '@shared/profile';
import { extensionState } from '../state';
import { clearLruCache } from '../lru-cache.decorator';
import { backendService } from '../services/backend.service';

const SERVER_STARTED_STDOUT_ANCHOR = 'HTTP Server is up';

interface ServerHooks {
  onStarted: () => void;
}

async function runServer(modelName: ModelName, profileName: ProfileName, 
    useCustomSystemPrompt: boolean, customSystemPrompt: string,
    config: ServerConfiguration, hooks?: ServerHooks) {
  const { serverDir, abortSignal, logger } = config;
  logger.info('Starting server...');

  let started = false;

  function stdoutListener(data: string) {
    //logger.info(data);
    if (started) {
      return;
    }
    //logger.info("From JS");

    if (data && data.includes(SERVER_STARTED_STDOUT_ANCHOR)) {
      hooks?.onStarted();
      started = true;
      logger.info('Server started');
    }
  }

  const model = MODEL_NAME_TO_ID_MAP[modelName];
  var systemPrompt: string = PROFILE_NAME_TO_PROMPT_MAP[profileName];
  if (useCustomSystemPrompt === true && customSystemPrompt !== undefined) {
    systemPrompt = customSystemPrompt
  }
  if (systemPrompt === undefined || systemPrompt === '') {
    systemPrompt = PROFILE_NAME_TO_PROMPT_MAP[ProfileName.CODE_GENERATION];
  }
  logger.info('model: ' + model);
  logger.info('system_prompt: ' + systemPrompt);

  await spawnCommand(config.qcomCodeGenExecutable, ['--model', model], {
    logger,
    cwd: serverDir,
    abortSignal,
    listeners: { stdout: stdoutListener },
  });
}

const logger = window.createOutputChannel(EXTENSION_SERVER_DISPLAY_NAME, { log: true });

const SERVER_DIR = join(__dirname, 'server');

export interface ServerConfiguration {
  qcomCodeGenExecutable: string;
  os: OS;
  serverDir: string;
  abortSignal: AbortSignal;
  logger: LogOutputChannel;
}

export class NativeCPlusPlusServerRunner {

  private _abortController = new AbortController();
  private _isConnectedToExternalServer = false;

  private readonly _stateController = new ServerStateController();
  get stateReporter() {
    return this._stateController.reporter;
  }

  async start() {
    if (this._stateController.state.status === ServerStatus.STARTED) {
      logger.info('Server is already started. Skipping start command');
      return;
    }

    const isAvailable = await backendService.healthCheck();
    if (isAvailable) {
      logger.info('Connecting to existing server backend ' + backendService._apiUrl);
      this._stateController.setStatus(ServerStatus.STARTED);
      this._stateController.setStage(null);
      this._isConnectedToExternalServer = true;
      return
    }

    this._stateController.setStatus(ServerStatus.STARTING);

    try {
      logger.info('Starting Server...');
      await this._start();
    } catch (e) {
      const error = e instanceof Error ? e : new Error(String(e));
      if (isAbortError(error)) {
        logger.debug('Server launch was aborted');
        return;
      }
      logger.error(`Server stopped with error:`);
      logger.error(error);
    } finally {
      this._stateController.setStage(null);
      this._stateController.setStatus(ServerStatus.STOPPED);
      logger.info('Server stopped');
    }
  }

  async _start() {
    clearLruCache();
    
    const os = detectOs();
    logger.info(`System detected: ${os}`);

    const config: ServerConfiguration = {
      qcomCodeGenExecutable: `${SERVER_DIR}/bin/qcom-code-gen-server.exe`,
      os,
      serverDir: SERVER_DIR,
      abortSignal: this._abortController.signal,
      logger,
    };

    this._stateController.setStage(ServerStartingStage.START_SERVER);

    const modelName = extensionState.config.model;
    const profileName = extensionState.config.profile;
    const useCustomSystemPrompt = extensionState.config.useCustomSystemPrompt;
    const customSystemPrompt = extensionState.config.customSystemPrompt;

    await runServer(modelName, profileName, useCustomSystemPrompt, customSystemPrompt,  config, {
      onStarted: () => {
        this._stateController.setStatus(ServerStatus.STARTED);
        this._stateController.setStage(null);
      },
    });
  }

  stop() {
    logger.info('Stopping...');
    if (!this._isConnectedToExternalServer) {
      this._abortController.abort();
      this._abortController = new AbortController();
    } else {
      this._stateController.setStage(null);
      this._stateController.setStatus(ServerStatus.STOPPED);
      logger.info('Server stopped');
    }
  }
}
