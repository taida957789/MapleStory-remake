#include "WzSourceFactory.h"
#include "WzFormatDetector.h"
#include "WzFile.h"
#include "WzPackage.h"

namespace ms
{

auto WzSourceFactory::Create(const std::string& path) -> std::unique_ptr<IWzSource>
{
    // Detect format
    WzFormatType formatType = WzFormatDetector::DetectFormat(path);

    switch (formatType) {
        case WzFormatType::DirectoryPackage:
            // New package format - use WzPackage
            return std::make_unique<WzPackage>();

        case WzFormatType::Bit64SingleFile:
        case WzFormatType::LegacySingleFile:
            // Single-file format (both 32-bit and 64-bit) - use WzFile
            // WzFile handles version detection automatically
            return std::make_unique<WzFile>();

        case WzFormatType::Unknown:
        default:
            // Unknown format
            return nullptr;
    }
}

auto WzSourceFactory::CreateAndOpen(const std::string& path) -> std::unique_ptr<IWzSource>
{
    auto source = Create(path);
    if (!source) {
        return nullptr;
    }

    if (!source->Open(path)) {
        return nullptr;
    }

    return source;
}

} // namespace ms
