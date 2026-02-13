#pragma once

#include "IWzSource.h"
#include "WzReader.h"
#include "WzTypes.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ms
{

class WzCanvas;
class WzDirectory;
class WzImage;
class WzNode;
class WzProperty;

/**
 * @brief WZ File parser
 *
 * Implements IWzSource for legacy single-file WZ archives.
 * Based on wzlibcpp reference implementation.
 *
 * Uses the new WzNode hierarchy:
 * - Root is a WzDirectory
 * - Directories create WzDirectory nodes
 * - .img files create WzImage nodes
 * - Properties create WzProperty nodes
 *
 * Supports lazy loading of WzImage nodes via enable_shared_from_this.
 */
class WzFile : public IWzSource,
               public std::enable_shared_from_this<WzFile>
{
public:
    WzFile();
    ~WzFile();

    // Non-copyable, movable
    WzFile(const WzFile&) = delete;
    auto operator=(const WzFile&) -> WzFile& = delete;
    WzFile(WzFile&&) noexcept;
    auto operator=(WzFile&&) noexcept -> WzFile&;

    // ========== IWzSource Interface Implementation ==========

    /**
     * @brief Open and parse a WZ file (IWzSource interface)
     */
    [[nodiscard]] bool Open(const std::string& path) override;

    /**
     * @brief Close the file (IWzSource interface)
     */
    void Close() override;

    /**
     * @brief Check if file is open (IWzSource interface)
     */
    [[nodiscard]] bool IsOpen() const override;

    /**
     * @brief Get root directory node (IWzSource interface)
     */
    [[nodiscard]] auto GetRoot() const -> std::shared_ptr<WzDirectory> override;

    /**
     * @brief Find node by path (IWzSource interface)
     */
    [[nodiscard]] auto FindNode(const std::string& path) -> std::shared_ptr<WzNode> override;

    /**
     * @brief Load an image's properties (IWzSource interface)
     */
    [[nodiscard]] bool LoadImage(WzImage* image) override;

    /**
     * @brief Get file path (IWzSource interface)
     */
    [[nodiscard]] auto GetPath() const -> std::string override { return m_sPath; }

    /**
     * @brief Get version (IWzSource interface)
     */
    [[nodiscard]] auto GetVersion() const -> std::int16_t override { return m_nVersion; }

    /**
     * @brief Get source type (IWzSource interface)
     */
    [[nodiscard]] auto GetSourceType() const -> WzSourceType override {
        return WzSourceType::SingleFile;
    }

    // ========== WzFile-Specific Methods (Backward Compatibility) ==========

    /**
     * @brief Open with specific IV (legacy API)
     */
    [[nodiscard]] auto Open(const std::string& path, const std::uint8_t* iv) -> bool;

private:
    /**
     * @brief Parse WZ file header and detect version
     */
    [[nodiscard]] auto ParseHeader() -> bool;

    /**
     * @brief Parse directory structure
     * @param parent Parent directory to add nodes to (nullptr for validation)
     */
    [[nodiscard]] auto ParseDirectories(std::shared_ptr<WzDirectory> parent) -> bool;

    /**
     * @brief Parse an image node
     * @param image Image node to populate with properties
     */
    [[nodiscard]] auto ParseImage(WzImage* image) -> bool;

    /**
     * @brief Parse property list into an image
     * @param target Image to add properties to
     * @param baseOffset Base offset for string block reading
     */
    [[nodiscard]] auto ParsePropertyList(WzImage* target, std::size_t baseOffset) -> bool;

    /**
     * @brief Parse property children into a property (for sub-properties)
     * @param target Property to add children to
     * @param baseOffset Base offset for string block reading
     */
    [[nodiscard]] auto ParsePropertyChildren(std::shared_ptr<WzProperty> target, std::size_t baseOffset) -> bool;

    /**
     * @brief Parse extended property
     * @param name Property name
     * @param target Property to populate
     * @param baseOffset Base offset for string block reading
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

    /**
     * @brief Try to decode WZ with a specific version number
     */
    [[nodiscard]] bool TryDecodeWithVersion(int version);

    /**
     * @brief Attempt 64-bit WZ detection (version range 770-780)
     */
    [[nodiscard]] bool Try64BitVersionDetection();

    std::string m_sPath;
    WzReader m_reader;
    std::shared_ptr<WzDirectory> m_pRoot;

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
