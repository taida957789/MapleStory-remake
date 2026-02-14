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
    test_gr2d_vector.cpp
    test_layer_interpolation.cpp
    test_secure_tear.cpp
    test_item_info.cpp
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
    ../src/wz/WzDirectory.cpp
    ../src/wz/WzFormatDetector.cpp
    ../src/wz/WzImage.cpp
    ../src/wz/WzNode.cpp
    ../src/wz/WzPackage.cpp
    ../src/wz/WzSourceFactory.cpp
    ../src/util/Rand32.cpp
    ../src/util/ZXString.cpp
    ../src/util/Singleton.cpp
    ../src/util/Logger.cpp
    ../src/graphics/Gr2DVector.cpp
    ../src/graphics/WzGr2DLayer.cpp
    ../src/graphics/WzGr2DCanvas.cpp
    ../src/templates/item/ItemInfo.cpp
    ../src/constants/EquipDataPath.cpp
    ../src/constants/WeaponConstants.cpp
)

target_include_directories(maplestory_tests PRIVATE
    ../src
    ../3rdparty/mio/include
    ../3rdparty/minimp3
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
