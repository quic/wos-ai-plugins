import { Features } from './features';

enum ModelId {
  LLAMA_CHAT_V2_7B = "llama-v2-7b",
  LLAMA_CHAT_V3_1_8B = "llama-v3.1-8b"
}

export enum ModelName {
  LLAMA_CHAT_V2_7B = 'llama-v2-7b',
  LLAMA_CHAT_V3_1_8B = "llama-v3.1-8b"
}

export const MODEL_NAME_TO_ID_MAP: Record<ModelName, ModelId> = {
  [ModelName.LLAMA_CHAT_V2_7B]: ModelId.LLAMA_CHAT_V2_7B,
  [ModelName.LLAMA_CHAT_V3_1_8B]: ModelId.LLAMA_CHAT_V3_1_8B,
};

export const MODEL_SUPPORTED_FEATURES: Record<ModelName, Features[]> = {
  [ModelName.LLAMA_CHAT_V2_7B]: [Features.CODE_COMPLETION],
  [ModelName.LLAMA_CHAT_V3_1_8B]: [Features.CODE_COMPLETION, Features.FIM],
};
