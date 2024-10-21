# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import os
import json
from enum import Enum
from .qairt_constants import *
import traceback
import time
import functools
import bpy

class LogMode(Enum):
    BEGIN = "w"
    RECORDING = "a"

process = None

def get_output_img_path(input_path):
    base_dir, file = os.path.split(input_path)
    file_name = file.split('.')[0] + "_controlnet"
    return os.path.join(PLUGIN_DIR, f"Outputs\\{file_name}.png")

def get_random_output_img_path(input_path):
    base_dir, file = os.path.split(input_path)
    file_name = file.split('.')[0] + "_" + str(time.time_ns())
    return os.path.join(PLUGIN_DIR, f"Outputs\\{file_name}.png")

def get_generated_img_default_path():
    return os.path.join(PLUGIN_DIR, "Outputs\\generated_image.png")


def read_pipeline_output(process, output_lines, output_callback, step_callback):
    try:
        line = process.stdout.readline().strip()
        output_lines.append(line)
        if step_callback is not None:
            step_callback(line)
        if "END_DATA" in line:
            response = "\n".join(output_lines)
            output_callback(response)
            return None
        return 0.05
    except Exception as e:
        response = "\n".join(output_lines)
        output_callback(f"{response}\n{str(e)}\n{traceback.format_exc()}")
        return None

def send_command(process, command, output_callback, step_callback=None):
    output_lines = []
    process.stdin.write(json.dumps(command) + '\n')
    process.stdin.flush()

    bpy.app.timers.register(functools.partial(read_pipeline_output, process, output_lines, output_callback, step_callback))


def show_error(message=""):
    def draw(self, context):
        self.layout.label(text=message)

    bpy.context.window_manager.popup_menu(draw, title="Error", icon="ERROR")

