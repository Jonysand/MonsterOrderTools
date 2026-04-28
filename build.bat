@echo off
cd /d D:\VisualStudioProjects\JonysandMHDanmuTools
"D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" JonysandMHDanmuTools.sln -p:Configuration=Debug -p:Platform=x64 -t:Build -m
