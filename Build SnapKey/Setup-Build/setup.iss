#define MyAppName "Thocktap"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Thock"
#define MyAppURL "https://thock.sa"
#define MyAppExeName "Thocktap.exe"
#define Folder "Thocktap"

[Setup]
AppId={{72AF690F-C35B-4E3F-B82B-8F75A06B960E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={localappdata}\{#MyAppName}
DisableDirPage=yes
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName=Thocktap
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir=C:\
OutputBaseFilename={#MyAppName}-{#MyAppVersion}-Setup
SetupIconFile=..\..\thock.ico
SolidCompression=yes
WizardStyle=classic
WizardImageFile=.\wizard_large.bmp
WizardSmallImageFile=.\wizard_small.bmp
VersionInfoVersion={#MyAppVersion}


[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\..\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion  
Source: "..\..\config.cfg"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\icon.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\icon_off.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\Thocktap.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\meta\*"; DestDir: "{app}\meta"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{group}\Uninstall SnapKey"; Filename: "{uninstallexe}"


[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

