cmake_minimum_required(VERSION 3.16)
project(MapleStory VERSION 1.0.0 LANGUAGES CXX)

# Modern C++ settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Linking options
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
elseif(MSVC)
    add_compile_options(/W4 /permissive-)
endif()

# SDL3 from submodule
if(BUILD_SHARED_LIBS)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
else()
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
endif()
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)  # Disable XTEST (optional X11 extension)
add_subdirectory(3rdparty/SDL EXCLUDE_FROM_ALL)

# Find zlib for WZ decompression
find_package(ZLIB REQUIRED)

# spdlog for logging
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Source files
set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/app/Configuration.cpp
    src/app/WvsContext.cpp
    src/stage/Stage.cpp
    src/stage/MapLoadable.cpp
    src/stage/Logo.cpp
    src/stage/Login.cpp
    src/ui/UIElement.cpp
    src/ui/UIButton.cpp
    src/ui/UIManager.cpp
    src/wz/WzResMan.cpp
    src/wz/WzCanvas.cpp
    src/wz/WzProperty.cpp
    src/wz/WzFile.cpp
    src/wz/WzReader.cpp
    src/wz/WzCrypto.cpp
    src/graphics/WzGr2D.cpp
    src/graphics/WzGr2DLayer.cpp
    src/input/InputSystem.cpp
    src/audio/SoundSystem.cpp
    src/network/ClientSocket.cpp
    src/network/InPacket.cpp
    src/network/OutPacket.cpp
    src/util/Singleton.cpp
    src/util/ZXString.cpp
    src/util/Logger.cpp
    src/debug/DebugOverlay.cpp
)

# Header files
set(HEADERS
    src/app/Application.h
    src/app/Configuration.h
    src/app/WvsContext.h
    src/stage/Stage.h
    src/stage/MapLoadable.h
    src/stage/Logo.h
    src/stage/Login.h
    src/ui/UIElement.h
    src/ui/UIButton.h
    src/ui/UIManager.h
    src/wz/WzResMan.h
    src/wz/WzCanvas.h
    src/wz/WzProperty.h
    src/wz/WzFile.h
    src/wz/WzReader.h
    src/wz/WzCrypto.h
    src/wz/WzTypes.h
    src/graphics/WzGr2D.h
    src/graphics/WzGr2DLayer.h
    src/graphics/WzGr2DTypes.h
    src/input/InputSystem.h
    src/audio/SoundSystem.h
    src/network/ClientSocket.h
    src/network/InPacket.h
    src/network/OutPacket.h
    src/util/Singleton.h
    src/util/ZXString.h
    src/util/Point.h
    src/util/Logger.h
    src/debug/DebugOverlay.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link SDL3, zlib, and spdlog
if(BUILD_SHARED_LIBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 ZLIB::ZLIB spdlog::spdlog)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static ZLIB::ZLIB spdlog::spdlog)
endif()

# Platform-specific settings
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

# Copy resources
if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
endif()

# Copy SDL3 shared library when using dynamic linking
if(BUILD_SHARED_LIBS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Debug canvas overlay
option(MS_DEBUG_CANVAS "Enable debug canvas overlay for development" ON)
if(MS_DEBUG_CANVAS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MS_DEBUG_CANVAS)
endif()
