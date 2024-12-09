# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import launch
import os
import qairt_constants as consts
import platform


# check python version to be 3.10.6+. As qai_appbuilder is built on 3.10.6
if (not platform.python_version().startswith("3.10.")) or (int(platform.python_version().split(".")[2])<6):
    raise Exception("Python version needs to be >=3.10.6 and <3.10.14")

# Always install QAI appbuilder for plugin upgradability.
launch.run_pip(
    f"install {consts.QAI_APPBUILDER_WHEEL}",
    "Install python QNN",
)
if not launch.is_installed("diffusers"):
    launch.run_pip("install diffusers", "diffusers")
if not launch.is_installed("onnx"):
    launch.run_pip("install onnx", "onnx")

from qairt_sd_pipeline import model_path_1_5, model_path_2_1
from huggingface_hub import hf_hub_download
import common_utils as utils
import zipfile
import shutil

def download_qairt_sdk():
    # Setup QAIRT SDK
    if not os.path.isdir(consts.QNN_SDK_ROOT):
        os.makedirs(consts.QAIRT_DIR, exist_ok=True)
        print(f"Downloading QAIRT SDK...")
        utils.download_url(consts.QNN_SDK_DOWNLOAD_URL, consts.SDK_SAVE_PATH)
        print(f"QAIRT SDK downloaded.")

        with zipfile.ZipFile(consts.SDK_SAVE_PATH, "r") as zip_ref:
            zip_ref.extractall(consts.EXTENSION_WS)
        shutil.move(
            os.path.join(consts.EXTENSION_WS, "qairt", consts.QAIRT_VERSION),
            os.path.join(consts.QNN_SDK_ROOT, ".."),
        )
        shutil.rmtree(os.path.join(consts.EXTENSION_WS, "qairt"))
        os.remove(consts.SDK_SAVE_PATH)

def setup_qairt_env():
    # Preparing all the binaries and libraries for execution.
    SDK_lib_dir = consts.QNN_SDK_ROOT + "\\lib\\arm64x-windows-msvc"
    SDK_hexagon_dir = consts.QNN_SDK_ROOT + "\\lib\\hexagon-v{}\\unsigned".format(
        consts.DSP_ARCH
    )

    # Copy necessary libraries to a common location
    libs = [
        "QnnHtp.dll",
        "QnnSystem.dll",
        "QnnHtpPrepare.dll",
        "QnnHtpV{}Stub.dll".format(consts.DSP_ARCH),
    ]
    hexagon_libs = [
        "libQnnHtpV{}Skel.so".format(consts.DSP_ARCH),
        "libqnnhtpv73.cat",
    ]
    for lib in libs:
        if os.path.isfile(os.path.join(consts.QNN_LIBS_DIR, lib)):
           os.remove(os.path.join(consts.QNN_LIBS_DIR, lib)) 
        shutil.copy(os.path.join(SDK_lib_dir, lib), consts.QNN_LIBS_DIR)

    for lib in hexagon_libs:
        if os.path.isfile(os.path.join(consts.QNN_LIBS_DIR, lib)):
            os.remove(os.path.join(consts.QNN_LIBS_DIR, lib))
        shutil.copy(os.path.join(SDK_hexagon_dir, lib), consts.QNN_LIBS_DIR)


def create_venv_for_qai_hub():
    if not os.path.isdir(consts.QAI_HUB_VENV_PATH):
        utils.run_command(f"python -m venv {consts.QAI_HUB_VENV_PATH}")

def install_qai_hub():
    utils.run_command(f"{consts.QAI_HUB_VENV_PYTHON_PATH} -m pip install qai-hub > NUL")
    utils.run_command(f"{consts.QAI_HUB_VENV_PYTHON_PATH} -m pip install qai_hub_models > NUL")
    if os.path.isfile(consts.QAI_HUB_CONFIG):
        shutil.copy(consts.QAI_HUB_CONFIG, consts.QAI_HUB_CONFIG_BACKUP)
    utils.run_command(f"{consts.QAI_HUB_VENV_PATH}\\Scripts\\qai-hub.exe configure --api_token {consts.HUB_ID} > NUL", False)

def restore_qai_hub_key():
    if os.path.isfile(consts.QAI_HUB_CONFIG_BACKUP):
        shutil.copy(consts.QAI_HUB_CONFIG_BACKUP, consts.QAI_HUB_CONFIG)

print(f"Downloading QAIRT model bin files...")

hf_hub_download(
    repo_id="qualcomm/Stable-Diffusion-v1.5",
    filename="UNet_Quantized.bin",
    local_dir=model_path_1_5,
    revision=consts.SD_MODEL_1_5_REVISION,
)
hf_hub_download(
    repo_id="qualcomm/Stable-Diffusion-v1.5",
    filename="TextEncoder_Quantized.bin",
    local_dir=model_path_1_5,
    revision=consts.SD_MODEL_1_5_REVISION,
)
hf_hub_download(
    repo_id="qualcomm/Stable-Diffusion-v1.5",
    filename="VAEDecoder_Quantized.bin",
    local_dir=model_path_1_5,
    revision=consts.SD_MODEL_1_5_REVISION,
)

hf_hub_download(
    repo_id="qualcomm/Stable-Diffusion-v2.1",
    filename="UNet_Quantized.bin",
    local_dir=model_path_2_1,
    revision=consts.SD_MODEL_2_1_REVISION,
)
hf_hub_download(
    repo_id="qualcomm/Stable-Diffusion-v2.1",
    filename="TextEncoder_Quantized.bin",
    local_dir=model_path_2_1,
    revision=consts.SD_MODEL_2_1_REVISION,
)
hf_hub_download(
    repo_id="qualcomm/Stable-Diffusion-v2.1",
    filename="VAEDecoder_Quantized.bin",
    local_dir=model_path_2_1,
    revision=consts.SD_MODEL_2_1_REVISION,
)
print(f"QAIRT model bin files downloaded.")

os.makedirs(consts.CACHE_DIR, exist_ok=True)
os.makedirs(consts.QNN_LIBS_DIR, exist_ok=True)

download_qairt_sdk()
setup_qairt_env()

create_venv_for_qai_hub()
install_qai_hub()
print("Downloading required models using qai-hub...")
utils.run_command(f"{consts.QAI_HUB_VENV_PYTHON_PATH} {consts.EXTENSION_WS}//qairt_hub_models.py timeembedding")
print("Downloaded required models.")
restore_qai_hub_key()
