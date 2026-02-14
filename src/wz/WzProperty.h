#pragma once

#include "WzNode.h"
#include "WzTypes.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>

namespace ms
{

class WzCanvas;
class WzFile;
class WzRaw;
class WzVideo;
class IWzSource;

/**
 * @brief WZ Property node
 *
 * Based on IWzProperty interface from the original MapleStory client.
 * GUID: 986515d9-0a0b-4929-8b4f-718682177b92
 *
 * Represents a node in the WZ file structure.
 * Can contain:
 * - Integer values
 * - String values
 * - Float/Double values
 * - Canvas (image) data
 * - Vector (point) data
 * - Sound data
 * - UOL (link to another property)
 * - Child properties (sub-nodes)
 */
class WzProperty : public WzNode
{
public:
    using Value = std::variant<
        std::monostate,
        std::int32_t,
        std::int64_t,
        float,
        double,
        std::string,
        std::shared_ptr<WzCanvas>,
        std::shared_ptr<WzRaw>,
        std::shared_ptr<WzVideo>,
        WzVector2D,
        WzSoundData>;

    WzProperty();
    explicit WzProperty(const std::string& name);
    ~WzProperty() override;

    // Non-copyable, movable
    WzProperty(const WzProperty&) = delete;
    auto operator=(const WzProperty&) -> WzProperty& = delete;
    WzProperty(WzProperty&&) noexcept = default;
    auto operator=(WzProperty&&) noexcept -> WzProperty& = default;

    // Override GetType from WzNode
    [[nodiscard]] auto GetType() const noexcept -> WzNodeType override;

    // Value getters
    [[nodiscard]] auto GetInt(std::int32_t defaultValue = 0) const -> std::int32_t;
    [[nodiscard]] auto GetLong(std::int64_t defaultValue = 0) const -> std::int64_t;
    [[nodiscard]] auto GetFloat(float defaultValue = 0.0F) const -> float;
    [[nodiscard]] auto GetDouble(double defaultValue = 0.0) const -> double;
    [[nodiscard]] auto GetString(const std::string& defaultValue = "") const -> std::string;
    [[nodiscard]] auto GetCanvas() const -> std::shared_ptr<WzCanvas>;
    [[nodiscard]] auto GetVector() const -> WzVector2D;
    [[nodiscard]] auto GetSound() const -> WzSoundData;

    // Node type
    [[nodiscard]] auto GetNodeType() const noexcept -> WzNodeType { return m_nodeType; }
    void SetNodeType(WzNodeType type) noexcept { m_nodeType = type; }

    // Check if this is a UOL (link) node
    [[nodiscard]] auto IsUOL() const noexcept -> bool { return m_nodeType == WzNodeType::UOL; }

    // Value setters
    void SetInt(std::int32_t value);
    void SetLong(std::int64_t value);
    void SetFloat(float value);
    void SetDouble(double value);
    void SetString(const std::string& value);
    void SetCanvas(std::shared_ptr<WzCanvas> canvas);
    void SetVector(const WzVector2D& vec);
    void SetVector(std::int32_t x, std::int32_t y);
    void SetSound(const WzSoundData& sound);
    void SetRaw(std::shared_ptr<WzRaw> raw);
    [[nodiscard]] auto GetRaw() const -> std::shared_ptr<WzRaw>;
    void SetVideo(std::shared_ptr<WzVideo> video);
    [[nodiscard]] auto GetVideo() const -> std::shared_ptr<WzVideo>;

    // Child access
    [[nodiscard]] auto GetChild(const std::string& name) -> std::shared_ptr<WzProperty>;
    [[nodiscard]] auto operator[](const std::string& name) -> std::shared_ptr<WzProperty>;
    void AddChild(std::shared_ptr<WzProperty> child);

    // Iteration
    [[nodiscard]] auto GetChildren()
        -> const std::map<std::string, std::shared_ptr<WzProperty>>&
    {
        EnsureLoaded();
        return m_children;
    }

    [[nodiscard]] auto GetChildCount() const -> std::size_t
    {
        EnsureLoaded();
        return m_children.size();
    }

    [[nodiscard]] auto HasChildren() const -> bool
    {
        EnsureLoaded();
        return !m_children.empty();
    }

    // Lazy loading support
    using LoadCallback = std::function<bool(std::shared_ptr<WzProperty>, std::size_t)>;

    void SetLoadInfo(std::size_t offset, WzFile* file);
    void SetLoadCallback(LoadCallback callback) { m_loadCallback = std::move(callback); }
    [[nodiscard]] auto NeedsLoad() const noexcept -> bool { return m_bNeedsLoad; }
    void SetLoaded() noexcept { m_bNeedsLoad = false; }
    [[nodiscard]] auto GetOffset() const noexcept -> std::size_t { return m_nOffset; }
    [[nodiscard]] auto GetWzFile() const noexcept -> WzFile* { return m_pWzFile; }

    /**
     * @brief Set WzFile pointer without enabling lazy loading
     *
     * Used for properties that need access to the WzFile for loading
     * data (like sounds) but don't need lazy loading themselves.
     */
    void SetWzFile(WzFile* file) noexcept { m_pWzFile = file; }

    /**
     * @brief Set IWzSource pointer for outlink resolution
     *
     * Used by WzPackage to enable cross-package outlink resolution.
     */
    void SetWzSource(IWzSource* source) noexcept { m_pWzSource = source; }

private:
    void EnsureLoaded() const;
    auto ResolveOutlinkCanvas(const std::string& outlinkPath) const -> std::shared_ptr<WzCanvas>;
    auto ResolveInlinkCanvas(const std::string& inlinkPath) const -> std::shared_ptr<WzCanvas>;

    Value m_value;
    WzNodeType m_nodeType{WzNodeType::NotSet};
    std::map<std::string, std::shared_ptr<WzProperty>> m_children;

    // Lazy loading (mutable because loading doesn't change logical state)
    mutable bool m_bNeedsLoad{false};
    std::size_t m_nOffset{0};
    WzFile* m_pWzFile{nullptr};
    IWzSource* m_pWzSource{nullptr};  // For outlink resolution in packages
    mutable LoadCallback m_loadCallback;
};

} // namespace ms
