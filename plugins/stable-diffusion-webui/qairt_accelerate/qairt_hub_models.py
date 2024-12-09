# =============================================================================
#
# Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# =============================================================================

import os
import sys
import shutil
import qai_hub
import common_utils as utils
import qairt_constants as consts

def convert_and_download_models():
    os.makedirs(consts.CONVERTION_DIR, exist_ok=True)
    if not os.path.isfile(consts.TIMESTEP_EMBEDDING_1_5_MODEL_PATH):
        model = qai_hub.get_model(consts.TIMESTEP_EMBEDDING_1_5_MODEL_ID)
        model.download(filename=consts.TIMESTEP_EMBEDDING_1_5_MODEL_PATH)
    if not os.path.isfile(consts.TIMESTEP_EMBEDDING_2_1_MODEL_PATH):
        model = qai_hub.get_model(consts.TIMESTEP_EMBEDDING_2_1_MODEL_ID)
        model.download(filename=consts.TIMESTEP_EMBEDDING_2_1_MODEL_PATH)
    if (not os.path.isfile(consts.ESRGAN_X4_MODEL_PATH)
        or not (os.path.isfile(consts.ESRGAN_X4_MODEL_CHECKSUM)
        and open(consts.ESRGAN_X4_MODEL_CHECKSUM).read() == consts.ESRGAN_X4_MODEL_ID)):
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
        open(consts.ESRGAN_X4_MODEL_CHECKSUM, 'w').write(consts.ESRGAN_X4_MODEL_ID)
        

def sd2_1_unclip_download():
    os.makedirs(consts.SD2_1_UNCLIP_PATH, exist_ok=True)
    for model_id, model_name in consts.SD2_1_UNCLIP.items():
        model_path = os.path.join(consts.SD2_1_UNCLIP_PATH, model_name)
        if not os.path.isfile(model_path):
            try:
                model = qai_hub.get_model(model_id)
                model.download(filename=model_path)
            except Exception:
                print(Exception)



if __name__ == "__main__":
    dwnld_model_type = sys.argv[1]
    if dwnld_model_type.lower() == 'timeembedding':
        convert_and_download_models()
    elif dwnld_model_type.lower() == 'sd_2_1_unclip':
        sd2_1_unclip_download()
    else:
        print("argument name missing or incorrect.")


