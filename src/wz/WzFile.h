#pragma once

#include "WzReader.h"
#include "WzTypes.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ms
{

class WzCanvas;
class WzProperty;

/**
 * @brief WZ File parser
 *
 * Handles opening and parsing WZ archive files.
 * Based on wzlibcpp reference implementation.
 */
class WzFile
{
public:
    WzFile();
    ~WzFile();

    // Non-copyable, movable
    WzFile(const WzFile&) = delete;
    auto operator=(const WzFile&) -> WzFile& = delete;
    WzFile(WzFile&&) noexcept;
    auto operator=(WzFile&&) noexcept -> WzFile&;

    /**
     * @brief Open and parse a WZ file
     */
    [[nodiscard]] auto Open(const std::string& path, const std::uint8_t* iv = WzKeys::ZERO_IV) -> bool;

    /**
     * @brief Close the file
     */
    void Close();

    /**
     * @brief Check if file is open
     */
    [[nodiscard]] auto IsOpen() const noexcept -> bool;

    /**
     * @brief Get root node
     */
    [[nodiscard]] auto GetRoot() const noexcept -> std::shared_ptr<WzProperty>;

    /**
     * @brief Find node by path
     *
     * Path format: "Map.wz/Map/Map0/000010000.img/info/bgm"
     */
    [[nodiscard]] auto FindNode(const std::string& path) -> std::shared_ptr<WzProperty>;

    /**
     * @brief Get file path
     */
    [[nodiscard]] auto GetPath() const noexcept -> const std::string& { return m_sPath; }

    /**
     * @brief Get version
     */
    [[nodiscard]] auto GetVersion() const noexcept -> std::int16_t { return m_nVersion; }

private:
    /**
     * @brief Parse WZ file header and detect version
     */
    [[nodiscard]] auto ParseHeader() -> bool;

    /**
     * @brief Parse directory structure
     */
    [[nodiscard]] auto ParseDirectories(std::shared_ptr<WzProperty> parent) -> bool;

    /**
     * @brief Parse an image node
     */
    [[nodiscard]] auto ParseImage(std::shared_ptr<WzProperty> node, std::size_t offset) -> bool;

    /**
     * @brief Parse property list
     */
    [[nodiscard]] auto ParsePropertyList(std::shared_ptr<WzProperty> target, std::size_t baseOffset) -> bool;

    /**
     * @brief Parse extended property
     */
    void ParseExtendedProperty(const std::u16string& name,
                               std::shared_ptr<WzProperty> target,
                               std::size_t baseOffset);

    /**
     * @brief Parse canvas property
     */
    [[nodiscard]] auto ParseCanvasProperty() -> WzCanvasData;

    /**
     * @brief Parse sound property
     */
    [[nodiscard]] auto ParseSoundProperty() -> WzSoundData;

public:
    /**
     * @brief Load raw sound bytes from a WzSoundData
     *
     * WZ sounds are MP3 encoded. This returns the raw MP3 data.
     *
     * @param soundData Sound metadata from GetSound()
     * @return Raw MP3 audio data, or empty vector on failure
     */
    [[nodiscard]] auto LoadSoundData(const WzSoundData& soundData) -> std::vector<std::uint8_t>;

private:
    /**
     * @brief Load and decompress canvas data
     */
    [[nodiscard]] auto LoadCanvasData(const WzCanvasData& canvasData) -> std::shared_ptr<WzCanvas>;

    /**
     * @brief Decompress DXT3 texture data
     */
    [[nodiscard]] static auto DecompressDxt3(const std::vector<std::uint8_t>& data,
                                              std::int32_t width, std::int32_t height)
        -> std::vector<std::uint8_t>;

    /**
     * @brief Decompress DXT5 texture data
     */
    [[nodiscard]] static auto DecompressDxt5(const std::vector<std::uint8_t>& data,
                                              std::int32_t width, std::int32_t height)
        -> std::vector<std::uint8_t>;

    /**
     * @brief Calculate WZ offset
     */
    [[nodiscard]] auto GetWzOffset() -> std::uint32_t;

    /**
     * @brief Calculate version hash
     */
    [[nodiscard]] static auto GetVersionHash(std::int32_t encrypted, std::int32_t real) -> std::uint32_t;

    std::string m_sPath;
    WzReader m_reader;
    std::shared_ptr<WzProperty> m_pRoot;

    // WZ file description
    std::uint32_t m_dwStart{};
    std::uint32_t m_dwHash{};
    std::int16_t m_nVersion{};

#ifdef MS_DEBUG_CANVAS
    // Current path during parsing (for debug canvas path tracking)
    std::string m_currentParsePath;
#endif
};

} // namespace ms
