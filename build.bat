@echo off
echo Building Null Relay (native)...
echo.

REM Edit these paths if your Raylib is installed elsewhere
set RAYLIB_INCLUDE=C:\raylib\include
set RAYLIB_LIB=C:\raylib\lib

if not exist "%RAYLIB_INCLUDE%\raylib.h" (
    echo ERROR: raylib.h not found.
    echo Please install Raylib and update the paths in build.bat
    echo Download: https://www.raylib.com/
    pause
    exit /b 1
)

gcc -o NullRelay.exe src/main.c ^
    -I%RAYLIB_INCLUDE% ^
    -L%RAYLIB_LIB% ^
    -lraylib -lopengl32 -lgdi32 -lwinmm ^
    -O2 -s -mwindows

if %ERRORLEVEL% EQU 0 (
    echo.
    echo SUCCESS! NullRelay.exe created.
    echo You can now distribute the single .exe
) else (
    echo.
    echo Build failed. Check that gcc (MinGW) and Raylib are installed.
)
pause
