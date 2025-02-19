param (
    [string]$qnn_sdk_root = $env:QNN_SDK_ROOT
)
if (-not $qnn_sdk_root) {
    throw "Error: The '-qnn_sdk_root' parameter is required."
}

function Prepare-Plugin-For-Build {
    # Copy QNN dependencies
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/Genie.dll" "$current_dir/lib/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/Genie.lib" "$current_dir/lib/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnHtp.dll" "$current_dir/lib/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnHtpPrepare.dll" "$current_dir/lib/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnHtpV73Stub.dll" "$current_dir/lib/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnHtpNetRunExtensions.dll" "$current_dir/lib/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnSystem.dll" "$current_dir/lib/"
    cp "$qnn_sdk_root/lib/hexagon-v73/unsigned/libQnnHtpV73Skel.so" "$current_dir/lib/"
    cp "$qnn_sdk_root/lib/hexagon-v73/unsigned/libqnnhtpv73.cat" "$current_dir/lib/"
}

function Build {
    cd "$current_dir/build_c"
    cmake -G "Visual Studio 17 2022" -A ARM64 ` ..
    cmake --build . --config Release
}

try {
    $ErrorActionPreference = "Stop"
    $initial_dir = (Get-Item .).FullName
    $current_dir = $PSScriptRoot

    New-Item "$current_dir/build_c" -ItemType directory -ea 0
    New-Item "$current_dir/lib" -ItemType directory -ea 0

    Prepare-Plugin-For-Build
    Build
}
finally {
    Set-Location -Path "$initial_dir"
}
