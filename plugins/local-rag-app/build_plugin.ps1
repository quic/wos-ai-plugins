param (
    [string]$qnn_sdk_root = $env:QNN_SDK_ROOT
)
if (-not $qnn_sdk_root) {
    throw "Error: The '-qnn_sdk_root' parameter is required."
}

function Prepare-Plugin-For-Build {
    # Copy QNN dependencies
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/Genie.lib" "$current_dir/"
    cp "$qnn_sdk_root/include/Genie/*.h" "$current_dir/Genie/"
}

function Prepare-Plugin-For-Release {
    # Copy QNN dependencies
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/Genie.dll" "$current_dir/target/aarch64-pc-windows-msvc/release/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnHtp.dll" "$current_dir/target/aarch64-pc-windows-msvc/release/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnHtpPrepare.dll" "$current_dir/target/aarch64-pc-windows-msvc/release/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnHtpV73Stub.dll" "$current_dir/target/aarch64-pc-windows-msvc/release/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnHtpNetRunExtensions.dll" "$current_dir/target/aarch64-pc-windows-msvc/release/"
    cp "$qnn_sdk_root/lib/aarch64-windows-msvc/QnnSystem.dll" "$current_dir/target/aarch64-pc-windows-msvc/release/"
    cp "$qnn_sdk_root/lib/hexagon-v73/unsigned/libQnnHtpV73Skel.so" "$current_dir/target/aarch64-pc-windows-msvc/release/"
    cp "$qnn_sdk_root/lib/hexagon-v73/unsigned/libqnnhtpv73.cat" "$current_dir/target/aarch64-pc-windows-msvc/release/"
}

function Build-RAG {
    Set-Location -Path "$current_dir"
    rustup target add aarch64-pc-windows-msvc
    cargo build --release --target aarch64-pc-windows-msvc
}

try {
    $initial_dir = (Get-Item .).FullName
    $current_dir = $PSScriptRoot

    Prepare-Plugin-For-Build
    Build-RAG
    Prepare-Plugin-For-Release
}
finally {
    Set-Location -Path "$initial_dir"
}
