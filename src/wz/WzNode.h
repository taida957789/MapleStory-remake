#pragma once

#include "WzTypes.h"
#include <memory>
#include <string>

namespace ms
{

/**
 * @brief Abstract base class for all WZ hierarchy nodes
 *
 * WzNode provides the foundation for the WZ file hierarchy, supporting:
 * - Name storage and retrieval
 * - Parent/child relationships via weak_ptr to avoid circular references
 * - Full path resolution from root to current node
 * - Type identification via pure virtual GetType()
 *
 * All node types (WzDirectory, WzImage, WzProperty derivatives) inherit from WzNode.
 * The class uses std::enable_shared_from_this to allow derived classes to safely
 * create shared_ptr references to themselves.
 *
 * Thread safety: WzNode is not thread-safe. Synchronization must be handled externally.
 */
class WzNode : public std::enable_shared_from_this<WzNode>
{
public:
    /**
     * @brief Construct a node with the given name
     * @param name Node name (not the full path, just the name)
     */
    explicit WzNode(std::string name);

    /**
     * @brief Default constructor (creates node with empty name)
     */
    WzNode() = default;

    /**
     * @brief Virtual destructor
     */
    virtual ~WzNode() = default;

    // Non-copyable
    WzNode(const WzNode&) = delete;
    WzNode& operator=(const WzNode&) = delete;

    // Movable
    WzNode(WzNode&&) noexcept = default;
    WzNode& operator=(WzNode&&) noexcept = default;

    /**
     * @brief Get the node type
     * @return WzNodeType enum value
     */
    [[nodiscard]] virtual auto GetType() const noexcept -> WzNodeType = 0;

    /**
     * @brief Get the node's name
     * @return Node name (not full path)
     */
    [[nodiscard]] auto GetName() const noexcept -> const std::string&;

    /**
     * @brief Set the node's name
     * @param name New node name
     */
    auto SetName(std::string name) noexcept -> void;

    /**
     * @brief Get the parent node
     * @return weak_ptr to parent (may be expired if parent was destroyed)
     */
    [[nodiscard]] auto GetParent() const noexcept -> std::weak_ptr<WzNode>;

    /**
     * @brief Set the parent node
     * @param parent shared_ptr to parent node
     */
    auto SetParent(std::shared_ptr<WzNode> parent) noexcept -> void;

    /**
     * @brief Get the full path from root to this node
     * @return Full path using "/" as separator (e.g., "Root/Child/GrandChild")
     */
    [[nodiscard]] auto GetPath() const -> std::string;

protected:
    std::string m_name;                    ///< Node name
    std::weak_ptr<WzNode> m_parent;        ///< Parent node (weak to avoid circular references)
};

} // namespace ms
