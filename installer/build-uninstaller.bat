@echo off
chcp 65001 >nul
echo ==========================================
echo MonsterOrderWilds Uninstaller Builder
echo ==========================================
echo.

REM Set Inno Setup path - check multiple locations
set "INNO_SETUP="

if exist "D:\Inno Setup 6\ISCC.exe" (
    set "INNO_SETUP=D:\Inno Setup 6\ISCC.exe"
) else if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" (
    set "INNO_SETUP=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
) else if exist "C:\Program Files\Inno Setup 6\ISCC.exe" (
    set "INNO_SETUP=C:\Program Files\Inno Setup 6\ISCC.exe"
)

REM Check if Inno Setup exists
if "%INNO_SETUP%"=="" (
    echo Error: Inno Setup compiler not found
    echo Please install Inno Setup 6.x
    echo Download: https://jrsoftware.org/isdl.php
    pause
    exit /b 1
)

echo Building uninstaller...
"%INNO_SETUP%" "%~dp0MonsterOrderWilds-Uninstaller.iss"

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ==========================================
echo Uninstaller built successfully!
echo Output: output\MonsterOrderWilds-Uninstaller.exe
echo ==========================================
pause
