@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PS_SCRIPT=%SCRIPT_DIR%scripts\commit_push_all.ps1"

if not exist "%PS_SCRIPT%" (
    echo [ERROR] Script not found: "%PS_SCRIPT%"
    exit /b 1
)

where powershell >nul 2>nul
if errorlevel 1 (
    echo [ERROR] powershell not found in PATH.
    exit /b 1
)

if "%~1"=="" (
    echo Usage: commit_push_all.bat -m "提交说明"
    echo Example: commit_push_all.bat -m "feat: 游戏中退出二级弹窗"
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
    echo.
    echo Commit/push failed with exit code %EXIT_CODE%.
) else (
    echo.
    echo Commit/push completed successfully.
)

exit /b %EXIT_CODE%
