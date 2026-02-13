#pragma once

#include "IWzSource.h"
#include "WzFile.h"
#include "WzTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace ms
{

class WzDirectory;
class WzImage;
class WzNode;
class WzProperty;

/**
 * @brief WZ Package parser for directory-based format
 *
 * Handles new MapleStory 64-bit directory structure with split files:
 * - Reads .ini files for LastWzIndex
 * - Loads multiple _NNN.wz data files
 * - Merges directory structures
 * - Supports recursive sub-packages (_Canvas, AbyssExpedition, Dragon, etc.)
 */
class WzPackage : public IWzSource {
public:
    WzPackage();
    ~WzPackage() override;

    // Non-copyable, movable
    WzPackage(const WzPackage&) = delete;
    auto operator=(const WzPackage&) -> WzPackage& = delete;
    WzPackage(WzPackage&&) noexcept;
    auto operator=(WzPackage&&) noexcept -> WzPackage&;

    // IWzSource interface implementation
    [[nodiscard]] bool Open(const std::string& directoryPath) override;
    void Close() override;
    [[nodiscard]] bool IsOpen() const override;

    [[nodiscard]] auto GetRoot() const -> std::shared_ptr<WzDirectory> override;
    [[nodiscard]] auto FindNode(const std::string& path) -> std::shared_ptr<WzNode> override;
    [[nodiscard]] bool LoadImage(WzImage* image) override;

    [[nodiscard]] auto GetPath() const -> std::string override;
    [[nodiscard]] auto GetVersion() const -> std::int16_t override;
    [[nodiscard]] auto GetSourceType() const -> WzSourceType override;

    /**
     * @brief Read LastWzIndex from .ini file (exposed for testing)
     */
    [[nodiscard]] int ReadLastWzIndex(const std::string& iniPath);

private:
    /**
     * @brief Load all split data files
     */
    [[nodiscard]] bool LoadDataFiles(const std::string& baseName, int lastIndex);

    /**
     * @brief Merge multiple WzFile directory structures
     */
    [[nodiscard]] bool MergeDirectories();

    /**
     * @brief Process subdirectories recursively
     */
    [[nodiscard]] bool ProcessSubdirectories(const std::string& parentPath);

    /**
     * @brief Recursively set WzSource on properties for cross-package outlink resolution
     */
    void SetWzSourceRecursive(const std::shared_ptr<WzProperty>& prop);

    std::string m_sDirectoryPath;
    std::vector<std::shared_ptr<WzFile>> m_dataFiles;
    std::vector<std::shared_ptr<WzPackage>> m_subPackages;  // Subdirectories like _Canvas, _Skill
    std::shared_ptr<WzDirectory> m_pMergedRoot;
    std::int16_t m_nVersion{};
    bool m_isOpen{false};
};

} // namespace ms
