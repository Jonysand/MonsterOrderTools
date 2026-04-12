# -*- coding: utf-8 -*-
"""
MonsterOrderWilds Installer Builder
发布构建脚本 - 自动完成版本读取、签名、打包
"""

import os
import re
import sys
import shutil
import subprocess
from pathlib import Path

# 项目根目录
PROJECT_DIR = Path(__file__).parent.parent
INSTALLER_DIR = Path(__file__).parent
FILES_DIR = INSTALLER_DIR / "files"
OUTPUT_DIR = INSTALLER_DIR / "output"
CONFIGS_DIR = PROJECT_DIR / "MonsterOrderWilds_configs"
FRAMEWORK_H = PROJECT_DIR / "MonsterOrderWilds" / "framework.h"
VERSION_INFO_RC = PROJECT_DIR / "MonsterOrderWilds" / "VersionInfo.rc"
ISS_FILE = INSTALLER_DIR / "MonsterOrderWilds.iss"
RELEASE_EXE = PROJECT_DIR / "x64" / "Release" / "MonsterOrderWilds.exe"
RELEASE_DLL = PROJECT_DIR / "x64" / "Release" / "MonsterOrderWildsGUI.dll"
CERT_FILE = PROJECT_DIR / "MonsterOrderWilds.cer"

# Inno Setup 路径
ISCC_PATHS = [
    Path(r"D:\Inno Setup 6\ISCC.exe"),
    Path(r"C:\Program Files (x86)\Inno Setup 6\ISCC.exe"),
    Path(r"C:\Program Files\Inno Setup 6\ISCC.exe"),
]


def print_step(step, total, message):
    print(f"\n[Step {step}/{total}] {message}")


def read_app_version():
    """从framework.h读取APP_VERSION"""
    print_step(1, 6, "Reading version from framework.h...")

    if not FRAMEWORK_H.exists():
        print(f"[Error] {FRAMEWORK_H} not found!")
        sys.exit(1)

    content = FRAMEWORK_H.read_text(encoding="utf-8")
    match = re.search(r"#define\s+APP_VERSION\s+(\d+)", content)

    if match:
        version = match.group(1)
        print(f"[OK] APP_VERSION = {version}")
        return version
    else:
        print("[Warning] Could not find APP_VERSION, using 1")
        return "1"


def find_iscc():
    """查找Inno Setup编译器"""
    print_step(2, 6, "Checking Inno Setup...")

    for path in ISCC_PATHS:
        if path.exists():
            print(f"[OK] Found: {path}")
            return path

    print("[Error] Inno Setup 6 not found!")
    print("Please install from: https://jrsoftware.org/isdl.php")
    sys.exit(1)


def check_source_files():
    """检查源文件"""
    print_step(3, 6, "Checking source files...")

    errors = []
    if not RELEASE_EXE.exists():
        errors.append(f"MonsterOrderWilds.exe not found in x64\\Release")
    if not RELEASE_DLL.exists():
        errors.append(f"MonsterOrderWildsGUI.dll not found in x64\\Release")

    if errors:
        for err in errors:
            print(f"[Error] {err}")
        print("Please build Release configuration first")
        sys.exit(1)

    print("[OK] Source files found")


def prepare_files():
    """准备打包文件"""
    print_step(4, 6, "Preparing files...")

    # 清理并创建目录
    if FILES_DIR.exists():
        shutil.rmtree(FILES_DIR)
    FILES_DIR.mkdir(parents=True)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # 复制文件
    shutil.copy2(RELEASE_EXE, FILES_DIR / "MonsterOrderWilds.exe")
    print("[OK] Copied MonsterOrderWilds.exe")

    shutil.copy2(RELEASE_DLL, FILES_DIR / "MonsterOrderWildsGUI.dll")
    print("[OK] Copied MonsterOrderWildsGUI.dll")

    # 复制配置文件
    for config_file in ["credentials.dat", "monster_list.json"]:
        config_src = CONFIGS_DIR / config_file
        if config_src.exists():
            shutil.copy2(config_src, FILES_DIR / config_file)
            print(f"[OK] Copied {config_file}")
        else:
            print(f"[Warning] {config_src} not found!")

    # 复制证书
    if CERT_FILE.exists():
        print("[OK] Certificate found, will be copied during signing")
    else:
        print("[Info] Certificate will be created during signing")


def ensure_certificate():
    """确保证书存在，返回证书主题"""
    cert_subject = "CN=小鬼科技"

    # 检查证书是否已存在
    check_cmd = [
        "powershell",
        "-NoProfile",
        "-Command",
        f"Get-ChildItem Cert:\\CurrentUser\\My | Where-Object {{ $_.Subject -eq '{cert_subject}' }} | Select-Object -First 1 -ExpandProperty Thumbprint",
    ]
    result = subprocess.run(check_cmd, capture_output=True, text=True)
    thumbprint = result.stdout.strip()

    if thumbprint:
        print(f"[OK] Using existing certificate: {thumbprint}")
        return cert_subject

    # 创建新证书
    print("[Info] Creating new certificate...")
    create_cmd = [
        "powershell",
        "-NoProfile",
        "-Command",
        f"New-SelfSignedCertificate -Subject '{cert_subject}' -Type CodeSigningCert "
        f"-KeyUsage DigitalSignature -KeyLength 2048 -KeyAlgorithm RSA "
        f"-HashAlgorithm SHA256 -NotAfter (Get-Date).AddYears(5) "
        f"-CertStoreLocation Cert:\\CurrentUser\\My | Out-Null",
    ]
    subprocess.run(create_cmd, capture_output=True, text=True)

    # 导出证书
    export_cmd = [
        "powershell",
        "-NoProfile",
        "-Command",
        f"$cert = Get-ChildItem Cert:\\CurrentUser\\My | Where-Object {{ $_.Subject -eq '{cert_subject}' }} | Select-Object -First 1; "
        f"Export-Certificate -Cert $cert -FilePath '{CERT_FILE}' -Force | Out-Null",
    ]
    subprocess.run(export_cmd, capture_output=True, text=True)
    print("[OK] Certificate created and exported")

    return cert_subject


def sign_files():
    """签名程序文件"""
    print_step(5, 6, "Signing program files...")

    # 确保证书存在
    cert_subject = ensure_certificate()

    # 签名 EXE
    print("[Info] Signing MonsterOrderWilds.exe...")
    sign_exe_cmd = [
        "powershell",
        "-NoProfile",
        "-Command",
        f"$cert = Get-ChildItem Cert:\\CurrentUser\\My | Where-Object {{ $_.Subject -eq '{cert_subject}' }} | Select-Object -First 1; "
        f"Set-AuthenticodeSignature -FilePath '{FILES_DIR}\\MonsterOrderWilds.exe' "
        f"-Certificate $cert -IncludeChain 'All' -TimestampServer 'http://timestamp.digicert.com' | Out-Null",
    ]
    result = subprocess.run(sign_exe_cmd, capture_output=True, text=True)
    if result.returncode == 0:
        print("[OK] MonsterOrderWilds.exe signed")
    else:
        print(f"[Warning] Signing exe: {result.stderr}")

    # 签名 DLL
    print("[Info] Signing MonsterOrderWildsGUI.dll...")
    sign_dll_cmd = [
        "powershell",
        "-NoProfile",
        "-Command",
        f"$cert = Get-ChildItem Cert:\\CurrentUser\\My | Where-Object {{ $_.Subject -eq '{cert_subject}' }} | Select-Object -First 1; "
        f"Set-AuthenticodeSignature -FilePath '{FILES_DIR}\\MonsterOrderWildsGUI.dll' "
        f"-Certificate $cert -IncludeChain 'All' -TimestampServer 'http://timestamp.digicert.com' | Out-Null",
    ]
    result = subprocess.run(sign_dll_cmd, capture_output=True, text=True)
    if result.returncode == 0:
        print("[OK] MonsterOrderWildsGUI.dll signed")
    else:
        print(f"[Warning] Signing dll: {result.stderr}")

    # 复制证书到 files 目录
    if CERT_FILE.exists():
        shutil.copy2(CERT_FILE, FILES_DIR / "MonsterOrderWilds.cer")
        print("[OK] Certificate copied to files directory")


def update_version_info(version):
    """更新VersionInfo.rc中的版本号"""
    if not VERSION_INFO_RC.exists():
        print(f"[Warning] {VERSION_INFO_RC} not found!")
        return

    content = VERSION_INFO_RC.read_text(encoding="utf-8")

    # FILEVERSION 和 PRODUCTVERSION 保持 "15,0,0,0" 格式（Windows资源文件要求）
    version_resource = f"{version},0,0,0"

    # 更新 FILEVERSION 和 PRODUCTVERSION
    new_content = re.sub(
        r"FILEVERSION \d+,\d+,\d+,\d+", f"FILEVERSION {version_resource}", content
    )
    new_content = re.sub(
        r"PRODUCTVERSION \d+,\d+,\d+,\d+",
        f"PRODUCTVERSION {version_resource}",
        new_content,
    )
    # 更新 FileVersion 和 ProductVersion 字符串为纯数字
    new_content = re.sub(
        r'VALUE "FileVersion", "\d+"', f'VALUE "FileVersion", "{version}"', new_content
    )
    new_content = re.sub(
        r'VALUE "ProductVersion", "\d+"',
        f'VALUE "ProductVersion", "{version}"',
        new_content,
    )

    if content != new_content:
        VERSION_INFO_RC.write_text(new_content, encoding="utf-8")
        print(f"[OK] Updated {VERSION_INFO_RC} with version {version}")
    else:
        print(f"[OK] Version already set to {version}")


def update_iss_version(version):
    """更新.iss文件中的版本号"""
    if not ISS_FILE.exists():
        print(f"[Error] {ISS_FILE} not found!")
        return

    content = ISS_FILE.read_text(encoding="utf-8")
    new_content = re.sub(
        r'#define MyAppVersion "v?\d+"', f'#define MyAppVersion "v{version}"', content
    )

    if content != new_content:
        ISS_FILE.write_text(new_content, encoding="utf-8")
        print(f"[OK] Updated {ISS_FILE} with version v{version}")
    else:
        print(f"[OK] Version already set to v{version}")


def build_installer(iscc_path, version):
    """构建安装包"""
    print_step(6, 6, f"Building installer (v{version})...")

    # 更新版本信息
    update_version_info(version)
    update_iss_version(version)

    # 调用Inno Setup
    result = subprocess.run(
        [str(iscc_path), str(ISS_FILE)], capture_output=True, text=True
    )

    print(result.stdout)
    if result.stderr:
        print(result.stderr)

    if result.returncode != 0:
        print("[Error] Build failed!")
        sys.exit(1)

    print(f"\n{'=' * 50}")
    print("  Build completed successfully!")
    print(f"{'=' * 50}")
    print(f"\nOutput: {OUTPUT_DIR / f'MonsterOrderWilds-Setup-v{version}.exe'}")

    # 打开输出目录
    os.startfile(OUTPUT_DIR)


def main():
    print("=" * 50)
    print("  MonsterOrderWilds Installer Builder")
    print("=" * 50)

    # 1. 读取版本
    version = read_app_version()

    # 2. 查找Inno Setup
    iscc = find_iscc()

    # 3. 检查源文件
    check_source_files()

    # 4. 准备文件
    prepare_files()

    # 5. 签名
    sign_files()

    # 6. 构建
    build_installer(iscc, version)

    input("\nPress Enter to exit...")


if __name__ == "__main__":
    main()
