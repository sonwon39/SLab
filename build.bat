@echo off
setlocal

set CONFIG=%1
set PLATFORM=%2
if "%CONFIG%"=="" set CONFIG=Debug
if "%PLATFORM%"=="" set PLATFORM=x64

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Visual Studio Build Tools required.
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"`) do (
    set "MSBUILD=%%i"
)

if not defined MSBUILD (
    echo [ERROR] MSBuild.exe not found.
    exit /b 1
)

echo [INFO] MSBuild: %MSBUILD%
echo [INFO] Configuration: %CONFIG% ^| Platform: %PLATFORM%

"%MSBUILD%" "%~dp0SLab.sln" ^
    /t:Build ^
    /p:Configuration=%CONFIG% ^
    /p:Platform=%PLATFORM% ^
    /p:RestorePackagesConfig=true ^
    /m ^
    /verbosity:minimal ^
    /nologo

if errorlevel 1 (
    echo [FAIL] Build failed.
    exit /b 1
)

echo [OK] Build succeeded.
endlocal
