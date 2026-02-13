#pragma once

#include "WzNode.h"
#include <map>
#include <memory>
#include <string>

namespace ms
{

/**
 * @brief Directory node that manages child nodes in the WZ hierarchy
 *
 * WzDirectory is a container node that holds child nodes (directories, images, properties).
 * It provides:
 * - Child node management (add, get, remove)
 * - Name-based child lookup
 * - Automatic parent-child relationship management
 * - Iterator access to all children
 *
 * Children are stored in a map by name and owned via shared_ptr.
 * When adding a child, the directory automatically sets itself as the child's parent.
 *
 * Thread safety: WzDirectory is not thread-safe. Synchronization must be handled externally.
 */
class WzDirectory : public WzNode
{
public:
    /**
     * @brief Construct a directory with the given name
     * @param name Directory name
     */
    explicit WzDirectory(std::string name);

    /**
     * @brief Virtual destructor
     */
    ~WzDirectory() override = default;

    // Non-copyable
    WzDirectory(const WzDirectory&) = delete;
    WzDirectory& operator=(const WzDirectory&) = delete;

    // Movable
    WzDirectory(WzDirectory&&) noexcept = default;
    WzDirectory& operator=(WzDirectory&&) noexcept = default;

    /**
     * @brief Get the node type
     * @return WzNodeType::Directory
     */
    [[nodiscard]] auto GetType() const noexcept -> WzNodeType override;

    /**
     * @brief Add a child node to this directory
     * @param child shared_ptr to the child node to add
     *
     * The child's parent is automatically set to this directory.
     * If a child with the same name already exists, it will be replaced.
     */
    auto AddChild(std::shared_ptr<WzNode> child) -> void;

    /**
     * @brief Get a child node by name
     * @param name Name of the child to retrieve
     * @return shared_ptr to the child, or nullptr if not found
     */
    [[nodiscard]] auto GetChild(const std::string& name) const -> std::shared_ptr<WzNode>;

    /**
     * @brief Get a child node by name (operator[] overload)
     * @param name Name of the child to retrieve
     * @return shared_ptr to the child, or nullptr if not found
     */
    [[nodiscard]] auto operator[](const std::string& name) const -> std::shared_ptr<WzNode>;

    /**
     * @brief Remove a child node by name
     * @param name Name of the child to remove
     * @return true if the child was removed, false if not found
     */
    auto RemoveChild(const std::string& name) -> bool;

    /**
     * @brief Remove all children from this directory
     */
    auto Clear() -> void;

    /**
     * @brief Get all children
     * @return const reference to the children map
     */
    [[nodiscard]] auto GetChildren() const noexcept -> const std::map<std::string, std::shared_ptr<WzNode>>&;

    /**
     * @brief Get the number of children
     * @return Number of child nodes
     */
    [[nodiscard]] auto GetChildCount() const noexcept -> std::size_t;

private:
    std::map<std::string, std::shared_ptr<WzNode>> m_children;  ///< Child nodes by name
};

} // namespace ms
