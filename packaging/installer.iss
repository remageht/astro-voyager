; Astro Voyager - free Inno Setup 6 installer script (v3.9.0 packaging).
; Free alternative to paid code signing: same staged payload as the portable
; zip (dist/stage/*), so installer content and zip content never diverge.
; Build in CI from the repo root:
;   iscc /DAPP_VERSION=<ver> packaging/installer.iss
; Saves live in %APPDATA% (see src/SaveGame.cpp) so uninstalling never
; touches user progress.

#define MyAppName "Astro Voyager"
#define MyAppExeName "astro-voyager.exe"
#define MyAppPublisher "remageht"
#define MyAppURL "https://github.com/remageht/astro-voyager"
#ifndef APP_VERSION
  ; Local smoke builds without the /D override still produce a valid script.
  #define APP_VERSION "dev"
#endif

[Setup]
AppId={{7A7ED731-78B8-49CD-81A7-D47C88BBFF91}
AppName={#MyAppName}
AppVersion={#APP_VERSION}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
LicenseFile=..\dist\stage\LICENSE
OutputDir=..\dist
OutputBaseFilename=AstroVoyager-v{#APP_VERSION}-setup
SetupIconFile=app.ico
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; Per-user install allowed without admin (dialog lets the user choose).
PrivilegesRequiredOverridesAllowed=dialog commandline
UninstallDisplayIcon={app}\{#MyAppExeName}

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; \
  GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\dist\stage\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; \
  Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Nothing extra: saves/settings live in %APPDATA%, not in {app}.
