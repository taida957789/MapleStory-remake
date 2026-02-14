#include "WzProperty.h"
#include "WzCanvas.h"
#include "WzDirectory.h"
#include "WzFile.h"
#include "WzImage.h"
#include "WzRaw.h"
#include "WzVideo.h"
#include "IWzSource.h"
#include "util/Logger.h"

namespace ms
{

WzProperty::WzProperty() = default;

WzProperty::WzProperty(const std::string& name)
    : WzNode(name)
{
}

WzProperty::~WzProperty() = default;

auto WzProperty::GetType() const noexcept -> WzNodeType
{
    return WzNodeType::Property;
}

auto WzProperty::GetInt(std::int32_t defaultValue) const -> std::int32_t
{
    if (const auto* val = std::get_if<std::int32_t>(&m_value))
        return *val;
    if (const auto* val = std::get_if<std::int64_t>(&m_value))
        return static_cast<std::int32_t>(*val);
    return defaultValue;
}

auto WzProperty::GetLong(std::int64_t defaultValue) const -> std::int64_t
{
    if (const auto* val = std::get_if<std::int64_t>(&m_value))
        return *val;
    if (const auto* val = std::get_if<std::int32_t>(&m_value))
        return static_cast<std::int64_t>(*val);
    return defaultValue;
}

auto WzProperty::GetFloat(float defaultValue) const -> float
{
    if (const auto* val = std::get_if<float>(&m_value))
        return *val;
    if (const auto* val = std::get_if<double>(&m_value))
        return static_cast<float>(*val);
    return defaultValue;
}

auto WzProperty::GetDouble(double defaultValue) const -> double
{
    if (const auto* val = std::get_if<double>(&m_value))
        return *val;
    if (const auto* val = std::get_if<float>(&m_value))
        return static_cast<double>(*val);
    return defaultValue;
}

auto WzProperty::GetString(const std::string& defaultValue) const -> std::string
{
    if (const auto* val = std::get_if<std::string>(&m_value))
        return *val;
    return defaultValue;
}

auto WzProperty::ResolveOutlinkCanvas(const std::string& outlinkPath) const -> std::shared_ptr<WzCanvas>
{
    // Try WzFile first (single-file format)
    if (m_pWzFile)
    {
        if (auto targetNode = m_pWzFile->FindNode(outlinkPath))
        {
            if (auto targetProp = std::dynamic_pointer_cast<WzProperty>(targetNode))
            {
                if (targetProp->GetNodeType() == WzNodeType::Canvas)
                {
                    return targetProp->GetCanvas();  // Recursive resolution
                }
            }
        }
    }

    // Try IWzSource (package format with cross-package outlinks)
    if (m_pWzSource)
    {
        // Strip package name prefix if present (e.g., "UI/_Canvas/..." -> "_Canvas/...")
        std::string relativePath = outlinkPath;
        if (auto root = m_pWzSource->GetRoot())
        {
            const std::string rootName = root->GetName();
            if (outlinkPath.starts_with(rootName + "/") || outlinkPath.starts_with(rootName + ".wz/"))
            {
                auto slashPos = outlinkPath.find('/');
                relativePath = outlinkPath.substr(slashPos + 1);
            }
        }

        if (auto targetNode = m_pWzSource->FindNode(relativePath))
        {
            if (auto targetProp = std::dynamic_pointer_cast<WzProperty>(targetNode))
            {
                if (targetProp->GetNodeType() == WzNodeType::Canvas)
                {
                    return targetProp->GetCanvas();  // Recursive resolution
                }
            }
        }
    }

    return nullptr;
}

auto WzProperty::ResolveInlinkCanvas(const std::string& inlinkPath) const -> std::shared_ptr<WzCanvas>
{
    // _inlink is relative to WzImage (img file level)
    // Walk up to find WzImage ancestor
    std::shared_ptr<WzNode> node = GetParent().lock();
    while (node)
    {
        if (node->GetType() == WzNodeType::Image)
            break;
        node = node->GetParent().lock();
    }

    if (!node)
        return nullptr;

    auto img = std::dynamic_pointer_cast<WzImage>(node);
    if (!img)
        return nullptr;

    // Navigate the path from WzImage level (e.g., "Wizet/24")
    std::shared_ptr<WzProperty> current;
    std::string::size_type start = 0;
    std::string::size_type end;

    // First segment: use WzImage::GetProperty
    end = inlinkPath.find('/', start);
    if (end != std::string::npos)
    {
        current = img->GetProperty(inlinkPath.substr(start, end - start));
        start = end + 1;
    }
    else
    {
        current = img->GetProperty(inlinkPath);
    }

    // Remaining segments: use WzProperty::GetChild
    while (current && end != std::string::npos)
    {
        end = inlinkPath.find('/', start);
        auto segment = (end != std::string::npos)
            ? inlinkPath.substr(start, end - start)
            : inlinkPath.substr(start);
        if (!segment.empty())
        {
            current = current->GetChild(segment);
        }
        start = (end != std::string::npos) ? end + 1 : inlinkPath.size();
    }

    if (current)
    {
        return current->GetCanvas();  // Recursive resolution
    }

    return nullptr;
}

auto WzProperty::GetCanvas() const -> std::shared_ptr<WzCanvas>
{
    EnsureLoaded();

    // _outlink takes precedence (absolute path, cross-package)
    auto outlinkIt = m_children.find("_outlink");
    if (outlinkIt != m_children.end())
    {
        if (auto outlinkProp = outlinkIt->second)
        {
            if (outlinkProp->GetNodeType() == WzNodeType::String)
            {
                if (auto resolved = ResolveOutlinkCanvas(outlinkProp->GetString()))
                {
                    const_cast<WzProperty*>(this)->m_value = resolved;
                    return resolved;
                }
            }
        }
    }

    // _inlink next (relative path within same img)
    auto inlinkIt = m_children.find("_inlink");
    if (inlinkIt != m_children.end())
    {
        if (auto inlinkProp = inlinkIt->second)
        {
            if (inlinkProp->GetNodeType() == WzNodeType::String)
            {
                if (auto resolved = ResolveInlinkCanvas(inlinkProp->GetString()))
                {
                    const_cast<WzProperty*>(this)->m_value = resolved;
                    return resolved;
                }
            }
        }
    }

    // Fallback to direct canvas value
    if (const auto* val = std::get_if<std::shared_ptr<WzCanvas>>(&m_value))
    {
        return *val;
    }

    return nullptr;
}

auto WzProperty::GetVector() const -> WzVector2D
{
    if (const auto* val = std::get_if<WzVector2D>(&m_value))
        return *val;
    return WzVector2D{};
}

auto WzProperty::GetSound() const -> WzSoundData
{
    if (const auto* val = std::get_if<WzSoundData>(&m_value))
        return *val;
    return WzSoundData{};
}

void WzProperty::SetInt(std::int32_t value)
{
    m_value = value;
    m_nodeType = WzNodeType::Int;
}

void WzProperty::SetLong(std::int64_t value)
{
    m_value = value;
    m_nodeType = WzNodeType::Int;
}

void WzProperty::SetFloat(float value)
{
    m_value = value;
    m_nodeType = WzNodeType::Float;
}

void WzProperty::SetDouble(double value)
{
    m_value = value;
    m_nodeType = WzNodeType::Double;
}

void WzProperty::SetString(const std::string& value)
{
    m_value = value;
    m_nodeType = WzNodeType::String;
}

void WzProperty::SetCanvas(std::shared_ptr<WzCanvas> canvas)
{
    m_value = std::move(canvas);
    m_nodeType = WzNodeType::Canvas;
}

void WzProperty::SetVector(const WzVector2D& vec)
{
    m_value = vec;
    m_nodeType = WzNodeType::Vector2D;
}

void WzProperty::SetVector(std::int32_t x, std::int32_t y)
{
    m_value = WzVector2D{x, y};
    m_nodeType = WzNodeType::Vector2D;
}

void WzProperty::SetSound(const WzSoundData& sound)
{
    m_value = sound;
    m_nodeType = WzNodeType::Sound;
}

void WzProperty::SetRaw(std::shared_ptr<WzRaw> raw)
{
    m_value = std::move(raw);
    m_nodeType = WzNodeType::RawData;
}

auto WzProperty::GetRaw() const -> std::shared_ptr<WzRaw>
{
    if (const auto* val = std::get_if<std::shared_ptr<WzRaw>>(&m_value))
        return *val;
    return nullptr;
}

void WzProperty::SetVideo(std::shared_ptr<WzVideo> video)
{
    m_value = std::move(video);
    m_nodeType = WzNodeType::Video;
}

auto WzProperty::GetVideo() const -> std::shared_ptr<WzVideo>
{
    if (const auto* val = std::get_if<std::shared_ptr<WzVideo>>(&m_value))
        return *val;
    return nullptr;
}

auto WzProperty::GetChild(const std::string& name) -> std::shared_ptr<WzProperty>
{
    // Trigger lazy loading if needed
    EnsureLoaded();

    const auto it = m_children.find(name);
    if (it != m_children.end())
        return it->second;
    return nullptr;
}

auto WzProperty::operator[](const std::string& name) -> std::shared_ptr<WzProperty>
{
    return GetChild(name);
}

void WzProperty::AddChild(std::shared_ptr<WzProperty> child)
{
    if (child)
    {
        // Only set parent if this object is managed by a shared_ptr
        // (i.e., if weak_from_this() would succeed)
        try
        {
            // Try to get a shared_ptr to this object
            auto self = shared_from_this();
            child->SetParent(self);
        }
        catch (const std::bad_weak_ptr&)
        {
            // This object is not managed by shared_ptr (e.g., stack object)
            // Don't set parent in this case - tests may use stack objects
        }
        m_children[child->GetName()] = std::move(child);
    }
}

void WzProperty::SetLoadInfo(std::size_t offset, WzFile* file)
{
    m_nOffset = offset;
    m_pWzFile = file;
    m_bNeedsLoad = true;
}

void WzProperty::EnsureLoaded() const
{
    if (!m_bNeedsLoad)
        return;

    if (m_loadCallback)
    {
        // Create a temporary shared_ptr that doesn't own this object
        // The callback will populate the children
        // Note: const_cast is safe here because lazy loading doesn't change logical state
        struct NoDelete
        {
            void operator()(WzProperty*) const {}
        };
        auto self = std::shared_ptr<WzProperty>(const_cast<WzProperty*>(this), NoDelete{});
        if (m_loadCallback(self, m_nOffset))
        {
            m_bNeedsLoad = false;
        }
    }
    else
    {
        // No callback, just mark as loaded
        m_bNeedsLoad = false;
    }
}

} // namespace ms
