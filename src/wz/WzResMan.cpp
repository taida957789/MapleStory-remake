#include "WzResMan.h"
#include "WzFile.h"
#include "WzProperty.h"
#include "WzTypes.h"
#include "util/Logger.h"

#include <filesystem>
#include <vector>

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

    LOG_INFO("Initializing WZ Resource Manager...");

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
        LOG_ERROR("Failed to initialize Base.wz");
        // Continue anyway - Base.wz may not exist in all clients
    }

    // Step 2: Load all WZ files in order
    // Based on CWvsApp::InitializeResMan - asNameOrder array
    if (!InitializeWzFiles())
    {
        LOG_WARN("Some WZ files failed to load");
        // Continue anyway - some files may be optional
    }

    m_bInitialized = true;
    LOG_INFO("WZ Resource Manager initialized");
    return true;
}

void WzResMan::Shutdown()
{
    m_propertyCache.clear();
    m_wzFiles.clear();
    m_wzVersions.clear();
    m_bInitialized = false;
}

auto WzResMan::InitializeBase() -> bool
{
    // Load Base.wz
    auto basePath = m_sBasePath + "/Base.wz";

    if (!std::filesystem::exists(basePath))
    {
        LOG_ERROR("Base.wz not found at: {}", basePath);
        return false;
    }

    auto baseFile = std::make_shared<WzFile>();
    if (!baseFile->Open(basePath))
    {
        LOG_ERROR("Failed to open Base.wz");
        return false;
    }

    m_wzFiles["Base"] = baseFile;
    LOG_INFO("Loaded Base.wz (version {})", baseFile->GetVersion());

    // Parse Version.img to get version info for all WZ files
    // Based on CWvsApp::InitializeResMan - pVersion = GetObjectA("Version.img")
    auto versionImg = baseFile->FindNode("Version.img");
    if (versionImg)
    {
        // Iterate through children to get version numbers
        // Each child is named after a WZ file (e.g., "UI", "Map")
        // and contains an integer version number
        for (const auto& [name, child] : versionImg->GetChildren())
        {
            auto version = child->GetInt();
            if (version != 0)
            {
                m_wzVersions[name] = version;
                LOG_DEBUG("  Version info: {} = {}", name, version);
            }
        }
    }
    else
    {
        LOG_DEBUG("Version.img not found in Base.wz");
    }

    return true;
}

auto WzResMan::InitializeWzFiles() -> bool
{
    // Load WZ files in order defined by asNameOrder
    // Based on CWvsApp::InitializeResMan
    bool allLoaded = true;

    for (const auto* wzName : WZ_LOAD_ORDER)
    {
        auto filePath = m_sBasePath + "/" + wzName + ".wz";

        if (!std::filesystem::exists(filePath))
        {
            LOG_DEBUG("WZ file not found (skipping): {}.wz", wzName);
            continue;
        }

        if (!LoadWzFile(wzName))
        {
            LOG_ERROR("Failed to load: {}.wz", wzName);
            allLoaded = false;
        }
    }

    return allLoaded;
}

auto WzResMan::GetProperty(const std::string& path) -> std::shared_ptr<WzProperty>
{
    // Check cache first
    const auto cacheIt = m_propertyCache.find(path);
    if (cacheIt != m_propertyCache.end())
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

    // Get or load WZ file
    auto wzFile = GetWzFile(wzName);
    if (!wzFile)
    {
        // Return empty property if file not found
        auto prop = std::make_shared<WzProperty>();
        m_propertyCache[path] = prop;
        return prop;
    }

    // Find node in WZ file
    std::shared_ptr<WzProperty> prop;
    if (subPath.empty())
    {
        prop = wzFile->GetRoot();
    }
    else
    {
        prop = wzFile->FindNode(subPath);
    }

    if (!prop)
    {
        prop = std::make_shared<WzProperty>();
    }

    m_propertyCache[path] = prop;
    return prop;
}

auto WzResMan::LoadWzFile(const std::string& name) -> bool
{
    // Check if already loaded
    if (m_wzFiles.find(name) != m_wzFiles.end())
        return true;

    auto wzFile = std::make_shared<WzFile>();
    auto filePath = m_sBasePath + "/" + name + ".wz";

    if (!wzFile->Open(filePath))
    {
        LOG_ERROR("Failed to load WZ file: {}", filePath);
        return false;
    }

    m_wzFiles[name] = wzFile;
    LOG_INFO("Loaded WZ file: {}.wz (version {})", name, wzFile->GetVersion());
    return true;
}

void WzResMan::FlushCachedObjects(int flags)
{
    // Based on IWzResMan::FlushCachedObjects
    // flags: 0 = flush all, other values may be used for selective flushing
    if (flags == 0)
    {
        // Flush all cached properties
        m_propertyCache.clear();
        LOG_DEBUG("Flushed all cached objects");
    }
}

auto WzResMan::GetWzVersion(const std::string& name) const -> std::int32_t
{
    auto it = m_wzVersions.find(name);
    if (it != m_wzVersions.end())
        return it->second;
    return 0;
}

auto WzResMan::GetWzFile(const std::string& name) -> std::shared_ptr<WzFile>
{
    auto it = m_wzFiles.find(name);
    if (it != m_wzFiles.end())
        return it->second;

    // Try to load the file on demand
    if (LoadWzFile(name))
        return m_wzFiles[name];

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
        LOG_DEBUG("Property has no sound data");
        return {};
    }

    // Get the parent WzFile from the property
    auto* wzFile = prop->GetWzFile();
    if (!wzFile)
    {
        LOG_ERROR("Property has no parent WzFile");
        return {};
    }

    // Load the raw audio bytes from the WZ file
    return wzFile->LoadSoundData(soundData);
}

} // namespace ms
