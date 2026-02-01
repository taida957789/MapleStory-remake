#pragma once

#include "util/Singleton.h"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ms
{

class WzProperty;
class WzCanvas;
class WzFile;

/**
 * @brief WZ Resource Manager
 *
 * Based on IWzResMan interface from the original MapleStory client.
 * GUID: 57dfe40b-3e20-4dbc-97e8-805a50f381bf
 *
 * Manages loading and caching of WZ file resources:
 * - Base.wz - Base data and version info
 * - UI.wz - UI elements
 * - Map.wz - Maps
 * - Character.wz - Character sprites
 * - Mob.wz - Monster data
 * - Skill.wz - Skill data
 * - Sound.wz - Audio files
 * - String.wz - String tables
 * - etc.
 *
 * WZ file loading order (from CWvsApp::InitializeResMan):
 * 1. Base.wz (contains Version.img)
 * 2. Character.wz, Mob.wz, Skill.wz, Reactor.wz, Npc.wz
 * 3. UI.wz, Quest.wz, Item.wz, Effect.wz, String.wz
 * 4. Etc.wz, Morph.wz, TamingMob.wz, Sound.wz, Map.wz
 */
class WzResMan final : public Singleton<WzResMan>
{
    friend class Singleton<WzResMan>;

public:
    /**
     * @brief WZ file loading order as defined in CWvsApp::InitializeResMan
     */
    static constexpr std::array<const char*, 15> WZ_LOAD_ORDER = {
        "Character", "Mob", "Skill", "Reactor", "Npc",
        "UI", "Quest", "Item", "Effect", "String",
        "Etc", "Morph", "TamingMob", "Sound", "Map"};

    /**
     * @brief Default WZ version for KMS (from decompiled code: 1029)
     */
    static constexpr std::int16_t DEFAULT_VERSION = 1029;

    [[nodiscard]] auto Initialize() -> bool;
    void Shutdown();

    /**
     * @brief Get property from path
     *
     * Based on IWzResMan::GetObjectA
     * Example: "UI.wz/Login.img/Title" or "UI/Login.img/Title"
     */
    [[nodiscard]] auto GetProperty(const std::string& path) -> std::shared_ptr<WzProperty>;

    /**
     * @brief Load raw sound data from a WzProperty
     *
     * The property should be a Sound_DX8 type containing WzSoundData.
     * Returns raw MP3 audio bytes.
     *
     * @param prop Property containing sound data
     * @return Raw MP3 audio data, or empty vector on failure
     */
    [[nodiscard]] auto LoadSoundData(const std::shared_ptr<WzProperty>& prop) -> std::vector<std::uint8_t>;

    /**
     * @brief Load a WZ file
     *
     * @param name WZ file name without extension (e.g., "UI", "Map")
     * @return True if loaded successfully
     */
    [[nodiscard]] auto LoadWzFile(const std::string& name) -> bool;

    /**
     * @brief Flush cached objects
     *
     * Based on IWzResMan::FlushCachedObjects
     * @param flags 0 = flush all, other values reserved
     */
    void FlushCachedObjects(int flags);

    /**
     * @brief Set base path for WZ files
     */
    void SetBasePath(const std::string& path) { m_sBasePath = path; }

    /**
     * @brief Get base path for WZ files
     */
    [[nodiscard]] auto GetBasePath() const noexcept -> const std::string& { return m_sBasePath; }

    /**
     * @brief Check if resource manager is initialized
     */
    [[nodiscard]] auto IsInitialized() const noexcept -> bool { return m_bInitialized; }

    /**
     * @brief Get version info for a WZ file
     */
    [[nodiscard]] auto GetWzVersion(const std::string& name) const -> std::int32_t;

private:
    WzResMan();
    ~WzResMan() override;

    /**
     * @brief Load Base.wz and parse Version.img
     *
     * Based on CWvsApp::InitializeResMan logic
     */
    [[nodiscard]] auto InitializeBase() -> bool;

    /**
     * @brief Load all WZ files in proper order
     */
    [[nodiscard]] auto InitializeWzFiles() -> bool;

    /**
     * @brief Get or load a WZ file
     */
    [[nodiscard]] auto GetWzFile(const std::string& name) -> std::shared_ptr<WzFile>;

    // Loaded WZ files
    std::unordered_map<std::string, std::shared_ptr<WzFile>> m_wzFiles;

    // Property cache
    std::map<std::string, std::shared_ptr<WzProperty>> m_propertyCache;

    // Version info from Version.img
    std::unordered_map<std::string, std::int32_t> m_wzVersions;

    // Base path
    std::string m_sBasePath;

    // Initialization flag
    bool m_bInitialized = false;
};

} // namespace ms
