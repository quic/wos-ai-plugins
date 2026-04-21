# =============================================================================
#
# Copyright (c) 2026, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================


import os
from modules.paths_internal import script_path, extensions_dir, models_path
from pathlib import Path


QAI_APPBUILDER_WHEEL_URL = "https://github.com/quic/ai-engine-direct-helper/releases/download/v2.38.0/qai_appbuilder-2.38.0-cp310-cp310-win_amd64.whl"
QAIRT_VERSION = "2.44.0.260225"
QNN_SDK_DOWNLOAD_URL = f"https://softwarecenter.qualcomm.com/api/download/software/sdks/Qualcomm_AI_Runtime_Community/All/{QAIRT_VERSION}/v{QAIRT_VERSION}.zip"
DSP_ARCHS = ["73", "81"]
EXTENSION_WS = os.path.join(extensions_dir, "qairt_accelerate")
QNN_LIBS_DIR = os.path.join(EXTENSION_WS, "qnn_assets", "qnn_libs")
CACHE_DIR = os.path.join(EXTENSION_WS, "qnn_assets", "cache")
CONVERTION_DIR = os.path.join(EXTENSION_WS, "qnn_assets", "model_conversion")


SDK_SAVE_PATH = os.path.join(EXTENSION_WS, f"{QAIRT_VERSION}.zip")
QAIRT_DIR = r"C:\Qualcomm\AIStack\QAIRT"

env_path = os.environ.get("QNN_SDK_ROOT")
if env_path:
    QNN_SDK_ROOT = Path(env_path)
else:
    QNN_SDK_ROOT = Path(rf"C:\Qualcomm\AIStack\QAIRT\{QAIRT_VERSION}")

DEFAULT_TXT2IMG_MODEL = "Stable-Diffusion-1.5"
DEFAULT_IMG2IMG_MODEL = "ControlNet-v10-sd15-canny"

CONTROLNET_MODELS = [DEFAULT_IMG2IMG_MODEL]
STABLE_DIFFUSION_MODELS = [DEFAULT_TXT2IMG_MODEL, "Stable-Diffusion-2.1"]

CONTROLNET_DIR = os.path.abspath(os.path.join(models_path, "Stable-diffusion", f"qcom-{'ControlNet-v10-sd15-canny'}"))
SD1_5_DIR = os.path.abspath(os.path.join(models_path, "Stable-diffusion", f"qcom-{'Stable-Diffusion-v1.5'}"))
SD2_1_DIR = os.path.abspath(os.path.join(models_path, "Stable-diffusion", f"qcom-{'Stable-Diffusion-v2.1'}"))
TIME_EMBEDDING_DIR = os.path.join(CONVERTION_DIR, "time_embedding")

TIMESTEP_EMBEDDING_1_5_MODEL_PATH = os.path.join(TIME_EMBEDDING_DIR, "time_embedding_sd_1_5.pt")
TIMESTEP_EMBEDDING_2_1_MODEL_PATH = os.path.join(TIME_EMBEDDING_DIR, "time_embedding_sd_2_1.pt")

TIME_EMBEDDING_1_5_MODEL_PATH = TIMESTEP_EMBEDDING_1_5_MODEL_PATH
TIME_EMBEDDING_2_1_MODEL_PATH = TIMESTEP_EMBEDDING_2_1_MODEL_PATH

PRE_COMPUTED_DATA_DIR = os.path.join(EXTENSION_WS, "pre_computed_data")

CONTROLNET_HF_URLS = {
    "controlnet.serialized.bin": "https://huggingface.co/qualcomm/ControlNet-Canny/resolve/v0.34.3/precompiled/qualcomm-snapdragon-x-elite/ControlNet-Canny_controlnet_w8a16.bin",
    "text_encoder.serialized.bin": "https://huggingface.co/qualcomm/ControlNet-Canny/resolve/v0.34.3/precompiled/qualcomm-snapdragon-x-elite/ControlNet-Canny_text_encoder_w8a16.bin",
    "unet.serialized.bin": "https://huggingface.co/qualcomm/ControlNet-Canny/resolve/v0.34.3/precompiled/qualcomm-snapdragon-x-elite/ControlNet-Canny_unet_w8a16.bin",
    "vae_decoder.serialized.bin": "https://huggingface.co/qualcomm/ControlNet-Canny/resolve/v0.34.3/precompiled/qualcomm-snapdragon-x-elite/ControlNet-Canny_vae_w8a16.bin",
}

SD1_5_HF_URLS = {
    "text_encoder.serialized.bin": "https://huggingface.co/qualcomm/Stable-Diffusion-v1.5/resolve/9e5bf02813f129951adee071c5a1723c9b239069/TextEncoderQuantizable.bin",
    "unet.serialized.bin": "https://huggingface.co/qualcomm/Stable-Diffusion-v1.5/resolve/9e5bf02813f129951adee071c5a1723c9b239069/UnetQuantizable.bin",
    "vae_decoder.serialized.bin": "https://huggingface.co/qualcomm/Stable-Diffusion-v1.5/resolve/9e5bf02813f129951adee071c5a1723c9b239069/VaeDecoderQuantizable.bin",
}

SD2_1_HF_URLS = {
    "text_encoder.serialized.bin": "https://huggingface.co/qualcomm/Stable-Diffusion-v2.1/resolve/ba5473e130aa0b9fb89dd8242ab95ae712abe728/TextEncoderQuantizable.bin",
    "unet.serialized.bin": "https://huggingface.co/qualcomm/Stable-Diffusion-v2.1/resolve/ba5473e130aa0b9fb89dd8242ab95ae712abe728/UnetQuantizable.bin",
    "vae_decoder.serialized.bin": "https://huggingface.co/qualcomm/Stable-Diffusion-v2.1/resolve/ba5473e130aa0b9fb89dd8242ab95ae712abe728/VaeDecoderQuantizable.bin",
}

