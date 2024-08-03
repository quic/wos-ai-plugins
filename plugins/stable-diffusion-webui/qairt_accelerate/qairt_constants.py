# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================


import os
from modules.paths_internal import script_path, extensions_dir

QNN_SDK_DOWNLOAD_URL="https://softwarecenter.qualcomm.com/api/download/software/qualcomm_neural_processing_sdk/v2.24.0.240626.zip"
QAI_APPBUILDER_WHEEL="https://github.com/quic/ai-engine-direct-helper/releases/download/v2.24.0/qai_appbuilder-2.24.0-cp310-cp310-win_amd64.whl"

QAIRT_VERSION = "2.24.0.240626"
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


HUB_ID="aac24f12d047e7f558d8effe4b2fdad0f5c2c341"
CONVERTION_DIR = os.path.join(EXTENSION_WS, "model_conversion")
ESRGAN_X4_MODEL_ID="meq29lx7n"
ESRGAN_X4_MODEL_PATH=os.path.join(CONVERTION_DIR,"esrgan.bin")
