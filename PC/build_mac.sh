#!/bin/bash
# Point to your NEW raylib_src folder
INCLUDE_PATH="-I./raylib_src/src"

# Path to the library you just built manually
LIBRARY_PATH="./raylib_src/src/libraylib.a"

clang++ src/*.cpp -o Undertail \
    -std=c++17 \
    -arch x86_64 -arch arm64 \
    $INCLUDE_PATH \
    $LIBRARY_PATH \
    -framework CoreVideo \
    -framework IOKit \
    -framework Cocoa \
    -framework GLUT \
    -framework OpenGL \
    -framework CoreAudio \
    -framework AudioToolbox