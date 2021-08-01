@echo off

REM Sets paths for Visual Studio 2017 build tools for x86. Visual Studio 2019
REM creates command prompt shortcuts for 2015 and 2019 but not 2017. For 2017,
REM open a plain command prompt and run this.

setlocal enabledelayedexpansion

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find **\vcvarsall.bat`) do (SET "vcvarsall=%%i")
where cl 1>NUL 2>NUL
if %errorlevel% neq 0 call "%vcvarsall%" x86 -vcvars_ver=14.16
if %errorlevel% neq 0 exit /b %errorlevel%
