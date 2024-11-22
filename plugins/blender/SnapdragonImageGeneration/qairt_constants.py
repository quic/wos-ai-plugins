# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================


import os
import pathlib

PLUGIN_DIR=pathlib.Path(__file__).parent.resolve()
VENV_PATH=os.path.join(PLUGIN_DIR, "venv")
VENV_PYTHON=os.path.join(VENV_PATH, "Scripts", "python.exe")

CONTROLNET_PIPELINE=os.path.join(PLUGIN_DIR, "model_pipelines", "ControlNetCanny.py")
OUTPUTS_DIR=os.path.join(PLUGIN_DIR, "Outputs")
LOGS_DIR=os.path.join(PLUGIN_DIR, "logs")
LOG_FILE=os.path.join(LOGS_DIR, "model_log.txt")

QNN_SDK_DOWNLOAD_URL="https://softwarecenter.qualcomm.com/api/download/software/qualcomm_neural_processing_sdk/v2.22.0.240425.zip"

QAIRT_VERSION = "2.22.0.240425"
DSP_ARCH = "73"  # For X-Elite device.

QNN_LIBS_DIR = os.path.join(PLUGIN_DIR, "qnn_assets", "qnn_libs")
SDK_SAVE_PATH= os.path.join(PLUGIN_DIR, f"{QAIRT_VERSION}.zip")
QAIRT_DIR=f"C:\\Qualcomm\\AIStack\\QAIRT"
QNN_SDK_ROOT=f"C:\\Qualcomm\\AIStack\\QAIRT\\{QAIRT_VERSION}"

CONTROLNET_DIR=os.path.join(PLUGIN_DIR, "qnn_assets", "models", "controlnet")
CACHE_DIR = os.path.join(PLUGIN_DIR, "qnn_assets", "models", "cache")
TIME_EMBEDDING_PATH=os.path.join(CACHE_DIR, "time-embedding")

HUB_ID="aac24f12d047e7f558d8effe4b2fdad0f5c2c341"
QAI_HUB_CONFIG = os.path.join(pathlib.Path.home(), ".qai_hub", "client.ini")
QAI_HUB_CONFIG_BACKUP = os.path.join(pathlib.Path.home(), ".qai_hub", "client.ini.bk")
CONVERTION_DIR = os.path.join(PLUGIN_DIR, "model_conversion")
ESRGAN_X4_MODEL_ID="monl8r2wq"
ESRGAN_X4_MODEL_PATH=os.path.join(CONVERTION_DIR,"esrgan.bin")

TIMESTEP_EMBEDDING_1_5_MODEL_ID="m7mrzdgxn"
TIMESTEP_EMBEDDING_1_5_MODEL_PATH=os.path.join(CONVERTION_DIR,"time_embedding_sd_1_5.pt")