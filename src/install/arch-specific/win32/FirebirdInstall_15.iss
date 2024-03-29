;  Initial Developer's Public License.
;  The contents of this file are subject to the  Initial Developer's Public
;  License Version 1.0 (the "License"). You may not use this file except
;  in compliance with the License. You may obtain a copy of the License at
;    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl
;  Software distributed under the License is distributed on an "AS IS" basis,
;  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
;  for the specific language governing rights and limitations under the
;  License.
;
;  The Original Code is copyright 2001-2003 Paul Reeves for IBPhoenix.
;
;  The Initial Developer of the Original Code is Paul Reeves for IBPhoenix.
;
;  All Rights Reserved.
;
;   Contributor(s):
;     Tilo Muetze, Theo ? and Michael Rimov for improved detection
;     of an existing install directory.
;     Simon Carter for the WinSock2 detection.
;     Philippe Makowski for internationalization and french translation

;   Usage Notes:
;
;   This script has been designed to work with Inno Setup v4.2.7
;   with Inno Setup Preprocessor v. 1.2.1.295. It is available
;   as a quick start pack from here:
;     http://www.jrsoftware.org/isdl.php#qsp
;
;
;   Known bugs and problems etc etc.
;
;   o The uninstaller can only uninstall what it installed.
;     For instance, if Firebird is installed as a service, but is actually
;     running as an application when the uninstaller is run then the
;     application will not be stopped and the uninstall will not complete
;     cleanly.
;
;     InnoSetup does not support script execution as part of uninstallation.
;     And this feature is not likely to be added in the near future. Here is
;     the definitive explanation from Jordan Russell, the developer of InnoSetup:
;
;       'The uninstaller currently relies on a 100% forward compatible uninstall
;       log format. When an existing uninstall log is appended to, and the
;       uninstaller EXE is replaced with a newer version, the previous log contents
;       are guaranteed to be processed properly. There are no such guarantees of
;       forward compatibility with the Pascal Scripting feature; some support
;       functions might be removed at some point, their declarations might change,
;       etc. For the uninstaller to support Pascal Scripting, it would likely have
;       to abandon the uninstall-log-appending concept, and instead create multiple
;       uninstaller EXEs (chained together somehow) and multiple uninstall logs.'
;
;     To work around this we will probably need to extend the instsvc/instreg
;     utilities to support uninstallation of applications.
;
;
;   o The uninstaller does not know how to stop multiple instances of a classic
;     server. They must be stopped manually.
;
;
;

;-------Innosetup script debug flags
;A dynamically generated sed script sets the appropriate define
;See BuildExecutableInstall.bat for more details.

;This define is not used in practice, but is retained for documentation
;purposes. If set to iss_release it implies that the defines for files,
;examples and compression are set.
#define iss_release

#define files
#define examples
#define compression

;#define iss_debug

#ifdef iss_debug
;Useful for cases where engine is built without examples.
#undef examples
;We speed up compilation (and hence testing) by not compressing contents.
#undef compression
#endif


;-------end of Innosetup script debug flags section

;-------Start of Innosetup script
#define msvc_version 6
#define FirebirdURL "http://www.firebirdsql.org"
#define BaseVer "1_5"
#define GroupnameVer "1.5"
#define release
#define no_pdb
#define i18n

;------If necessary we can turn off i18n by uncommenting this undefine
;#undef  i18n

;Some strings to distinguish the name of final executable
#ifdef ship_pdb
#define pdb_str="_pdb"
#else
#define pdb_str=""
#endif
#ifdef debug
#define debug_str="_debug"
#else
#define debug_str=""
#endif

#define package_number="0"

[Setup]
AppName={cm:MyAppName}
;The following is important - all ISS install packages should
;duplicate this for v1.5. See the InnoSetup help for details.
AppID=FBDBServer_{#BaseVer}
AppVerName=Firebird 1.5.6
AppPublisher=Firebird Project
AppPublisherURL={#FirebirdURL}
AppSupportURL={#FirebirdURL}
AppUpdatesURL={#FirebirdURL}
DefaultDirName={code:ChooseInstallDir|{pf}\Firebird\Firebird_{#BaseVer}}
DefaultGroupName=Firebird {#GroupnameVer}
AllowNoIcons=true
SourceDir=..\..\..\..\..\firebird2
LicenseFile=src\install\misc\IPLicense.txt
AlwaysShowComponentsList=true
WizardImageFile=src\install\arch-specific\win32\firebird_install_logo1.bmp
PrivilegesRequired=admin
UninstallDisplayIcon={code:ChooseUninstallIcon|{app}\bin\fbserver.exe}
OutputDir=builds\install_images
OutputBaseFilename=Firebird-1.5.0-{#package_number}-Win32{#debug_str}{#pdb_str}
#ifdef compression
Compression=lzma
SolidCompression=true
#else
Compression=none
SolidCompression=false
#endif
ShowTasksTreeLines=false
LanguageDetectionMethod=uilanguage

[Languages]
Name: en; MessagesFile: compiler:Default.isl; InfoBeforeFile: src\install\arch-specific\win32\installation_readme.txt; InfoAfterFile: src\install\arch-specific\win32\readme.txt;
#ifdef i18n
Name: ba; MessagesFile: compiler:Languages\Bosnian.isl; InfoBeforeFile: src\install\arch-specific\win32\ba\Instalacija_ProcitajMe.txt; InfoAfterFile: src\install\arch-specific\win32\ba\ProcitajMe.txt;
Name: fr; MessagesFile: compiler:Languages\French.isl; InfoBeforeFile: src\install\arch-specific\win32\fr\installation_lisezmoi.txt; InfoAfterFile: src\install\arch-specific\win32\fr\lisezmoi.txt;
Name: de; MessagesFile: compiler:Languages\German.isl; InfoBeforeFile: src\install\arch-specific\win32\de\installation_liesmich.txt; InfoAfterFile: src\install\arch-specific\win32\de\liesmich.txt
Name: es; MessagesFile: compiler:Languages\Spanish.isl; InfoBeforeFile: src\install\arch-specific\win32\es\leame_instalacion.txt; InfoAfterFile: src\install\arch-specific\win32\es\leame.txt;
Name: hu; MessagesFile: compiler:Languages\Hungarian.isl; InfoBeforeFile: src\install\arch-specific\win32\hu\telepitesi_segedlet.txt; InfoAfterFile: src\install\arch-specific\win32\hu\olvass_el.txt;
Name: it; MessagesFile: compiler:Languages\Italian.isl; InfoBeforeFile: src\install\arch-specific\win32\it\leggimi_installazione.txt; InfoAfterFile: src\install\arch-specific\win32\it\leggimi.txt
Name: pl; MessagesFile: compiler:Languages\Polish.isl; InfoBeforeFile: src\install\arch-specific\win32\pl\instalacja_czytajto.txt; InfoAfterFile: src\install\arch-specific\win32\pl\czytajto.txt;
Name: pt; MessagesFile: compiler:Languages\PortugueseStd.isl; InfoBeforeFile: src\install\arch-specific\win32\pt\instalacao_leia-me.txt; InfoAfterFile: src\install\arch-specific\win32\pt\leia-me.txt
Name: ru; MessagesFile: compiler:Languages\Russian.isl; InfoBeforeFile: src\install\arch-specific\win32\ru\installation_readme.txt; InfoAfterFile: src\install\arch-specific\win32\ru\readme.txt;
Name: si; MessagesFile: compiler:Languages\Slovenian.isl; InfoBeforeFile: src\install\arch-specific\win32\si\instalacija_precitajMe.txt; InfoAfterFile: src\install\arch-specific\win32\readme.txt;
#endif

[Messages]
en.BeveledLabel=English
#ifdef i18n
ba.BeveledLabel=Bosanski
fr.BeveledLabel=Fran�ais
de.BeveledLabel=Deutsch
es.BeveledLabel=Espa�ol
hu.BeveledLabel=Magyar
it.BeveledLabel=Italiano
pl.BeveledLabel=Polski
pt.BeveledLabel=Portugu�s
ru.BeveledLabel=<0420><0443><0441><0441><043A><0438><0439>
si.BeveledLabel=Slovenski
#endif

[CustomMessages]
#include "custom_messages.inc"
#ifdef i18n
#include "ba\custom_messages_ba.inc"
#include "fr\custom_messages_fr.inc"
#include "de\custom_messages_de.inc"
#include "es\custom_messages_es.inc"
#include "hu\custom_messages_hu.inc"
#include "it\custom_messages_it.inc"
#include "pl\custom_messages_pl.inc"
#include "pt\custom_messages_pt.inc"
#include "ru\custom_messages_ru.inc"
#include "si\custom_messages_si.inc"
#endif

#ifdef iss_debug
; By default, the languages available at runtime depend on the user's
; code page. A user with the Western European code page set will not
; even see that we support installation with the Slovenian language
; for example.
; It can be useful when debugging to force the display of all available
; languages by setting LanguageCodePage to 0. Of course, if the langauge
; is not supported by the user's current code page it be unusable.
[LangOptions]
LanguageCodePage=0
#endif

[Types]
Name: ServerInstall; Description: {cm:ServerInstall}
Name: DeveloperInstall; Description: {cm:DeveloperInstall}
Name: ClientInstall; Description: {cm:ClientInstall}
Name: CustomInstall; Description: {cm:CustomInstall}; Flags: iscustom;

[Components]
Name: ServerComponent; Description: {cm:ServerComponent}; Types: ServerInstall;
Name: ServerComponent\ClassicServerComponent; Description: {cm:ClassicServerComponent}; Types: ServerInstall; Flags: exclusive;
Name: ServerComponent\SuperServerComponent; Description: {cm:SuperServerComponent}; Types: ServerInstall; Flags: exclusive;
Name: DevAdminComponent; Description: {cm:DevAdminComponent}; Types: ServerInstall DeveloperInstall;
Name: ClientComponent; Description: {cm:ClientComponent}; Types: ServerInstall DeveloperInstall ClientInstall CustomInstall; Flags: fixed disablenouninstallwarning;

[Tasks]
;Server tasks
Name: UseGuardianTask; Description: {cm:UseGuardianTask}; Components: ServerComponent\SuperServerComponent; MinVersion: 4.0,4.0; Check: ConfigureFirebird;
Name: UseApplicationTask; Description: {cm:UseApplicationTaskMsg}; GroupDescription: {cm:TaskGroupDescription}; Components: ServerComponent; MinVersion: 4,4; Flags: exclusive; Check: ConfigureFirebird;
Name: UseServiceTask; Description: {cm:UseServiceTask}; GroupDescription: {cm:TaskGroupDescription}; Components: ServerComponent; MinVersion: 0,4; Flags: exclusive; Check: ConfigureFirebird;
Name: AutoStartTask; Description: {cm:AutoStartTask}; Components: ServerComponent; MinVersion: 4,4; Check: ConfigureFirebird;
;Allow user to not install cpl applet
Name: InstallCPLAppletTask; Description: {cm:InstallCPLAppletTask}; Components: ServerComponent\SuperServerComponent; MinVersion: 4.0,4.0; Check: ShowInstallCPLAppletTask;
;Name: MenuGroupTask; Description: Create a Menu &Group; Components: DevAdminComponent; MinVersion: 4,4
;Copying of client libs to <sys>
Name: CopyFbClientToSysTask; Description: {cm:CopyFbClientToSysTask}; Components: ClientComponent; MinVersion: 4,4; Flags: Unchecked; Check: ShowCopyFbClientLibTask;
Name: CopyFbClientAsGds32Task; Description: {cm:CopyFbClientAsGds32Task}; Components: ClientComponent; MinVersion: 4,4; Check: ShowCopyGds32Task;


[Run]
;Always register Firebird
Filename: {app}\bin\instreg.exe; Parameters: "install "; StatusMsg: {cm:instreg}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized

Filename: {app}\bin\instclient.exe; Parameters: "install fbclient"; StatusMsg: {cm:instclientCopyFbClient}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized; Check: CopyFBClientLib;
Filename: {app}\bin\instclient.exe; Parameters: "install gds32"; StatusMsg: {cm:instclientGenGds32}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized; Check: CopyGds32


;If on NT/Win2k etc and 'Install and start service' requested
Filename: {app}\bin\instsvc.exe; Parameters: "install {code:ServiceStartFlags|""""} "; StatusMsg: {cm:instsvcSetup}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; Check: ConfigureFirebird;
Filename: {app}\bin\instsvc.exe; Description: {cm:instsvcStartQuestion}; Parameters: start; StatusMsg: {cm:instsvcStartMsg}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized postinstall; Tasks: UseServiceTask; Check: StartEngine

;If 'start as application' requested
Filename: {code:StartApp|{app}\bin\fbserver.exe}; Description: {cm:instappStartQuestion}; Parameters: -a; StatusMsg: {cm:instappStartMsg}; MinVersion: 0,4.0; Components: ServerComponent; Flags: nowait postinstall; Tasks: UseApplicationTask; Check: StartEngine

;Jump to a landing page.
Filename: "{#FirebirdURL}/afterinstall"; Description: "After installation - What Next?"; Flags: postinstall shellexec skipifsilent; Components: ServerComponent DevAdminComponent;

[Registry]
;If user has chosen to start as App they may well want to start automatically. That is handled by a function below.
;Unless we set a marker here the uninstall will leave some annoying debris.
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: Firebird; ValueData: ; Flags: uninsdeletevalue; Tasks: UseApplicationTask; Check: ConfigureFirebird;

;This doesn't seem to get cleared automatically by instreg on uninstall, so lets make sure of it
Root: HKLM; Subkey: "SOFTWARE\Firebird Project"; Flags: uninsdeletekeyifempty; Components: ClientComponent DevAdminComponent ServerComponent

;Clean up Invalid registry entries from previous installs.
Root: HKLM; Subkey: "SOFTWARE\FirebirdSQL"; ValueType: none; Flags: deletekey;

;User _may_ be installing over an existing 1.5 install, and it may have been set to run as application on startup
;so we had better delete this entry unless they have chosen to autostart as application
; - except that this seems to be broken. Bah!
;Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; Valuetype: none; ValueName: 'Firebird'; ValueData: ''; flags: deletevalue; Check: IsNotAutoStartApp;

[Icons]
Name: {group}\Firebird Server; Filename: {app}\bin\fb_inet_server.exe; Parameters: -a; Flags: runminimized; MinVersion: 4.0,4.0;  Check: InstallServerIcon; IconIndex: 0; Components: ServerComponent\ClassicServerComponent; Comment: {cm:RunCSNoGuardian}
Name: {group}\Firebird Server; Filename: {app}\bin\fbserver.exe; Parameters: -a; Flags: runminimized; MinVersion: 4.0,4.0;  Check: InstallServerIcon; IconIndex: 0; Components: ServerComponent\SuperServerComponent; Comment: {cm:RunSSNoGuardian}
Name: {group}\Firebird Guardian; Filename: {app}\bin\fbguard.exe; Parameters: -a; Flags: runminimized; MinVersion: 4.0,4.0;  Check: InstallGuardianIcon; IconIndex: 1; Components: ServerComponent\SuperServerComponent; Comment: {cm:RunSSWithGuardian}
Name: {group}\Firebird ISQL Tool; Filename: {app}\bin\isql.exe; WorkingDir: {app}; MinVersion: 4.0,4.0;  Comment: {cm:RunISQL}
#define App_Name = SetupSetting("AppName")
Name: {group}\Firebird 1.5.6 Release Notes; Filename: {app}\doc\Firebird_v1.5.6.ReleaseNotes.pdf; MinVersion: 4.0,4.0; Components: DevAdminComponent; Comment: {#App_Name}.4 {cm:ReleaseNotes}
Name: {group}\Firebird 1.5 Quick Start Guide; Filename: {app}\doc\Firebird-1.5-QuickStart.pdf; MinVersion: 4.0,4.0; Components: DevAdminComponent; Comment: {#App_Name} Quick Start Guide (in English)
Name: "{group}\After Installation"; Filename: "{app}\doc\After_Installation.url"; Comment: "New User? Here's a quick guide to what you should do next."
Name: "{group}\Firebird Web-site"; Filename: "{app}\doc\firebirdsql.org.url"
;Always install the original english version of the readme
Name: {group}\Firebird 1.5.6 Readme; Filename: {app}\readme.txt; MinVersion: 4.0,4.0; Components: DevAdminComponent;
#ifdef i18n
;And install translated readme.txt if non-default language is chosen.
Name: {group}\{cm:IconReadme}; Filename: {app}\{cm:ReadMeFile}; MinVersion: 4.0,4.0; Components: DevAdminComponent; Check: NonDefaultLanguage;
#endif
Name: {group}\{cm:IconUninstall}; Filename: {uninstallexe}; Comment: Uninstall Firebird

[Files]
#ifdef files
Source: src\install\misc\IPLicense.txt; DestDir: {app}; Components: ClientComponent; Flags: sharedfile ignoreversion
Source: src\install\misc\IDPLicense.txt; DestDir: {app}; Components: ClientComponent; Flags: sharedfile ignoreversion
Source: src\install\arch-specific\win32\After_Installation.url; DestDir: {app}\doc; Components: ServerComponent DevAdminComponent; Flags: sharedfile ignoreversion
Source: src\install\arch-specific\win32\firebirdsql.org.url; DestDir: {app}\doc; Components: ServerComponent DevAdminComponent; Flags: sharedfile ignoreversion
;Always install the original english version
Source: src\install\arch-specific\win32\readme.txt; DestDir: {app}; Components: DevAdminComponent; Flags: ignoreversion
#ifdef i18n
;Translated files
Source: src\install\arch-specific\win32\ba\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: ba;
Source: src\install\arch-specific\win32\fr\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: fr;
Source: src\install\arch-specific\win32\de\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: de;
Source: src\install\arch-specific\win32\es\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: es;
Source: src\install\arch-specific\win32\hu\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: hu;
Source: src\install\arch-specific\win32\it\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: it;
Source: src\install\arch-specific\win32\pl\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: pl;
Source: src\install\arch-specific\win32\pt\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: pt;
Source: src\install\arch-specific\win32\ru\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: ru;
Source: src\install\arch-specific\win32\si\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: si;
#endif
Source: output\firebird.conf; DestDir: {app}; DestName: firebird.conf.default; Components: ServerComponent; check: FirebirdConfExists;
Source: output\firebird.conf; DestDir: {app}; DestName: firebird.conf; Components: ServerComponent; Flags: uninsneveruninstall; check: NoFirebirdConfExists
Source: output\aliases.conf; DestDir: {app}; Components: ClientComponent; Flags: uninsneveruninstall onlyifdoesntexist
Source: output\security.fdb; DestDir: {app}; Components: ServerComponent; Flags: uninsneveruninstall onlyifdoesntexist
Source: output\security.fbk; DestDir: {app}; Components: ServerComponent; Flags: ignoreversion
Source: output\firebird.msg; DestDir: {app}; Components: ClientComponent; Flags: sharedfile ignoreversion
Source: output\firebird.log; DestDir: {app}; Components: ServerComponent; Flags: uninsneveruninstall skipifsourcedoesntexist external dontcopy

Source: output\bin\gbak.exe; DestDir: {app}\bin; Components: DevAdminComponent; Flags: sharedfile ignoreversion
Source: output\bin\gdef.exe; DestDir: {app}\bin; Components: DevAdminComponent; Flags: ignoreversion
Source: output\bin\gfix.exe; DestDir: {app}\bin; Components: DevAdminComponent; Flags: sharedfile ignoreversion
Source: output\bin\gpre.exe; DestDir: {app}\bin; Components: DevAdminComponent; Flags: ignoreversion
Source: output\bin\gsec.exe; DestDir: {app}\bin; Components: DevAdminComponent; Flags: sharedfile ignoreversion
Source: output\bin\gsplit.exe; DestDir: {app}\bin; Components: DevAdminComponent; Flags: sharedfile ignoreversion
Source: output\bin\gstat.exe; DestDir: {app}\bin; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: output\bin\fbguard.exe; DestDir: {app}\bin; Components: ServerComponent\SuperServerComponent; Flags: sharedfile ignoreversion
Source: output\bin\fb_inet_server.exe; DestDir: {app}\bin; Components: ServerComponent\ClassicServerComponent; Flags: sharedfile ignoreversion uninsrestartdelete
Source: output\bin\fbserver.exe; DestDir: {app}\bin; Components: ServerComponent\SuperServerComponent; Flags: sharedfile ignoreversion
Source: output\bin\fb_lock_print.exe; DestDir: {app}\bin; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: output\bin\ib_util.dll; DestDir: {app}\bin; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: output\bin\instclient.exe; DestDir: {app}\bin; Components: ClientComponent; Flags: sharedfile ignoreversion
Source: output\bin\instreg.exe; DestDir: {app}\bin; Components: ClientComponent; Flags: sharedfile ignoreversion
Source: output\bin\instsvc.exe; DestDir: {app}\bin; Components: ServerComponent; MinVersion: 0,4.0; Flags: sharedfile ignoreversion
Source: output\bin\isql.exe; DestDir: {app}\bin; Components: DevAdminComponent; Flags: ignoreversion
Source: output\bin\qli.exe; DestDir: {app}\bin; Components: DevAdminComponent; Flags: ignoreversion
Source: output\bin\fbclient.dll; DestDir: {app}\bin; Components: ClientComponent; Flags: overwritereadonly sharedfile promptifolder

;Install MS libs locally if Win2K or later, else place in <sys> if NT4 or Win95/98/ME.
;As there is no default location to store these system files they need to
;be manually placed in the output\system32 prior to running this script.
;NOTE: These dll's MUST be sourced from the vcredist package
;if using MSVC6 and NOT from the local system32 directory.
Source: output\system32\msvcrt.dll; DestDir: {app}\bin; Components: ClientComponent; MinVersion: 0,5.0;
Source: output\system32\msvcrt.dll; DestDir: {sys}; Components: ClientComponent; OnlyBelowVersion: 5.0,5.0; Flags: sharedfile onlyifdoesntexist uninsneveruninstall;
Source: output\system32\msvcp{#msvc_version}0.dll; DestDir: {app}\bin; Components: ClientComponent; MinVersion: 0,5.0;
Source: output\system32\msvcp{#msvc_version}0.dll; DestDir: {sys}; Components: ClientComponent; OnlyBelowVersion: 5.0,5.0; Flags: sharedfile onlyifdoesntexist uninsneveruninstall;

;Always install the original english version
Source: output\doc\*.*; DestDir: {app}\doc; Components: DevAdminComponent; Flags: skipifsourcedoesntexist  ignoreversion
Source: output\doc\sql.extensions\*.*; DestDir: {app}\doc\sql.extensions; Components: DevAdminComponent; Flags: skipifsourcedoesntexist ignoreversion

Source: output\help\*.*; DestDir: {app}\help; Components: DevAdminComponent; Flags: ignoreversion;
Source: output\include\*.*; DestDir: {app}\include; Components: DevAdminComponent; Flags: ignoreversion;
Source: output\intl\fbintl.dll; DestDir: {app}\intl; Components: ServerComponent; Flags: sharedfile ignoreversion;
Source: output\lib\*.*; DestDir: {app}\lib; Components: DevAdminComponent; Flags: ignoreversion;
Source: output\UDF\ib_udf.dll; DestDir: {app}\UDF; Components: ServerComponent; Flags: sharedfile ignoreversion;
Source: output\UDF\fbudf.dll; DestDir: {app}\UDF; Components: ServerComponent; Flags: sharedfile ignoreversion;
Source: output\UDF\fbudf.txt; DestDir: {app}\doc; Components: ServerComponent; Flags: sharedfile ignoreversion;
Source: output\UDF\*.sql; DestDir: {app}\UDF; Components: ServerComponent; Flags: ignoreversion;
;Note - Win9x requires 8.3 filenames for the uninsrestartdelete option to work
Source: output\system32\Firebird2Control.cpl; DestDir: {sys}; Components: ServerComponent\SuperServerComponent; MinVersion: 0,4.0; Flags: sharedfile ignoreversion promptifolder restartreplace uninsrestartdelete; Check: InstallCPLApplet
Source: output\system32\Firebird2Control.cpl; DestDir: {sys}; Destname: FIREBI~1.CPL; Components: ServerComponent\SuperServerComponent; MinVersion: 4.0,0; Flags: sharedfile ignoreversion promptifolder restartreplace uninsrestartdelete; Check: InstallCPLApplet
#endif
#ifdef examples
Source: output\examples\*.*; DestDir: {app}\examples; Components: DevAdminComponent; Flags: ignoreversion;
#endif

#ifdef ship_pdb
Source: output\bin\fbclient.pdb; DestDir: {app}\bin; Components: ClientComponent;
Source: output\bin\fb_inet_server.pdb; DestDir: {app}\bin; Components: ServerComponent\ClassicServerComponent;
Source: output\bin\fbserver.pdb; DestDir: {app}\bin; Components: ServerComponent\SuperServerComponent;
#endif

[UninstallRun]
Filename: {app}\bin\instsvc.exe; Parameters: " stop"; StatusMsg: {cm:instsvcStopMsg}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; RunOnceId: StopService
Filename: {app}\bin\instsvc.exe; Parameters: " remove"; StatusMsg: {cm:instsvcRemove}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; RunOnceId: RemoveService
Filename: {app}\bin\instclient.exe; Parameters: " remove gds32"; StatusMsg: {cm:instclientDecLibCountGds32}; MinVersion: 4.0,4.0; Flags: runminimized;
Filename: {app}\bin\instclient.exe; Parameters: " remove fbclient"; StatusMsg: {cm:instclientDecLibCountFbClient}; MinVersion: 4.0,4.0; Flags: runminimized;
Filename: {app}\bin\instreg.exe; Parameters: " remove"; StatusMsg: {cm:instreg}; MinVersion: 4.0,4.0; Flags: runminimized; RunOnceId: RemoveRegistryEntry

[UninstallDelete]
Type: files; Name: {app}\*.lck
Type: files; Name: {app}\*.evn


[_ISTool]
EnableISX=true

[Code]
program Setup;

//Var
//  ProductVersion = '1.5.0';

// Some global variables are also in FirebirdInstallEnvironmentChecks.inc
// This is not ideal, but then this scripting environment is not ideal, either.
// The basic point of the include files is to isolate chunks of code that are
// a) Form a module or have common functionality
// b) Debugged.
// This hopefully keeps the main script simpler to follow.

Var
  InstallRootDir: String;
  FirebirdConfSaved: String;
  ForceInstall: Boolean;        // If /force set on command-line we install _and_
                                 // configure. Default is to install and configure only if
                                 // no other working installation is found (unless we are installing
                                 // over the same version)

  //These three command-line options change the default behaviour
  // during a scripted install
  // They also control whether their associated task checkboxes are displayed
  // during an interactive install
  NoCPL: Boolean;               // pass /nocpl on command-line.
  NoLegacyClient: Boolean;      // pass /nogds32 on command line.
  CopyFbClient: Boolean;     // pass /copyfbclient on command line.


#include "FirebirdInstallSupportFunctions.inc"

#include "FirebirdInstallEnvironmentChecks.inc"


function SummarizeInstalledProducts: String;
var
  InstallSummaryArray: TArrayofString;
  product: Integer;
  i: Integer;
  StatusDescription: String;
  InstallSummary: String;
  prodstr: String;
begin

//do nothing gracefully if we are called by accident.
if ProductsInstalledCount = 0 then
  exit;

SetArrayLength(InstallSummaryArray,ProductsInstalledCount);
for product := 0 to MaxProdInstalled -1 do begin
  if (ProductsInstalledArray[product].InstallType <> NotInstalled) then begin
      InstallSummaryArray[i] := Format1(ProductsInstalledArray[product].Description,
            ProductsInstalledArray[product].ActualVersion);

    if (ProductsInstalledArray[product].ServerVersion <> '') then begin
      if ((ProductsInstalledArray[product].InstallType AND ClassicServerInstall) = ClassicServerInstall) then
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:ClassicServerInstall}')
      else
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:SuperServerInstall}')
      end
    else begin
      if (ProductsInstalledArray[product].GBAKVersion <> '') then
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:DeveloperInstall}')
      else
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:ClientInstall}')
    end;

    if ((ProductsInstalledArray[product].InstallType AND BrokenInstall) = BrokenInstall) then
      InstallSummaryArray[i] := InstallSummaryArray[i]
      + #13 + ExpandConstant('{cm:PreviousInstallBroken}')
    else
      InstallSummaryArray[i] := InstallSummaryArray[i]
      + #13 + ExpandConstant('{cm:PreviousInstallGood}')
    ;

    i:= i+1;
  end;
end;

for i:=0 to ProductsInstalledCount-1 do
  InstallSummary := InstallSummary + InstallSummaryArray[i] + #13;

//If FB2 is installed
If ((ProductsInstalled AND FB2) = FB2) then
      InstallSummary := InstallSummary
      +#13 + ExpandConstant('{cm:InstallSummarySuffix1}')
      +#13 + ExpandConstant('{cm:InstallSummarySuffix2}')
      +#13 + ExpandConstant('{cm:InstallSummarySuffix3}')
      +#13 + ExpandConstant('{cm:InstallSummarySuffix4}')
      +#13
;

if ProductsInstalledCount = 1 then
  StatusDescription := Format2(ExpandConstant('{cm:InstalledProducts}'), IntToStr(ProductsInstalledCount), ExpandConstant('{cm:InstalledProdCountSingular}'))
else
  StatusDescription := Format2(ExpandConstant('{cm:InstalledProducts}'), IntToStr(ProductsInstalledCount), ExpandConstant('{cm:InstalledProdCountPlural}'));

  Result := StatusDescription
    +#13
    +#13 + InstallSummary
    +#13 + ExpandConstant('{cm:InstallSummaryCancel1}')
    +#13 + ExpandConstant('{cm:InstallSummaryCancel2}')
    +#13
    +#13 + ExpandConstant('{cm:InstallSummaryCancel3}')
    +#13
end;

function AnalysisAssessment: boolean;
var
  MsgText: String;
  MsgResult: Integer;
begin
  result := false;

  //We've got all this information. What do we do with it?

  if ProductsInstalledCount = 0 then begin
    result := true;
    exit;
  end;


  //If Fb15 RC is installed then we can install over it.
  //unless we find the server running.
  if (ProductsInstalledCount = 1) AND
    (((ProductsInstalled AND FB15) = FB15) OR
     ((ProductsInstalled AND FB15RC) = FB15RC)) then
    if ( FirebirdDefaultServerRunning ) then begin
      result := false;
      MsgBox( #13+ExpandConstant('{cm:FbRunning1,1.5}')
      +#13
      +#13+ExpandConstant('{cm:FbRunning2}')
      +#13+ExpandConstant('{cm:FbRunning3}')
      +#13, mbError, MB_OK);
      exit;
      end
    else begin
      result := true;
      exit;
    end
  ;

  if ForceInstall then begin
    result := true;
    exit;
  end;

  //Otherwise, show user the analysis report.
  MsgText := SummarizeInstalledProducts;
  MsgResult := MsgBox(MsgText,mbConfirmation,MB_YESNO);
  if (MsgResult = IDNO ) then
    result := true;
    //but we don't configure
    if ((InstallAndConfigure AND Configure) = Configure) then
      InstallAndConfigure := InstallAndConfigure - Configure;
end;


function InitializeSetup(): Boolean;
var
  i: Integer;
  CommandLine: String;
begin

  result := true;

  if not CheckWinsock2 then begin
    result := False;
    exit;
  end

  CommandLine:=GetCmdTail;
  if pos('FORCE',Uppercase(CommandLine))>0 then
    ForceInstall:=True;
    
  if pos('NOCPL', Uppercase(CommandLine))>0 then
    NoCPL := True;

  if GetWindowsOS = OS_VISTA then
    NoCPL := True;

  if pos('NOGDS32', Uppercase(CommandLine))>0 then
    NoLegacyClient := True;

  if pos('COPYFBCLIENT', Uppercase(CommandLine))>0 then
    CopyFbClient := True;

  //By default we want to install and confugure,
  //unless subsequent analysis suggests otherwise.
  InstallAndConfigure := Install + Configure;

  InstallRootDir := '';
  InitExistingInstallRecords;
  AnalyzeEnvironment;
  result := AnalysisAssessment;
  if result then
    //There is a possibility that all our efforts to detect an
    //install were in vain and a server _is_ running...
    if ( FirebirdDefaultServerRunning ) then begin
      result := false;
      MsgBox( #13+ExpandConstant('{cm:FbRunning1, }')
      +#13
      +#13+ExpandConstant('{cm:FbRunning2}')
      +#13+ExpandConstant('{cm:FbRunning3}')
      +#13, mbError, MB_OK);
      exit;
    end;
end;


procedure DeInitializeSetup();
var
  ErrCode: Integer;
begin
  // Did the install fail because winsock 2 was not installed?
  if Winsock2Failure then
    // Ask user if they want to visit the Winsock2 update web page.
    if MsgBox(ExpandConstant('{cm:Winsock2Web1}')+#13#13+ExpandConstant('{cm:Winsock2Web2}'), mbInformation, MB_YESNO) = idYes then
      // User wants to visit the web page
      InstShellExec(sMSWinsock2Update, '', '', SW_SHOWNORMAL, ErrCode);
end;


//This function tries to find an existing install of Firebird 1.5
//If it succeeds it suggests that directory for the install
//Otherwise it suggests the default for Fb 1.5
function ChooseInstallDir(Default: String): String;
var
  InterBaseRootDir,
  FirebirdRootDir: String;
begin
  //The installer likes to call this FOUR times, which makes debugging a pain,
  //so let's test for a previous call.
  if (InstallRootDir = '') then begin

    // Try to find the value of "RootDirectory" in the Firebird
    // registry settings. This is either where Fb 1.0 exists or Fb 1.5
    InterBaseRootDir := GetInterBaseDir;
    FirebirdRootDir := GetFirebirdDir;

    if (FirebirdRootDir <> '') and ( FirebirdRootDir = InterBaseRootDir ) then  //Fb 1.0 must be installed so don't overwrite it.
      InstallRootDir := Default;

    if (( InstallRootDir = '' ) and
        ( FirebirdRootDir = Default )) then // Fb 1.5 is already installed,
      InstallRootDir := Default;             // so we offer to install over it

    if (( InstallRootDir = '') and
        ( FirebirdVer[0] = 1 ) and ( FirebirdVer[1] = 5 ) ) then   // Firebird 1.5 is installed
      InstallRootDir := FirebirdRootDir;                            // but the user has changed the default

    if (( InstallRootDir = '') and
        ( FirebirdVer[0] = 2 ) and ( FirebirdVer[1] = 0 ) ) then   // Firebird 2.0 is installed
      InstallRootDir := Default;                                   // but the user has changed the default
    // if we haven't found anything then try the FIREBIRD env var
    // User may have preferred location for Firebird, but has possibly
    // uninstalled previous version
    if (InstallRootDir = '') then
      InstallRootDir:=getenv('FIREBIRD');

    //if no existing locations found make sure we default to the default.
    if (InstallRootDir = '') then
      InstallRootDir := Default;

  end; // if InstallRootDir = '' then begin

  Result := ExpandConstant(InstallRootDir);

end;


function ServiceStartFlags(Default: String): String;
var
  ServerType: String;
  SvcParams: String;
begin
  servertype := '';
  SvcParams := '';
  if ClassicInstallChosen then
    ServerType := ' -classic '
  else
    ServerType := ' -superserver ';


  if ShouldProcessEntry('ServerComponent', 'AutoStartTask')= srYes then
    SvcParams := ' -auto '
  else
    SvcParams := ' -demand ';

  if ShouldProcessEntry('ServerComponent', 'UseGuardianTask')= srYes then
    SvcParams := SvcParams + ServerType + ' -guardian'
  else
    SvcParams := SvcParams + ServerType;

  Result := SvcParams;
end;

function InstallGuardianIcon(): Boolean;
begin
  result := false;
  if ShouldProcessEntry('ServerComponent', 'UseApplicationTask')= srYes then
    if ShouldProcessEntry('ServerComponent', 'UseGuardianTask')= srYes then
      result := true;
end;

function InstallServerIcon(): Boolean;
begin
  result := false;
  if ShouldProcessEntry('ServerComponent', 'UseApplicationTask')= srYes then
    if ShouldProcessEntry('ServerComponent', 'UseGuardianTask')= srNo then
      result := true;
end;

function StartApp(Default: String): String;
begin
  if ShouldProcessEntry('ServerComponent', 'UseGuardianTask')= srYes then begin
    Result := GetAppPath+'\bin\fbguard.exe';
    if ClassicInstallChosen then
      Result := Result + ' -c';
    end
  else
    if ClassicInstallChosen then
      Result := GetAppPath+'\bin\fb_inet_server.exe'
    else
      Result := GetAppPath+'\bin\fbserver.exe';


end;

function IsNotAutoStartApp: boolean;
//Support function to help remove unwanted registry entry.
begin
  result := true;
  if ( ShouldProcessEntry('ServerComponent', 'AutoStartTask')= srYes) and
    ( ShouldProcessEntry('ServerComponent', 'UseApplicationTask')= srYes ) then
  result := false;
end;


procedure RemoveSavedConfIfNoDiff;
//Compare firebird.conf with the one we just saved.
//If they match then we can remove the saved one.
var
  FirebirdConfStr: String;
  SavedConfStr: String;
begin
  LoadStringFromFile( GetAppPath+'\firebird.conf', FirebirdConfStr );
  LoadStringFromFile( FirebirdConfSaved, SavedConfStr );

  if CompareStr( FirebirdConfStr, SavedConfStr ) = 0 then
    DeleteFile( FirebirdConfSaved );
end;

procedure UpdateFirebirdConf;
// Update firebird conf. If user has deselected the guardian we should update
// firebird.conf accordingly. Otherwise we leave the file unchanged.
var
  fbconf: TArrayOfString;
  i, position: Integer;
  ArraySize: Integer;
  FileChanged: boolean;
begin
  //There is no simple, foolproof and futureproof way to check whether
  //we are doing a server install, so the easiest way is to see if a
  //firebird.conf exists. If it doesn't then we don't care.
  if FileExists(GetAppPath+'\firebird.conf') then begin
    if ShouldProcessEntry('ServerComponent', 'UseGuardianTask') = srNo  then
      ReplaceLine(GetAppPath+'\firebird.conf','GuardianOption','GuardianOption = 0','#');
  end;
end;


function NonDefaultLanguage: boolean;
//return true if language other than default is chosen
begin
  result := (ActiveLanguage <> 'en');
end;


procedure CurPageChanged(CurPage: Integer);
begin
  case CurPage of
    wpInfoBefore:   WizardForm.INFOBEFOREMEMO.font.name:='Courier New';
    wpInfoAfter:    WizardForm.INFOAFTERMEMO.font.name:='Courier New';
    wpSelectTasks:  WizardForm.TASKSLIST.height := WizardForm.TASKSLIST.height+30;
    wpFinished:     ; // Create some links to Firebird and IBP here?.
  end;
end;


procedure CurStepChanged(CurStep: Integer);
var
  AppStr: String;
  ReadMeFileStr: String;
begin
   case CurStep of
    csCopy: begin
              SetupSharedFilesArray;
              GetSharedLibCountBeforeCopy;
      end;

    csFinished: begin

      //If user has chosen to install an app and run it automatically set up the registry accordingly
      //so that the server or guardian starts evertime they login.
      if (ShouldProcessEntry('ServerComponent', 'AutoStartTask')= srYes) and
              ( ShouldProcessEntry('ServerComponent', 'UseApplicationTask')= srYes ) then begin
        AppStr := StartApp('')+' -a';
        RegWriteStringValue (HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Run', 'Firebird', AppStr);
      end;
      
      //Fix up conf file
      RemoveSavedConfIfNoDiff;
      UpdateFirebirdConf;
      
      //Reset shared library count if necessary
      CheckSharedLibCountAtEnd;

      //Move lang specific readme from doc dir to root of install.
      if NonDefaultLanguage then begin
        ReadMeFileStr := ExpandConstant('{cm:ReadMeFile}');
        if FileCopy(GetAppPath+'\doc\'+ReadMeFileStr, GetAppPath+'\'+ReadMeFileStr, false) then
          DeleteFile(GetAppPath+'\doc\'+ReadMeFileStr);
      end;

    end;
  end;
end;

function StartEngine: boolean;
begin
  if ConfigureFirebird then
    result := not FirebirdOneRunning;
end;


function ChooseUninstallIcon(Default: String): String;
begin
  if ClassicInstallChosen then
    result := GetAppPath+'\bin\fb_inet_server.exe'
  else
    result := GetAppPath+'\bin\fbserver.exe';

end;

//InnoSetup has a Check Parameter that allows installation if the function returns true.
//For firebird.conf we want to do two things:
// o if firebird.conf already exists then install firebird.conf.default
// o if firebird.conf does not exist then install firebird.conf
//
// This double test is also needed because the uninstallation rules are different for each file.
// We never uninstall firebird.conf. We always uninstall firebird.conf.default.

function FirebirdConfExists: boolean;
begin
  Result := fileexists(GetAppPath+'\firebird.conf');
end;

function NoFirebirdConfExists: boolean;
begin
  Result := not fileexists(GetAppPath+'\firebird.conf');
end;

begin
end.



