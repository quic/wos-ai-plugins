# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import os
import qairt_constants as consts
import zipfile
import shutil
import zipfile
from huggingface_hub import hf_hub_download
import torch
import numpy as np
from diffusers.models.embeddings import get_timestep_embedding, TimestepEmbedding
from diffusers import UNet2DConditionModel
from diffusers import DPMSolverMultistepScheduler
import qai_hub
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
        print(f"QAIRT SDK downloaded.")

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
        if not os.path.isfile(os.path.join(consts.QNN_LIBS_DIR, lib)):
            shutil.copy(os.path.join(SDK_lib_dir, lib), consts.QNN_LIBS_DIR)

    for lib in hexagon_libs:
        if not os.path.isfile(os.path.join(consts.QNN_LIBS_DIR, lib)):
            shutil.copy(os.path.join(SDK_hexagon_dir, lib), consts.QNN_LIBS_DIR)

def install_qai_hub():
    if os.path.isfile(consts.QAI_HUB_CONFIG):
        shutil.copy(consts.QAI_HUB_CONFIG, consts.QAI_HUB_CONFIG_BACKUP)
    run(f"\"{consts.VENV_PATH}\\Scripts\\qai-hub.exe\" configure --api_token {consts.HUB_ID} > NUL", False)

def restore_qai_hub_key():
    if os.path.isfile(consts.QAI_HUB_CONFIG_BACKUP):
        shutil.copy(consts.QAI_HUB_CONFIG_BACKUP, consts.QAI_HUB_CONFIG)

def convert_and_download_models():
    os.makedirs(consts.CONVERTION_DIR, exist_ok=True)
    if not os.path.isfile(consts.TIMESTEP_EMBEDDING_1_5_MODEL_PATH):
        model = qai_hub.get_model(consts.TIMESTEP_EMBEDDING_1_5_MODEL_ID)
        model.download(filename=consts.TIMESTEP_EMBEDDING_1_5_MODEL_PATH)
    if not os.path.isfile(consts.ESRGAN_X4_MODEL_PATH):
        try:
            model = qai_hub.get_model(consts.ESRGAN_X4_MODEL_ID)
            model.download(filename=consts.ESRGAN_X4_MODEL_PATH)
        except Exception:
            run(
                f'cmd /c "{consts.VENV_PYTHON}" -m qai_hub_models.models.esrgan.export '
                + f'--device "Snapdragon X Elite CRD" --height 512 --width 512 --target-runtime qnn '
                + f"--skip-profiling --skip-inferencing --skip-summary --output-dir {consts.CONVERTION_DIR}"
            )
            shutil.move(
                os.path.join(consts.CONVERTION_DIR, "esrgan.so"),
                consts.ESRGAN_X4_MODEL_PATH,
            )

print(f"Downloading QAIRT model bin files...")

os.makedirs(consts.QNN_LIBS_DIR, exist_ok=True)
os.makedirs(consts.CONTROLNET_DIR, exist_ok=True)
os.makedirs(consts.TIME_EMBEDDING_PATH, exist_ok=True)
os.makedirs(consts.LOGS_DIR, exist_ok=True)

download_qairt_sdk()
setup_qairt_env()

# Download required models
CONTROLNET_MODEL_REVISION = "f71ca660a2a5f77edccdd4aca27187e94292123d"
hf_hub_download(
    repo_id="qualcomm/ControlNet",
    filename="ControlNet_Quantized.bin",
    local_dir=consts.CONTROLNET_DIR,
    revision=CONTROLNET_MODEL_REVISION,
)
hf_hub_download(
    repo_id="qualcomm/ControlNet",
    filename="TextEncoder_Quantized.bin",
    local_dir=consts.CONTROLNET_DIR,
    revision=CONTROLNET_MODEL_REVISION,
)
hf_hub_download(
    repo_id="qualcomm/ControlNet",
    filename="UNet_Quantized.bin",
    local_dir=consts.CONTROLNET_DIR,
    revision=CONTROLNET_MODEL_REVISION,
)
hf_hub_download(
    repo_id="qualcomm/ControlNet",
    filename="VAEDecoder_Quantized.bin",
    local_dir=consts.CONTROLNET_DIR,
    revision=CONTROLNET_MODEL_REVISION,
)

install_qai_hub()
print("Downloading required models using qai-hub...")
convert_and_download_models()
print("Downloaded required models.")
restore_qai_hub_key()

time_embeddings = TimestepEmbedding(320, 1280)
time_embeddings.load_state_dict(torch.load(consts.TIMESTEP_EMBEDDING_1_5_MODEL_PATH))
scheduler = DPMSolverMultistepScheduler(
    num_train_timesteps=1000,
    beta_start=0.00085,
    beta_end=0.012,
    beta_schedule="scaled_linear",
)


def get_timestep(step):
    return np.int32(scheduler.timesteps.numpy()[step])

def get_time_embedding(timestep):
    timestep = torch.tensor([timestep])
    t_emb = get_timestep_embedding(timestep, 320, True, 0)
    emb = time_embeddings(t_emb).detach().numpy()
    return emb

def gen_time_embedding(user_step):
    scheduler.set_timesteps(user_step)

    time_emb_path = os.path.join(consts.TIME_EMBEDDING_PATH, str(user_step))
    os.makedirs(time_emb_path, exist_ok=True)
    for step in range(user_step):
        file_path = os.path.join(time_emb_path, str(step) + ".raw")
        timestep = get_timestep(step)
        time_embedding = get_time_embedding(timestep)
        time_embedding.tofile(file_path)

gen_time_embedding(10)
gen_time_embedding(15)
gen_time_embedding(20)
gen_time_embedding(25)
gen_time_embedding(30)
gen_time_embedding(35)
gen_time_embedding(40)
gen_time_embedding(50)
