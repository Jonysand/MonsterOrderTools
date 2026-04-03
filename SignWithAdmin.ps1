# 需要管理员权限的签名脚本
param(
    [string]$ExePath = "x64\Release\MonsterOrderWilds.exe",
    [string]$DllPath = "x64\Release\MonsterOrderWildsGUI.dll"
)

# 检查管理员权限
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "此脚本需要管理员权限，正在重新运行..." -ForegroundColor Yellow
    Start-Process PowerShell -Verb RunAs "-ExecutionPolicy Bypass -File `"$PSCommandPath`" -ExePath `"$ExePath`""
    exit
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  程序签名工具（管理员模式）" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 证书名称
$CertSubject = "CN=小鬼科技"

# 检查证书是否存在
$Cert = Get-ChildItem -Path Cert:\LocalMachine\My | Where-Object { $_.Subject -eq $CertSubject }

if (-NOT $Cert) {
    Write-Host "[信息] 未找到证书，正在创建..." -ForegroundColor Yellow
    
    # 创建自签名证书
    $Cert = New-SelfSignedCertificate `
        -Subject $CertSubject `
        -Type CodeSigningCert `
        -KeyUsage DigitalSignature `
        -KeyLength 2048 `
        -KeyAlgorithm RSA `
        -HashAlgorithm SHA256 `
        -NotAfter (Get-Date).AddYears(5) `
        -CertStoreLocation Cert:\LocalMachine\My
    
    if ($Cert) {
        Write-Host "[成功] 证书已创建: $($Cert.Thumbprint)" -ForegroundColor Green
        
        # 添加到受信任的根证书颁发机构
        Write-Host "[信息] 添加到受信任的根证书颁发机构..." -ForegroundColor Yellow
        $Store = New-Object System.Security.Cryptography.X509Certificates.X509Store("Root", "LocalMachine")
        $Store.Open("ReadWrite")
        $Store.Add($Cert)
        $Store.Close()
        Write-Host "[成功] 已添加到受信任的根证书颁发机构" -ForegroundColor Green
    } else {
        Write-Host "[错误] 证书创建失败" -ForegroundColor Red
        pause
        exit 1
    }
} else {
    Write-Host "[信息] 使用现有证书: $($Cert.Thumbprint)" -ForegroundColor Green
}

# 导出证书
$CertPath = "D:\VisualStudioProjects\JonysandMHDanmuTools\MonsterOrderWilds.cer"
Export-Certificate -Cert $Cert -FilePath $CertPath -Force | Out-Null
Write-Host "[信息] 证书已导出到: $CertPath" -ForegroundColor Cyan

# 检查文件
$FullExePath = "D:\VisualStudioProjects\JonysandMHDanmuTools\$ExePath"
if (-NOT (Test-Path $FullExePath)) {
    Write-Host "[错误] 找不到文件: $FullExePath" -ForegroundColor Red
    pause
    exit 1
}

# 签名 EXE
Write-Host ""
Write-Host "[信息] 正在签名 EXE..." -ForegroundColor Yellow

$Signature = Set-AuthenticodeSignature -FilePath $FullExePath -Certificate $Cert -TimestampServer "http://timestamp.digicert.com"

if ($Signature.Status -eq "Valid") {
    Write-Host "[成功] EXE 签名成功" -ForegroundColor Green
} else {
    Write-Host "[错误] EXE 签名失败: $($Signature.StatusMessage)" -ForegroundColor Red
}

# 签名 DLL
Write-Host ""
Write-Host "[信息] 正在签名 DLL..." -ForegroundColor Yellow

$FullDllPath = "D:\VisualStudioProjects\JonysandMHDanmuTools\$DllPath"
if (Test-Path $FullDllPath) {
    $DllSignature = Set-AuthenticodeSignature -FilePath $FullDllPath -Certificate $Cert -TimestampServer "http://timestamp.digicert.com"
    
    if ($DllSignature.Status -eq "Valid") {
        Write-Host "[成功] DLL 签名成功" -ForegroundColor Green
    } else {
        Write-Host "[错误] DLL 签名失败: $($DllSignature.StatusMessage)" -ForegroundColor Red
    }
} else {
    Write-Host "[警告] 找不到 DLL 文件: $FullDllPath" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  签名完成" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "下一步:" -ForegroundColor Yellow
Write-Host "  1. 在目标机器上安装 MonsterOrderWilds.cer" -ForegroundColor White
Write-Host "  2. 安装到\"受信任的根证书颁发机构\"" -ForegroundColor White

Write-Host ""
pause