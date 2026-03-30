$proc = Start-Process -FilePath "D:\VisualStudioProjects\JonysandMHDanmuTools\x64\Debug\MonsterOrderWilds.exe" -PassThru -NoNewWindow -Wait
Write-Host "Exit code:" $proc.ExitCode
