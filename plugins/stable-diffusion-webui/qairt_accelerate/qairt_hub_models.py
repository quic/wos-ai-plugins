# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import common_utils as utils
import os
import qairt_constants as consts
import shutil
import qai_hub

def convert_and_download_models():
    os.makedirs(consts.CONVERTION_DIR, exist_ok=True)
    if not os.path.isfile(consts.ESRGAN_X4_MODEL_PATH):
        try:
            model = qai_hub.get_model(consts.ESRGAN_X4_MODEL_ID)
            model.download(filename=consts.ESRGAN_X4_MODEL_PATH)
        except Exception:
            utils.run_command(f"{consts.QAI_HUB_VENV_PYTHON_PATH} -m qai_hub_models.models.esrgan.export " +
                        f"--device \"Snapdragon X Elite CRD\" --height 512 --width 512 --target-runtime qnn " +
                        f"--skip-profiling --skip-inferencing --skip-summary --output-dir {consts.CONVERTION_DIR}")
            shutil.move(
                os.path.join(consts.CONVERTION_DIR,"esrgan.so"),
                consts.ESRGAN_X4_MODEL_PATH,
            )

convert_and_download_models()
