@echo off

REM This script is run by the continuous integration server to build and run
REM the MPack unit test suite with MSVC on Windows.

setlocal enabledelayedexpansion

REM Find build tools
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find **\vcvarsall.bat`) do (SET "vcvarsall=%%i")

IF "%AMALGAMATED%"=="1" (
    cd .build\amalgamation
)

IF "%COMPILER%"=="cl-2019-x64" (
    call "%vcvarsall%" amd64
)
IF "%COMPILER%"=="cl-2015-x86" (
    call "%vcvarsall%" x86 -vcvars_ver=14.0
)

REM Run the "more" variant of unit tests
call tools\unit.bat more