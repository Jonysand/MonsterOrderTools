@echo off
chcp 65001 >nul 2>&1
echo ========================================
echo   MonsterOrderWilds Installer Builder
echo ========================================
echo.
echo [Info] Launching Python build script...
echo.
python "%~dp0build.py"
if errorlevel 1 (
    echo.
    echo [Error] Build failed or Python not found.
    echo.
    pause
    exit /b 1
)