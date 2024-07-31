# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import modules.scripts as scripts
import gradio as gr
from modules.processing import Processed, get_fixed_seed
from modules.processing import StableDiffusionProcessing
import time
from PIL import Image
import qairt_sd_pipeline as sd
import qairt_constants as consts

DEFAULT_MODEL = "Stable-Diffusion-1.5"
sd.pipeline = sd.QnnStableDiffusionPipeline(DEFAULT_MODEL)
sd.upscaler_pipeline = sd.UpscalerPipeline("ESRGAN_4x", consts.ESRGAN_X4_MODEL_PATH)

def modelExecuteCallback(result, sd_input):
    if Image.isImageType(result):
        print("Image Generation successful")
    elif None == result:
        print("modelExecuteCallback result: No image generated")

class Script(scripts.Script):

    def title(self):
        return "Accelerate with Qualcomm AI Runtime"

    def show(self, is_img2img):
        return True

    def reload_pipeline(self, qnn_model_name):
        if sd.pipeline:
            del sd.pipeline
        sd.pipeline = sd.QnnStableDiffusionPipeline(qnn_model_name)
        return qnn_model_name

    def ui(self, is_img2img):
        with gr.Blocks() as demo:
            qnn_model_name = gr.Dropdown(label="Model to use", choices=["Stable-Diffusion-1.5", "Stable-Diffusion-2.1"], value=DEFAULT_MODEL, visible=True)
            qnn_model_name.change(fn=self.reload_pipeline,inputs=qnn_model_name, outputs=qnn_model_name)
        return [qnn_model_name]

    def get_info_message(self, sd_input: sd.StableDiffusionInput, qnn_model_name, image):
        message = f"{sd_input.user_prompt}\nNegative prompt: {sd_input.uncond_prompt}\n"
        message += f"Steps: {sd_input.user_step}, Sampler: {sd_input.sampler_name}, "
        message += f"CFG scale: {sd_input.user_text_guidance}, Seed: {sd_input.user_seed}, "
        message += f"Size: {image.size[0]}x{image.size[1]}, "
        message += f"Model: {qnn_model_name}, Enable Hires: {sd_input.is_high_resolution}"
        if sd_input.is_high_resolution:
            message += f", Upscaler model: {sd_input.upscaler_model_used}"
        return message

    def run(self, p: StableDiffusionProcessing, qnn_model_name):
        time_start = time.time()

        user_seed = get_fixed_seed(p.seed)
        supported_samplers = ["DPM++ 2M"]
        if (p.sampler_name not in supported_samplers):
            p.sampler_name = "DPM++ 2M"
        sd_input = sd.StableDiffusionInput(
            p.prompt,
            p.negative_prompt,
            user_seed,
            p.steps,
            float(p.cfg_scale),
            p.enable_hr,
            p.sampler_name,
            qnn_model_name,
            p.hr_upscaler
        )
        if sd.upscaler_pipeline.model_name != sd_input.upscaler_model_name:
            del sd.upscaler_pipeline
            sd.upscaler_pipeline = sd.UpscalerPipeline(sd_input.upscaler_model_name, sd_input.upscaler_model_path)

        image = sd.pipeline.model_execute(
            sd_input, lambda result: modelExecuteCallback(result, sd_input), sd.upscaler_pipeline
        )

        time_end = time.time()
        print("time consumes for inference {}(s)".format(str(time_end - time_start)))

        return Processed(
            p,
            [image],
            seed=user_seed,
            info=self.get_info_message(sd_input, qnn_model_name, image),
        )
