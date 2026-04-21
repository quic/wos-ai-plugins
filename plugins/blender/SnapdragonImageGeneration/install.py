# =============================================================================
#
# Copyright (c) 2026, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import os
import qairt_constants as consts
import zipfile
import shutil
import zipfile
import torch
import numpy as np
from diffusers.models.embeddings import get_timestep_embedding, TimestepEmbedding
from diffusers import UNet2DConditionModel
from diffusers import DPMSolverMultistepScheduler
import requests
import subprocess


def run(command, desc=None, errdesc=None, custom_env=None, live: bool = True) -> str:
    if desc is not None:
        print(desc)

    run_kwargs = {
        "args": command,
        "shell": True,
        "env": os.environ if custom_env is None else custom_env,
        "errors": "ignore",
    }

    if not live:
        run_kwargs["stdout"] = run_kwargs["stderr"] = subprocess.PIPE

    result = subprocess.run(**run_kwargs)

    if result.returncode != 0:
        error_bits = [
            f"{errdesc or 'Error running command'}.",
            f"Command: {command}",
            f"Error code: {result.returncode}",
        ]
        if result.stdout:
            error_bits.append(f"stdout: {result.stdout}")
        if result.stderr:
            error_bits.append(f"stderr: {result.stderr}")
        raise RuntimeError("\n".join(error_bits))

    return result.stdout or ""


def download_url(url, save_path, chunk_size=128):
    r = requests.get(url, stream=True)
    with open(save_path, "wb") as fd:
        for chunk in r.iter_content(chunk_size=chunk_size):
            fd.write(chunk)


def download_qairt_sdk():
    # Setup QAIRT SDK
    if not os.path.isdir(consts.QNN_SDK_ROOT):
        os.makedirs(consts.QAIRT_DIR, exist_ok=True)
        print(f"Downloading QAIRT SDK...")
        download_url(consts.QNN_SDK_DOWNLOAD_URL, consts.SDK_SAVE_PATH)

        with zipfile.ZipFile(consts.SDK_SAVE_PATH, "r") as zip_ref:
            zip_ref.extractall(consts.LONG_PATH_PREFIX_PLUGIN_DIR)
        shutil.move(
            os.path.join(consts.PLUGIN_DIR, "qairt", consts.QAIRT_VERSION),
            os.path.join(consts.QNN_SDK_ROOT, ".."),
        )
        shutil.rmtree(os.path.join(consts.PLUGIN_DIR, "qairt"))
        os.remove(consts.SDK_SAVE_PATH)


def setup_qairt_env():
    # Preparing all the binaries and libraries for execution.
    SDK_lib_dir = consts.QNN_SDK_ROOT + "\\lib\\arm64x-windows-msvc"
    libs = [
        "QnnHtp.dll",
        "QnnSystem.dll",
        "QnnHtpPrepare.dll",
        "QnnHtpV73Stub.dll",
        "QnnHtpV81Stub.dll",
    ]

   # Copy necessary libraries to a common location
    for lib in libs:
        src = os.path.join(SDK_lib_dir, lib)
        dst = os.path.join(consts.QNN_LIBS_DIR, lib)
        if not os.path.isfile(dst):
            shutil.copy(src, dst)

    for arch in consts.DSP_ARCH:
        SDK_hexagon_dir = consts.QNN_SDK_ROOT + f"\\lib\\hexagon-v{arch}\\unsigned"

        arch_libs = [
            f"libQnnHtpV{arch}Skel.so",
            f"libqnnhtpv{arch}.cat",
        ]

        for lib in arch_libs:
            src = os.path.join(SDK_hexagon_dir, lib)
            dst = os.path.join(consts.QNN_LIBS_DIR, lib)
            if not os.path.isfile(dst):
                shutil.copy(src, dst)


def controlnet_download():
    os.makedirs(consts.CONTROLNET_DIR, exist_ok=True)

    print("Downloading ControlNet-Canny model binaries...")

    for filename, url in consts.CONTROLNET_HF_URLS.items():
        save_path = os.path.join(consts.CONTROLNET_DIR, filename)

        if os.path.isfile(save_path):
            print(f"Already exists: {filename}")
            continue

        print(f"Downloading {filename} ...")
        download_url(url, save_path)


print(f"Downloading QAIRT model bin files...")

os.makedirs(consts.QNN_LIBS_DIR, exist_ok=True)
os.makedirs(consts.CONTROLNET_DIR, exist_ok=True)
os.makedirs(consts.LOGS_DIR, exist_ok=True)

download_qairt_sdk()
setup_qairt_env()
controlnet_download()


scheduler = DPMSolverMultistepScheduler(
    num_train_timesteps=1000,
    beta_start=0.00085,
    beta_end=0.012,
    beta_schedule="scaled_linear",
)

def get_timestep(step):
    return np.int32(scheduler.timesteps.numpy()[step])
