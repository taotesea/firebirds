@echo off

:: Set env vars
@call setenvvar.bat
@if errorlevel 1 (goto :END)

:: verify that prepare was run before
@if not exist %ROOT_PATH%\gen\dbs\metadata.fdb (goto :HELP_PREP & goto :END)

:: verify that boot was run before
@if not exist %ROOT_PATH%\gen\gpre_boot.exe (goto :HELP_BOOT & goto :END)

:: verify that the dinkum patches exist if msvc6 is the build environment
if "%VS_VER%"=="msvc6" (
	(findstr /c:"strcpy(s, pt->ww_timefmt);" "%MSDevDir%"\..\..\VC98\CRT\SRC\STRFTIME.C) || (goto :HELP_DINKUM & exit /B 1)
)

::===========
:: Read input values
@set DBG=
@set DBG_DIR=release
@set CLEAN=/build
for %%v in ( %1 %2 ) do (
	@if /I "%%v"=="DEBUG" ((set DBG=TRUE) && (set DBG_DIR=debug))
	@if /I "%%v"=="CLEAN" (set CLEAN=/rebuild)
)


::==========
:: MAIN
@del make_all.log 2>nul
@if "%DBG%"=="" (
	call :RELEASE
) else (
	call :DEBUG
)
@call :MOVE
@echo.
@echo The following errors and warnings occurred during the build:
@type make_all.log | findstr error(s)
@echo.
@echo.
@echo If no errors occurred you may now build the examples with
@echo.
@echo    make_examples.bat
@echo.

@goto :END

::===========
:RELEASE
@echo Building release
if "%VS_VER%"=="msvc6" (
	@msdev %ROOT_PATH%\builds\win32\%VS_VER%\Firebird2.dsw /MAKE "fbserver - Win32 Release" "fbguard - Win32 Release" "fb_lock_print - Win32 Release" "fb_inet_server - Win32 Release" "gbak - Win32 Release" "gpre - Win32 Release" "gsplit - Win32 Release" "gdef - Win32 Release" "gfix - Win32 Release" "gsec - Win32 Release" "gstat - Win32 Release" "instreg - Win32 Release" "instsvc - Win32 Release" "instclient - Win32 Release" "isql - Win32 Release" "qli - Win32 Release" "fbclient - Win32 Release"  "fbudf - Win32 Release" "ib_udf - Win32 Release" "ib_util - Win32 Release" "intl - Win32 Release" "fb2control - Win32 Release" "fbembed - Win32 Release" %CLEAN% /OUT make_all.log
) else (
	@devenv %ROOT_PATH%\builds\win32\%VS_VER%\Firebird2.sln %CLEAN% release /OUT make_all.log
)
@goto :EOF

::===========
:DEBUG
@echo Building debug
if "%VS_VER%"=="msvc6" (
	@msdev %ROOT_PATH%\builds\win32\%VS_VER%\Firebird2.dsw /MAKE "fbserver - Win32 Debug" "fbguard - Win32 Debug" "fb_lock_print - Win32 Debug" "fb_inet_server - Win32 Debug" "gbak - Win32 Debug" "gpre - Win32 Debug" "gsplit - Win32 Debug" "gdef - Win32 Debug" "gfix - Win32 Debug" "gsec - Win32 Debug" "gstat - Win32 Debug" "instreg - Win32 Debug" "instsvc - Win32 Debug" "instclient - Win32 Debug" "isql - Win32 Debug" "qli - Win32 Debug" "fbclient - Win32 Debug" "fbudf - Win32 Debug" "ib_udf - Win32 Debug" "ib_util - Win32 Debug" "intl - Win32 Debug" "fb2control - Win32 Debug" "fbembed - Win32 Debug" %CLEAN% /OUT make_all.log
) else (
	@devenv %ROOT_PATH%\builds\win32\%VS_VER%\Firebird2.sln %CLEAN% debug /OUT make_all.log
)
@goto :EOF

::===========
:MOVE
@echo Copying files to output
@del %ROOT_PATH%\temp\%DBG_DIR%\firebird\bin\*.exp 2>nul
@del %ROOT_PATH%\temp\%DBG_DIR%\firebird\bin\*.lib 2>nul
@rmdir /q /s %ROOT_PATH%\output 2>nul
@mkdir %ROOT_PATH%\output
@mkdir %ROOT_PATH%\output\bin
@mkdir %ROOT_PATH%\output\intl
@mkdir %ROOT_PATH%\output\udf
@mkdir %ROOT_PATH%\output\help
@mkdir %ROOT_PATH%\output\doc
@mkdir %ROOT_PATH%\output\include
@mkdir %ROOT_PATH%\output\lib
@mkdir %ROOT_PATH%\output\system32
@copy %ROOT_PATH%\temp\%DBG_DIR%\firebird\bin\* %ROOT_PATH%\output\bin >nul
@copy %ROOT_PATH%\temp\%DBG_DIR%\firebird\intl\* %ROOT_PATH%\output\intl >nul
@copy %ROOT_PATH%\temp\%DBG_DIR%\firebird\udf\* %ROOT_PATH%\output\udf >nul
@copy %ROOT_PATH%\temp\%DBG_DIR%\fb2control\Firebird2Control.cpl %ROOT_PATH%\output\system32 >nul

@copy %ROOT_PATH%\gen\dbs\SECURITY.FDB %ROOT_PATH%\output\security.fdb >nul
@copy %ROOT_PATH%\gen\dbs\HELP.fdb %ROOT_PATH%\output\help\help.fdb >nul
@copy %ROOT_PATH%\gen\firebird.msg %ROOT_PATH%\output\firebird.msg >nul

@copy %ROOT_PATH%\ChangeLog %ROOT_PATH%\output\doc\ChangeLog.txt >nul
@copy %ROOT_PATH%\doc\WhatsNew %ROOT_PATH%\output\doc\WhatsNew.txt >nul
@copy %ROOT_PATH%\src\jrd\ibase.h %ROOT_PATH%\output\include >nul
@copy %ROOT_PATH%\src\include\gen\iberror.h %ROOT_PATH%\output\include >nul
@copy %ROOT_PATH%\src\install\misc\firebird.conf %ROOT_PATH%\output >nul
@copy install_super.bat %ROOT_PATH%\output\bin >nul
@copy install_classic.bat %ROOT_PATH%\output\bin >nul
@copy uninstall.bat %ROOT_PATH%\output\bin >nul

@goto :EOF

::==============
:HELP_PREP
@echo.
@echo    You must run prepare.bat before running this script
@echo.
@goto :EOF

::==============
:HELP_BOOT
@echo.
@echo    You must run make_boot.bat before running this script
@echo.
@goto :EOF

::==============
:HELP_DINKUM
@echo.
@echo    You must apply the Dinkum patches before building 
@echo    Firebird with Microsoft Visual Studio 6.
@echo.
@echo    They are available here:
@echo        http://www.dinkumware.com/vc_fixes.html
@echo.
@echo    You should also review the readme here:
@echo        firebird2\builds\win32\msvc6\README_MSVC6.txt
@echo.
@goto :EOF



:END
