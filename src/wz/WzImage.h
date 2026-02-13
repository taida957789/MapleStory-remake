#pragma once

#include "WzNode.h"
#include "WzTypes.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace ms
{

class WzProperty;
class WzFile;

/**
 * @brief WzImage represents a .img file in the WZ hierarchy
 *
 * WzImage is a container for WzProperty nodes and supports lazy loading.
 * When first created, the image is not loaded (m_loaded = false).
 * The actual property data is loaded on-demand from the WzFile.
 *
 * WzImage maintains:
 * - File offset and size for lazy loading
 * - Checksum for integrity verification
 * - Map of properties (name -> WzProperty)
 * - Weak reference to parent WzFile
 *
 * Thread safety: WzImage is not thread-safe. Synchronization must be handled externally.
 */
class WzImage : public WzNode
{
public:
    /**
     * @brief Construct an image with the given name
     * @param name Image name (typically filename like "Map.img")
     */
    explicit WzImage(std::string name);

    /**
     * @brief Virtual destructor
     */
    ~WzImage() override;

    // Non-copyable, movable
    WzImage(const WzImage&) = delete;
    WzImage& operator=(const WzImage&) = delete;
    WzImage(WzImage&&) noexcept = default;
    WzImage& operator=(WzImage&&) noexcept = default;

    /**
     * @brief Get the node type
     * @return WzNodeType::Image
     */
    [[nodiscard]] auto GetType() const noexcept -> WzNodeType override;

    /**
     * @brief Check if the image has been loaded
     * @return true if loaded, false otherwise
     */
    [[nodiscard]] auto IsLoaded() const noexcept -> bool;

    /**
     * @brief Mark the image as loaded
     */
    auto MarkLoaded() noexcept -> void;

    /**
     * @brief Set the file offset for lazy loading
     * @param offset Byte offset in the WZ file
     */
    auto SetOffset(std::size_t offset) noexcept -> void;

    /**
     * @brief Get the file offset
     * @return Byte offset in the WZ file
     */
    [[nodiscard]] auto GetOffset() const noexcept -> std::size_t;

    /**
     * @brief Set the size of the image data
     * @param size Size in bytes
     */
    auto SetSize(std::size_t size) noexcept -> void;

    /**
     * @brief Get the size of the image data
     * @return Size in bytes
     */
    [[nodiscard]] auto GetSize() const noexcept -> std::size_t;

    /**
     * @brief Set the checksum for integrity verification
     * @param checksum Checksum value
     */
    auto SetChecksum(std::uint32_t checksum) noexcept -> void;

    /**
     * @brief Get the checksum
     * @return Checksum value
     */
    [[nodiscard]] auto GetChecksum() const noexcept -> std::uint32_t;

    /**
     * @brief Set the WZ file reference for lazy loading
     * @param file Shared pointer to the WZ file
     */
    auto SetWzFile(std::shared_ptr<WzFile> file) noexcept -> void;

    /**
     * @brief Get the WZ file reference
     * @return Weak pointer to the WZ file (may be expired)
     */
    [[nodiscard]] auto GetWzFile() const noexcept -> std::weak_ptr<WzFile>;

    /**
     * @brief Add a property to this image
     * @param property Shared pointer to the property
     *
     * Sets the property's parent to this image.
     */
    auto AddProperty(std::shared_ptr<WzProperty> property) -> void;

    /**
     * @brief Get a property by name
     * @param name Property name
     * @return Shared pointer to the property, or nullptr if not found
     */
    [[nodiscard]] auto GetProperty(const std::string& name) -> std::shared_ptr<WzProperty>;

    /**
     * @brief Get a property by name (operator[] syntax)
     * @param name Property name
     * @return Shared pointer to the property, or nullptr if not found
     */
    [[nodiscard]] auto operator[](const std::string& name) -> std::shared_ptr<WzProperty>;

    /**
     * @brief Get all properties
     * @return Map of property name to property
     */
    [[nodiscard]] auto GetProperties() const noexcept -> const std::map<std::string, std::shared_ptr<WzProperty>>&;

    /**
     * @brief Get the number of properties
     * @return Property count
     */
    [[nodiscard]] auto GetPropertyCount() const noexcept -> std::size_t;

    /**
     * @brief Clear all properties and reset loaded state
     */
    auto Clear() -> void;

private:
    std::map<std::string, std::shared_ptr<WzProperty>> m_properties;
    std::weak_ptr<WzFile> m_file;
    std::size_t m_offset{0};
    std::size_t m_size{0};
    std::uint32_t m_checksum{0};
    bool m_loaded{false};
};

} // namespace ms
