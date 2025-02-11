param (
    [string]$qnn_sdk_root = $env:QNN_SDK_ROOT
)
if (-not $qnn_sdk_root) {
    throw "Error: The '-qnn_sdk_root' parameter is required."
}

function Build-UI {
    npm install
    npm run vsce:package
}

$ErrorActionPreference = "Stop"
.\server\build.ps1 -qnn_sdk_root "$qnn_sdk_root"
Build-UI
