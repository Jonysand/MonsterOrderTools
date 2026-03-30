@echo off
chcp 65001 >nul
cd /d "%~dp0x64\Debug"
start "MonsterOrderWilds Tests" cmd /c "MonsterOrderWilds.exe && pause"
