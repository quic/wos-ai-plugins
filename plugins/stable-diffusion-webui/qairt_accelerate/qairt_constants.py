# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================


import os
from modules.paths_internal import script_path, extensions_dir, models_path
from pathlib import Path

QNN_SDK_DOWNLOAD_URL="https://softwarecenter.qualcomm.com/api/download/software/qualcomm_neural_processing_sdk/v2.22.0.240425.zip"
QAI_APPBUILDER_WHEEL="https://github.com/quic/ai-engine-direct-helper/releases/download/v2.23.1/qai_appbuilder-2.22.0-cp310-cp310-win_amd64.whl"

QAIRT_VERSION = "2.22.0.240425"
DSP_ARCH = "73"  # For X-Elite device.
EXTENSION_WS = os.path.join(extensions_dir, "qairt_accelerate")
VENV_PYTHON_PATH = f"{EXTENSION_WS}\\..\\..\\venv\\Scripts\\python.exe"
QAI_HUB_VENV_PATH = f"{EXTENSION_WS}\\qai_hub_venv"
QAI_HUB_VENV_PYTHON_PATH = f"{EXTENSION_WS}\\qai_hub_venv\\Scripts\\python.exe"
QNN_LIBS_DIR = os.path.join(EXTENSION_WS, "qnn_assets", "qnn_libs")
CACHE_DIR = os.path.join(EXTENSION_WS, "qnn_assets", "cache")
SDK_SAVE_PATH= EXTENSION_WS + f"\\{QAIRT_VERSION}.zip"
QAIRT_DIR=f"C:\\Qualcomm\\AIStack\\QAIRT"
QNN_SDK_ROOT=f"C:\\Qualcomm\\AIStack\\QAIRT\\{QAIRT_VERSION}"

SD_MODEL_1_5_REVISION="120de88f304daa9d5fa726ddccdfe086b6349801"
SD_MODEL_2_1_REVISION="52f821ad5420d1b0408a8b856733f9e372e7776a"

CONTROLNET_1_0_SD_1_5_CANNY_REPO_ID="qualcomm/ControlNet"
CONTROLNET_1_0_SD_1_5_CANNY_REVISION="0d33fd550da477d324a396b6e95914f65bcb3333"

DEFAULT_TXT2IMG_MODEL = "Stable-Diffusion-1.5"
DEFAULT_IMG2IMG_MODEL = "ControlNet-v10-sd15-canny"
SD2_1_UNCLIP_MODELS = 'Stable-Diffusion2.1-Unclip'


SD_UNCLIP_MODELS = [SD2_1_UNCLIP_MODELS]
CONTROLNET_MODELS = [DEFAULT_IMG2IMG_MODEL]
STABLE_DIFFUSION_MODELS = [DEFAULT_TXT2IMG_MODEL, "Stable-Diffusion-2.1"]


HUB_ID="aac24f12d047e7f558d8effe4b2fdad0f5c2c341"
QAI_HUB_CONFIG = os.path.join(Path.home(), ".qai_hub", "client.ini")
QAI_HUB_CONFIG_BACKUP = os.path.join(Path.home(), ".qai_hub", "client.ini.bk")
CONVERTION_DIR = os.path.join(EXTENSION_WS, "model_conversion")
ESRGAN_X4_MODEL_ID="mq3e3v1km"
ESRGAN_X4_MODEL_CHECKSUM=os.path.join(CONVERTION_DIR,".esrgan")
ESRGAN_X4_MODEL_PATH=os.path.join(CONVERTION_DIR,"esrgan.bin")

SD2_1_UNCLIP_PATH=os.path.abspath(
        os.path.join(
            models_path, "Stable-diffusion", f"qcom-{'sd2_1_unclip'}"
        )
    )
SD2_1_UNCLIP = {
    'mq244v8jm': 'ImageEncoder_Quantized_16.bin', 
    'mmr66vy6m': 'TextEncoder_Quantized.bin', 
    'mq9pp3rln': 'Unet_Quantized.bin', 
    'mqpzze5vn': 'Vae_Quantized.bin',
    'm9m5v7p6n': 'unclip_time_embeddings.pt',
    'mwn0g87xm': 'unclip_class_embeddings.pt'
    }


TIMESTEP_EMBEDDING_1_5_MODEL_ID="m7mrzdgxn"
TIMESTEP_EMBEDDING_2_1_MODEL_ID="m0q96xyyq"
TIMESTEP_EMBEDDING_1_5_MODEL_PATH=os.path.join(CONVERTION_DIR,"time_embedding_sd_1_5.pt")
TIMESTEP_EMBEDDING_2_1_MODEL_PATH=os.path.join(CONVERTION_DIR,"time_embedding_sd_2_1.pt")
