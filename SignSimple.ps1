# 简化版签名脚本（可能需要管理员权限）
param(
    [string]$ExePath = "x64\Release\MonsterOrderWilds.exe"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  程序签名工具" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 证书名称
$CertSubject = "CN=小鬼科技"

# 检查证书是否存在
$Cert = Get-ChildItem -Path Cert:\CurrentUser\My | Where-Object { $_.Subject -eq $CertSubject }

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
        -CertStoreLocation Cert:\CurrentUser\My
    
    if ($Cert) {
        Write-Host "[成功] 证书已创建: $($Cert.Thumbprint)" -ForegroundColor Green
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

# 签名
Write-Host ""
Write-Host "[信息] 正在签名..." -ForegroundColor Yellow

$Signature = Set-AuthenticodeSignature -FilePath $FullExePath -Certificate $Cert -IncludeChain "All" -TimestampServer "http://timestamp.digicert.com"

if ($Signature.Status -eq "Valid") {
    Write-Host ""
    Write-Host "[成功] 签名完成！" -ForegroundColor Green
    Write-Host "  状态: $($Signature.Status)" -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "[错误] 签名失败: $($Signature.StatusMessage)" -ForegroundColor Red
}

Write-Host ""
pause