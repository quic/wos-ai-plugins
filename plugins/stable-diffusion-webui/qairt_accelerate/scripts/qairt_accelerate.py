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
import qairt_constants as consts
from pipeline_utils import StableDiffusionInput
from pipeline_cache import PipelineCache
from modules.ui_components import InputAccordion
from modules.ui_common import create_refresh_button

def modelExecuteCallback(result, sd_input):
    if Image.isImageType(result):
        print("Image Generation successful")
    elif None == result:
        print("modelExecuteCallback result: No image generated")


class Script(scripts.Script):
    pipeline_cache: PipelineCache = None
    
    def __init__(self):
        self.pipeline_cache = PipelineCache()

    def title(self):
        return "Accelerate with Qualcomm AI Runtime"

    def show(self, is_img2img):
        return True

    def ui(self, is_img2img):
        with gr.Blocks() as demo:
            if is_img2img:
                qnn_model_name = gr.Dropdown(
                    label="Model to use",
                    choices=consts.CONTROLNET_MODELS + consts.SD_UNCLIP_MODELS,
                    value=consts.DEFAULT_IMG2IMG_MODEL,
                    visible=True,
                )
            else:
                qnn_model_name = gr.Dropdown(
                    label="Model to use",
                    choices=consts.STABLE_DIFFUSION_MODELS,
                    value=consts.DEFAULT_TXT2IMG_MODEL,
                    visible=True,
                )
            qnn_model_name.change(
                fn=self.pipeline_cache.reload_pipeline, inputs=qnn_model_name, outputs=qnn_model_name
            )
            with InputAccordion(False, label="Upscale", elem_id=self.elem_id("enable"), visible=is_img2img) as enable_upscale:
                with gr.Row():
                    upscaler_model = gr.Dropdown(
                        label="Upscaler model to use",
                        choices=["ESRGAN_4x"],
                        value="",
                        visible=is_img2img,
                    )
                    upscaler_model.change(
                        fn=self.pipeline_cache.reload_pipeline, inputs=qnn_model_name, outputs=qnn_model_name
                    )

            gr.Markdown(
                """
            ###
            ### Note:
            - The respective model will be downloaded onto local disk if it is not yet on local disk.
            - First inference involves loading of the model. Therefore, the first inference will be slower than subsequent inferences.
            - For accurate performance measurements, it is recommended to exclude this slower first inference.
            """
            )
        return [qnn_model_name, enable_upscale, upscaler_model]

    def get_info_message(self, sd_input: StableDiffusionInput, qnn_model_name, image):
        message = f"{sd_input.user_prompt}\nNegative prompt: {sd_input.uncond_prompt}\n"
        message += f"Steps: {sd_input.user_step}, Sampler: {sd_input.sampler_name}, "
        message += (
            f"CFG scale: {sd_input.user_text_guidance}, Seed: {sd_input.user_seed}, "
        )
        message += f"Size: {image.size[0]}x{image.size[1]}, "
        message += (
            f"Model: {qnn_model_name}, Enable Hires: {sd_input.is_high_resolution}"
        )
        if sd_input.is_high_resolution:
            message += f", Upscaler model: {sd_input.upscaler_model_used}"
        return message

    def run(self, p: StableDiffusionProcessing, qnn_model_name, enable_upscale, upscaler_model=None):
        time_start = time.time()

        user_seed = get_fixed_seed(p.seed)

        sd_input = StableDiffusionInput(
            self.is_txt2img,
            p.prompt,
            p.negative_prompt,
            user_seed,
            p.steps,
            float(p.cfg_scale),
            p.sampler_name,
            qnn_model_name,
        )
        if self.is_img2img:
            if len(p.init_images) == 0:
                raise Exception("Input image is empty!")
            sd_input.set_image(p.init_images[0])
            sd_input.set_high_res(enable_upscale)
            sd_input.set_upscaler_model_name(upscaler_model)
        else:
            sd_input.set_upscaler_model_name(p.hr_upscaler)
            sd_input.set_high_res(p.enable_hr)

        
        if self.pipeline_cache.pipeline and self.pipeline_cache.pipeline.model_name: 
            print(f"self.pipeline : {self.pipeline_cache.pipeline} self.pipeline.model_name : {self.pipeline_cache.pipeline.model_name} qnn_model_name {qnn_model_name}")
        self.pipeline_cache.reload_pipeline(qnn_model_name)
        self.pipeline_cache.reload_upscaler_pipeline(sd_input.upscaler_model_name)

        image = self.pipeline_cache.pipeline.model_execute(
            sd_input,
            lambda result: modelExecuteCallback(result, sd_input),
            self.pipeline_cache.upscaler_pipeline,
        )
        time_end = time.time()
        print("time consumes for inference {}(s)".format(str(time_end - time_start)))

        return Processed(
            p,
            [image],
            seed=user_seed,
            info=self.get_info_message(sd_input, qnn_model_name, image),
        )
