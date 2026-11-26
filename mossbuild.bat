@echo off
rem One-click build for Moss.
rem
rem   mossbuild.bat program.ms            -> program.exe next to the source
rem   mossbuild.bat program.ms -o app.exe -> explicit output path
rem
rem With gcc available this produces a native executable (moss build).
rem Without gcc it falls back to the embedded-VM executable (moss pack).
rem Use -p/--pack to force the embedded-VM form.
rem
rem You can also drag a .ms file onto this script.
setlocal
set "HERE=%~dp0"
set "MOSS=%HERE%moss.exe"

if "%~1"=="" goto usage
if "%~1"=="-h" goto usage
if "%~1"=="--help" goto usage

set "SRC="
set "OUT="
set "PACK=0"

:loop
if "%~1"=="" goto haveargs
if /I "%~1"=="-o" goto opt_o
if /I "%~1"=="-p" goto opt_pack
if /I "%~1"=="--pack" goto opt_pack
if defined SRC goto extra
set "SRC=%~1"
shift
goto loop

:opt_o
shift
if "%~1"=="" goto missing_out
set "OUT=%~1"
shift
goto loop

:opt_pack
set "PACK=1"
shift
goto loop

:extra
echo [mossbuild] error: unexpected argument "%~1"
exit /b 1

:missing_out
echo [mossbuild] error: -o needs a file name
exit /b 1

:haveargs
if not defined SRC goto usage
if not defined OUT for %%F in ("%SRC%") do set "OUT=%%~dpnF.exe"

if not exist "%MOSS%" (
  echo [mossbuild] error: moss.exe not found next to this script.
  exit /b 1
)
if not exist "%SRC%" (
  echo [mossbuild] error: source file not found: %SRC%
  exit /b 1
)

set "HAVECC=0"
if defined MOSS_CC set "HAVECC=1"
if "%HAVECC%"=="0" (
  where gcc >nul 2>nul
  if not errorlevel 1 set "HAVECC=1"
)

if "%PACK%"=="1" goto do_pack
if "%HAVECC%"=="1" goto do_build

echo [mossbuild] gcc not found; using the embedded VM ^(moss pack^).
echo [mossbuild]   Install MinGW-w64/MSYS2 or set MOSS_CC for native executables.
goto do_pack

:do_build
"%MOSS%" build "%SRC%" -o "%OUT%"
goto after

:do_pack
"%MOSS%" pack "%SRC%" -o "%OUT%"

:after
if errorlevel 1 (
  echo [mossbuild] build failed.
  exit /b 1
)
echo [mossbuild] done: %OUT%
exit /b 0

:usage
echo Moss one-click build
echo.
echo   mossbuild.bat ^<file.ms^> [-o out.exe] [-p]
echo.
echo   With gcc: native executable. Without gcc: embedded-VM executable.
echo   -p / --pack forces the embedded-VM form.
echo   Without -o the output is placed next to the source file.
exit /b 0
