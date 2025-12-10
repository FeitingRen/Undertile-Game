@echo off
:: --- CONFIGURATION ---
:: Point this to your raylib folder. 
:: If you used the installer, it's likely C:\raylib\raylib
set RAYLIB_PATH=C:\raylib\raylib

:: Compiler Name (g++ comes with the raylib installer/w64devkit)
set COMPILER=g++

:: --- BUILD COMMAND ---
echo Compiling Undertale Clone...

g++ src/*.cpp -o Undertail.exe -O2 -Wall -Wno-missing-braces -static -I C:/raylib/raylib/include -L C:/raylib/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm

:: --- CHECK FOR SUCCESS ---
if %errorlevel% equ 0 (
    echo ---------------------------------------
    echo BUILD SUCCESSFUL!
    echo Running Game...
    echo ---------------------------------------
    Undertail.exe
) else (
    echo ---------------------------------------
    echo BUILD FAILED. See errors above.
    echo ---------------------------------------
    pause
)