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
from modules import paths, shared, modelloader
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
from diffusers import UNet2DConditionModel
from diffusers.models.embeddings import get_timestep_embedding
from diffusers import (
    DDIMScheduler,
    DPMSolverMultistepScheduler,
    EulerAncestralDiscreteScheduler,
    EulerDiscreteScheduler,
    HeunDiscreteScheduler,
    LMSDiscreteScheduler,
    PNDMScheduler,
)
import qairt_constants as consts

model_path_1_5 = os.path.abspath(
    os.path.join(paths.models_path, "Stable-diffusion", "qcom-Stable-Diffusion-v1.5")
)
model_path_2_1 = os.path.abspath(
    os.path.join(paths.models_path, "Stable-diffusion", "qcom-Stable-Diffusion-v2.1")
)


class TextEncoder(QNNContext):
    # @timer
    def Inference(self, input_data, sd_version):
        input_datas = [input_data]
        output_data = super().Inference(input_datas, perf_profile="burst")[0]

        # Output of Text encoder should be of shape (1, 77, 768)
        if sd_version == "1.5":
            output_data = output_data.reshape((1, 77, 768))
        elif sd_version == "2.1":
            output_data = output_data.reshape((1, 77, 1024))
        return output_data


class Unet(QNNContext):
    # @timer
    def Inference(self, input_data_1, input_data_2, input_data_3):
        # We need to reshape the array to 1 dimensionality before send it to the network. 'input_data_2' already is 1 dimensionality, so doesn't need to reshape.
        input_data_1 = input_data_1.reshape(input_data_1.size)
        input_data_3 = input_data_3.reshape(input_data_3.size)

        input_datas = [input_data_1, input_data_2, input_data_3]
        output_data = super().Inference(input_datas, perf_profile="burst")[0]

        output_data = output_data.reshape(1, 64, 64, 4)
        return output_data


class VaeDecoder(QNNContext):
    # @timer
    def Inference(self, input_data):
        input_data = input_data.reshape(input_data.size)
        input_datas = [input_data]

        output_data = super().Inference(input_datas, perf_profile="burst")[0]

        return output_data


class UpscaleModel(QNNContextProc):
    # @timer
    def Inference(self, mem, input_data):
        input_datas = [input_data]
        output_data = super().Inference(mem, input_datas, perf_profile="burst")[0]
        return output_data


class StableDiffusionInput:
    user_prompt = ""
    uncond_prompt = ""
    user_seed = np.int64(0)
    user_step = 20
    user_text_guidance = 7.5
    is_high_resolution = False
    upscaler_model_name = None
    sampler_name = None
    model_name = None

    def __init__(
        self,
        prompt,
        un_prompt,
        seed,
        step,
        text_guidance,
        high_resolution,
        sampler_name,
        model_name,
        upscaler_model_name,
    ):
        self.user_prompt = prompt
        self.uncond_prompt = un_prompt
        self.user_seed = seed
        self.user_step = step
        self.user_text_guidance = text_guidance
        self.is_high_resolution = high_resolution
        self.sampler_name = sampler_name
        self.model_name = model_name
        self.upscaler_model_name = upscaler_model_name
        self.upscaler_model_used, self.upscaler_model_path = get_upscaler_model(
            upscaler_model_name
        )

        assert (
            isinstance(self.user_seed, int) == True
        ), "user_seed should be of type int"
        assert (
            isinstance(self.user_step, int) == True
        ), "user_step should be of type int"
        assert (
            isinstance(self.user_text_guidance, float) == True
        ), "user_text_guidance should be of type float"


def get_upscaler_model(model_name):
    # This is the default model, more upscale models will be added.
    return ("ESRGAN_4x", consts.ESRGAN_X4_MODEL_PATH)


class UpscalerPipeline:
    model = None
    model_mem = None
    model_name = None

    def __init__(self, model_name, model_path):
        self.model_name = model_name
        name = "upscale"
        # process names
        model_proc = "~upscale"
        # share memory names.
        model_mem_name = name + "~memory"

        # Instance for RealESRGan which inherited from the class QNNContextProc, the model will be loaded into a separate process.
        self.model = UpscaleModel(name, model_proc, model_path)
        self.model_mem = QNNShareMemory(model_mem_name, 1024 * 1024 * 50)  # 50M

    # Release all the models.
    def __del__(self):
        del self.model
        del self.model_mem

    def execute(self, input):
        return self.model.Inference(self.model_mem, [input])


class QnnStableDiffusionPipeline:
    TOKENIZER_MAX_LENGTH = 77  # Define Tokenizer output max length (must be 77)

    default_scheduler = DPMSolverMultistepScheduler(
        num_train_timesteps=1000,
        beta_start=0.00085,
        beta_end=0.012,
        beta_schedule="scaled_linear",
    )
    scheduler = None
    tokenizer = None
    text_encoder = None
    unet = None
    vae_decoder = None

    unet_time_embeddings_1_5 = None
    unet_time_embeddings_2_1 = None

    sd_version = None

    def __init__(self, model_name):
        self.set_model_version(model_name)

        self.unet_time_embeddings_1_5 = UNet2DConditionModel.from_pretrained(
            "runwayml/stable-diffusion-v1-5",
            subfolder="unet",
            cache_dir=consts.CACHE_DIR,
        ).time_embedding
        self.unet_time_embeddings_2_1 = UNet2DConditionModel.from_pretrained(
            "stabilityai/stable-diffusion-2-1-base",
            subfolder="unet",
            cache_dir=consts.CACHE_DIR,
        ).time_embedding

        self.load_model()

        torch.from_numpy(
            np.array([1])
        )  # Let LazyImport to import the torch & numpy lib here.

    def is_sd_1_5(self):
        return self.sd_version == "1.5"

    def run_tokenizer(self, prompt):
        text_input = self.tokenizer(
            prompt,
            padding="max_length",
            max_length=self.TOKENIZER_MAX_LENGTH,
            truncation=True,
        )
        text_input = np.array(text_input.input_ids, dtype=np.float32)
        return text_input

    def run_scheduler(
        self,
        sd_input,
        noise_pred_uncond,
        noise_pred_text,
        latent_in,
        timestep,
    ):
        # Convert all inputs from NHWC to NCHW
        noise_pred_uncond = np.transpose(noise_pred_uncond, (0, 3, 1, 2)).copy()
        noise_pred_text = np.transpose(noise_pred_text, (0, 3, 1, 2)).copy()
        latent_in = np.transpose(latent_in, (0, 3, 1, 2)).copy()

        # Convert all inputs to torch tensors
        noise_pred_uncond = torch.from_numpy(noise_pred_uncond)
        noise_pred_text = torch.from_numpy(noise_pred_text)
        latent_in = torch.from_numpy(latent_in)

        # Merge noise_pred_uncond and noise_pred_text based on user_text_guidance
        noise_pred = noise_pred_uncond + sd_input.user_text_guidance * (
            noise_pred_text - noise_pred_uncond
        )

        # Run Scheduler step
        latent_out = self.scheduler.step(
            noise_pred, timestep, latent_in
        ).prev_sample.numpy()

        # Convert latent_out from NCHW to NHWC
        latent_out = np.transpose(latent_out, (0, 2, 3, 1)).copy()

        return latent_out

    # Function to get timesteps
    def get_timestep(self, step):
        return np.int32(self.scheduler.timesteps.numpy()[step])

    def get_time_embedding(self, timestep, unet_time_embeddings):
        timestep = torch.tensor([timestep])
        t_emb = get_timestep_embedding(timestep, 320, True, 0)
        emb = unet_time_embeddings(t_emb).detach().numpy()
        return emb

    def set_scheduler(self, config, sampler_name):
        if sampler_name == "Euler a":
            self.scheduler = EulerAncestralDiscreteScheduler.from_config(config)
        elif sampler_name == "Euler":
            self.scheduler = EulerDiscreteScheduler.from_config(config)
        elif sampler_name == "LMS":
            self.scheduler = LMSDiscreteScheduler.from_config(config)
        elif sampler_name == "Heun":
            self.scheduler = HeunDiscreteScheduler.from_config(config)
        elif sampler_name == "DPM++ 2M":
            self.scheduler = DPMSolverMultistepScheduler.from_config(
                config,
                algorithm_type="dpmsolver++",
                use_karras_sigmas=False,
            )
        elif sampler_name == "LMS Karras":
            self.scheduler = LMSDiscreteScheduler.from_config(
                config, use_karras_sigmas=True
            )
        elif sampler_name == "DPM++ 2M Karras":
            self.scheduler = DPMSolverMultistepScheduler.from_config(
                config,
                algorithm_type="dpmsolver++",
                use_karras_sigmas=True,
            )
        elif sampler_name == "DDIM":
            self.scheduler = DDIMScheduler.from_config(config)
        elif sampler_name == "PLMS":
            self.scheduler = PNDMScheduler.from_config(config)
        else:
            self.scheduler = DPMSolverMultistepScheduler(
                num_train_timesteps=1000,
                beta_start=0.00085,
                beta_end=0.012,
                beta_schedule="scaled_linear",
            )

    def set_model_version(self, model_name):
        if model_name == "Stable-Diffusion-1.5":
            self.sd_version = "1.5"
        elif model_name == "Stable-Diffusion-2.1":
            self.sd_version = "2.1"

    def load_model(self):
        QNNConfig.Config(
            consts.QNN_LIBS_DIR, Runtime.HTP, LogLevel.ERROR, ProfilingLevel.BASIC
        )
        # model names
        model_text_encoder = "text_encoder"
        model_unet = "model_unet"
        model_vae_decoder = "vae_decoder"

        model_path = None
        if self.is_sd_1_5():
            model_path = model_path_1_5
            # Initializing the Tokenizer
            self.tokenizer = CLIPTokenizer.from_pretrained(
                "openai/clip-vit-base-patch32", cache_dir=consts.CACHE_DIR
            )
        else:
            model_path = model_path_2_1
            self.tokenizer = CLIPTokenizer.from_pretrained(
                "stabilityai/stable-diffusion-2-1-base",
                subfolder="tokenizer",
                revision="main",
                cache_dir=consts.CACHE_DIR,
            )
        text_encoder_model = "{}\\TextEncoder_Quantized.bin".format(model_path)
        unet_model = "{}\\UNet_Quantized.bin".format(model_path)
        vae_decoder_model = "{}\\VAEDecoder_Quantized.bin".format(model_path)
        print(f"Loading models from {model_path}")
        # Instance for Unet
        self.unet = Unet(model_unet, unet_model)

        # Instance for TextEncoder
        self.text_encoder = TextEncoder(model_text_encoder, text_encoder_model)

        # Instance for VaeDecoder
        self.vae_decoder = VaeDecoder(model_vae_decoder, vae_decoder_model)

    def reload_model(self, model_name):
        self.set_model_version(model_name)
        del self.tokenizer
        del self.text_encoder
        del self.unet
        del self.vae_decoder
        self.load_model()

    # Execute the Stable Diffusion pipeline
    def model_execute(
        self,
        sd_input: StableDiffusionInput,
        callback,
        upscaler_pipeline: UpscalerPipeline,
    ) -> Image:
        image = None
        PerfProfile.SetPerfProfileGlobal(PerfProfile.BURST)
        self.set_scheduler(self.default_scheduler.config, sd_input.sampler_name)

        self.scheduler.set_timesteps(
            sd_input.user_step
        )  # Setting up user provided time steps for Scheduler

        # Run Tokenizer
        cond_tokens = self.run_tokenizer(sd_input.user_prompt)
        uncond_tokens = self.run_tokenizer(sd_input.uncond_prompt)

        # Run Text Encoder on Tokens
        uncond_text_embedding = self.text_encoder.Inference(
            uncond_tokens, self.sd_version
        )
        user_text_embedding = self.text_encoder.Inference(cond_tokens, self.sd_version)

        # Initialize the latent input with random initial latent
        random_init_latent = torch.randn(
            (1, 4, 64, 64), generator=torch.manual_seed(sd_input.user_seed)
        ).numpy()
        latent_in = random_init_latent.transpose(0, 2, 3, 1)

        # Run the loop for user_step times
        for step in range(sd_input.user_step):
            # print(f"Step {step} Running...")

            time_step = self.get_timestep(step)
            unet_time_embeddings = self.unet_time_embeddings_1_5
            if not self.is_sd_1_5():
                unet_time_embeddings = self.unet_time_embeddings_2_1
            time_embedding = self.get_time_embedding(time_step, unet_time_embeddings)

            unconditional_noise_pred = self.unet.Inference(
                latent_in, time_embedding, uncond_text_embedding
            )
            conditional_noise_pred = self.unet.Inference(
                latent_in, time_embedding, user_text_embedding
            )

            latent_in = self.run_scheduler(
                sd_input,
                unconditional_noise_pred,
                conditional_noise_pred,
                latent_in,
                time_step,
            )
            callback(step)

        # Run VAE
        import datetime

        now = datetime.datetime.now()
        output_image = self.vae_decoder.Inference(latent_in)
        formatted_time = now.strftime("%Y_%m_%d_%H_%M_%S")

        if len(output_image) == 0:
            callback(None)
        else:
            image_size = 512

            # Run RealESRGan
            if sd_input.is_high_resolution:
                print(f"Upscaler used: {sd_input.upscaler_model_path}")
                output_image = upscaler_pipeline.execute(output_image)
                image_size = 2048

            output_image = np.clip(output_image * 255.0, 0.0, 255.0).astype(np.uint8)
            output_image = output_image.reshape(image_size, image_size, -1)
            image = Image.fromarray(output_image, mode="RGB")  # .save(image_path)

            callback(image)

        PerfProfile.RelPerfProfileGlobal()
        return image

    # Release all the models.
    def __del__(self):
        del self.text_encoder
        del self.unet
        del self.vae_decoder
        if self.scheduler:
            del self.scheduler
        del self.tokenizer

    def is_model_loaded(self):
        return self.unet != None


pipeline = None
upscaler_pipeline: UpscalerPipeline = None
