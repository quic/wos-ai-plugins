@echo off
setlocal

:: Define constants
set QAIRT_VERSION=2.24.0.240626
set QAIRT_DIR=C:\Qualcomm\AIStack\QAIRT
set QNN_SDK_ROOT=%QAIRT_DIR%\%QAIRT_VERSION%
set SDK_SAVE_PATH=QNN_sdk.zip
set PLUGIN_DIR=%HOMEPATH%/AppData/Roaming/GIMP/2.99/plug-ins/sd-snapdragon
set MODEL_DATA_DIR=%PLUGIN_DIR%\StableDiffusionData
set QNN_SDK_DOWNLOAD_URL="https://softwarecenter.qualcomm.com/api/download/software/qualcomm_neural_processing_sdk/v%QAIRT_VERSION%.zip"
set DSP_ARCH=73
set model_data_base_url="https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-models/models/riffusion_quantized/v1"
set hugging_face_base_url="https://huggingface.co/qualcomm/Stable-Diffusion-v1.5/resolve"
set STABLE_DIFFUSION_MODEL_REVISION=026f708a17b5e3edb2a6a5cb5435d717db840f21

:: Main script execution
mkdir "%PLUGIN_DIR%"
xcopy ".\sd-snapdragon" "%PLUGIN_DIR%" /E /H /C /I /Y
mkdir "%MODEL_DATA_DIR%"

:: download and setup QAIRT SDK
if not exist "%QNN_SDK_ROOT%" (
    mkdir "%QAIRT_DIR%"
    echo Downloading QAIRT SDK...
    :: call :download_file "%SDK_SAVE_PATH%" "%QNN_SDK_DOWNLOAD_URL%"
    powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%QNN_SDK_DOWNLOAD_URL%', '%SDK_SAVE_PATH%')"
    echo QAIRT SDK downloaded.
    powershell -Command "Expand-Archive -Path '%SDK_SAVE_PATH%' -DestinationPath './'"
    move ".\qairt\%QAIRT_VERSION%" "%QAIRT_DIR%\"
    rmdir /s /q ".\qairt"
    del "%SDK_SAVE_PATH%"
)

:: setup QAIRT environment
set SDK_lib_dir=%QNN_SDK_ROOT%\lib\aarch64-windows-msvc
set SDK_hexagon_dir=%QNN_SDK_ROOT%\lib\hexagon-v%DSP_ARCH%\unsigned

:: Copy necessary libraries
for %%L in (QnnHtpNetRunExtensions.dll QnnHtp.dll QnnSystem.dll QnnHtpPrepare.dll QnnHtpV%DSP_ARCH%Stub.dll) do (
    copy /b/v/y "%SDK_lib_dir%\%%L" "%PLUGIN_DIR%"
)

for %%L in (libQnnHtpV%DSP_ARCH%Skel.so libqnnhtpv%DSP_ARCH%.cat) do (
    copy /b/v/y "%SDK_hexagon_dir%\%%L" "%PLUGIN_DIR%"
)

:: Download required models
echo Downloading QAIRT model bin files...
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%hugging_face_base_url%/%STABLE_DIFFUSION_MODEL_REVISION%/TextEncoder_Quantized.bin', '%MODEL_DATA_DIR%\text_encoder.serialized.bin')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%hugging_face_base_url%/%STABLE_DIFFUSION_MODEL_REVISION%/UNet_Quantized.bin', '%MODEL_DATA_DIR%\unet.serialized.bin')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%hugging_face_base_url%/%STABLE_DIFFUSION_MODEL_REVISION%/VAEDecoder_Quantized.bin', '%MODEL_DATA_DIR%\vae_decoder.serialized.bin')"

echo Downloading Stable Diffusion 1.5 Model data...
:: Download files
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%model_data_base_url%/betas.bin', '%MODEL_DATA_DIR%\betas.bin')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%model_data_base_url%/lambdas.bin', '%MODEL_DATA_DIR%\lambdas.bin')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%model_data_base_url%/openai-clip-vit-base-patch32', '%MODEL_DATA_DIR%\openai-clip-vit-base-patch32')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%model_data_base_url%/sd_precomute_data.tar', '%MODEL_DATA_DIR%\sd_precomute_data.tar')"

echo Download complete.

endlocal
