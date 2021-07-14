@echo off

REM Builds and runs the unit test suite under the Visual Studio C compiler on
REM Windows.
REM
REM Pass a configuration to run or pass "all" to run all configurations.
REM
REM You can run this in a normal command prompt or in a Visual Studio Build
REM Tools command prompt. It will find the build tools automatically if needed
REM but it will run a lot faster if they are already available.

setlocal enabledelayedexpansion

REM Find build tools
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find **\vcvarsall.bat`) do (SET "vcvarsall=%%i")

REM Enable build tools if needed
where cl 1>NUL 2>NUL
if %errorlevel% neq 0 call "%vcvarsall%" amd64
if %errorlevel% neq 0 exit /b %errorlevel%

REM Configure unit tests
python test\unit\configure.py
if %errorlevel% neq 0 exit /b %errorlevel%

REM Run unit tests
ninja -f test\.build\build.ninja %*
if %errorlevel% neq 0 exit /b %errorlevel%
