#include "CameraCtrl.h"

#include "app/Application.h"
#include "app/UpdateManager.h"
#include "graphics/WzGr2D.h"

#include <cmath>

namespace ms
{

// -----------------------------------------------------------------------
// Helper: get current update time (matches original ::get_update_time)
// -----------------------------------------------------------------------

static auto get_update_time() -> std::int32_t
{
    return static_cast<std::int32_t>(Application::GetInstance().GetUpdateTime());
}

// -----------------------------------------------------------------------
// Interpolation functions
// Matching IDA decompilation at 0x666550–0x666908.
// All compute t = clamp((currentTime - tStart) / tDelay, 0, 1),
// then return easing(t) * (nEnd - nStart).
// Power is hardcoded to 5.0 for Poly/MinusPoly/MixPoly.
// -----------------------------------------------------------------------

namespace Interpolation
{

static auto ClampedT(std::int32_t tStart, std::int32_t tDelay) -> float
{
    if (tDelay == 0) return 1.0F;
    auto t = static_cast<float>(get_update_time() - tStart)
           / static_cast<float>(tDelay);
    if (t < 0.0F) return 0.0F;
    if (t > 1.0F) return 1.0F;
    return t;
}

auto s_Linear(std::int32_t tStart, std::int32_t tDelay,
              float nStart, float nEnd) -> float
{
    const auto range = nEnd - nStart;
    if (tDelay == 0) return range;
    return ClampedT(tStart, tDelay) * range;
}

auto s_Sine(std::int32_t tStart, std::int32_t tDelay,
            float nStart, float nEnd) -> float
{
    constexpr auto kHalfPi = 3.14159265358979323846F * 0.5F;
    const auto range = nEnd - nStart;
    if (tDelay == 0) return range;
    const auto t = ClampedT(tStart, tDelay);
    return std::sin(t * kHalfPi) * range;
}

auto s_Cosine(std::int32_t tStart, std::int32_t tDelay,
              float nStart, float nEnd) -> float
{
    constexpr auto kHalfPi = 3.14159265358979323846F * 0.5F;
    const auto range = nEnd - nStart;
    if (tDelay == 0) return range;
    const auto t = ClampedT(tStart, tDelay);
    return (1.0F - std::cos(t * kHalfPi)) * range;
}

auto s_Polynomial(std::int32_t tStart, std::int32_t tDelay,
                  float nStart, float nEnd) -> float
{
    const auto range = nEnd - nStart;
    if (tDelay == 0) return range;
    const auto t = ClampedT(tStart, tDelay);
    return std::pow(t, 5.0F) * range;
}

auto s_MinusPoly(std::int32_t tStart, std::int32_t tDelay,
                 float nStart, float nEnd) -> float
{
    const auto range = nEnd - nStart;
    if (tDelay == 0) return range;
    const auto t = ClampedT(tStart, tDelay);
    return (std::pow(t - 1.0F, 5.0F) + 1.0F) * range;
}

auto s_MixPoly(std::int32_t tStart, std::int32_t tDelay,
               float nStart, float nEnd) -> float
{
    const auto range = nEnd - nStart;
    if (tDelay == 0) return range;

    const auto halfDelay = static_cast<std::int32_t>(
        static_cast<float>(tDelay) * 0.5F);
    const auto halfRange = 0.5F * range;

    // Check if we're past the halfway point
    const auto elapsed = static_cast<float>(get_update_time() - tStart);
    if (elapsed - static_cast<float>(halfDelay) > 0.0F)
    {
        // Second half: MinusPoly from midpoint to end
        const auto midStart = nStart + halfRange;
        return halfRange + s_MinusPoly(tStart + halfDelay, halfDelay,
                                       midStart, nEnd);
    }

    // First half: Polynomial from start to midpoint
    return s_Polynomial(tStart, halfDelay, nStart, nStart + halfRange);
}

} // namespace Interpolation

// -----------------------------------------------------------------------
// CameraCtrl::Command
// -----------------------------------------------------------------------

namespace CameraCtrl
{

auto Command::ValidateCmd() -> bool
{
    if (tDelay < 0)
        return false;

    switch (type)
    {
    case Interpolation::Linear:
        m_pInterpolation = Interpolation::s_Linear;
        break;
    case Interpolation::Sine:
        m_pInterpolation = Interpolation::s_Sine;
        break;
    case Interpolation::Cosine:
        m_pInterpolation = Interpolation::s_Cosine;
        break;
    case Interpolation::Polynomial:
        m_pInterpolation = Interpolation::s_Polynomial;
        break;
    case Interpolation::MinusPoly:
        m_pInterpolation = Interpolation::s_MinusPoly;
        break;
    case Interpolation::MixPoly:
        m_pInterpolation = Interpolation::s_MixPoly;
        break;
    default:
        return false;
    }

    return m_pInterpolation != nullptr;
}

auto Command::Begin() -> bool
{
    if (!ValidateCmd())
        return false;

    bBegin = true;
    tStart = get_update_time();

    // Back up current camera state from center vector
    auto* pVec = get_gr().GetCenterVec();

    // Absolute position (center x, y)
    ptAbs_Backup.x = pVec->GetX();
    ptAbs_Backup.y = pVec->GetY();

    // Relative position (rx, ry)
    ptRel_Backup.x = pVec->GetRX();
    ptRel_Backup.y = pVec->GetRY();

    // Screen scale backup (TODO: implement GetScreenScale in WzGr2D)
    nScale_Backup = 0;

    return true;
}

// -----------------------------------------------------------------------
// Command subclass Update methods
// Each returns true when the command is finished (tStart + tDelay <= now).
// Full interpolation logic is stubbed — only completion check for now.
// -----------------------------------------------------------------------

auto AbsMoveCommand::Update() -> bool
{
    if (!m_pInterpolation)
        return true;

    // Compute target relative position
    const auto targetRX = ptRel_Backup.x + ptDest.x - ptAbs_Backup.x;
    const auto targetRY = ptRel_Backup.y + ptDest.y - ptAbs_Backup.y;

    // Interpolate offset from backup
    const auto fX = static_cast<float>(ptRel_Backup.x) +
        m_pInterpolation(tStart, tDelay,
                         static_cast<float>(ptRel_Backup.x),
                         static_cast<float>(targetRX));
    const auto fY = static_cast<float>(ptRel_Backup.y) +
        m_pInterpolation(tStart, tDelay,
                         static_cast<float>(ptRel_Backup.y),
                         static_cast<float>(targetRY));

    // Apply via RelMove on center vector
    auto* pVec = get_gr().GetCenterVec();
    pVec->RelMove(static_cast<std::int32_t>(fX),
                  static_cast<std::int32_t>(fY));

    return tDelay + tStart - get_update_time() <= 0;
}

auto RelMoveCommand::Update() -> bool
{
    if (!m_pInterpolation)
        return true;

    // Interpolate offset from backup to backup + ptOffset
    const auto targetRX = ptRel_Backup.x + ptOffset.x;
    const auto targetRY = ptRel_Backup.y + ptOffset.y;

    const auto fX = static_cast<float>(ptRel_Backup.x) +
        m_pInterpolation(tStart, tDelay,
                         static_cast<float>(ptRel_Backup.x),
                         static_cast<float>(targetRX));
    const auto fY = static_cast<float>(ptRel_Backup.y) +
        m_pInterpolation(tStart, tDelay,
                         static_cast<float>(ptRel_Backup.y),
                         static_cast<float>(targetRY));

    auto* pVec = get_gr().GetCenterVec();
    pVec->RelMove(static_cast<std::int32_t>(fX),
                  static_cast<std::int32_t>(fY));

    return tDelay + tStart - get_update_time() <= 0;
}

auto ReturnToUserCommand::Update() -> bool
{
    // TODO: full implementation from IDA 0x66b1e0
    // Moves camera back to user position with interpolation
    return tDelay + tStart - get_update_time() <= 0;
}

auto ScaleCommand::Update() -> bool
{
    // TODO: full implementation from IDA 0x6689e0
    // Interpolates screen scale between nStartScale and nEndScale
    return tDelay + tStart - get_update_time() <= 0;
}

auto ScaleAbsMoveCommand::Update() -> bool
{
    // TODO: full implementation from IDA 0x668be0
    // Combined absolute move + scale interpolation
    return tDelay + tStart - get_update_time() <= 0;
}

auto ScaleRelMoveCommand::Update() -> bool
{
    // TODO: full implementation from IDA 0x669000
    // Combined relative move + scale interpolation
    return tDelay + tStart - get_update_time() <= 0;
}

auto FloatCommand::Update() -> bool
{
    // TODO: full implementation from IDA 0x66bd10
    // Floating/bobbing camera effect
    return tDelay + tStart - get_update_time() <= 0;
}

auto FreeFromUserCommand::Update() -> bool
{
    // TODO: full implementation from IDA 0x66c240
    // Detach camera from user character
    return tDelay + tStart - get_update_time() <= 0;
}

auto StickToUserCommand::Update() -> bool
{
    // TODO: full implementation from IDA 0x66c6a0
    // Reattach camera to user character
    return tDelay + tStart - get_update_time() <= 0;
}

// -----------------------------------------------------------------------
// CameraCtrl::Manager
// -----------------------------------------------------------------------

Manager::Manager()
{
    UpdateManager::s_Attach(static_cast<IUpdatable*>(this));
}

Manager::~Manager()
{
    UpdateManager::s_Detach(static_cast<IUpdatable*>(this));
    m_queueCmds.clear();
    m_ScaleCmds.clear();
    m_pCmd.reset();
}

void Manager::Update()
{
    if (m_pCmd)
    {
        // Current command in progress — tick it
        if (m_pCmd->Update())
        {
            m_pCmd.reset();
            // Original calls g_gr->raw_forceFilter(-1) to reset filter
        }
    }
    else if (!m_queueCmds.empty())
    {
        // No active command — pop next from queue
        auto& front = m_queueCmds.front();
        if (front->Begin() && !front->Update())
        {
            // Command started and not yet finished — make it current
            m_pCmd = front;
            // Original calls g_gr->raw_forceFilter(2) to set bilinear filter
        }
        m_queueCmds.pop_front();
    }
}

void Manager::QueueCommand(std::shared_ptr<Command> pCmd)
{
    if (pCmd)
    {
        m_queueCmds.push_back(std::move(pCmd));
    }
}

void Manager::OnSetField()
{
    m_pCmd.reset();
    m_queueCmds.clear();
    // Original also calls g_gr->raw_forceFilter(-1) here
}

} // namespace CameraCtrl

} // namespace ms
