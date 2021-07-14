@echo off

REM This script is run by the continuous integration server to build and run
REM the MPack unit test suite with MSVC on Windows.

REM Run the "more" variant of unit tests
call tools\unit.bat more