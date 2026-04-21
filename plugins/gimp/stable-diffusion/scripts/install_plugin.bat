REM =============================================================================
REM
REM Copyright (c) 2026, Qualcomm Innovation Center, Inc. All rights reserved.
REM
REM SPDX-License-Identifier: BSD-3-Clause
REM
REM =============================================================================

@echo off
setlocal ENABLEDELAYEDEXPANSION

:: Define constants
set QAIRT_VERSION=2.44.0.260225
set QAIRT_DIR=C:\Qualcomm\AIStack\QAIRT
set QNN_SDK_ROOT=%QAIRT_DIR%\%QAIRT_VERSION%
set SDK_SAVE_PATH=QNN_sdk.zip
set PLUGIN_DIR=%HOMEPATH%/AppData/Roaming/GIMP/2.99/plug-ins/sd-snapdragon
set MODEL_DATA_DIR=%PLUGIN_DIR%\StableDiffusionData
set QNN_SDK_DOWNLOAD_URL="https://softwarecenter.qualcomm.com/api/download/software/sdks/Qualcomm_AI_Runtime_Community/All/%QAIRT_VERSION%/v%QAIRT_VERSION%.zip"
set model_data_base_url="https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-models/models/riffusion_quantized/v1"
set hugging_face_base_url="https://huggingface.co/qualcomm/Stable-Diffusion-v1.5/resolve"
set STABLE_DIFFUSION_MODEL_REVISION=9e5bf02813f129951adee071c5a1723c9b239069

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

:: Copy necessary libraries
for %%L in (QnnHtpNetRunExtensions.dll QnnHtp.dll QnnSystem.dll QnnHtpPrepare.dll) do (
    copy /b/v/y "%SDK_lib_dir%\%%L" "%PLUGIN_DIR%"
)

for %%A in (73 81) do (
    set "SDK_hexagon_dir=%QNN_SDK_ROOT%\lib\hexagon-v%%A\unsigned"

    copy /b/v/y "%SDK_lib_dir%\QnnHtpV%%AStub.dll" "%PLUGIN_DIR%"
    copy /b/v/y "!SDK_hexagon_dir!\libQnnHtpV%%ASkel.so" "%PLUGIN_DIR%"
    copy /b/v/y "!SDK_hexagon_dir!\libqnnhtpv%%A.cat" "%PLUGIN_DIR%"
)

:: Python ENV check
@echo off
FOR /F "tokens=2 delims= " %%i IN ('python --version 2^>nul') DO (
    SET "PY_VER=%%i"
)

IF "!PY_VER!"=="3.10.6" (
    echo Python 3.10.6 is already installed.
) ELSE (
    echo Python 3.10.6 is not installed. Installing now...
    powershell -Command "Invoke-WebRequest -Uri https://www.python.org/ftp/python/3.10.6/python-3.10.6-amd64.exe -OutFile python_installer.exe"
    start /wait python_installer.exe /quiet PrependPath=1
    timeout /t 5 >nul
    del /f /q python_installer.exe
    echo Python 3.10.6 installation completed.
)

SET "PYTHON_PATH=%LocalAppData%\Programs\Python\Python310\python.exe"
SET "PATH=%LocalAppData%\Programs\Python\Python310\;%PATH%"

@echo off
SET "EXPECTED_PATH=%LocalAppData%\Programs\Python\Python310\python.exe"

FOR /F "usebackq tokens=*" %%i IN (`where python`) DO (
    SET "PYTHON_PATH=%%i"
    GOTO :found
)

:found
echo Found Python at: %PYTHON_PATH%
IF /I NOT "%PYTHON_PATH%"=="%EXPECTED_PATH%" (
    echo Error: Python path does not match expected installation.
    echo Expected: %EXPECTED_PATH%
    echo Found:    %PYTHON_PATH%
    echo Exiting script...
    exit /b 1
)

echo Python path verified.

@echo off
python -c "import requests" 2>NUL || (
    echo Installing requests module...
    "%PYTHON_PATH%" -m pip install requests
    "%PYTHON_PATH%" -m pip install numpy
)

:: Download required models
echo Downloading QAIRT model bin files...
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%hugging_face_base_url%/%STABLE_DIFFUSION_MODEL_REVISION%/TextEncoderQuantizable.bin', '%MODEL_DATA_DIR%\text_encoder.serialized.bin')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%hugging_face_base_url%/%STABLE_DIFFUSION_MODEL_REVISION%/UnetQuantizable.bin', '%MODEL_DATA_DIR%\unet.serialized.bin')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%hugging_face_base_url%/%STABLE_DIFFUSION_MODEL_REVISION%/VaeDecoderQuantizable.bin', '%MODEL_DATA_DIR%\vae_decoder.serialized.bin')"

echo Downloading Stable Diffusion 1.5 Model data...
:: Download files
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%model_data_base_url%/betas.bin', '%MODEL_DATA_DIR%\betas.bin')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%model_data_base_url%/lambdas.bin', '%MODEL_DATA_DIR%\lambdas.bin')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%model_data_base_url%/openai-clip-vit-base-patch32', '%MODEL_DATA_DIR%\openai-clip-vit-base-patch32')"
powershell -Command "(New-Object System.Net.WebClient).DownloadFile('%model_data_base_url%/sd_precomute_data.tar', '%MODEL_DATA_DIR%\sd_precomute_data.tar')"

echo Download complete.

endlocal
