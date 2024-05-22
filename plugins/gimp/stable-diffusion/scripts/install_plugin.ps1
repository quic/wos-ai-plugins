# This script assumes to be in a directory where sd-snapdragon plugin dir is available.

function Download-Stable-Diffusion-Model-Data {
    echo "Downloading Stable Diffusion 1.5 Model data..."
    If (-not (Test-Path "$data_path/betas.bin" -PathType Leaf)) {
        Invoke-WebRequest "$model_data_base_url/betas.bin" -OutFile "$data_path/betas.bin"
    }
    If (-not (Test-Path "$data_path/lambdas.bin" -PathType Leaf)) {
        Invoke-WebRequest "$model_data_base_url/lambdas.bin" -OutFile "$data_path/lambdas.bin"
    }
    If (-not (Test-Path "$data_path/openai-clip-vit-base-patch32" -PathType Leaf)) {
        Invoke-WebRequest "$model_data_base_url/openai-clip-vit-base-patch32" `
            -OutFile "$data_path/openai-clip-vit-base-patch32"
    }
    If (-not (Test-Path "$data_path/sd_precomute_data.tar" -PathType Leaf)) {
        Invoke-WebRequest "$model_data_base_url/sd_precomute_data.tar" `
            -OutFile "$data_path/sd_precomute_data.tar"
    }
}

function Download-Stable-Diffusion-Models {
    echo "Downloading Stable Diffusion 1.5 Models..."
    If (-not (Test-Path "$data_path/text_encoder.serialized.bin" -PathType Leaf)) {
        Invoke-WebRequest "$models_base_url/text_encoder.serialized.bin" `
            -OutFile "$data_path/text_encoder.serialized.bin"
    }
    If (-not (Test-Path "$data_path/unet.serialized.bin" -PathType Leaf)) {
        Invoke-WebRequest "$models_base_url/unet.serialized.bin" `
            -OutFile "$data_path/unet.serialized.bin"
    }
    If (-not (Test-Path "$data_path/vae_decoder.serialized.bin" -PathType Leaf)) {
        Invoke-WebRequest "$models_base_url/vae_decoder.serialized.bin" `
            -OutFile "$data_path/vae_decoder.serialized.bin"
    }
}

function Download-QNN-SDK-Libraries {
    echo "Downloading QNN SDK..."
    If (-not (Test-Path "$current_dir/qairt/2.22.0.240425")) {
        Invoke-WebRequest "$qnn_sdk_url" -OutFile "$current_dir/qnn_sdk.zip"
        Expand-Archive "$current_dir/qnn_sdk.zip" -DestinationPath "$current_dir/"
        Remove-Item "$current_dir/qnn_sdk.zip"
    }
    $qnn_sdk_root = "$current_dir/qairt/2.22.0.240425"

    cp "$qnn_sdk_root/lib/arm64x-windows-msvc/QnnHtp.dll" "$current_dir/$plugin_name/"
    cp "$qnn_sdk_root/lib/arm64x-windows-msvc/QnnHtpPrepare.dll" "$current_dir/$plugin_name/"
    cp "$qnn_sdk_root/lib/arm64x-windows-msvc/QnnHtpV73Stub.dll" "$current_dir/$plugin_name/"
    cp "$qnn_sdk_root/lib/hexagon-v73/unsigned/libQnnHtpV73Skel.so" "$current_dir/$plugin_name/"
    cp "$qnn_sdk_root/lib/arm64x-windows-msvc/QnnHtpNetRunExtensions.dll" "$current_dir/$plugin_name/"
    cp "$qnn_sdk_root/lib/arm64x-windows-msvc/QnnSystem.dll" "$current_dir/$plugin_name/"
}

function Copy-Plugin-To-Gimp {
    Copy-Item -Path "$current_dir\$plugin_name" -Destination "$gimp_plugin_path" -Recurse -Force
}

try {
    $plugin_name = "sd-snapdragon"
    $ai_hub_base_url = "https://qaihub-public-assets.s3.us-west-2.amazonaws.com"
    $qnn_sdk_url = "https://softwarecenter.qualcomm.com/api/download/software/qualcomm_neural_processing_sdk/v2.22.0.240425.zip"
    $model_data_base_url = "$ai_hub_base_url/qai-hub-models/models/riffusion_quantized/v1"
    $models_base_url = "$ai_hub_base_url/qai-hub-models/models/stable_diffusion_v1_5_quantized/v1/QNN220"
    $gimp_plugin_path = "C:\Users\$Env:UserName\AppData\Roaming\GIMP\2.10\plug-ins"
    $ErrorActionPreference = "Stop"
    $initial_dir = (Get-Item .).FullName
    $current_dir = $PSScriptRoot
    $data_path = "$current_dir/$plugin_name/StableDiffusionData"
    New-Item "$data_path" -ItemType directory -ea 0
    New-Item "$gimp_plugin_path" -ItemType directory -ea 0

    If (-not (Test-Path "$current_dir/$plugin_name")) {
        throw "Didn't find '$plugin_name' under $current_dir"
    }

    Download-QNN-SDK-Libraries
    Download-Stable-Diffusion-Model-Data
    Download-Stable-Diffusion-Models
    Copy-Plugin-To-Gimp
    echo "Plugin installation done!"
}
finally {
    Set-Location -Path "$initial_dir"
}
