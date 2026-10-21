@echo off
setlocal

set "ROOT=%~dp0"
cd /d "%ROOT%"

if not exist "%ROOT%steam_server.exe" (
    echo [ERROR] steam_server.exe not found in %ROOT%
    echo Run setup.bat first to build it and install frontend dependencies.
    pause
    exit /b 1
)

if not exist "%ROOT%frontend\node_modules" (
    echo Installing frontend dependencies, this only happens once...
    pushd "%ROOT%frontend"
    call npm install
    popd
)

echo Starting backend on port 8080...
start "SteamSearch Backend" cmd /k "cd /d "%ROOT%" && .\steam_server.exe"

echo Starting frontend dev server...
start "SteamSearch Frontend" cmd /k "cd /d "%ROOT%frontend" && npm run dev"

echo Waiting for the servers to come up...
timeout /t 5 /nobreak >nul

start http://localhost:5173

echo.
echo Done. Two windows were opened for the backend and frontend servers.
echo Close them (or this window won't stop them) when you're done - closing
echo those windows ends the servers. If the site didn't open at
echo http://localhost:5173, check the frontend window for the real port
echo ^(Vite picks a different one if 5173 is already busy^).

endlocal
