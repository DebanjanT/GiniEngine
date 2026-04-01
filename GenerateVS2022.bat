@echo off
REM Generate Visual Studio 2022 project for Gini Engine
REM Run this script on Windows from the project root directory

echo Generating Visual Studio 2022 project...

REM Create build directory if it doesn't exist
if not exist "build_vs2022" mkdir build_vs2022

REM Generate Visual Studio 2022 solution
cmake -S . -B build_vs2022 -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Visual Studio 2022 project generated!
    echo.
    echo Solution file: build_vs2022\GiniEngine.sln
    echo.
    echo To open in Visual Studio:
    echo   1. Double-click build_vs2022\GiniEngine.sln
    echo   2. Or run: start build_vs2022\GiniEngine.sln
    echo ========================================
    echo.
    
    REM Ask if user wants to open the solution
    set /p OPEN_VS="Open in Visual Studio now? (y/n): "
    if /i "%OPEN_VS%"=="y" (
        start build_vs2022\GiniEngine.sln
    )
) else (
    echo.
    echo ERROR: CMake generation failed!
    echo Make sure CMake is installed and in your PATH.
    echo.
)

pause
