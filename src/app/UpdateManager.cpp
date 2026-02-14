#include "UpdateManager.h"

#include "IUpdatable.h"

#include <algorithm>

namespace ms
{

// Global static lists (originally ZList<T*> with file-scope static init)
std::list<IPreUpdatable*>  UpdateManager::m_slPreUpdates;
std::list<IUpdatable*>     UpdateManager::m_slUpdates;
std::list<IPostUpdatable*> UpdateManager::m_slPostUpdates;

// --- Attach ---

void UpdateManager::s_Attach(IPreUpdatable* pUpdate)
{
    if (pUpdate)
        m_slPreUpdates.push_back(pUpdate);
}

void UpdateManager::s_Attach(IUpdatable* pUpdate)
{
    if (pUpdate)
        m_slUpdates.push_back(pUpdate);
}

void UpdateManager::s_Attach(IPostUpdatable* pUpdate)
{
    if (pUpdate)
        m_slPostUpdates.push_back(pUpdate);
}

// --- Detach ---

void UpdateManager::s_Detach(IPreUpdatable* pUpdate)
{
    auto it = std::find(m_slPreUpdates.begin(), m_slPreUpdates.end(), pUpdate);
    if (it != m_slPreUpdates.end())
        m_slPreUpdates.erase(it);
}

void UpdateManager::s_Detach(IUpdatable* pUpdate)
{
    auto it = std::find(m_slUpdates.begin(), m_slUpdates.end(), pUpdate);
    if (it != m_slUpdates.end())
        m_slUpdates.erase(it);
}

void UpdateManager::s_Detach(IPostUpdatable* pUpdate)
{
    auto it = std::find(m_slPostUpdates.begin(), m_slPostUpdates.end(), pUpdate);
    if (it != m_slPostUpdates.end())
        m_slPostUpdates.erase(it);
}

// --- Per-frame dispatch ---

void UpdateManager::s_PreUpdate()
{
    for (auto* p : m_slPreUpdates)
        p->PreUpdate();
}

void UpdateManager::s_Update()
{
    for (auto* p : m_slUpdates)
        p->Update();
}

void UpdateManager::s_PostUpdate()
{
    for (auto* p : m_slPostUpdates)
        p->PostUpdate();
}

// --- Cleanup ---

void UpdateManager::s_CleanUp(CleanUpMsg msg)
{
    switch (msg)
    {
    case Clear_AllUpdates:
        m_slPreUpdates.clear();
        m_slUpdates.clear();
        m_slPostUpdates.clear();
        break;
    case Clear_PreUpdates:
        m_slPreUpdates.clear();
        break;
    case Clear_Updates:
        m_slUpdates.clear();
        break;
    case Clear_PostUpdates:
        m_slPostUpdates.clear();
        break;
    }
}

} // namespace ms
