@echo off
setlocal enabledelayedexpansion

set "ROOT=%~dp0"
cd /d "%ROOT%"

echo ===================================
echo  SteamSearch Setup
echo ===================================
echo.

:: git (needed by CMake FetchContent to pull Crow + Asio)
where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] git is not installed or not on PATH.
    echo Install it from https://git-scm.com/downloads and re-run this script.
    pause
    exit /b 1
)

:: Node.js / npm (for the frontend)
where npm >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Node.js/npm is not installed or not on PATH.
    echo Install it from https://nodejs.org/ and re-run this script.
    pause
    exit /b 1
)

:: CMake: use PATH if present, else fall back to CLion's bundled copy
set "CMAKE_EXE="
where cmake >nul 2>nul
if not errorlevel 1 (
    set "CMAKE_EXE=cmake"
) else (
    for /d %%d in ("C:\Program Files\JetBrains\CLion*") do (
        if exist "%%d\bin\cmake\win\x64\bin\cmake.exe" set "CMAKE_EXE=%%d\bin\cmake\win\x64\bin\cmake.exe"
    )
)
if "%CMAKE_EXE%"=="" (
    echo [ERROR] Could not find CMake.
    echo Install it from https://cmake.org/download/, or install CLion
    echo ^(https://www.jetbrains.com/clion/^), which bundles it, then re-run.
    pause
    exit /b 1
)

:: C++ compiler: use PATH if present, else fall back to CLion's bundled MinGW
where g++ >nul 2>nul
if errorlevel 1 (
    for /d %%d in ("C:\Program Files\JetBrains\CLion*") do (
        if exist "%%d\bin\mingw\bin\g++.exe" set "PATH=%%d\bin\mingw\bin;%PATH%"
    )
)
where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Could not find a C++ compiler ^(g++^).
    echo Install CLion ^(https://www.jetbrains.com/clion/^), which bundles MinGW,
    echo or install MinGW-w64 yourself, then re-run.
    pause
    exit /b 1
)

echo Found required tools.
echo.
echo Configuring backend build...
"%CMAKE_EXE%" -S . -B cmake-build-debug
if errorlevel 1 goto :cmakefail

echo.
echo Building backend ^(first run also downloads Crow + Asio, can take a minute^)...
"%CMAKE_EXE%" --build cmake-build-debug --target steam_server
if errorlevel 1 goto :cmakefail

echo.
echo Installing frontend dependencies...
pushd "%ROOT%frontend"
call npm install
popd

echo.
echo ===================================
echo  Setup complete!
echo  Run start.bat to launch the app.
echo ===================================
pause
exit /b 0

:cmakefail
echo.
echo [ERROR] Backend build failed - see the output above for details.
pause
exit /b 1
