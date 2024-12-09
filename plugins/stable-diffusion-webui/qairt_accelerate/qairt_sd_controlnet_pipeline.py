# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

from PIL import Image
import os
import cv2
import numpy as np
import torch
from transformers import CLIPTokenizer
from modules import paths
from qai_appbuilder import (
    QNNContext,
    Runtime,
    LogLevel,
    ProfilingLevel,
    PerfProfile,
    QNNConfig,
)
from diffusers.models.embeddings import get_timestep_embedding, TimestepEmbedding
import qairt_constants as consts
from pipeline_utils import StableDiffusionInput, download_qualcomm_models_hf, UpscalerPipeline, QPipeline, set_scheduler
from modules.safe import unsafe_torch_load as load


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

def get_model_path(model_name):
    return os.path.abspath(
        os.path.join(
            paths.models_path, "Stable-diffusion", f"qcom-{model_name}"
        )
    )

class QnnControlNetPipeline(QPipeline):
    TOKENIZER_MAX_LENGTH = 77  # Define Tokenizer output max length (must be 77)

    scheduler = None
    tokenizer = None
    text_encoder = None
    unet = None
    vae_decoder = None
    controlnet = None

    unet_time_embeddings_1_5 = None
    unet_time_embeddings_2_1 = None

    def __init__(self, model_name):
        print(f"model_name in CNPipeline: {model_name}")
        super().__init__(model_name)
        self.unet_time_embeddings_1_5 = TimestepEmbedding(320, 1280)
        self.unet_time_embeddings_1_5.load_state_dict(
            load(consts.TIMESTEP_EMBEDDING_1_5_MODEL_PATH)
        )
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

    def make_canny_image(self, input_image: Image):
        image = np.asarray(input_image)

        # Get edges for input with Canny Edge Detection
        low_threshold = 100
        high_threshold = 200
        image = cv2.resize(image, (512, 512))

        image = cv2.Canny(image, low_threshold, high_threshold)
        # cv2.imwrite(os.path.join(consts.OUTPUTS_DIR, "canny.png"), image)
        image = image[:, :, None]
        image = np.concatenate([image, image, image], axis=2)

        image = Image.fromarray(image)

        image = np.array(image)
        image = image[None, :]

        image = image.astype(np.float32) / 255.0
        return image

    def load_model(self):
        QNNConfig.Config(
            consts.QNN_LIBS_DIR, Runtime.HTP, LogLevel.ERROR, ProfilingLevel.BASIC
        )
        # model names
        model_text_encoder = "text_encoder"
        model_unet = "model_unet"
        model_vae_decoder = "vae_decoder"
        model_controlnet = "controlnet"

        model_path = get_model_path(model_name=self.model_name)
        if (not os.path.exists(model_path) or not os.path.exists(os.path.join(model_path, "TextEncoder_Quantized.bin"))
                or not os.path.exists(os.path.join(model_path, "ControlNet_Quantized.bin"))
                or not os.path.exists(os.path.join(model_path, "UNet_Quantized.bin"))
                or not os.path.exists(os.path.join(model_path, "VAEDecoder_Quantized.bin"))):
            download_qualcomm_models_hf(
                model_path,
                consts.CONTROLNET_1_0_SD_1_5_CANNY_REPO_ID,
                [
                    "TextEncoder_Quantized.bin",
                    "ControlNet_Quantized.bin",
                    "UNet_Quantized.bin",
                    "VAEDecoder_Quantized.bin",
                ],
                consts.CONTROLNET_1_0_SD_1_5_CANNY_REVISION,
            )
        # Initializing the Tokenizer
        self.tokenizer = CLIPTokenizer.from_pretrained(
            "openai/clip-vit-base-patch32", cache_dir=consts.CACHE_DIR
        )
        text_encoder_model = "{}\\TextEncoder_Quantized.bin".format(model_path)
        unet_model = "{}\\UNet_Quantized.bin".format(model_path)
        vae_decoder_model = "{}\\VAEDecoder_Quantized.bin".format(model_path)
        controlnet_model = "{}\\ControlNet_Quantized.bin".format(model_path)
        print(f"Loading models from {model_path}")
        # Instance for Unet
        self.unet = Unet(model_unet, unet_model)

        # Instance for TextEncoder
        self.text_encoder = TextEncoder(model_text_encoder, text_encoder_model)

        # Instance for VaeDecoder
        self.vae_decoder = VaeDecoder(model_vae_decoder, vae_decoder_model)

        # Instance for ControlNet
        self.controlnet = ControlNet(model_controlnet, controlnet_model)

    def reload_model(self, model_name):
        del self.tokenizer
        del self.text_encoder
        del self.unet
        del self.vae_decoder
        del self.controlnet
        self.load_model()

    # Execute the Stable Diffusion pipeline
    def model_execute(
        self,
        sd_input: StableDiffusionInput,
        callback,
        upscaler_pipeline: UpscalerPipeline,
    ) -> Image:
        image = sd_input.input_image
        PerfProfile.SetPerfProfileGlobal(PerfProfile.BURST)
        self.scheduler = set_scheduler("stable-diffusion-v1-5/stable-diffusion-v1-5", sd_input.sampler_name)

        self.scheduler.set_timesteps(
            sd_input.user_step
        )  # Setting up user provided time steps for Scheduler

        # Run Tokenizer
        cond_tokens = self.run_tokenizer(sd_input.user_prompt)
        uncond_tokens = self.run_tokenizer(sd_input.uncond_prompt)

        # Run Text Encoder on Tokens
        uncond_text_embedding = self.text_encoder.Inference(uncond_tokens)
        user_text_embedding = self.text_encoder.Inference(cond_tokens)

        # Initialize the latent input with random initial latent
        random_init_latent = torch.randn(
            (1, 4, 64, 64), generator=torch.manual_seed(sd_input.user_seed)
        ).numpy()
        latent_in = random_init_latent.transpose(0, 2, 3, 1)

        # image = load_image(input_image_path)
        canny_image = self.make_canny_image(image)

        # Run the loop for user_step times
        for step in range(sd_input.user_step):
            # print(f"Step {step} Running...")

            time_step = self.get_timestep(step)
            time_embedding = self.get_time_embedding(
                time_step, self.unet_time_embeddings_1_5
            )

            controlnet_out = self.controlnet.Inference(
                latent_in, time_embedding, user_text_embedding, canny_image
            )

            conditional_noise_pred = self.unet.Inference(
                latent_in, time_embedding, user_text_embedding, *controlnet_out
            )

            controlnet_out = self.controlnet.Inference(
                latent_in, time_embedding, uncond_text_embedding, canny_image
            )

            unconditional_noise_pred = self.unet.Inference(
                latent_in, time_embedding, uncond_text_embedding, *controlnet_out
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
        del self.controlnet

    def is_model_loaded(self):
        return self.unet != None

