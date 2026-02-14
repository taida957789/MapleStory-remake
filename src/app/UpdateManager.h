#pragma once

#include <cstdint>
#include <list>

namespace ms
{

class IPreUpdatable;
class IUpdatable;
class IPostUpdatable;

/**
 * @brief Manages per-frame update callbacks across three phases.
 *
 * Based on UpdateManager from the original MapleStory client.
 * All methods and data are static — there are no instances.
 *
 * Three global ZList's hold non-owning raw pointers:
 *   m_slPreUpdates  — ZList<IPreUpdatable *>
 *   m_slUpdates     — ZList<IUpdatable *>
 *   m_slPostUpdates — ZList<IPostUpdatable *>
 *
 * The game loop calls s_PreUpdate / s_Update / s_PostUpdate each frame.
 */
class UpdateManager
{
public:
    enum CleanUpMsg : std::int32_t
    {
        Clear_AllUpdates  = 0,
        Clear_PreUpdates  = 1,
        Clear_Updates     = 2,
        Clear_PostUpdates = 3,
    };

    // --- Attach (add to tail) ---
    static void s_Attach(IPreUpdatable* pUpdate);
    static void s_Attach(IUpdatable* pUpdate);
    static void s_Attach(IPostUpdatable* pUpdate);

    // --- Detach (find and remove) ---
    static void s_Detach(IPreUpdatable* pUpdate);
    static void s_Detach(IUpdatable* pUpdate);
    static void s_Detach(IPostUpdatable* pUpdate);

    // --- Per-frame dispatch ---
    static void s_PreUpdate();
    static void s_Update();
    static void s_PostUpdate();

    // --- Cleanup ---
    static void s_CleanUp(CleanUpMsg msg);

private:
    UpdateManager() = delete;

    static std::list<IPreUpdatable*>  m_slPreUpdates;
    static std::list<IUpdatable*>     m_slUpdates;
    static std::list<IPostUpdatable*> m_slPostUpdates;
};

} // namespace ms
