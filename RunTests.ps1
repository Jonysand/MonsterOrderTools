$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "D:\VisualStudioProjects\JonysandMHDanmuTools\x64\Debug\MonsterOrderWilds.exe"
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.CreateNoWindow = $false

$proc = [System.Diagnostics.Process]::Start($psi)
Start-Sleep -Seconds 3
$proc.StandardInput.WriteLine()
$output = $proc.StandardOutput.ReadToEnd()
$proc.WaitForExit()

Write-Host $output
Write-Host "Exit code:" $proc.ExitCode
