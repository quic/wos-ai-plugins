# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import bpy
from bpy.props import (
    StringProperty,
    IntProperty,
    FloatProperty,
    EnumProperty,
    BoolProperty,
)
import subprocess
from .utils import *
from .qairt_constants import *
import random
import json
import time
import re


bl_info = {
    "name": "Snapdragon Image Generation",
    "author": "Snapdragon AI contributors",
    "description": "Use ControlNet-CannyEdge model to generate new images similar to the given input image.",
    "blender": (3, 1, 0),
    "version": (1, 0, 0),
    "location": "Image Editor -> Sidebar -> SnapdragonAI",
    "category": "Paint",
}


def log(message):
    with open(LOG_FILE, LogMode.RECORDING.value) as log_file:
        log_file.write(f"{message}\n")


def process_command(
    command,
    start_marker,
    end_marker,
    log_file_path,
    log_mode,
    on_done_callback,
    step_callback=None,
):
    global process

    def output_callback(output):
        with open(log_file_path, log_mode) as log_file:
            log_file.write(f"{output}\n")

        is_success = False
        if output.find(start_marker) != -1 and output.find(end_marker) != -1:
            start_index = output.find(start_marker) + len(start_marker)
            end_index = output.find(end_marker)
            is_success = output[start_index:end_index].strip() == 'True'
            log(f"striped={output[start_index:end_index].strip()} is_success={is_success}")
        on_done_callback(is_success)


    send_command(process, command, output_callback, step_callback)


class Model_OT_LoadPanel(bpy.types.Operator):
    bl_idname = "server.load_model"
    bl_label = "Load Model"
    bl_description = "Load the model"

    @classmethod
    def poll(cls, context):
        return not context.scene.is_model_loading

    def execute(self, context):
        global process

        def step_callback(log_line):
            total_models = 4
            pattern = "Time\:\ model_initialize"
            if log_line and re.search(pattern, log_line):
                context.scene.progress += 1 / total_models

        log_file_path = LOG_FILE
        if process is None or not context.scene.is_model_loaded:
            env = os.environ.copy()
            env["PYTHONPATH"] = (
                f"{PLUGIN_DIR}{os.pathsep}{env.get('PYTHONPATH', '')}"
            )

            process = subprocess.Popen(
                [VENV_PYTHON, CONTROLNET_PIPELINE],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                env=env,
            )

            load_command = {"action": "load_model", "params": {}}
            start_marker = "LOAD_MODEL_RESULT_START"
            end_marker = "LOAD_MODEL_RESULT_END"

            def on_done_callback(is_success):
                if is_success:
                    context.scene.is_model_loaded = True
                else:
                    context.scene.is_model_loaded = False
                    show_error(f"Failed while loading models.\n Look at the generated addon logs for more details {LOG_FILE}")
                context.scene.is_model_loading = False
                context.scene.progress = 0

            context.scene.is_model_loading = True
            process_command(
                load_command,
                start_marker,
                end_marker,
                log_file_path,
                LogMode.BEGIN.value,
                on_done_callback=on_done_callback,
                step_callback=step_callback,
            )

        else:
            context.scene.is_model_loaded = True
            self.report({"INFO"}, "Model is already loaded.")
        return {"FINISHED"}


class Model_OT_UnloadPanel(bpy.types.Operator):
    bl_idname = "server.unload_model"
    bl_label = "Unload Model"
    bl_description = "Unload the model"

    @classmethod
    def poll(cls, context):
        return (
            not context.scene.is_model_unloading and not context.scene.is_model_running
        )

    def execute(self, context):
        global process

        def step_callback(log_line):
            total_models = 4
            pattern = "Time\:\ model_destroy"
            if log_line and re.search(pattern, log_line):
                context.scene.progress += 1 / total_models

        log_file_path = LOG_FILE
        if process is not None and context.scene.is_model_loaded:
            unload_command = {"action": "unload_model", "params": {}}
            start_marker = "UNLOAD_MODEL_RESULT_START"
            end_marker = "UNLOAD_MODEL_RESULT_END"

            def on_done_callback(is_success):
                if is_success:
                    context.scene.is_model_loaded = False
                else:
                    context.scene.is_model_loaded = True
                    show_error(f"Failed while unloading models.\n Look at the generated addon logs for more details {LOG_FILE}")
                context.scene.is_model_unloading = False
                context.scene.progress = 0

            context.scene.is_model_unloading = True
            process_command(
                unload_command,
                start_marker,
                end_marker,
                log_file_path,
                LogMode.RECORDING.value,
                on_done_callback=on_done_callback,
                step_callback=step_callback,
            )
        else:
            context.scene.is_model_loaded = False
            self.report({"INFO"}, "Model is already unloaded.")
        return {"FINISHED"}


class CONVERSION_PT_ConversionPanel(bpy.types.Panel):
    bl_label = "3D to 2D Conversion"
    bl_idname = "CONVERSION_PT_conversion_panel"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Snapdragon Image Generation"

    def draw(self, context):
        layout = self.layout
        conversion_settings = context.scene.conversion_settings

        layout.prop(conversion_settings, "image_resolution")
        layout.operator("conversion.conversion_operator", text="Convert 3D to 2D")


class CONVERSION_OT_ConversionOperator(bpy.types.Operator):
    bl_idname = "conversion.conversion_operator"
    bl_label = "Convert 3D to 2D"
    bl_description = "Convert the 3D object scene to a 2D image"

    def execute(self, context):
        scene = context.scene
        conversion_settings = scene.conversion_settings
        output_path = bpy.path.abspath(
            os.path.join(OUTPUTS_DIR, f"2d_image_{str(time.time_ns())}.png")
        )

        if not scene.camera:
            self.report({"ERROR"}, "No camera found in the scene")
            return {"CANCELLED"}

        scene.render.image_settings.file_format = "PNG"
        if conversion_settings.image_resolution == "512_512":
            scene.render.resolution_x = 512
            scene.render.resolution_y = 512
        else:
            show_error("Invalid resolution selected")
        scene.render.filepath = output_path

        bpy.ops.render.render("INVOKE_DEFAULT", write_still=True)

        conversion_settings.input_image_path = output_path

        self.report({"INFO"}, f"Image saved at:{output_path}")
        return {"FINISHED"}


class RENDER_OT_ShowImage(bpy.types.Operator):
    bl_idname = "render.show_image"
    bl_label = "Show Rendered Image"
    bl_description = "Show the image"

    image_path: StringProperty(
        name="Image Path",
        description="Path to the image file",
        default="",
        maxlen=512,
        subtype="FILE_PATH",
    )

    def execute(self, context):
        image = bpy.data.images.load(filepath=self.image_path)

        is_image_loaded = False
        for window in bpy.data.window_managers["WinMan"].windows:
            for area in window.screen.areas:
                if area.type == "IMAGE_EDITOR":
                    is_image_loaded = True
                    area.spaces.active.image = image
                    break

        if not is_image_loaded:
            screen = bpy.context.screen
            for area in screen.areas:
                if area.type == 'VIEW_3D':
                    area.type = 'IMAGE_EDITOR'
                    area.spaces.active.image = image
                    break
        return {"FINISHED"}

    def invoke(self, context, event):
        wm = context.window_manager
        return wm.invoke_props_dialog(self)


class ConversionSettings(bpy.types.PropertyGroup):
    image_resolution: EnumProperty(
        name="Image Resolution",
        description="Size of the input image to the ControlNet model",
        items=[
            ("512_512", "512x512", "Generate image at 512x512 resolution"),
        ],
        default="512_512",
    )

    input_image_path: StringProperty(
        name="Input Image Path",
        description="Path of the last converted image",
        default="",
        subtype="FILE_PATH",
        update=lambda self, context: self.update_output_path(),
    )

    output_img_path: StringProperty(
        name="Output Image Path",
        description="Path of the generated image",
        default=get_generated_img_default_path(),
        subtype="FILE_PATH",
    )

    def update_output_path(self):
        if self.input_image_path:
            self.output_img_path = get_output_img_path(self.input_image_path)


class IMAGE_PT_GeneratorPanel(bpy.types.Panel):
    bl_label = "Image Generation"
    bl_idname = "IMAGE_PT_generator_panel"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Snapdragon Image Generation"

    def draw(self, context):
        layout = self.layout
        generator = context.scene.image_generator
        conversion_settings = context.scene.conversion_settings

        layout.prop(generator, "model")
        layout.prop(generator, "device")
        layout.prop(generator, "prompt")
        layout.prop(generator, "negative_prompt")
        layout.prop(conversion_settings, "input_image_path")
        layout.prop(generator, "seed")
        layout.prop(generator, "guidance_scale")
        layout.prop(generator, "steps")
        layout.prop(generator, "output_resolution")
        layout.prop(conversion_settings, "output_img_path")

        col = layout.column(align=True)
        row1 = col.row(align=True)
        row1.scale_y = 1.5
        row2 = col.row(align=True)
        row2.scale_y = 1.5
        if not context.scene.is_model_loaded:
            row1.operator("server.load_model", icon="PLAY", text="Load Models")
        else:
            row1.operator("server.unload_model", icon="X", text="Unload Models")
        row1.operator("image.generate", icon="PLAY", text="Generate")
        if (
            context.scene.is_model_running
            or context.scene.is_model_loading
            or context.scene.is_model_unloading
        ):
            row2.progress(factor=context.scene.progress, type="BAR")


class IMAGE_OT_GenerateOperator(bpy.types.Operator):
    bl_label = "Generate Image"
    bl_idname = "image.generate"
    bl_description = "Generate an image using the model"

    @classmethod
    def poll(cls, context):
        return (
            context.scene.is_model_loaded
            and not context.scene.is_model_running
            and not context.scene.is_model_unloading
        )

    def execute(self, context):
        scene = context.scene
        generate = scene.image_generator
        conversion_settings = scene.conversion_settings

        def step_callback(log_line):
            pattern = "Step\ ([0-9]+)\ Running\.\.\."
            if log_line:
                match = re.search(pattern, log_line)
                if match:
                    total_steps = int(generate.steps)
                    current_step = int(match.group(1)) + 0.5
                    context.scene.progress = current_step / total_steps

        if process is None or not context.scene.is_model_loaded:
            self.report({"INFO"}, "Model is not loaded.")
        else:
            input_path = conversion_settings.input_image_path
            if os.path.isfile(conversion_settings.output_img_path):
                conversion_settings.output_img_path = utils.get_random_output_img_path(
                    conversion_settings.output_img_path
                )
            output_path = conversion_settings.output_img_path
            prompt = generate.prompt
            negative_prompt = generate.negative_prompt
            seed = generate.seed
            steps = generate.steps
            cfg_scale = generate.guidance_scale
            output_resolution = generate.output_resolution

            context.scene.is_model_running = True

            def on_done_callback(is_success):
                if is_success:
                    bpy.ops.render.show_image(image_path=output_path)
                else:
                    show_error(f"Image generation failed.\n Look at the generated addon logs for more details {LOG_FILE}")
                context.scene.is_model_running = False
                context.scene.progress = 0

            self.generate_image(
                input_path,
                output_path,
                prompt,
                negative_prompt,
                seed,
                steps,
                cfg_scale,
                output_resolution,
                step_callback,
                on_done_callback=on_done_callback,
            )

        return {"FINISHED"}

    def generate_image(
        self,
        input_path,
        output_path,
        prompt,
        negative_prompt,
        seed,
        steps,
        cfg_scale,
        output_resolution,
        step_callback,
        on_done_callback,
    ):

        log_file_path = LOG_FILE
        is_high_resolution = 1 if output_resolution == "2048" else 0
        if seed == -1:
            seed = random.randrange(4294967294)
        data = {
            "input_path": input_path,
            "output_path": output_path,
            "prompt": prompt,
            "negative_prompt": negative_prompt,
            "seed": seed,
            "steps": steps,
            "cfg_scale": cfg_scale,
            "output_resolution": is_high_resolution,
        }

        exec_command = {"action": "run_model", "params": data}
        log(json.dumps(exec_command, indent=2))
        start_marker = "RUN_MODEL_RESULT_START"
        end_marker = "RUN_MODEL_RESULT_END"
        process_command(
            exec_command,
            start_marker,
            end_marker,
            log_file_path,
            LogMode.RECORDING.value,
            on_done_callback=on_done_callback,
            step_callback=step_callback,
        )


class ImageGeneratorProperties(bpy.types.PropertyGroup):
    model: EnumProperty(
        name="Model",
        description="Model to run",
        items=[
            (
                "controlnet_canny",
                "ControlNet Canny Quantized",
                "Generating visual arts from text prompt and input guiding image.",
            ),
        ],
        default="controlnet_canny",
    )

    device: EnumProperty(
        name="Device",
        description="Device to use",
        items=[
            ("NPU", "NPU", "Qualcomm X-Elite NPU"),
        ],
        default="NPU",
    )

    prompt: StringProperty(
        name="Prompt",
        description="Text prompt to guide the image generation",
        default="",
    )

    negative_prompt: StringProperty(
        name="Negative Prompt",
        description="Text prompt to avoid during the image generation",
        default="",
    )

    seed: IntProperty(
        name="Seed", description="Seed value for random number generation", default=10
    )

    guidance_scale: FloatProperty(
        name="Guidance Scale",
        description="Scale for guidance during image generation",
        default=9,
        min=5.0,
        max=15.0,
    )

    steps: EnumProperty(
        name="Steps",
        description="Number of steps for the image generation process",
        items=[
            ("10", "10 Steps", "10 steps for Unet"),
            ("15", "15 Steps", "15 steps for Unet"),
            ("20", "20 Steps", "20 steps for Unet"),
            ("25", "25 Steps", "25 steps for Unet"),
            ("30", "30 Steps", "30 steps for Unet"),
            ("35", "35 Steps", "35 steps for Unet"),
            ("40", "40 Steps", "40 steps for Unet"),
            ("50", "50 Steps", "50 steps for Unet"),
        ],
        default="20",
    )

    output_resolution: EnumProperty(
        name="Output Image Size",
        description="Size of the output image",
        items=[
            ("512", "512x512", "Generate image at 512x512 resolution"),
            ("2048", "2048x2048", "Generate image at 2048x2048 resolution"),
        ],
        default="512",
    )


def register():
    bpy.utils.register_class(CONVERSION_PT_ConversionPanel)
    bpy.utils.register_class(CONVERSION_OT_ConversionOperator)
    bpy.utils.register_class(RENDER_OT_ShowImage)
    bpy.utils.register_class(Model_OT_LoadPanel)
    bpy.utils.register_class(Model_OT_UnloadPanel)
    bpy.utils.register_class(ConversionSettings)

    bpy.types.Scene.conversion_settings = bpy.props.PointerProperty(
        type=ConversionSettings
    )

    bpy.utils.register_class(IMAGE_PT_GeneratorPanel)
    bpy.utils.register_class(IMAGE_OT_GenerateOperator)
    bpy.utils.register_class(ImageGeneratorProperties)

    bpy.types.Scene.image_generator = bpy.props.PointerProperty(
        type=ImageGeneratorProperties
    )

    def step_progress_update(self, context):
        if hasattr(context.area, "regions"):
            for region in context.area.regions:
                if region.type == "UI":
                    region.tag_redraw()
        return None

    bpy.types.Scene.progress = bpy.props.FloatProperty(
        name="", default=0, min=0, max=1, update=step_progress_update
    )
    bpy.types.Scene.is_model_loaded = bpy.props.BoolProperty(
        name="Model Loaded", default=False
    )
    bpy.types.Scene.is_model_loading = bpy.props.BoolProperty(
        name="Is model loading", default=False
    )
    bpy.types.Scene.is_model_unloading = bpy.props.BoolProperty(
        name="Is model loading", default=False
    )
    bpy.types.Scene.is_model_running = bpy.props.BoolProperty(
        name="Is model running", default=False
    )


def unregister():
    bpy.utils.unregister_class(CONVERSION_PT_ConversionPanel)
    bpy.utils.unregister_class(CONVERSION_OT_ConversionOperator)
    bpy.utils.unregister_class(RENDER_OT_ShowImage)
    bpy.utils.unregister_class(Model_OT_LoadPanel)
    bpy.utils.unregister_class(Model_OT_UnloadPanel)
    bpy.utils.unregister_class(ConversionSettings)

    del bpy.types.Scene.conversion_settings

    bpy.utils.unregister_class(IMAGE_PT_GeneratorPanel)
    bpy.utils.unregister_class(IMAGE_OT_GenerateOperator)
    bpy.utils.unregister_class(ImageGeneratorProperties)

    del bpy.types.Scene.image_generator


if __name__ == "__main__":
    register()
