cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME Undertale_PC)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

# --- 1. CONFIGURATION: PATH TO YOUR LOCAL RAYLIB ---
# Ensure this path is 100% correct.
set(RAYLIB_DIR "/Users/qc/VSCode_Projects/Electronics_Undertale/PC/raylib_src") 

# --- 2. INCLUDES ---
include_directories(${RAYLIB_DIR}/src)

# --- 3. SOURCES ---
set(SOURCES
    src/main.cpp
    src/Globals.cpp
    src/Utils.cpp
    src/Player.cpp
    src/Battle.cpp
)

# --- 4. EXECUTABLE ---
add_executable(${PROJECT_NAME} ${SOURCES})

# Explicitly tell THIS target where to find headers (Fixes "raylib.h not found")
target_include_directories(${PROJECT_NAME} PRIVATE "${RAYLIB_DIR}/src")

# --- 5. LINKING ---
if(MSVC) 
    # Windows Linker Setup
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        "${RAYLIB_DIR}/src/raylib.lib" 
        winmm kernel32 opengl32 gdi32 user32 shell32)
        
    target_link_options(${PROJECT_NAME} PRIVATE "/ENTRY:mainCRTStartup")
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    
else() 
    # macOS / Linux Linker Setup
    
    # 1. Link the local library file (.a)
    target_link_libraries(${PROJECT_NAME} PRIVATE "${RAYLIB_DIR}/src/libraylib.a")

    # 2. Link the required Mac Frameworks (Matches your build_mac.sh)
    if(APPLE)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            "-framework CoreVideo"
            "-framework IOKit"
            "-framework Cocoa"
            "-framework GLUT"
            "-framework OpenGL"
            "-framework CoreAudio"
            "-framework AudioToolbox"
        )
    else()
        # Linux standard libs
        target_link_libraries(${PROJECT_NAME} PRIVATE m pthread dl)
    endif()
endif()

# --- 6. ASSETS ---
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)