; MonsterOrderWilds Installer Script
; Inno Setup 6.x

#define MyAppName "MonsterOrderWilds"
#define MyAppVersion "v15"
#define MyAppPublisher "JonysandMHDanmuTools"
#define MyAppExeName "MonsterOrderWilds.exe"

[Setup]
AppId={{MonsterOrderWilds-Setup}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=output
OutputBaseFilename=MonsterOrderWilds-Setup-{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\MonsterOrderWilds\MonsterOrderWilds.ico
Uninstallable=no
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
UsePreviousAppDir=yes

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Types]
Name: "full"; Description: "完全安装"
Name: "custom"; Description: "自定义安装"; Flags: iscustom

[Components]
Name: "main"; Description: "主程序文件"; Types: full custom; Flags: fixed
Name: "cert"; Description: "安装签名证书（推荐，可避免WindowsDefender警告）"; Types: full

[Files]
Source: "files\MonsterOrderWilds.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "files\MonsterOrderWildsGUI.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "files\MonsterOrderWilds.cer"; DestDir: "{app}"; Flags: ignoreversion; Components: cert
Source: "files\credentials.dat"; DestDir: "{app}\MonsterOrderWilds_configs"; Flags: ignoreversion; Components: main
Source: "files\monster_list.json"; DestDir: "{app}\MonsterOrderWilds_configs"; Flags: ignoreversion; Components: main
; cppjieba 分词字典文件
Source: "..\MonsterOrderWilds\dict\jieba.dict.utf8"; DestDir: "{app}\dict"; Flags: ignoreversion; Components: main
Source: "..\MonsterOrderWilds\dict\hmm_model.utf8"; DestDir: "{app}\dict"; Flags: ignoreversion; Components: main
Source: "..\MonsterOrderWilds\dict\idf.utf8"; DestDir: "{app}\dict"; Flags: ignoreversion; Components: main
Source: "..\MonsterOrderWilds\dict\stop_words.utf8"; DestDir: "{app}\dict"; Flags: ignoreversion; Components: main
Source: "..\MonsterOrderWilds\dict\user.dict.utf8"; DestDir: "{app}\dict"; Flags: ignoreversion; Components: main
; 舰长打卡AI配置说明
Source: "files\弹幕习惯词黑白名单配置.txt"; DestDir: "{app}\dict"; Flags: ignoreversion; Components: main

[InstallDelete]
; Clean old program files but preserve config directory
Type: files; Name: "{app}\*.exe"
Type: files; Name: "{app}\*.dll"
Type: files; Name: "{app}\*.pdb"
Type: files; Name: "{app}\*.cer"
Type: files; Name: "{app}\*.lib"
Type: files; Name: "{app}\*.exp"

[Run]
; Install certificate if selected
Filename: "certutil"; Parameters: "-addstore -f ""Root"" ""{app}\MonsterOrderWilds.cer"""; \
    StatusMsg: "正在安装签名证书..."; Flags: runhidden; Components: cert; \
    Check: IsAdminInstallMode

[Code]
var
  CertPage: TInputOptionWizardPage;

procedure InitializeWizard;
begin
  { Create certificate installation page }
  CertPage := CreateInputOptionPage(wpSelectComponents,
    '安装签名证书',
    '关于证书安装',
    '本程序使用自签名证书，安装证书可以：' + #13#10 + #13#10 +
    '  * 避免 Windows SmartScreen 安全警告' + #13#10 +
    '  * 确保程序来源可信' + #13#10 +
    '  * 提升运行体验' + #13#10 + #13#10 +
    '提示：安装证书需要管理员权限。' + #13#10 +
    '如果不安装，首次运行时可能需要手动绕过SmartScreen警告。',
    True, False);
  CertPage.Add('安装证书到系统（需要管理员权限）');
  
  { Default: install certificate if admin }
  CertPage.Values[0] := IsAdminInstallMode;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  
  { Update component selection based on certificate page }
  if CurPageID = CertPage.ID then
  begin
    WizardSelectComponents('');
    if CertPage.Values[0] then
      WizardSelectComponents('cert');
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  { Skip certificate page if not admin }
  if PageID = CertPage.ID then
    Result := not IsAdminInstallMode
  else
    Result := False;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    { Run application after installation if not silent }
    if not WizardSilent then
    begin
      if MsgBox('安装完成！是否立即运行程序？', mbConfirmation, MB_YESNO) = IDYES then
      begin
        Exec(ExpandConstant('{app}\{#MyAppExeName}'), '', '', SW_SHOWNORMAL, ewNoWait, ResultCode);
      end;
    end;
  end;
end;