# =============================================================================
#
# Copyright (c) 2026, Qualcomm Innovation Center, Inc. All rights reserved.
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
    f"install {consts.QAI_APPBUILDER_WHEEL_URL}",
    "Install python QNN",
)
if not launch.is_installed("diffusers"):
    launch.run_pip("install diffusers", "diffusers")
if not launch.is_installed("onnx"):
    launch.run_pip("install onnx", "onnx")

import common_utils as utils
import zipfile
import shutil
import requests

def download_if_missing(url, save_path):
    if not os.path.isfile(save_path):
        utils.download_url(url, save_path)


def download_model_files(file_map, target_dir):
    os.makedirs(target_dir, exist_ok=True)
    for filename, url in file_map.items():
        if not url:
            raise RuntimeError(f"Missing direct URL for {filename}")
        save_path = os.path.join(target_dir, filename)
        download_if_missing(url, save_path)


def copy_time_embeddings():
    os.makedirs(consts.TIME_EMBEDDING_DIR, exist_ok=True)

    files_to_copy = {
        "time_embedding_sd_1_5.pt": consts.TIMESTEP_EMBEDDING_1_5_MODEL_PATH,
        "time_embedding_sd_2_1.pt": consts.TIMESTEP_EMBEDDING_2_1_MODEL_PATH,
    }

    for filename, dest_path in files_to_copy.items():
        src_path = os.path.join(consts.PRE_COMPUTED_DATA_DIR, filename)

        if os.path.exists(dest_path):
            continue

        shutil.copy2(src_path, dest_path)
        print(f"copying {src_path} → {dest_path}")


def download_qairt_sdk():
    # Setup QAIRT 
    if not os.path.isdir(consts.QNN_SDK_ROOT):
        os.makedirs(consts.QAIRT_DIR, exist_ok=True)
        print(f"Downloading QAIRT SDK...")
        utils.download_url(consts.QNN_SDK_DOWNLOAD_URL, consts.SDK_SAVE_PATH)

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
    SDK_lib_dir = os.path.join(consts.QNN_SDK_ROOT, "lib", "arm64x-windows-msvc")
    os.makedirs(consts.QNN_LIBS_DIR, exist_ok=True)
    
    # Copy common libraries once (outside the loop)
    common_libs = ["QnnHtp.dll", "QnnSystem.dll", "QnnHtpPrepare.dll"]
    for lib in common_libs:
        src = os.path.join(SDK_lib_dir, lib)
        dst = os.path.join(consts.QNN_LIBS_DIR, lib)
        if os.path.isfile(dst):
            os.remove(dst)
        shutil.copy(src, dst)
    
    # Copy architecture-specific libraries
    for arch in consts.DSP_ARCHS:
        SDK_hexagon_dir = os.path.join(consts.QNN_SDK_ROOT, "lib", f"hexagon-v{arch}", "unsigned")
        
        arch_specific_libs = [f"QnnHtpV{arch}Stub.dll"]
        hexagon_libs = [f"libQnnHtpV{arch}Skel.so", f"libqnnhtpv{arch}.cat"]
        
        for lib in arch_specific_libs:
            src = os.path.join(SDK_lib_dir, lib)
            dst = os.path.join(consts.QNN_LIBS_DIR, lib)
            if os.path.isfile(dst):
                os.remove(dst)
            shutil.copy(src, dst)
        
        for lib in hexagon_libs:
            src = os.path.join(SDK_hexagon_dir, lib)
            dst = os.path.join(consts.QNN_LIBS_DIR, lib)
            if os.path.isfile(dst):
                os.remove(dst)
            shutil.copy(src, dst)


def main():
    download_qairt_sdk()
    setup_qairt_env()
    print("Downloading controlnet binaries..")
    download_model_files(consts.CONTROLNET_HF_URLS, consts.CONTROLNET_DIR)
    print("Downloading SD 1.5 binaries..")
    download_model_files(consts.SD1_5_HF_URLS, consts.SD1_5_DIR)
    print("Downloading SD 2.1 binaries..")
    download_model_files(consts.SD2_1_HF_URLS, consts.SD2_1_DIR)
    copy_time_embeddings()


if __name__ == "__main__":
    main()