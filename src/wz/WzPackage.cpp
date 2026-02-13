#include "WzPackage.h"
#include "WzDirectory.h"
#include "WzImage.h"
#include "WzNode.h"
#include "WzProperty.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace ms
{

WzPackage::WzPackage() = default;
WzPackage::~WzPackage() = default;

WzPackage::WzPackage(WzPackage&&) noexcept = default;
auto WzPackage::operator=(WzPackage&&) noexcept -> WzPackage& = default;

auto WzPackage::Open(const std::string& directoryPath) -> bool
{
    Close();

    if (!std::filesystem::exists(directoryPath) ||
        !std::filesystem::is_directory(directoryPath))
    {
        return false;
    }

    m_sDirectoryPath = directoryPath;

    // Find .ini file in directory
    std::string iniPath;
    std::string baseName;

    for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
    {
        if (entry.path().extension() == ".ini")
        {
            iniPath = entry.path().string();
            baseName = entry.path().stem().string();
            break;
        }
    }

    if (iniPath.empty())
    {
        return false;
    }

    // Read last index
    // Note: lastIndex = -1 means single .wz file (not split)
    int lastIndex = ReadLastWzIndex(iniPath);

    // Load all data files (handles both single and split files)
    if (!LoadDataFiles(baseName, lastIndex))
    {
        Close();
        return false;
    }

    // Merge directories
    if (!MergeDirectories())
    {
        Close();
        return false;
    }

    // Process subdirectories (_Canvas, _Skill, etc.)
    (void)ProcessSubdirectories(m_sDirectoryPath);

    m_isOpen = true;
    return true;
}

void WzPackage::Close()
{
    m_dataFiles.clear();
    m_subPackages.clear();
    m_pMergedRoot.reset();
    m_sDirectoryPath.clear();
    m_nVersion = 0;
    m_isOpen = false;
}

auto WzPackage::IsOpen() const -> bool
{
    return m_isOpen;
}

auto WzPackage::GetRoot() const -> std::shared_ptr<WzDirectory>
{
    return m_pMergedRoot;
}

auto WzPackage::FindNode(const std::string& path) -> std::shared_ptr<WzNode>
{
    if (!m_pMergedRoot)
    {
        return nullptr;
    }

    // Parse path and navigate
    std::shared_ptr<WzNode> current = m_pMergedRoot;
    std::string::size_type start = 0;
    std::string::size_type end;

    while ((end = path.find('/', start)) != std::string::npos)
    {
        auto segment = path.substr(start, end - start);
        if (!segment.empty())
        {
            if (auto dir = std::dynamic_pointer_cast<WzDirectory>(current))
            {
                current = dir->GetChild(segment);
            }
            else if (auto img = std::dynamic_pointer_cast<WzImage>(current))
            {
                // Trigger lazy loading if needed
                if (!img->IsLoaded())
                {
                    if (!LoadImage(img.get()))
                        return nullptr;
                }
                current = img->GetProperty(segment);
            }
            else if (auto prop = std::dynamic_pointer_cast<WzProperty>(current))
            {
                current = prop->GetChild(segment);
            }
            else
            {
                return nullptr;
            }

            if (!current)
                return nullptr;
        }
        start = end + 1;
    }

    // Handle final segment
    if (start < path.length())
    {
        auto segment = path.substr(start);
        if (!segment.empty())
        {
            if (auto dir = std::dynamic_pointer_cast<WzDirectory>(current))
            {
                current = dir->GetChild(segment);
            }
            else if (auto img = std::dynamic_pointer_cast<WzImage>(current))
            {
                if (!img->IsLoaded())
                {
                    if (!LoadImage(img.get()))
                        return nullptr;
                }
                current = img->GetProperty(segment);
            }
            else if (auto prop = std::dynamic_pointer_cast<WzProperty>(current))
            {
                current = prop->GetChild(segment);
            }
        }
    }

    return current;
}

auto WzPackage::LoadImage(WzImage* image) -> bool
{
    if (!image)
    {
        return false;
    }

    // Find which data file contains this image
    for (const auto& wzFile : m_dataFiles)
    {
        // Try to load image using each data file
        if (wzFile->LoadImage(image))
        {
            // Set WzSource on all properties for cross-package outlink resolution
            for (const auto& [name, prop] : image->GetProperties())
            {
                SetWzSourceRecursive(prop);
            }
            return true;
        }
    }

    // If not found in main package, try subpackages (_Canvas, _Skill, etc.)
    for (const auto& subPackage : m_subPackages)
    {
        if (subPackage->LoadImage(image))
        {
            // Set WzSource on all properties for cross-package outlink resolution
            for (const auto& [name, prop] : image->GetProperties())
            {
                SetWzSourceRecursive(prop);
            }
            return true;
        }
    }

    return false;
}

void WzPackage::SetWzSourceRecursive(const std::shared_ptr<WzProperty>& prop)
{
    if (!prop)
    {
        return;
    }

    // Set this package as the WzSource
    prop->SetWzSource(this);

    // Recursively set for all children
    for (const auto& [name, child] : prop->GetChildren())
    {
        SetWzSourceRecursive(child);
    }
}

auto WzPackage::GetPath() const -> std::string
{
    return m_sDirectoryPath;
}

auto WzPackage::GetVersion() const -> std::int16_t
{
    return m_nVersion;
}

auto WzPackage::GetSourceType() const -> WzSourceType
{
    return WzSourceType::Package;
}

int WzPackage::ReadLastWzIndex(const std::string& iniPath)
{
    std::ifstream iniFile(iniPath);
    if (!iniFile.is_open())
    {
        return -1;
    }

    std::string line;
    while (std::getline(iniFile, line))
    {
        // Format: LastWzIndex|N
        auto pos = line.find('|');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);

            // Trim whitespace from key
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);

            if (key == "LastWzIndex")
            {
                std::string value = line.substr(pos + 1);

                // Trim whitespace from value
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);

                try
                {
                    return std::stoi(value);
                }
                catch (const std::invalid_argument&)
                {
                    return -1;
                }
                catch (const std::out_of_range&)
                {
                    return -1;
                }
            }
        }
    }

    return -1;
}

bool WzPackage::LoadDataFiles(const std::string& baseName, int lastIndex)
{
    // Handle single .wz file case (LastWzIndex = -1)
    if (lastIndex < 0)
    {
        std::string wzPath = m_sDirectoryPath + "/" + baseName + ".wz";

        if (std::filesystem::exists(wzPath))
        {
            auto wzFile = std::make_shared<WzFile>();
            if (wzFile->Open(wzPath))
            {
                m_nVersion = wzFile->GetVersion();
                m_dataFiles.push_back(std::move(wzFile));
                return true;
            }
        }

        return false;
    }

    // Handle split files case (LastWzIndex >= 0)
    for (int i = 0; i <= lastIndex; ++i)
    {
        // Format: BaseName_NNN.wz (e.g., Base_000.wz)
        std::ostringstream oss;
        oss << m_sDirectoryPath << "/" << baseName << "_"
            << std::setfill('0') << std::setw(3) << i << ".wz";

        std::string wzPath = oss.str();

        if (!std::filesystem::exists(wzPath))
        {
            // Try without padding if file doesn't exist
            oss.str("");
            oss << m_sDirectoryPath << "/" << baseName << "_" << i << ".wz";
            wzPath = oss.str();

            if (!std::filesystem::exists(wzPath))
            {
                continue;  // Skip missing files
            }
        }

        auto wzFile = std::make_shared<WzFile>();
        if (wzFile->Open(wzPath))
        {
            // Save version from first successfully opened file
            if (m_nVersion == 0)
            {
                m_nVersion = wzFile->GetVersion();
            }

            m_dataFiles.push_back(std::move(wzFile));
        }
    }

    return !m_dataFiles.empty();
}

bool WzPackage::MergeDirectories()
{
    if (m_dataFiles.empty())
    {
        return false;
    }

    // Create merged root with package directory name
    std::filesystem::path dirPath(m_sDirectoryPath);
    std::string rootName = dirPath.filename().string();
    m_pMergedRoot = std::make_shared<WzDirectory>(rootName);

    // Merge all data file roots into one
    for (const auto& wzFile : m_dataFiles)
    {
        auto root = wzFile->GetRoot();
        if (!root)
        {
            continue;
        }

        // Copy all children from this root to merged root
        for (const auto& [name, child] : root->GetChildren())
        {
            // Add child to merged root
            // Note: This creates a shallow copy - the actual nodes are shared
            m_pMergedRoot->AddChild(child);
        }
    }

    return m_pMergedRoot->GetChildCount() > 0;
}

bool WzPackage::ProcessSubdirectories(const std::string& parentPath)
{
    // Scan directory for subdirectories starting with "_"
    if (!std::filesystem::exists(parentPath) || !std::filesystem::is_directory(parentPath))
    {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(parentPath))
    {
        if (!entry.is_directory())
        {
            continue;
        }

        // Check if this directory contains a .ini file (indicating it's a sub-package)
        bool hasIniFile = false;
        for (const auto& subEntry : std::filesystem::directory_iterator(entry.path()))
        {
            if (subEntry.path().extension() == ".ini")
            {
                hasIniFile = true;
                break;
            }
        }

        if (!hasIniFile)
        {
            continue;  // Not a sub-package, skip
        }

        // Create and open sub-package
        auto subPackage = std::make_shared<WzPackage>();
        if (!subPackage->Open(entry.path().string()))
        {
            continue;  // Skip if failed to open
        }

        // Get the root directory from sub-package
        auto subRoot = subPackage->GetRoot();
        if (!subRoot)
        {
            continue;
        }

        // Add sub-package root as a child directory to merged root
        if (m_pMergedRoot)
        {
            m_pMergedRoot->AddChild(subRoot);
        }

        // Store sub-package to keep it alive
        m_subPackages.push_back(std::move(subPackage));

        // Recursively process subdirectories of this subdirectory
        (void)ProcessSubdirectories(entry.path().string());
    }

    return true;
}

} // namespace ms
