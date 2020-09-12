@echo off

REM Builds and runs the unit test suite under MSVC on Windows.
REM
REM This should be run in a normal command prompt, not a Visual Studio Build
REM Tools command prompt. This script will find the build tools itself.
REM
REM This script is run by the continuous integration server to test MPack with
REM MSVC on Windows.

setlocal enabledelayedexpansion

REM Find vcvarsall.bat for latest Visual Studio
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find **\vcvarsall.bat`) do (SET "vcvarsall=%%i")

REM Load compiler into PATH using vcvarsall.bat
call "%vcvarsall%" amd64
if %errorlevel% neq 0 exit /b %errorlevel%

REM Load Python 3.8 into PATH on Appveyor
if "%APPVEYOR%"=="True" (
    path C:\Python38-x64;!PATH!
)

REM Run Python configuration
python test\unit\configure.py
if %errorlevel% neq 0 exit /b %errorlevel%

REM Build and run unit tests with Ninja
ninja -f build\unit\build.ninja all
if %errorlevel% neq 0 exit /b %errorlevel%