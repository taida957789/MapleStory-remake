#pragma once

#include <memory>
#include <string>
#include <cstdint>

namespace ms
{

class WzDirectory;
class WzImage;
class WzNode;

/**
 * @brief Type of WZ data source
 */
enum class WzSourceType {
    SingleFile,   // Legacy: single .wz file
    Package       // New: directory + multiple _NNN.wz files
};

/**
 * @brief Abstract interface for WZ data sources
 *
 * Provides unified access to both legacy single-file WZ archives
 * and new directory-based package format.
 */
class IWzSource {
public:
    virtual ~IWzSource() = default;

    // Basic operations
    virtual bool Open(const std::string& path) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;

    // Unified access interface
    virtual auto GetRoot() const -> std::shared_ptr<WzDirectory> = 0;
    virtual auto FindNode(const std::string& path) -> std::shared_ptr<WzNode> = 0;
    virtual bool LoadImage(WzImage* image) = 0;

    // Metadata
    virtual auto GetPath() const -> std::string = 0;
    virtual auto GetVersion() const -> std::int16_t = 0;
    virtual auto GetSourceType() const -> WzSourceType = 0;
};

} // namespace ms
