@echo off
setlocal

set "VCROOT=Z:\home\sirix\.local\share\kjm-build\v100-toolchain\vc_stdx86\Program Files\Microsoft Visual Studio 10.0\VC"
set "VSIDE=Z:\home\sirix\.local\share\kjm-build\v100-toolchain\vc_stdx86\Program Files\Microsoft Visual Studio 10.0\Common7\IDE"
set "VCRUNTIME=Z:\home\sirix\.local\share\kjm-build\v100-toolchain\vc_stdx86\Win\System"
set "SDKROOT=Z:\home\sirix\.local\share\kjm-build\v100-toolchain\sdkbuild\Program Files\Microsoft SDKs\Windows\v7.1"
set "DEPS=Z:\home\sirix\.local\share\kjm-build\dependencies"
set "REPO=Z:\home\sirix\Documents\Projects\KenshiJobManager"
set "OUT=%REPO%\build\ScannerProbe"

set "PATH=%VCROOT%\bin\x86_amd64;%VSIDE%;%VCRUNTIME%;%PATH%"
set "LIB=%VCROOT%\lib\amd64;%SDKROOT%\Lib\x64;%DEPS%\KenshiLib\Libraries"

"%VCROOT%\bin\x86_amd64\link.exe" /nologo /DLL /MACHINE:X64 /SUBSYSTEM:CONSOLE /INCREMENTAL:NO /DEBUG /OPT:NOREF /OPT:NOICF /OUT:"%OUT%\KenshiJobManagement.dll" /PDB:"%OUT%\KenshiJobManagement.pdb" "%OUT%\KenshiJobManagement.obj" KenshiLib.lib MyGUIEngine_x64.lib OgreMain_x64.lib User32.lib
exit /b %errorlevel%
