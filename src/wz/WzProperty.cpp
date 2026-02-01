#include "WzProperty.h"
#include "WzCanvas.h"

namespace ms
{

WzProperty::WzProperty() = default;

WzProperty::WzProperty(const std::string& name)
    : m_sName(name)
{
}

WzProperty::~WzProperty() = default;

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

auto WzProperty::GetCanvas() const -> std::shared_ptr<WzCanvas>
{
    // Trigger lazy loading if needed (canvas may be loaded lazily)
    EnsureLoaded();

    if (const auto* val = std::get_if<std::shared_ptr<WzCanvas>>(&m_value))
        return *val;
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
}

void WzProperty::SetLong(std::int64_t value)
{
    m_value = value;
}

void WzProperty::SetFloat(float value)
{
    m_value = value;
}

void WzProperty::SetDouble(double value)
{
    m_value = value;
}

void WzProperty::SetString(const std::string& value)
{
    m_value = value;
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
        m_children[child->GetName()] = std::move(child);
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
