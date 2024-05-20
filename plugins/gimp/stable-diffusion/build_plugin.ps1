#Requires -RunAsAdministrator

function Prepare-Plugin-Release {
    New-Item "$current_dir/build/plugin-release/sd-snapdragon" -ItemType directory -ea 0
    cp "$current_dir/build/Release/*" "$current_dir/build/plugin-release/sd-snapdragon/"
    cp "$current_dir/ui/*.py" "$current_dir/build/plugin-release/sd-snapdragon/"
    cp "$current_dir/config/*" "$current_dir/build/plugin-release/sd-snapdragon/"
    cp "$current_dir/resources/*" "$current_dir/build/plugin-release/sd-snapdragon/"
    cp "$current_dir/src/tokenizer/target/aarch64-pc-windows-msvc/release/tokenizer.dll" `
        "$current_dir/build/plugin-release/sd-snapdragon/"
    cp "$current_dir/scripts/install_plugin.ps1" "$current_dir/build/plugin-release/"
}

function Build-Tokenizer {
    Set-Location -Path "$current_dir/src/tokenizer"
    rustup target add aarch64-pc-windows-msvc
    cargo build --release --target aarch64-pc-windows-msvc
}

function Download-QNN-SDK {
    If (-not (Test-Path "$current_dir/build/2.20.0.240223")) {
        Invoke-WebRequest "$qnn_sdk_url" -OutFile "$current_dir/build/qnn_sdk.zip"
        Expand-Archive "$current_dir/build/qnn_sdk.zip" -DestinationPath "$current_dir/build/"
    }
    $qnn_sdk_root = "$current_dir/build/2.20.0.240223"
    [System.Environment]::SetEnvironmentVariable("QNN_SDK_ROOT", $qnn_sdk_root, "Machine")
    write-output "QNN SDK root : " $qnn_sdk_root
}

function Run-vcpkg {
    write-output "vcpkg root: " $vcpkg_root
    If (-not (Test-Path $vcpkg_root)){
        cd "$current_dir/build"
        git clone "$vcpkg_url"
        .\vcpkg\bootstrap-vcpkg.bat
        .\vcpkg\vcpkg integrate install
    }
}

function Build-Stable-Diffusion {
    cd "$current_dir/build"
    cmake -G "Visual Studio 17 2022" -A ARM64 `
        -DCMAKE_TOOLCHAIN_FILE="$vcpkg_root/scripts/buildsystems/vcpkg.cmake" ..
    cmake --build . --config Release
}

try {
    $vcpkg_url = "https://github.com/microsoft/vcpkg"
    $ai_hub_base_url = "https://qaihub-public-assets.s3.us-west-2.amazonaws.com/qai-hub-apps"
    $qnn_sdk_url = "$ai_hub_base_url/qnn/linux/2.20.0.240223.zip"
    $ErrorActionPreference = "Stop"
    $initial_dir = (Get-Item .).FullName
    $current_dir = $PSScriptRoot
    $vcpkg_root = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath("$current_dir\build\vcpkg")

    New-Item "$current_dir/build" -ItemType directory -ea 0
    cp "$current_dir/vcpkg.json" "$current_dir/build/"
    cp "$current_dir/vcpkg-configuration.json" "$current_dir/build/"

    Download-QNN-SDK
    Build-Tokenizer
    Run-vcpkg
    Build-Stable-Diffusion
    Prepare-Plugin-Release
}
finally {
    Set-Location -Path "$initial_dir"
}
