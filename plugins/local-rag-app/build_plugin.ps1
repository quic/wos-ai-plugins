function Build-RAG {
    Set-Location -Path "$current_dir"
    rustup target add aarch64-pc-windows-msvc
    cargo build --release --target aarch64-pc-windows-msvc
}

try {
    $initial_dir = (Get-Item .).FullName
    $current_dir = $PSScriptRoot

    Build-RAG
}
finally {
    Set-Location -Path "$initial_dir"
}
