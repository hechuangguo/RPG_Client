@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PS_SCRIPT=%SCRIPT_DIR%scripts\sync_all.ps1"

if not exist "%PS_SCRIPT%" (
    echo [ERROR] Script not found: "%PS_SCRIPT%"
    exit /b 1
)

where powershell >nul 2>nul
if errorlevel 1 (
    echo [ERROR] powershell not found in PATH.
    exit /b 1
)

echo Running sync script (allows local uncommitted changes)...
powershell -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" -AllowDirty %*
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
    echo.
    echo Sync failed with exit code %EXIT_CODE%.
) else (
    echo.
    echo Sync completed successfully.
)

exit /b %EXIT_CODE%
