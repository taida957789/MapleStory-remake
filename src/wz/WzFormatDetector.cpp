#include "WzFormatDetector.h"

#include <fstream>
#include <filesystem>

namespace ms
{

auto WzFormatDetector::DetectFormat(const std::string& path) -> WzFormatType
{
    if (path.empty()) {
        return WzFormatType::Unknown;
    }

    // Check if path is a directory
    if (IsDirectory(path)) {
        // Look for .ini file in directory
        std::filesystem::path dirPath(path);
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".ini") {
                // Found .ini file - this is a package directory
                return WzFormatType::DirectoryPackage;
            }
        }
        // Directory but no .ini - unknown format
        return WzFormatType::Unknown;
    }

    // Check if it's a .wz file
    if (!FileExists(path)) {
        return WzFormatType::Unknown;
    }

    std::filesystem::path filePath(path);
    if (filePath.extension() != ".wz") {
        return WzFormatType::Unknown;
    }

    // Check if it's a valid WZ file by reading version
    int version = ReadWzVersion(path);
    if (version < 0) {
        // Invalid WZ file
        return WzFormatType::Unknown;
    }

    // Check if it's a 64-bit WZ file (version 770-780)
    if (version >= 770 && version <= 780) {
        return WzFormatType::Bit64SingleFile;
    }

    // Valid WZ file with legacy version
    return WzFormatType::LegacySingleFile;
}

bool WzFormatDetector::Is64BitPackageFormat(const std::string& filePath)
{
    int version = ReadWzVersion(filePath);

    // 64-bit clients use version range 770-780
    // This is based on observed behavior from MapleStory 64-bit clients
    return version >= 770 && version <= 780;
}

bool WzFormatDetector::IsDirectory(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

bool WzFormatDetector::FileExists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
}

auto WzFormatDetector::ReadWzVersion(const std::string& filePath) -> int
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return -1;
    }

    // WZ file header structure:
    // char[4] signature ("PKG1")
    // uint64_t fileSize
    // uint32_t dataStart
    // char[] description (null-terminated)
    // After description: uint16_t version (encrypted)

    // Read signature
    char signature[4];
    file.read(signature, 4);
    if (!file || std::string(signature, 4) != "PKG1") {
        return -1;
    }

    // Skip fileSize (8 bytes)
    file.seekg(8, std::ios::cur);

    // Read dataStart
    std::uint32_t dataStart;
    file.read(reinterpret_cast<char*>(&dataStart), sizeof(dataStart));
    if (!file) {
        return -1;
    }

    // Skip to version field (description is null-terminated, typically small)
    // Description typically ends around offset 0x1E or so
    // Version is at dataStart - 2
    if (dataStart < 2) {
        return -1;
    }

    file.seekg(dataStart - 2, std::ios::beg);
    std::int16_t encryptedVersion;
    file.read(reinterpret_cast<char*>(&encryptedVersion), sizeof(encryptedVersion));
    if (!file) {
        return -1;
    }

    // The version is stored encrypted, but for detection we can use the encrypted value
    // Since we just need to detect the range, we can return the encrypted version
    // In practice, 64-bit WZ files have encrypted version values in a specific range
    return static_cast<int>(encryptedVersion);
}

} // namespace ms
