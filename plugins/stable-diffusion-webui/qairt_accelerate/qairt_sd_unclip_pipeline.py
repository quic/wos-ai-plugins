# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import time
from PIL import Image
import PIL
import os
import sys
import shutil
import cv2
import numpy as np
import torch
from skimage.transform import resize
from modules.safe import unsafe_torch_load as load
from transformers import CLIPTokenizer, CLIPImageProcessor, CLIPVisionModelWithProjection, CLIPTextModel
# from modules import paths, shared, modelloader
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
from diffusers import UNet2DConditionModel, DDPMScheduler
from diffusers.pipelines.stable_diffusion.stable_unclip_image_normalizer import StableUnCLIPImageNormalizer
from diffusers.models.embeddings import get_timestep_embedding, TimestepEmbedding, Timesteps
import install
import common_utils as utils
import qairt_constants as consts
from pipeline_utils import StableDiffusionInput, download_qualcomm_models_hf, UpscalerPipeline, QPipeline, set_scheduler


class TextEncoder(QNNContext):
    # @timer
    def Inference(self, input_data):
        input_datas = [input_data]
        output_datas = super().Inference(input_datas, perf_profile="burst")
        output_data = output_datas[0]
        output_data = output_data.reshape((1, 77, 1024))
        return output_data


class Unet(QNNContext):
    # @timer
    def Inference(self, input_data_1, input_data_2, input_data_3):
        # We need to reshape the array to 1 dimensionality before send it to the network. 
        # 'input_data_2' already is 1 dimensionality, so doesn't need to reshape.
        input_data_1 = input_data_1.reshape(input_data_1.size)
        input_data_3 = input_data_3.reshape(input_data_3.size)

        input_datas = [input_data_1, input_data_2, input_data_3]
        
        output_data = super().Inference(input_datas, perf_profile="burst")[0]
        output_data = output_data.reshape(1, 96, 96, 4)
        return output_data


class VaeDecoder(QNNContext):
    # @timer
    def Inference(self, input_data):
        #input_data = input_data.reshape(input_data.size)
        input_datas = [input_data]

        output_data = super().Inference(input_datas, perf_profile="burst")
        output_data = output_data[0].reshape(1, 768, 768, 3)
        
        return output_data

class ImageEncoder(QNNContext):
    def Inference(self, input_data):
        # input_data = input_data.reshape(input_data.size)
        input_datas = [input_data]
        output_data = super().Inference(input_datas, perf_profile="burst")
        output_data = output_data[1].reshape(1, 768)
        return output_data


class QnnStableDiffusionUnClipPipeline(QPipeline):
    TOKENIZER_MAX_LENGTH = 77  # Define Tokenizer output max length (must be 77)
    scheduler = None
    tokenizer = None
    text_encoder = None
    noise_scheduler = None
    feature_extractor = None
    unet = None
    vae_decoder = None
    image_encoder = None
    image_embeds = None
    image_noise_embeds = None
    unet_time_embeddings = None

    def __init__(self, model_name):
        print(f"model_name in Pipeline: {model_name}")
        super().__init__(model_name)
        self.load_model()

        torch.from_numpy(
            np.array([1])
        )  # Let LazyImport to import the torch & numpy lib here.

    def run_tokenizer(self, prompt):
        text_input = self.tokenizer(
            prompt,
            padding="max_length",
            max_length=self.TOKENIZER_MAX_LENGTH,
            truncation=True,
        )
        text_input = np.array(text_input.input_ids, dtype=np.float32)
        return text_input
    
    def image_embedding(self, image):
        image_embeds = None
        repeat_by = 1 # batch_size or num_images_per_prompt
        """
            Add noise to the image embeddings. The amount of noise is controlled by a `noise_level` input. A higher
            `noise_level` increases the variance in the final un-noised images.

            The noise is applied in two ways:
            1. A noise schedule is applied directly to the embeddings.
            2. A vector of sinusoidal time embeddings are appended to the output.

            In both cases, the amount of noise is controlled by the same `noise_level`.

            The embeddings are normalized before the noise is applied and un-normalized after the noise is applied.
        """
        noise_level = 10 
        image = self.feature_extractor(images=image, return_tensors="np").pixel_values
        image = np.transpose(image, (0, 2, 3, 1)).copy()
        image_embeds = self.image_encoder.Inference(image)
        image_embeds = torch.from_numpy(image_embeds)
        noise_level = torch.tensor([noise_level])
        noise = torch.randn(image_embeds.shape, device=image_embeds.device, dtype=image_embeds.dtype)
        noise_level = torch.tensor([noise_level] * image_embeds.shape[0], device=image_embeds.device)
        image_embeds = self.noise_scheduler.add_noise(image_embeds, timesteps=noise_level, noise=noise)
        noise_level = get_timestep_embedding(
            timesteps=noise_level, embedding_dim=image_embeds.shape[-1], flip_sin_to_cos=True, downscale_freq_shift=0
        )
        image_embeds = torch.cat((image_embeds, noise_level), 1)
        return image_embeds

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
        ).prev_sample.detach().numpy()

        # Convert latent_out from NCHW to NHWC
        latent_out = np.transpose(latent_out, (0, 2, 3, 1)).copy()

        return latent_out

    def get_timestep(self, step):
        return np.int32(self.scheduler.timesteps.numpy()[step])

    def get_time_embedding(self, timestep):
        timestep = torch.tensor([timestep])
        t_emb = get_timestep_embedding(timestep, 320, True, 0)
        emb = self.unet_time_embeddings(t_emb).detach().numpy()
        return emb

    def load_model(self):
        QNNConfig.Config(
            consts.QNN_LIBS_DIR, Runtime.HTP, LogLevel.ERROR, ProfilingLevel.BASIC
        )
        # model names
        model_text_encoder = "text_encoder"
        model_unet = "model_unet"
        model_vae_decoder = "vae_decoder"
        model_image_encoder = "image_encoder"

        #check and download the required models.
        if not os.path.isfile(consts.QAI_HUB_VENV_PYTHON_PATH):
            install.create_venv_for_qai_hub()
            inatall.install_qai_hub()
        print("Downloading required models using qai-hub...")
        utils.run_command(f"{consts.QAI_HUB_VENV_PYTHON_PATH} {consts.EXTENSION_WS}//qairt_hub_models.py sd_2_1_unclip")
        print("Downloaded required models.")
        install.restore_qai_hub_key()

        model_path = consts.SD2_1_UNCLIP_PATH

        self.unet_time_embeddings = TimestepEmbedding(320, 1280)
        self.unet_time_embeddings.load_state_dict(load(f'{consts.SD2_1_UNCLIP_PATH}//unclip_time_embeddings.pt'))
        self.unet_class_embeddings = TimestepEmbedding(1536, 1280)
        self.unet_class_embeddings.load_state_dict(load(f'{consts.SD2_1_UNCLIP_PATH}//unclip_class_embeddings.pt'))
        self.tokenizer = CLIPTokenizer.from_pretrained(
            "stabilityai/stable-diffusion-2-1-unclip-small",
            subfolder="tokenizer",
            revision="main",
            cache_dir=consts.CACHE_DIR,)
        self.noise_scheduler = DDPMScheduler.from_pretrained(
            "stabilityai/stable-diffusion-2-1-unclip-small", 
            subfolder="image_noising_scheduler", 
            revision="main",
            cache_dir=consts.CACHE_DIR)
        self.feature_extractor = CLIPImageProcessor.from_pretrained(
            "stabilityai/stable-diffusion-2-1-unclip-small", 
            subfolder="feature_extractor", 
            revision="main",
            cache_dir=consts.CACHE_DIR,)
        image_encodel_model = "{}\\ImageEncoder_Quantized_16.bin".format(model_path)
        text_encoder_model = "{}\\TextEncoder_Quantized.bin".format(model_path)
        unet_model = "{}\\Unet_Quantized.bin".format(model_path)
        vae_decoder_model = "{}\\Vae_Quantized.bin".format(model_path)
        # Instance for Unet
        self.unet = Unet(model_unet, unet_model)
        # Instance for TextEncoder
        self.text_encoder = TextEncoder(model_text_encoder, text_encoder_model)
        # Instance for VaeDecoder
        self.vae_decoder = VaeDecoder(model_vae_decoder, vae_decoder_model)
        #Instance for ImageEncoder
        self.image_encoder = ImageEncoder(model_image_encoder, image_encodel_model)

    def reload_model(self, model_name):
        del self.tokenizer
        del self.noise_scheduler
        del self.feature_extractor
        del self.image_encoder
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
        image = sd_input.input_image
        # input_image = np.asarray(sd_input.input_image) # sd_input.input_image
        PerfProfile.SetPerfProfileGlobal(PerfProfile.BURST)
        self.scheduler = set_scheduler("stabilityai/stable-diffusion-2-1-unclip-small", sd_input.sampler_name)

        # Run Tokenizer
        cond_tokens = self.run_tokenizer(sd_input.user_prompt)
        uncond_tokens = self.run_tokenizer(sd_input.uncond_prompt)

        # Run Text Encoder on Tokens
        user_text_embedding = self.text_encoder.Inference(cond_tokens)
        uncond_text_embedding = self.text_encoder.Inference(uncond_tokens)

        # Run Image encoder on Images
        self.image_embeds = self.image_embedding(sd_input.input_image)
        self.image_noise_embeds = torch.zeros_like(self.image_embeds)

        # Initialize the latent input with random initial latent
        random_init_latent = torch.randn(
            (1, 4, 96, 96), generator=torch.manual_seed(sd_input.user_seed)
        ).numpy()
        self.scheduler.set_timesteps(
            sd_input.user_step
        )  # Setting up user provided time steps for Scheduler
        latent_in = np.transpose(random_init_latent, (0, 2, 3, 1)).copy()
        class_cond_emb = self.unet_class_embeddings(self.image_embeds).detach().numpy()
        class_uncond_emb = self.unet_class_embeddings(self.image_noise_embeds).detach().numpy()

        
        # Run the loop for user_step times
        for step in range(sd_input.user_step):
            # print(f"Step {step} Running...")

            time_step = self.get_timestep(step)
            time_embedding1 = self.get_time_embedding(
                time_step)
            time_cond_embedding = np.add(time_embedding1, class_cond_emb)
            time_uncond_embedding = np.add(time_embedding1, class_uncond_emb)

            conditional_noise_pred = self.unet.Inference(
                latent_in, time_cond_embedding, user_text_embedding)
            unconditional_noise_pred = self.unet.Inference(
                latent_in, time_uncond_embedding, uncond_text_embedding)

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
            image_size = 768

            # Run RealESRGan
            if sd_input.is_high_resolution:
                output_image = resize(output_image, (1, 512, 512, 3), anti_aliasing=True)
                print(f"Upscaler used: {sd_input.upscaler_model_path}")
                output_image = upscaler_pipeline.execute(output_image)
                image_size = 2048

            output_image = np.clip(output_image * 255.0, 0.0, 255.0).astype(np.uint8)
            output_image = output_image.reshape(image_size, image_size, -1)
            image = Image.fromarray(output_image, mode="RGB")  # .save(image_path)

            callback(image)

        PerfProfile.RelPerfProfileGlobal()

        # PerfProfile.RelPerfProfileGlobal()
        return image

    # Release all the models.
    def __del__(self):
        del self.text_encoder
        del self.noise_scheduler
        del self.feature_extractor
        del self.image_encoder
        del self.unet
        del self.vae_decoder
        if self.scheduler:
            del self.scheduler
        del self.tokenizer

    def is_model_loaded(self):
        return self.unet != None
