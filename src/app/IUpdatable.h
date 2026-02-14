#pragma once

namespace ms
{

/**
 * @brief Interface for objects in the pre-update phase.
 *
 * Objects implementing this interface and attached via
 * UpdateManager::s_Attach receive PreUpdate() calls each frame
 * before the main update phase.
 *
 * VFT layout (IPreUpdatable_vtbl, 8 bytes):
 *   0x00  ~IPreUpdatable
 *   0x04  PreUpdate
 */
class IPreUpdatable
{
public:
    virtual ~IPreUpdatable() = default;

    virtual void PreUpdate() = 0;
};

/**
 * @brief Interface for objects in the main update phase.
 *
 * Objects implementing this interface and attached via
 * UpdateManager::s_Attach receive Update() calls each frame.
 *
 * VFT layout (IUpdatable_vtbl, 8 bytes):
 *   0x00  ~IUpdatable
 *   0x04  Update
 */
class IUpdatable
{
public:
    virtual ~IUpdatable() = default;

    virtual void Update() = 0;
};

/**
 * @brief Interface for objects in the post-update phase.
 *
 * Objects implementing this interface and attached via
 * UpdateManager::s_Attach receive PostUpdate() calls each frame
 * after the main update phase.
 *
 * VFT layout (IPostUpdatable_vtbl, 8 bytes):
 *   0x00  ~IPostUpdatable
 *   0x04  PostUpdate
 */
class IPostUpdatable
{
public:
    virtual ~IPostUpdatable() = default;

    virtual void PostUpdate() = 0;
};

} // namespace ms
