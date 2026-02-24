; KingDubby Windows Installer Script (Inno Setup)
; Requires Inno Setup 6.x - https://jrsoftware.org/isinfo.php

#define MyAppName "KingDubby"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Scale Navigator LLC"
#define MyAppURL "https://scalenavigator.com"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=license.txt
OutputDir=.
OutputBaseFilename=KingDubby-{#MyAppVersion}-Windows
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation (VST3)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Plugin"; Types: full custom

[Files]
; VST3 Plugin - install to Common Files VST3 folder
Source: "..\Builds\VisualStudio2022\x64\Release\VST3\KingDubby.vst3\*"; DestDir: "{commoncf64}\VST3\KingDubby.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{group}\Scale Navigator Website"; Filename: "{#MyAppURL}"

[Messages]
WelcomeLabel2=KingDubby - Dub Tape Delay Plugin%n%nA recreation of the classic Lowcoders plugin from 2008.%n%nRevived in 2026 by Scale Navigator.%n%nThis will install KingDubby VST3 on your computer.

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // Any post-install actions
  end;
end;
