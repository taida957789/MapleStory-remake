#pragma once

#include <string>

namespace ms
{

/**
 * @brief Format type of WZ data
 */
enum class WzFormatType {
    Unknown,         // Cannot determine format
    LegacySingleFile, // Legacy .wz file (32-bit)
    Bit64SingleFile,  // 64-bit .wz file
    DirectoryPackage  // New directory-based package format
};

/**
 * @brief WZ Format detection utility
 *
 * Detects whether a path points to:
 * - Legacy single-file WZ archive (32-bit)
 * - 64-bit single-file WZ archive
 * - New directory-based package format
 *
 * Usage:
 * @code
 *   WzFormatType type = WzFormatDetector::DetectFormat("Base.wz");
 *   if (type == WzFormatType::DirectoryPackage) {
 *       // Open as package
 *   } else if (type == WzFormatType::LegacySingleFile) {
 *       // Open as single file
 *   }
 * @endcode
 */
class WzFormatDetector {
public:
    /**
     * @brief Detect WZ format type from path
     *
     * Detection logic:
     * - If path is a directory containing .ini file -> DirectoryPackage
     * - If path is a .wz file:
     *   - Check version range (770-780) -> Bit64SingleFile
     *   - Otherwise -> LegacySingleFile
     * - Otherwise -> Unknown
     *
     * @param path File or directory path
     * @return Detected format type
     */
    [[nodiscard]] static auto DetectFormat(const std::string& path) -> WzFormatType;

    /**
     * @brief Check if a WZ file uses 64-bit package format
     *
     * Checks file header for version in range 770-780, which indicates
     * 64-bit MapleStory client.
     *
     * @param filePath Path to .wz file
     * @return true if file is 64-bit format, false otherwise
     */
    [[nodiscard]] static bool Is64BitPackageFormat(const std::string& filePath);

private:
    /**
     * @brief Check if path is a directory
     */
    [[nodiscard]] static bool IsDirectory(const std::string& path);

    /**
     * @brief Check if file exists
     */
    [[nodiscard]] static bool FileExists(const std::string& path);

    /**
     * @brief Read version from WZ file header
     */
    [[nodiscard]] static auto ReadWzVersion(const std::string& filePath) -> int;
};

} // namespace ms
