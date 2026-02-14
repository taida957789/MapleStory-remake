#include "WzResMan.h"
#include "WzDirectory.h"
#include "WzFile.h"
#include "WzImage.h"
#include "WzNode.h"
#include "WzProperty.h"
#include "WzTypes.h"
#include "IWzSource.h"
#include "WzSourceFactory.h"
#include "WzFormatDetector.h"

#include <filesystem>
#include <vector>
#include <iostream>

namespace ms
{

WzResMan::WzResMan() = default;

WzResMan::~WzResMan()
{
    Shutdown();
}

auto WzResMan::Initialize() -> bool
{
    if (m_bInitialized)
        return true;

    // Initializing WZ Resource Manager

    // If base path is not set, try to auto-detect
    if (m_sBasePath.empty())
    {
        // Try current directory
        m_sBasePath = ".";
    }

    // Step 1: Load Base.wz first (contains Version.img)
    // Based on CWvsApp::InitializeResMan
    if (!InitializeBase())
    {
        // Failed to initialize Base.wz
        // Continue anyway - Base.wz may not exist in all clients
    }

    // Step 2: Load all WZ files in order
    // Based on CWvsApp::InitializeResMan - asNameOrder array
    if (!InitializeWzFiles())
    {
        // Some WZ files failed to load
        // Continue anyway - some files may be optional
    }

    m_bInitialized = true;
    return true;
}

void WzResMan::Shutdown()
{
    m_nodeCache.clear();
    m_wzSources.clear();
    m_wzVersions.clear();
    m_bInitialized = false;
}

auto WzResMan::InitializeBase() -> bool
{
    // Try to load Base.wz (single file) or Base/ (package directory)
    std::string basePath = m_sBasePath + "/Base.wz";
    std::string baseDir = m_sBasePath + "/Base";

    // Check which format exists
    std::string path;
    if (std::filesystem::exists(basePath))
    {
        path = basePath;
    }
    else if (std::filesystem::exists(baseDir) && std::filesystem::is_directory(baseDir))
    {
        path = baseDir;
    }
    else
    {
        // Base not found (neither .wz file nor directory)
        return false;
    }

    // Use WzSourceFactory to automatically detect and load
    auto baseSource = WzSourceFactory::CreateAndOpen(path);
    if (!baseSource)
    {
        // Failed to open Base
        return false;
    }

    m_wzSources["Base"] = std::move(baseSource);

    return true;
}

auto WzResMan::InitializeWzFiles() -> bool
{
    // Automatically discover and load all WZ sources in base path
    return DiscoverWzSources();
}

auto WzResMan::DiscoverWzSources() -> bool
{
    if (!std::filesystem::exists(m_sBasePath) || !std::filesystem::is_directory(m_sBasePath))
    {
        // Failed: base path doesn't exist or isn't a directory
        return false;
    }

    int loadedCount = 0;
    int failedCount = 0;

    // DEBUG: Log discovery process
    // Base path OK, starting WZ source discovery

    // Scan directory for .wz files and package directories
    for (const auto& entry : std::filesystem::directory_iterator(m_sBasePath))
    {
        std::string name;
        std::string path = entry.path().string();

        // Check if it's a .wz file
        if (entry.is_regular_file() && entry.path().extension() == ".wz")
        {
            name = entry.path().stem().string();

            // Skip if already loaded (e.g., Base.wz loaded in InitializeBase)
            if (m_wzSources.find(name) != m_wzSources.end())
            {
                continue;
            }

            auto source = WzSourceFactory::CreateAndOpen(path);
            if (source)
            {
                m_wzSources[name] = std::move(source);
                loadedCount++;
            }
            else
            {
                failedCount++;
            }
        }
        // Check if it's a package directory (contains .ini file)
        else if (entry.is_directory())
        {
            name = entry.path().filename().string();

            // Skip if already loaded
            if (m_wzSources.find(name) != m_wzSources.end())
            {
                continue;
            }

            // Check if directory contains a .ini file
            bool hasIniFile = false;
            for (const auto& subEntry : std::filesystem::directory_iterator(path))
            {
                if (subEntry.path().extension() == ".ini")
                {
                    hasIniFile = true;
                    break;
                }
            }

            if (hasIniFile)
            {
                auto source = WzSourceFactory::CreateAndOpen(path);
                if (source)
                {
                    m_wzSources[name] = std::move(source);
                    loadedCount++;
                }
                else
                {
                    failedCount++;
                }
            }
        }
    }

    // Return true even if some sources failed to load
    // (some may be optional or corrupted)
    return loadedCount > 0;
}

auto WzResMan::GetNode(const std::string& path) -> std::shared_ptr<WzNode>
{
    // Check cache first
    const auto cacheIt = m_nodeCache.find(path);
    if (cacheIt != m_nodeCache.end())
    {
        return cacheIt->second;
    }

    // Parse path: "UI.wz/Login.img/Title" or "UI/Login.img/Title"
    std::string wzName;
    std::string subPath;

    auto slashPos = path.find('/');
    if (slashPos != std::string::npos)
    {
        wzName = path.substr(0, slashPos);
        subPath = path.substr(slashPos + 1);
    }
    else
    {
        wzName = path;
    }

    // Remove .wz extension if present
    if (wzName.size() > 3 && wzName.substr(wzName.size() - 3) == ".wz")
    {
        wzName = wzName.substr(0, wzName.size() - 3);
    }

    // Get or load WZ source
    auto wzSource = GetWzSource(wzName);
    if (!wzSource)
    {
        // Return nullptr if source not found
        return nullptr;
    }

    // Find node in WZ source
    std::shared_ptr<WzNode> node;
    if (subPath.empty())
    {
        // Return root directory
        node = wzSource->GetRoot();
    }
    else
    {
        // FindNode handles lazy loading of images
        node = wzSource->FindNode(subPath);
    }

    if (node)
    {
        m_nodeCache[path] = node;
    }

    return node;
}

auto WzResMan::GetDirectory(const std::string& path) -> std::shared_ptr<WzDirectory>
{
    auto node = GetNode(path);
    if (!node)
    {
        return nullptr;
    }

    // Use dynamic_pointer_cast for type-safe downcasting
    return std::dynamic_pointer_cast<WzDirectory>(node);
}

auto WzResMan::GetImage(const std::string& path) -> std::shared_ptr<WzImage>
{
    auto node = GetNode(path);
    if (!node)
    {
        return nullptr;
    }

    // Use dynamic_pointer_cast for type-safe downcasting
    return std::dynamic_pointer_cast<WzImage>(node);
}

auto WzResMan::GetProperty(const std::string& path) -> std::shared_ptr<WzProperty>
{
    auto node = GetNode(path);
    if (!node)
        return nullptr;

    // Direct WzProperty match
    if (auto prop = std::dynamic_pointer_cast<WzProperty>(node))
        return prop;

    // WzImage: load and wrap as root WzProperty
    // This handles paths like "Character/Weapon/01302000.img" where the
    // terminal node is a WzImage rather than a WzProperty.
    auto img = std::dynamic_pointer_cast<WzImage>(node);
    if (!img)
        return nullptr;

    if (!img->IsLoaded())
    {
        // Parse WZ source name from path (first segment)
        std::string wzName;
        auto slashPos = path.find('/');
        if (slashPos != std::string::npos)
            wzName = path.substr(0, slashPos);
        else
            wzName = path;

        if (wzName.size() > 3 && wzName.substr(wzName.size() - 3) == ".wz")
            wzName = wzName.substr(0, wzName.size() - 3);

        auto wzSource = GetWzSource(wzName);
        if (!wzSource || !wzSource->LoadImage(img.get()))
            return nullptr;
    }

    // Create a root WzProperty wrapping the image's properties
    auto rootProp = std::make_shared<WzProperty>(img->GetName());
    for (const auto& [name, child] : img->GetProperties())
    {
        rootProp->AddChild(child);
    }
    return rootProp;
}

auto WzResMan::LoadWzFile(const std::string& name) -> bool
{
    // Check if already loaded
    if (m_wzSources.find(name) != m_wzSources.end())
        return true;

    // Try both .wz file and package directory
    std::string filePath = m_sBasePath + "/" + name + ".wz";
    std::string dirPath = m_sBasePath + "/" + name;

    std::string path;
    if (std::filesystem::exists(filePath))
    {
        path = filePath;
    }
    else if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath))
    {
        path = dirPath;
    }
    else
    {
        // WZ source not found
        return false;
    }

    auto wzSource = WzSourceFactory::CreateAndOpen(path);
    if (!wzSource)
    {
        // Failed to load WZ source
        return false;
    }

    m_wzSources[name] = std::move(wzSource);
    return true;
}

void WzResMan::FlushCachedObjects(int flags)
{
    // Based on IWzResMan::FlushCachedObjects
    // flags: 0 = flush all, other values may be used for selective flushing
    if (flags == 0)
    {
        // Flush all cached nodes
        m_nodeCache.clear();
        // Flushed all cached objects
    }
}

auto WzResMan::GetWzVersion(const std::string& name) const -> std::int32_t
{
    auto it = m_wzVersions.find(name);
    if (it != m_wzVersions.end())
        return it->second;
    return 0;
}

auto WzResMan::GetWzSource(const std::string& name) -> std::shared_ptr<IWzSource>
{
    auto it = m_wzSources.find(name);
    if (it != m_wzSources.end())
        return it->second;

    // Try to load the source on demand
    if (LoadWzFile(name))
        return m_wzSources[name];

    return nullptr;
}

auto WzResMan::LoadSoundData(const std::shared_ptr<WzProperty>& prop) -> std::vector<std::uint8_t>
{
    if (!prop)
    {
        return {};
    }

    // Get sound metadata from property
    auto soundData = prop->GetSound();
    if (soundData.size <= 0 || soundData.offset == 0)
    {
        // Property has no sound data
        return {};
    }

    // Navigate up the parent chain to find the WzImage
    auto parent = prop->GetParent().lock();
    std::shared_ptr<WzImage> image;

    while (parent)
    {
        if (parent->GetType() == WzNodeType::Image)
        {
            // Found the image - downcast to WzImage
            image = std::dynamic_pointer_cast<WzImage>(parent);
            break;
        }
        parent = parent->GetParent().lock();
    }

    if (!image)
    {
        // Could not find parent WzImage
        return {};
    }

    // Get the WzFile from the image
    auto wzFile = image->GetWzFile().lock();
    if (!wzFile)
    {
        // Image has no parent WzFile
        return {};
    }

    // Load the raw audio bytes from the WZ file
    return wzFile->LoadSoundData(soundData);
}

} // namespace ms
