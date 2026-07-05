@echo off
setlocal

set CONFIG=%1
set PLATFORM=%2
if "%CONFIG%"=="" set CONFIG=Debug
if "%PLATFORM%"=="" set PLATFORM=x64

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"`) do (
    set "MSBUILD=%%i"
)

"%MSBUILD%" "%~dp0SLab.sln" /t:Clean /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /verbosity:minimal /nologo
endlocal
