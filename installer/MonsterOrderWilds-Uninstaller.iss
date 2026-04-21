; MonsterOrderWilds Uninstaller Script
; Inno Setup 6.x
; 独立的卸载程序，提供清理数据和注册表选项

#define MyAppName "MonsterOrderWilds"
#define MyAppPublisher "JonysandMHDanmuTools"

[Setup]
AppId={{MonsterOrderWilds-Uninstaller}}
AppName={#MyAppName} 卸载程序
AppVersion=1.0
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=output
OutputBaseFilename=MonsterOrderWilds-Uninstaller
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\MonsterOrderWilds\MonsterOrderWilds.ico
Uninstallable=no
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
DisableWelcomePage=no
DisableFinishedPage=no

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Code]
var
  InstallPath: string;
  DataCleanPage: TInputOptionWizardPage;
  RegistryCleanPage: TInputOptionWizardPage;
  
const
  REG_INSTALL_PATH = 'Software\{#MyAppPublisher}\{#MyAppName}';
  REG_CONFIG_PATH = 'Software\MonsterOrderWilds';

function GetInstallPathFromRegistry(): string;
begin
  Result := '';
  RegQueryStringValue(HKEY_CURRENT_USER, REG_INSTALL_PATH, 'InstallPath', Result);
end;

function IsProgramRunning(): Boolean;
var
  ResultCode: Integer;
begin
  Result := False;
  // 检查主程序是否运行
  if Exec('tasklist', '/FI "IMAGENAME eq MonsterOrderWilds.exe" /NH', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    if ResultCode = 0 then
      Result := True;
  end;
end;

function KillProgram(): Boolean;
var
  ResultCode: Integer;
begin
  Result := False;
  // 强制结束主程序
  if Exec('taskkill', '/F /IM MonsterOrderWilds.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    Result := (ResultCode = 0) or (ResultCode = 128); // 128 = 进程不存在
  end;
  // 等待进程完全退出
  Sleep(1000);
end;

procedure InitializeWizard;
begin
  // 从注册表读取安装路径
  InstallPath := GetInstallPathFromRegistry();
  
  // 如果找不到安装路径，提示用户选择
  if InstallPath = '' then
  begin
    InstallPath := ExpandConstant('{autopf}\{#MyAppName}');
  end;
  
  // 创建数据清理选项页面
  DataCleanPage := CreateInputOptionPage(wpWelcome,
    '清理用户数据',
    '选择是否删除用户配置文件和数据',
    '卸载程序可以保留您的配置文件，以便下次安装时恢复设置。' + #13#10 + #13#10 +
    '安装目录: ' + InstallPath + #13#10 + #13#10 +
    '请选择要执行的操作：',
    True, False);
  DataCleanPage.Add('保留用户数据（推荐）');
  DataCleanPage.Add('删除所有用户数据和配置文件');
  DataCleanPage.Values[0] := True; // 默认保留数据
  
  // 创建注册表清理选项页面
  RegistryCleanPage := CreateInputOptionPage(DataCleanPage.ID,
    '清理注册表项',
    '选择是否删除注册表中的配置',
    '以下注册表项将被处理：' + #13#10 + #13#10 +
    '  HKEY_CURRENT_USER\' + REG_INSTALL_PATH + #13#10 +
    '    - 安装路径记录' + #13#10 +
    '  HKEY_CURRENT_USER\' + REG_CONFIG_PATH + #13#10 +
    '    - 用户ID代码 (IdCode)' + #13#10 + #13#10 +
    '请选择要执行的操作：',
    True, False);
  RegistryCleanPage.Add('保留注册表项');
  RegistryCleanPage.Add('删除所有注册表项');
  RegistryCleanPage.Values[0] := True; // 默认保留注册表
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  
  if CurPageID = wpWelcome then
  begin
    // 检查程序是否正在运行
    if IsProgramRunning() then
    begin
      if MsgBox('MonsterOrderWilds 正在运行。' + #13#10 + 
                '需要先关闭程序才能继续卸载。' + #13#10 + #13#10 +
                '是否强制关闭程序？', mbConfirmation, MB_YESNO) = IDYES then
      begin
        if not KillProgram() then
        begin
          MsgBox('无法关闭程序，请手动关闭后重试。', mbError, MB_OK);
          Result := False;
        end;
      end
      else
      begin
        Result := False;
      end;
    end;
  end;
end;

procedure DeleteDirectory(const DirPath: string; DeleteRoot: Boolean);
var
  FindRec: TFindRec;
  FullPath: string;
begin
  if not DirExists(DirPath) then Exit;
  
  if FindFirst(DirPath + '\*', FindRec) then
  begin
    try
      repeat
        if (FindRec.Name <> '.') and (FindRec.Name <> '..') then
        begin
          FullPath := DirPath + '\' + FindRec.Name;
          if FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY <> 0 then
          begin
            // 递归删除子目录
            DeleteDirectory(FullPath, True);
          end
          else
          begin
            // 删除文件
            DeleteFile(FullPath);
          end;
        end;
      until not FindNext(FindRec);
    finally
      FindClose(FindRec);
    end;
  end;
  
  if DeleteRoot then
    RemoveDir(DirPath);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ConfigDir: string;
  DeleteData: Boolean;
  DeleteRegistry: Boolean;
  ResultCode: Integer;
begin
  if CurStep = ssInstall then
  begin
    // 获取用户选择
    DeleteData := DataCleanPage.Values[1];
    DeleteRegistry := RegistryCleanPage.Values[1];
    
    // 1. 删除程序文件
    WizardForm.StatusLabel.Caption := '正在删除程序文件...';
    
    // 删除主程序目录下的文件
    DeleteFile(InstallPath + '\MonsterOrderWilds.exe');
    DeleteFile(InstallPath + '\MonsterOrderWildsGUI.dll');
    DeleteFile(InstallPath + '\MonsterOrderWilds.cer');
    DeleteFile(InstallPath + '\*.pdb');
    DeleteFile(InstallPath + '\*.lib');
    DeleteFile(InstallPath + '\*.exp');
    
    // 删除字典目录
    DeleteDirectory(InstallPath + '\dict', True);
    
    // 2. 根据用户选择处理数据
    if DeleteData then
    begin
      WizardForm.StatusLabel.Caption := '正在删除用户数据...';
      ConfigDir := InstallPath + '\MonsterOrderWilds_configs';
      DeleteDirectory(ConfigDir, True);
    end;
    
    // 3. 根据用户选择处理注册表
    if DeleteRegistry then
    begin
      WizardForm.StatusLabel.Caption := '正在清理注册表...';
      // 删除安装路径注册表项
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, REG_INSTALL_PATH);
      // 删除配置注册表项
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, REG_CONFIG_PATH);
    end
    else
    begin
      // 即使保留注册表，也删除安装路径记录（因为程序已卸载）
      RegDeleteValue(HKEY_CURRENT_USER, REG_INSTALL_PATH, 'InstallPath');
    end;
    
    // 4. 删除证书（如果存在且以管理员权限运行）
    if IsAdminInstallMode then
    begin
      WizardForm.StatusLabel.Caption := '正在移除证书...';
      Exec('certutil', '-delstore "Root" "MonsterOrderWilds"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    end;
    
    // 5. 如果配置目录为空，删除整个程序目录
    if DeleteData then
    begin
      // 检查目录是否为空
      if DirExists(InstallPath) then
      begin
        RemoveDir(InstallPath);
      end;
    end;
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpFinished then
  begin
    // 在结束页面显示卸载摘要
    if DataCleanPage.Values[1] then
      WizardForm.FinishedLabel.Caption := '卸载完成！' + #13#10 + #13#10 +
        '所有程序文件和用户数据已删除。'
    else
      WizardForm.FinishedLabel.Caption := '卸载完成！' + #13#10 + #13#10 +
        '程序文件已删除，用户数据已保留。' + #13#10 +
        '配置文件位置: ' + InstallPath + '\MonsterOrderWilds_configs';
  end;
end;
