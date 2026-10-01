@echo off
setlocal

:: Default Debug; any argument → Release
set "CONFIG=Debug"
if not "%~1"=="" set "CONFIG=Release"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"

if not defined MSBUILD (
  echo MSBuild not found. Install Visual Studio 2022 with C++ desktop workload.
  exit /b 1
)

"%MSBUILD%" Polished_Examples.sln /m /v:m /p:Configuration=%CONFIG% /p:Platform=x64
if errorlevel 1 exit /b 1

echo Built all example projects ^(%CONFIG%^|x64^).
