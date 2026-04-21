# MonsterOrderWilds 卸载程序

## 功能概述

独立的卸载程序，提供完整的卸载功能，包括：

1. **删除所有运行时需要的文件**
   - MonsterOrderWilds.exe (主程序)
   - MonsterOrderWildsGUI.dll (GUI组件)
   - MonsterOrderWilds.cer (证书文件)
   - dict/ 目录 (分词字典文件)
   - 其他运行时生成的文件 (*.pdb, *.lib, *.exp)

2. **可选清理数据**
   - 保留用户数据（默认）：保留 `MonsterOrderWilds_configs/` 目录下的所有配置文件
   - 删除所有数据：完全删除配置目录，包括：
     - MainConfig.cfg (主配置文件)
     - credentials.dat (凭据数据)
     - monster_list.json (怪物列表)
     - 其他用户数据文件

3. **可选清理注册表**
   - 保留注册表项（默认）：保留用户ID等配置，方便下次安装恢复
   - 删除所有注册表项：完全清理以下注册表路径：
     - `HKEY_CURRENT_USER\Software\JonysandMHDanmuTools\MonsterOrderWilds`
     - `HKEY_CURRENT_USER\Software\MonsterOrderWilds`

4. **证书清理**
   - 如果以管理员权限运行，会自动从系统中移除已安装的程序证书

## 使用方法

### 方式一：通过安装包使用
安装程序会自动将卸载程序安装到程序目录，运行 `MonsterOrderWilds-Uninstaller.exe` 即可。

### 方式二：独立使用
直接运行 `MonsterOrderWilds-Uninstaller.exe`，卸载程序会自动从注册表读取安装路径。

如果注册表中没有安装路径记录，卸载程序会使用默认路径：`%ProgramFiles%\MonsterOrderWilds`

## 构建说明

### 前提条件
- 安装 [Inno Setup 6.x](https://jrsoftware.org/isdl.php)
- 确保安装程序已构建（因为安装程序包含卸载程序）

### 构建步骤

#### 方法1：使用批处理脚本（推荐）
```batch
cd installer
build-uninstaller.bat
```

#### 方法2：使用Python构建脚本（完整构建）
```batch
cd installer
build-installer.bat
```

这会同时构建：
- 卸载程序 (`output/MonsterOrderWilds-Uninstaller.exe`)
- 安装程序 (`output/MonsterOrderWilds-Setup-v{version}.exe`)

#### 方法3：手动构建
```batch
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" MonsterOrderWilds-Uninstaller.iss
```

## 卸载流程

1. **检查程序运行状态**
   - 如果程序正在运行，提示用户关闭
   - 用户可选择强制结束进程

2. **删除程序文件**
   - 删除主程序文件
   - 删除字典文件
   - 删除证书文件

3. **处理用户数据**（根据用户选择）
   - 保留：保留配置目录
   - 删除：删除整个配置目录

4. **处理注册表**（根据用户选择）
   - 保留：仅删除安装路径记录
   - 删除：删除所有相关注册表项

5. **清理证书**（管理员权限）
   - 从系统证书存储中移除程序证书

6. **完成**
   - 显示卸载摘要
   - 提示用户数据保留位置（如果选择了保留数据）

## 注意事项

1. **数据备份**：卸载前请确认是否需要备份配置文件
2. **管理员权限**：删除证书需要管理员权限，普通权限卸载会跳过证书清理
3. **ID代码**：如果选择保留注册表，用户的ID代码会被保留，下次安装时自动恢复
4. **多用户系统**：卸载程序只清理当前用户的注册表项和数据

## 文件说明

- `MonsterOrderWilds-Uninstaller.iss` - Inno Setup 卸载程序脚本
- `build-uninstaller.bat` - 卸载程序构建批处理
- `build.py` - 完整构建脚本（包含安装程序和卸载程序）
