
@echo off
for /f "tokens=2 delims= " %%i in ('python --version') do set version=%%i
set min_version=3.10.6
set max_version=3.10.14

echo Python version: %version%
set valid_version=False

REM Compare version with min_version and max_version
for /f "tokens=1-3 delims=." %%a in ("%version%") do (
    if %%a equ 3 if %%b equ 10 if %%c geq 6 if %%c leq 14 (
        set valid_version=True
    )
)
if "%valid_version%"=="False" (
    echo Python version %version% is not within the required range %min_version%, %max_version%
    exit
)

echo Python version is between %min_version% and %max_version%
python -m venv "%~dp0\venv"
"%~dp0\venv\Scripts\pip.exe" install -r "%~dp0\requirements.txt"
"%~dp0\venv\Scripts\python.exe" "%~dp0\install.py"
