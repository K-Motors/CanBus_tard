[Setup]
AppName=CanBus-tard
AppVersion=1.0.0
AppPublisher=Ton Nom
DefaultDirName={autopf}\CanBus-tard
DefaultGroupName=CanBus-tard
OutputDir=installer
OutputBaseFilename=CanBus-tard-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons"

[Files]
Source: "deploy\CanBus-tard.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "deploy\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\CanBus-tard"; Filename: "{app}\CanBus-tard.exe"
Name: "{commondesktop}\CanBus-tard"; Filename: "{app}\CanBus-tard.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\CanBus-tard.exe"; Description: "Launch CanBus-tard"; Flags: postinstall nowait skipifsilent