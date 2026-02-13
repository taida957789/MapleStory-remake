#pragma once

#include "IWzSource.h"
#include <memory>
#include <string>

namespace ms
{

/**
 * @brief Factory for creating appropriate IWzSource implementations
 *
 * Automatically detects WZ format and creates the correct source type:
 * - WzFile for legacy single-file WZ archives (32-bit and 64-bit)
 * - WzPackage for new directory-based package format
 *
 * Usage:
 * @code
 *   auto source = WzSourceFactory::Create("Base.wz");
 *   if (source && source->Open("Base.wz")) {
 *       auto root = source->GetRoot();
 *       // Use unified interface
 *   }
 * @endcode
 */
class WzSourceFactory {
public:
    /**
     * @brief Create appropriate IWzSource for given path
     *
     * Detection logic:
     * 1. Detect format using WzFormatDetector
     * 2. Create WzPackage for DirectoryPackage format
     * 3. Create WzFile for single-file formats (legacy and 64-bit)
     * 4. Return nullptr for Unknown format
     *
     * The returned source is NOT opened. Caller must call Open().
     *
     * @param path File or directory path
     * @return Unique pointer to IWzSource, or nullptr if format unknown
     */
    [[nodiscard]] static auto Create(const std::string& path)
        -> std::unique_ptr<IWzSource>;

    /**
     * @brief Create and open IWzSource for given path
     *
     * Convenience method that creates and opens in one call.
     *
     * @param path File or directory path
     * @return Unique pointer to opened IWzSource, or nullptr on failure
     */
    [[nodiscard]] static auto CreateAndOpen(const std::string& path)
        -> std::unique_ptr<IWzSource>;
};

} // namespace ms
