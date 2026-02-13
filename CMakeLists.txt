cmake_minimum_required(VERSION 3.16)
project(MapleStory VERSION 1.0.0 LANGUAGES CXX)

# Modern C++ settings
set(CMAKE_CXX_STANDARD 20)
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

# FreeType from submodule
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)  # Use our own zlib
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/freetype EXCLUDE_FROM_ALL)

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
    src/stage/Loading.cpp
    src/stage/Login.cpp
    src/ui/UIElement.cpp
    src/ui/UIButton.cpp
    src/ui/UIChannelSelect.cpp
    src/ui/UIEdit.cpp
    src/ui/LayoutMan.cpp
    src/ui/UIManager.cpp
    src/ui/UINewCharRaceSelect.cpp
    src/ui/UISelectChar.cpp
    src/ui/UIWorldSelect.cpp
    src/ui/WndMan.cpp
    src/wz/WzResMan.cpp
    src/wz/WzCanvas.cpp
    src/wz/WzProperty.cpp
    src/wz/WzFile.cpp
    src/wz/WzReader.cpp
    src/wz/WzCrypto.cpp
    src/wz/WzDirectory.cpp
    src/wz/WzFormatDetector.cpp
    src/wz/WzImage.cpp
    src/wz/WzNode.cpp
    src/wz/WzPackage.cpp
    src/wz/WzSourceFactory.cpp
    src/graphics/WzGr2D.cpp
    src/graphics/WzGr2DLayer.cpp
    src/graphics/WzGr2DCanvas.cpp
    src/graphics/Gr2DVector.cpp
    src/graphics/VecCtrl.cpp
    src/input/InputSystem.cpp
    src/audio/SoundSystem.cpp
    src/network/ClientSocket.cpp
    src/network/InPacket.cpp
    src/network/OutPacket.cpp
    src/util/Singleton.cpp
    src/util/Rand32.cpp
    src/util/ZXString.cpp
    src/util/Logger.cpp
    src/debug/DebugOverlay.cpp
    src/text/TextRenderer.cpp
    src/animation/AnimationDisplayer.cpp
    src/animation/ActionFrame.cpp
    src/animation/ActionData.cpp
    src/animation/ActionMan.cpp
    src/animation/LoadItemAction.cpp
    src/animation/LoadCharacterAction.cpp
    src/animation/SpriteInstance.cpp
    src/animation/SpriteSource.cpp
    src/life/Life.cpp
    src/user/avatar/Avatar.cpp
    src/user/avatar/AvatarLook.cpp
    src/user/User.cpp
    src/user/UserLocal.cpp
    src/constants/ActionConstants.cpp
    src/constants/ActionHelpers.cpp
    src/constants/EquipDataPath.cpp
    src/constants/FieldConstants.cpp
    src/constants/JobConstants.cpp
    src/constants/WeaponConstants.cpp
    src/templates/morph/MorphTemplate.cpp
    src/templates/item/ItemInfo.cpp
    src/models/GW_ItemSlotBase.cpp
    src/models/GW_ItemSlotEquip.cpp
    src/models/GW_ItemSlotPet.cpp
)

# Header files
set(HEADERS
    src/app/Application.h
    src/app/Configuration.h
    src/app/WvsContext.h
    src/stage/Stage.h
    src/stage/MapLoadable.h
    src/stage/Logo.h
    src/stage/Loading.h
    src/stage/Login.h
    src/ui/UIElement.h
    src/ui/UIButton.h
    src/ui/UIChannelSelect.h
    src/ui/UIEdit.h
    src/ui/UIManager.h
    src/ui/UIWorldSelect.h
    src/ui/WndMan.h
    src/wz/WzResMan.h
    src/wz/WzCanvas.h
    src/wz/WzProperty.h
    src/wz/WzFile.h
    src/wz/WzReader.h
    src/wz/WzCrypto.h
    src/wz/WzTypes.h
    src/wz/WzDirectory.h
    src/wz/WzFormatDetector.h
    src/wz/WzImage.h
    src/wz/WzNode.h
    src/wz/WzPackage.h
    src/wz/WzSourceFactory.h
    src/wz/IWzSource.h
    src/graphics/WzGr2D.h
    src/graphics/WzGr2DLayer.h
    src/graphics/WzGr2DTypes.h
    src/graphics/WzGr2DCanvas.h
    src/graphics/Gr2DVector.h
    src/input/InputSystem.h
    src/audio/SoundSystem.h
    src/network/ClientSocket.h
    src/network/InPacket.h
    src/network/OutPacket.h
    src/util/Rand32.h
    src/util/Singleton.h
    src/util/ZXString.h
    src/util/Point.h
    src/util/Logger.h
    src/util/security/TSecType.h
    src/util/security/ZtlSecureTear.h
    src/debug/DebugOverlay.h
    src/text/TextRenderer.h
    src/animation/AnimationDisplayer.h
    src/animation/ActionFrame.h
    src/animation/ActionData.h
    src/animation/ActionMan.h
    src/animation/CharacterImgEntry.h
    src/animation/LoadItemAction.h
    src/animation/LoadCharacterAction.h
    src/animation/SpriteInstance.h
    src/animation/SpriteSource.h
    src/animation/CharacterActionFrameEntry.h
    src/animation/TamingMobActionFrameEntry.h
    src/user/avatar/Avatar.h
    src/user/avatar/AvatarLook.h
    src/user/UserLocal.h
    src/enums/BodyPart.h
    src/enums/CharacterAction.h
    src/enums/MoveActionType.h
    src/constants/ActionHelpers.h
    src/constants/ActionConstants.h
    src/constants/EquipDataPath.h
    src/constants/FieldConstants.h
    src/constants/JobConstants.h
    src/templates/morph/MorphTemplate.h
    src/templates/item/Addition.h
    src/templates/item/AreaBuffItem.h
    src/templates/item/BagInfo.h
    src/templates/item/BitsCaseItem.h
    src/templates/item/BridleItem.h
    src/templates/item/BundleItem.h
    src/templates/item/CoreItem.h
    src/templates/item/CoupleChairItem.h
    src/templates/item/DayOfWeekItemStat.h
    src/templates/item/DecomposerInstallItem.h
    src/templates/item/DressUpClothesItem.h
    src/templates/item/DyeingItem.h
    src/templates/item/EquipItem.h
    src/templates/item/EquipSlotLevelMinusItem.h
    src/templates/item/ExpiredProtectingItem.h
    src/templates/item/ExtendExpireDateItem.h
    src/templates/item/FixedOption.h
    src/templates/item/GachaponAggScope.h
    src/templates/item/GachaponItemInfo.h
    src/templates/item/GatheringToolItem.h
    src/templates/item/GroupEffectInfo.h
    src/templates/item/GrowthOption.h
    src/templates/item/ItemInfo.h
    src/templates/item/ItemInfoTypes.h
    src/templates/item/ItemName.h
    src/templates/item/ItemPotCreateItem.h
    src/templates/item/ItemPotCureItem.h
    src/templates/item/ItemSkill.h
    src/templates/item/KarmaScissorsItem.h
    src/templates/item/ParamEquipStat.h
    src/templates/item/PetFoodItem.h
    src/templates/item/PieceItemInfo.h
    src/templates/item/ProtectOnDieItem.h
    src/templates/item/RecipeOpenItem.h
    src/templates/item/SetAction.h
    src/templates/item/SetEffect.h
    src/templates/item/SetItemInfo.h
    src/templates/item/SetTowerChair.h
    src/templates/item/TextEquipParam.h
    src/templates/item/VariableStat.h
    src/constants/WeaponConstants.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/3rdparty/freetype/include
    ${CMAKE_SOURCE_DIR}/3rdparty/minimp3
    ${CMAKE_SOURCE_DIR}/3rdparty/mio/include
)

# Link SDL3, zlib, spdlog, and FreeType
if(BUILD_SHARED_LIBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 ZLIB::ZLIB spdlog::spdlog freetype)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static ZLIB::ZLIB spdlog::spdlog freetype)
endif()

# Platform-specific settings
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

# Symlink resources (avoid copying 77GB+ of WZ data on every build)
if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
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
