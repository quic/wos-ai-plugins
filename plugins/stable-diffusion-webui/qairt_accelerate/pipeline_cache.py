# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

from qairt_sd_pipeline import QnnStableDiffusionPipeline
from qairt_sd_controlnet_pipeline import QnnControlNetPipeline
from qairt_sd_unclip_pipeline import QnnStableDiffusionUnClipPipeline
import qairt_constants as consts
from pipeline_utils import UpscalerPipeline, get_upscaler_model

class Singleton(type):
    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


class PipelineCache(metaclass=Singleton):
    pipeline = None
    upscaler_pipeline = None

    def __init__(self):
        self.pipeline = QnnStableDiffusionPipeline(consts.DEFAULT_TXT2IMG_MODEL)
        self.upscaler_pipeline = UpscalerPipeline(
            "ESRGAN_4x", consts.ESRGAN_X4_MODEL_PATH
        )

    def reload_pipeline(self, qnn_model_name):
        if self.pipeline.model_name == qnn_model_name:
            return qnn_model_name

        print(f"### Reloading model {qnn_model_name}")
        del self.pipeline

        if qnn_model_name in consts.CONTROLNET_MODELS:
            self.pipeline = QnnControlNetPipeline(qnn_model_name)
        elif qnn_model_name in consts.SD_UNCLIP_MODELS:
            self.pipeline = QnnStableDiffusionUnClipPipeline(qnn_model_name)
        elif qnn_model_name in consts.STABLE_DIFFUSION_MODELS:
            self.pipeline = QnnStableDiffusionPipeline(qnn_model_name)
        else:
            raise Exception(f"Unrecognized model {qnn_model_name}!") 
        return qnn_model_name

    def reload_upscaler_pipeline(self, upscaler_model_name):
        if upscaler_model_name == "":
            del self.upscaler_pipeline
            return
        if self.upscaler_pipeline.model_name != upscaler_model_name:
            del self.upscaler_pipeline
            _, upscaler_model_path = get_upscaler_model(upscaler_model_name)
            self.upscaler_pipeline = UpscalerPipeline(
                upscaler_model_name, upscaler_model_path
            )
