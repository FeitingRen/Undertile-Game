set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME Undertale_PC)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

# --- Dependencies ---
# Assuming you are using FetchContent or have raylib in a standard location
# If you are using a template, keep your existing FetchContent_Declare/MakeAvailable lines here.
find_package(raylib 4.5 QUIET)
if (NOT raylib_FOUND) # If not found on system, download it
    include(FetchContent)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/master.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE # Fixes CMake Warning (dev) regarding CMP0135
    )
    FetchContent_MakeAvailable(raylib)
endif()

# --- Assets Folder ---
# Copy the assets folder to the build directory so the game can find them
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# --- Sources ---
# This was the missing part causing LNK2019 errors
set(SOURCES
    src/main.cpp
    src/Globals.cpp
    src/Utils.cpp
    src/Player.cpp
    src/Battle.cpp
)

# --- Executable ---
add_executable(${PROJECT_NAME} ${SOURCES})

# --- Linking ---
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Windows specific: Hide console window in Release builds
if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE "/ENTRY:mainCRTStartup")
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()