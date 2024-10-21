# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import time
from PIL import Image
import os
import shutil
import cv2
import numpy as np
import torch
from transformers import CLIPTokenizer
from diffusers import DPMSolverMultistepScheduler
from diffusers.utils import load_image
from diffusers.utils import PIL_INTERPOLATION
import sys
from torchvision import transforms
import json
import qairt_constants as consts
import traceback

from qai_appbuilder import (
    QNNContext,
    QNNContextProc,
    QNNShareMemory,
    Runtime,
    LogLevel,
    ProfilingLevel,
    PerfProfile,
    QNNConfig,
    timer,
)

####################################################################

OUT_H, OUT_W = 512, 512

tokenizer = None
scheduler = None
tokenizer_max_length = 77  # Define Tokenizer output max length (must be 77)

# model objects.
text_encoder = None
unet = None
vae_decoder = None
realesrgan = None
realesrgan_mem = None

# Any user defined prompt
user_prompt = ""
uncond_prompt = ""
user_seed = np.int64(1)
user_step = 20  # User defined step value, any integer value in {20, 30, 50}
user_text_guidance = 9.0  # User define text guidance, any float value in [5.0, 15.0]
user_high_resolution = False
input_image_path = None
output_image_path = None

####################################################################


def reset():
    global scheduler
    global tokenizer
    global text_encoder
    global unet
    global vae_decoder
    global controlnet
    global realesrgan
    global realesrgan_mem

    global user_prompt
    global uncond_prompt
    global user_seed
    global user_step
    global user_text_guidance
    global user_high_resolution
    global input_image_path
    global output_image_path

    tokenizer = None
    scheduler = None

    # model objects.
    text_encoder = None
    unet = None
    vae_decoder = None
    realesrgan = None
    realesrgan_mem = None

    # Any user defined prompt
    user_prompt = ""
    uncond_prompt = ""
    user_seed = np.int64(1)
    user_step = 20  # User defined step value, any integer value in {20, 30, 50}
    user_text_guidance = (
        9.0  # User define text guidance, any float value in [5.0, 15.0]
    )
    input_image_path = None
    user_high_resolution = True
    output_image_path = None


####################################################################


class TextEncoder(QNNContext):
    # @timer
    def Inference(self, input_data):
        input_datas = [input_data]
        output_data = super().Inference(input_datas)[0]

        # Output of Text encoder should be of shape (1, 77, 768)
        output_data = output_data.reshape((1, 77, 768))
        return output_data


class Unet(QNNContext):
    # @timer
    def Inference(
        self,
        input_data_1,
        input_data_2,
        input_data_3,
        input_data_4,
        input_data_5,
        input_data_6,
        input_data_7,
        input_data_8,
        input_data_9,
        input_data_10,
        input_data_11,
        input_data_12,
        input_data_13,
        input_data_14,
        input_data_15,
        input_data_16,
    ):
        # We need to reshape the array to 1 dimensionality before send it to the network. 'input_data_2' already is 1 dimensionality, so doesn't need to reshape.
        input_data_1 = input_data_1.reshape(input_data_1.size)
        input_data_3 = input_data_3.reshape(input_data_3.size)
        input_data_4 = input_data_4.reshape(input_data_4.size)
        input_data_5 = input_data_5.reshape(input_data_5.size)
        input_data_6 = input_data_6.reshape(input_data_6.size)

        input_data_7 = input_data_7.reshape(input_data_7.size)
        input_data_8 = input_data_8.reshape(input_data_8.size)
        input_data_9 = input_data_9.reshape(input_data_9.size)
        input_data_10 = input_data_10.reshape(input_data_10.size)
        input_data_11 = input_data_11.reshape(input_data_11.size)

        input_data_12 = input_data_12.reshape(input_data_12.size)
        input_data_13 = input_data_13.reshape(input_data_13.size)
        input_data_14 = input_data_14.reshape(input_data_14.size)
        input_data_15 = input_data_15.reshape(input_data_15.size)
        input_data_16 = input_data_16.reshape(input_data_16.size)

        input_datas = [
            input_data_1,
            input_data_2,
            input_data_3,
            input_data_4,
            input_data_5,
            input_data_6,
            input_data_7,
            input_data_8,
            input_data_9,
            input_data_10,
            input_data_11,
            input_data_12,
            input_data_13,
            input_data_14,
            input_data_15,
            input_data_16,
        ]

        output_data = super().Inference(input_datas)[0]
        output_data = output_data.reshape(1, 64, 64, 4)
        return output_data


class VaeDecoder(QNNContext):
    # @timer
    def Inference(self, input_data):
        input_data = input_data.reshape(input_data.size)
        input_datas = [input_data]

        output_data = super().Inference(input_datas)[0]

        return output_data


class RealESRGan(QNNContextProc):
    # @timer
    def Inference(self, realesrgan_mem, input_data):
        input_datas = [input_data]

        output_data = super().Inference(realesrgan_mem, input_datas)[0]

        return output_data


class ControlNet(QNNContext):
    # @timer
    def Inference(self, input_data_1, input_data_2, input_data_3, input_data_4):
        # We need to reshape the array to 1 dimensionality before send it to the network. 'input_data_2' already is 1 dimensionality, so doesn't need to reshape.

        input_data_1 = input_data_1.reshape(input_data_1.size)
        input_data_3 = input_data_3.reshape(input_data_3.size)
        input_data_4 = input_data_4.reshape(input_data_4.size)

        input_datas = [input_data_1, input_data_2, input_data_3, input_data_4]
        output_data = super().Inference(input_datas)
        return output_data


####################################################################


def model_initialize():
    global scheduler
    global tokenizer
    global text_encoder
    global unet
    global vae_decoder
    global controlnet
    global realesrgan
    global realesrgan_mem

    result = True

    # model names
    model_text_encoder = "text_encoder"
    model_unet = "model_unet"
    model_vae_decoder = "vae_decoder"
    model_realesrgan = "realesrgan"
    model_controlnet = "controlnet"

    # process names
    model_realesrgan_proc = "~realesrgan"

    # share memory names.
    model_realesrgan_mem = model_realesrgan + "~memory"

    # models' path.
    text_encoder_model = "{}\\{}_Quantized.bin".format(
        consts.CONTROLNET_DIR, "TextEncoder"
    )
    unet_model = "{}\\{}_Quantized.bin".format(consts.CONTROLNET_DIR, "UNet")
    vae_decoder_model = "{}\\{}_Quantized.bin".format(
        consts.CONTROLNET_DIR, "VAEDecoder"
    )
    controlnet_model = "{}\\{}_Quantized.bin".format(
        consts.CONTROLNET_DIR, "ControlNet"
    )
    realesrgan_model = consts.ESRGAN_X4_MODEL_PATH

    # Instance for Unet
    unet = Unet(model_unet, unet_model)

    # Instance for TextEncoder
    text_encoder = TextEncoder(model_text_encoder, text_encoder_model)

    # Instance for VaeDecoder
    vae_decoder = VaeDecoder(model_vae_decoder, vae_decoder_model)

    # Instance for ControlNet
    controlnet = ControlNet(model_controlnet, controlnet_model)

    # Instance for RealESRGan which inherited from the class QNNContextProc, the model will be loaded into a separate process.
    realesrgan = RealESRGan(model_realesrgan, model_realesrgan_proc, realesrgan_model)
    realesrgan_mem = QNNShareMemory(model_realesrgan_mem, 1024 * 1024 * 50)  # 50M

    # Initializing the Tokenizer
    tokenizer = CLIPTokenizer.from_pretrained(
        "openai/clip-vit-base-patch32", cache_dir=consts.CACHE_DIR
    )

    # Scheduler - initializing the Scheduler.
    scheduler = DPMSolverMultistepScheduler(
        num_train_timesteps=1000,
        beta_start=0.00085,
        beta_end=0.012,
        beta_schedule="scaled_linear",
    )

    torch.from_numpy(
        np.array([1])
    )  # Let LazyImport to import the torch & numpy lib here.

    return result


def run_tokenizer(prompt):
    text_input = tokenizer(
        prompt, padding="max_length", max_length=tokenizer_max_length, truncation=True
    )
    text_input = np.array(text_input.input_ids, dtype=np.float32)
    return text_input


# These parameters can be configured through GUI 'settings'.
def setup_parameters(
    input_img_path,
    output_img_path,
    prompt,
    un_prompt,
    seed,
    step,
    text_guidance,
    high_resolution,
):

    global user_prompt
    global uncond_prompt
    global user_seed
    global user_step
    global user_text_guidance
    global user_high_resolution
    global input_image_path
    global output_image_path

    user_prompt = prompt
    uncond_prompt = un_prompt
    user_seed = seed
    user_step = step
    user_text_guidance = text_guidance
    user_high_resolution = high_resolution
    input_image_path = input_img_path
    output_image_path = output_img_path

    assert isinstance(user_seed, np.int64) == True, "user_seed should be of type int64"
    assert isinstance(user_step, int) == True, "user_step should be of type int"
    assert (
        isinstance(user_text_guidance, float) == True
    ), "user_text_guidance should be of type float"
    assert (
        user_text_guidance >= 5.0 and user_text_guidance <= 15.0
    ), "user_text_guidance should be a float from [5.0, 15.0]"


def run_scheduler(noise_pred_uncond, noise_pred_text, latent_in, timestep):
    # Convert all inputs from NHWC to NCHW
    noise_pred_uncond = np.transpose(noise_pred_uncond, (0, 3, 1, 2)).copy()
    noise_pred_text = np.transpose(noise_pred_text, (0, 3, 1, 2)).copy()
    latent_in = np.transpose(latent_in, (0, 3, 1, 2)).copy()

    # Convert all inputs to torch tensors
    noise_pred_uncond = torch.from_numpy(noise_pred_uncond)
    noise_pred_text = torch.from_numpy(noise_pred_text)
    latent_in = torch.from_numpy(latent_in)

    # Merge noise_pred_uncond and noise_pred_text based on user_text_guidance
    noise_pred = noise_pred_uncond + user_text_guidance * (
        noise_pred_text - noise_pred_uncond
    )

    # Run Scheduler step
    latent_out = scheduler.step(noise_pred, timestep, latent_in).prev_sample.numpy()

    # Convert latent_out from NCHW to NHWC
    latent_out = np.transpose(latent_out, (0, 2, 3, 1))

    return latent_out


# Function to get timesteps
def get_timestep(step):
    return np.int32(scheduler.timesteps.numpy()[step])


def make_canny_image(input_image: Image):
    image = np.asarray(input_image)

    # Get edges for input with Canny Edge Detection
    low_threshold = 100
    high_threshold = 200

    image = cv2.Canny(image, low_threshold, high_threshold)
    cv2.imwrite(os.path.join(consts.OUTPUTS_DIR, "canny.png"), image)
    image = image[:, :, None]
    image = np.concatenate([image, image, image], axis=2)

    image = Image.fromarray(image)

    image = np.array(image)
    image = image[None, :]

    image = image.astype(np.float32) / 255.0
    return image


# Execute the Stable Diffusion pipeline
def model_execute(callback):
    PerfProfile.SetPerfProfileGlobal(PerfProfile.BURST)

    scheduler.set_timesteps(
        user_step
    )  # Setting up user provided time steps for Scheduler

    # Run Tokenizer
    cond_tokens = run_tokenizer(user_prompt)
    uncond_tokens = run_tokenizer(uncond_prompt)

    # Run Text Encoder on Tokens
    uncond_text_embedding = text_encoder.Inference(uncond_tokens)
    user_text_embedding = text_encoder.Inference(cond_tokens)

    # Initialize the latent input with random initial latent
    latents_shape = (1, 4, OUT_H // 8, OUT_W // 8)
    random_init_latent = torch.randn(
        latents_shape, generator=torch.manual_seed(user_seed)
    ).numpy()
    latent_in = random_init_latent.transpose(0, 2, 3, 1)

    image = load_image(input_image_path)
    canny_image = make_canny_image(image)

    time_emb_path = os.path.join(consts.TIME_EMBEDDING_PATH, str(user_step))

    # Run the loop for user_step times
    for step in range(user_step):
        print(f"Step {step} Running...")
        sys.stdout.flush()

        timestep = get_timestep(step)  # time_embedding = get_time_embedding(timestep)
        file_path = os.path.join(time_emb_path, str(step) + ".raw")
        time_embedding = np.fromfile(file_path, dtype=np.float32)

        controlnet_out = controlnet.Inference(
            latent_in, time_embedding, user_text_embedding, canny_image
        )

        conditional_noise_pred = unet.Inference(
            latent_in, time_embedding, user_text_embedding, *controlnet_out
        )

        controlnet_out = controlnet.Inference(
            latent_in, time_embedding, uncond_text_embedding, canny_image
        )

        unconditional_noise_pred = unet.Inference(
            latent_in, time_embedding, uncond_text_embedding, *controlnet_out
        )

        latent_in = run_scheduler(
            unconditional_noise_pred, conditional_noise_pred, latent_in, timestep
        )
        callback(step)

    # Run VAE
    output_image = vae_decoder.Inference(latent_in)
    output_image = output_image.reshape(OUT_H, OUT_W, -1)

    PerfProfile.RelPerfProfileGlobal()

    if len(output_image) == 0:
        callback(None)
        return False
    else:
        image_size = 512

        # Run RealESRGan
        if user_high_resolution:
            output_image = realesrgan.Inference(realesrgan_mem, [output_image])
            image_size = 2048

        image_path = output_image_path
        output_image = np.clip(output_image * 255.0, 0.0, 255.0).astype(np.uint8)
        output_image = output_image.reshape(image_size, image_size, -1)
        Image.fromarray(output_image, mode="RGB").save(image_path)
        callback(image_path)
        return True


# Release all the models.
def model_destroy():
    global text_encoder
    global unet
    global vae_decoder
    global realesrgan
    global realesrgan_mem

    del text_encoder
    del unet
    del vae_decoder
    del realesrgan
    del realesrgan_mem


def SetQNNConfig():
    QNNConfig.Config(
        consts.QNN_LIBS_DIR, Runtime.HTP, LogLevel.WARN, ProfilingLevel.BASIC
    )


def modelExecuteCallback(result):
    if (None == result) or isinstance(
        result, str
    ):  # None == Image generates failed. 'str' == image_path: generated new image path.
        if None == result:
            result = "None"
        print("modelExecuteCallback result: " + result)

    else:
        result = (result + 1) * 100
        result = int(result / user_step)
        result = str(result)
        print("modelExecuteCallback result: " + result)


def load_model():
    reset()
    SetQNNConfig()
    model_initialize()
    return True


def unload_model():
    model_destroy()
    return True


def run_model(
    input_img_path,
    output_img_path,
    user_prompt,
    uncond_prompt,
    user_seed,
    user_step,
    user_text_guidance,
    user_high_resolution,
):

    user_seed = np.int64(int(user_seed))
    user_step = int(user_step)
    user_text_guidance = float(user_text_guidance)
    user_high_resolution = bool(user_high_resolution)

    setup_parameters(
        input_img_path,
        output_img_path,
        user_prompt,
        uncond_prompt,
        user_seed,
        user_step,
        user_text_guidance,
        user_high_resolution,
    )

    is_generated = model_execute(modelExecuteCallback)
    return is_generated


def handle_command(command):

    action = command.get("action")
    params = command.get("params", {})
    start_time = time.time()
    if action == "load_model":
        result = load_model()
        result_str = f"LOAD_MODEL_RESULT_START\n{result}\nLOAD_MODEL_RESULT_END"
        elapsed_time = time.time() - start_time
        print(f"Time taken for loading model :{elapsed_time}\n")
        return result_str

    elif action == "run_model":
        args = list(params.values())
        result = run_model(*args)
        result_str = f"RUN_MODEL_RESULT_START\n{result}\nRUN_MODEL_RESULT_END"
        elapsed_time = time.time() - start_time
        print(f"Time taken for running the model :{elapsed_time}\n")
        return result_str

    elif action == "unload_model":
        result = unload_model()
        result_str = f"UNLOAD_MODEL_RESULT_START\n{result}\nUNLOAD_MODEL_RESULT_END"
        elapsed_time = time.time() - start_time
        print(f"Time taken for unloading the model :{elapsed_time}\n")
        return result_str


if __name__ == "__main__":
    try:
        while True:
            command_line = sys.stdin.readline().strip()
            if not command_line:
                break

            command = json.loads(command_line)
            response = handle_command(command)
            print(response)
            print("\nEND_DATA\n")
            sys.stdout.flush()
    except Exception as e:
        print(f"\n{str(e)}\n{traceback.format_exc()}")
        print("\nEND_DATA\n")
        sys.stdout.flush()
