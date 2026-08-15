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
set "INCLUDE=%VCROOT%\include;%SDKROOT%\Include;%DEPS%\KenshiLib\Include;%DEPS%\KenshiLib\Include\ogre;%DEPS%\boost_1_60_0"
set "LIB=%VCROOT%\lib\amd64;%SDKROOT%\Lib\x64;%DEPS%\KenshiLib\Libraries"

if not exist "%OUT%" mkdir "%OUT%"
"%VCROOT%\bin\x86_amd64\cl.exe" /nologo /c /Od /Ob0 /Zi /Oy- /MD /EHsc /W4 /DKJM_SCANNER_PROBE /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DBOOST_ALL_NO_LIB /DBOOST_ERROR_CODE_HEADER_ONLY /DNDEBUG /D_CONSOLE /DUNICODE /D_UNICODE /Fd"%OUT%\KenshiJobManagement-compile.pdb" /Fo"%OUT%\KenshiJobManagement.obj" "%REPO%\src\KenshiJobManagement.cpp"
exit /b %errorlevel%
