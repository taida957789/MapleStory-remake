cmake_minimum_required(VERSION 3.16)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Include Google Test
include(GoogleTest)

# Test sources
set(TEST_SOURCES
    test_packet.cpp
    test_point.cpp
    test_wz.cpp
    test_canvas.cpp
)

# Find zlib for WZ decompression
find_package(ZLIB REQUIRED)

# Create test executable
add_executable(maplestory_tests
    ${TEST_SOURCES}
    ../src/network/InPacket.cpp
    ../src/network/OutPacket.cpp
    ../src/wz/WzProperty.cpp
    ../src/wz/WzCanvas.cpp
    ../src/wz/WzFile.cpp
    ../src/wz/WzReader.cpp
    ../src/wz/WzCrypto.cpp
    ../src/wz/WzResMan.cpp
    ../src/util/ZXString.cpp
    ../src/util/Singleton.cpp
    ../src/util/Logger.cpp
)

target_include_directories(maplestory_tests PRIVATE
    ../src
)

# Link SDL3, zlib, spdlog, and Google Test
if(BUILD_SHARED_LIBS)
    target_link_libraries(maplestory_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        SDL3::SDL3
        ZLIB::ZLIB
        spdlog::spdlog
    )
else()
    target_link_libraries(maplestory_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        SDL3::SDL3-static
        ZLIB::ZLIB
        spdlog::spdlog
    )
endif()

# Discover tests
gtest_discover_tests(maplestory_tests)
