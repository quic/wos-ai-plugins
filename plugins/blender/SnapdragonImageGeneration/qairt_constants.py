# =============================================================================
#
# Copyright (c) 2026, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================


import os
import pathlib

# Function to convert a given path to UNC path
def to_unc_path(path):
    return f"\\\\?\\{path}"

PLUGIN_DIR=pathlib.Path(__file__).parent.resolve()
LONG_PATH_PREFIX_PLUGIN_DIR=to_unc_path(PLUGIN_DIR)
VENV_PATH=os.path.join(PLUGIN_DIR, "venv")
VENV_PYTHON=os.path.join(VENV_PATH, "Scripts", "python.exe")

CONTROLNET_PIPELINE=os.path.join(PLUGIN_DIR, "model_pipelines", "ControlNetCanny.py")
OUTPUTS_DIR=os.path.join(PLUGIN_DIR, "Outputs")
LOGS_DIR=os.path.join(PLUGIN_DIR, "logs")
LOG_FILE=os.path.join(LOGS_DIR, "model_log.txt")

QNN_SDK_DOWNLOAD_URL="https://softwarecenter.qualcomm.com/api/download/software/sdks/Qualcomm_AI_Runtime_Community/All/2.44.0.260225/v2.44.0.260225.zip"

QAIRT_VERSION = "2.44.0.260225"
DSP_ARCH = ["73", "81"]  # For X-Elite device.

QNN_LIBS_DIR = os.path.join(PLUGIN_DIR, "qnn_assets", "qnn_libs")
SDK_SAVE_PATH= os.path.join(LONG_PATH_PREFIX_PLUGIN_DIR, f"{QAIRT_VERSION}.zip")
QAIRT_DIR=f"C:\\Qualcomm\\AIStack\\QAIRT"
QNN_SDK_ROOT=f"C:\\Qualcomm\\AIStack\\QAIRT\\{QAIRT_VERSION}"

CONTROLNET_DIR=os.path.join(PLUGIN_DIR, "qnn_assets", "models", "controlnet")
CACHE_DIR = os.path.join(PLUGIN_DIR, "qnn_assets", "models", "cache")
CONVERTION_DIR = os.path.join(PLUGIN_DIR, "model_conversion")

#ControlNet -Canny 0.34.3 bin files
CONTROLNET_HF_URLS = {
    "controlnet.bin":
        "https://huggingface.co/qualcomm/ControlNet-Canny/resolve/v0.34.3/precompiled/qualcomm-snapdragon-x-elite/ControlNet-Canny_controlnet_w8a16.bin",

    "text_encoder.bin":
        "https://huggingface.co/qualcomm/ControlNet-Canny/resolve/v0.34.3/precompiled/qualcomm-snapdragon-x-elite/ControlNet-Canny_text_encoder_w8a16.bin",

    "unet.bin":
        "https://huggingface.co/qualcomm/ControlNet-Canny/resolve/v0.34.3/precompiled/qualcomm-snapdragon-x-elite/ControlNet-Canny_unet_w8a16.bin",

    "vae.bin":
        "https://huggingface.co/qualcomm/ControlNet-Canny/resolve/v0.34.3/precompiled/qualcomm-snapdragon-x-elite/ControlNet-Canny_vae_w8a16.bin",
}
