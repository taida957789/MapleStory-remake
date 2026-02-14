#pragma once

#include "app/IUpdatable.h"
#include "util/Point.h"
#include "util/Singleton.h"

#include <cstdint>
#include <deque>
#include <memory>

namespace ms
{

// -----------------------------------------------------------------------
// Interpolation
// -----------------------------------------------------------------------

namespace Interpolation
{

enum Type : std::int32_t
{
    Linear     = 0,
    Sine       = 1,
    Cosine     = 2,
    Polynomial = 3,
    MinusPoly  = 4,
    MixPoly    = 5,
};

/// Function pointer type stored in Command::m_pInterpolation.
/// Signature: (tStart, tDelay, nStart, nEnd) -> offset from nStart.
/// Internally calls get_update_time() to compute progress.
using InterpFn = float (*)(std::int32_t tStart, std::int32_t tDelay,
                           float nStart, float nEnd);

/// Static interpolation functions (matching original Interpolation::s_* names).
auto s_Linear(std::int32_t tStart, std::int32_t tDelay,
              float nStart, float nEnd) -> float;
auto s_Sine(std::int32_t tStart, std::int32_t tDelay,
            float nStart, float nEnd) -> float;
auto s_Cosine(std::int32_t tStart, std::int32_t tDelay,
              float nStart, float nEnd) -> float;
auto s_Polynomial(std::int32_t tStart, std::int32_t tDelay,
                  float nStart, float nEnd) -> float;
auto s_MinusPoly(std::int32_t tStart, std::int32_t tDelay,
                 float nStart, float nEnd) -> float;
auto s_MixPoly(std::int32_t tStart, std::int32_t tDelay,
               float nStart, float nEnd) -> float;

} // namespace Interpolation

// -----------------------------------------------------------------------
// CameraCtrl
// -----------------------------------------------------------------------

namespace CameraCtrl
{

/**
 * @brief Base camera command.
 *
 * Original: CameraCtrl::Command : ZRefCounted (52 bytes).
 * vtable: { ~Command, Update } — Begin is non-virtual.
 */
struct Command
{
    virtual ~Command() = default;

    /// Called per update tick. Returns true when the command is finished.
    virtual auto Update() -> bool { return true; }

    /// Called once when the command starts (non-virtual).
    /// Validates type, backs up camera state, sets tStart.
    auto Begin() -> bool;

    /// Validate interpolation type and set m_pInterpolation.
    auto ValidateCmd() -> bool;

    Interpolation::Type type{Interpolation::Linear};
    std::int32_t tDelay{};
    Interpolation::InterpFn m_pInterpolation{};
    bool bBegin{};
    std::int32_t tStart{};
    Point2D ptAbs_Backup;
    Point2D ptRel_Backup;
    std::int32_t nScale_Backup{};
};

// -----------------------------------------------------------------------
// Command subclasses
// -----------------------------------------------------------------------

struct AbsMoveCommand : Command
{
    Point2D ptDest;
    auto Update() -> bool override;
};

struct RelMoveCommand : Command
{
    Point2D ptOffset;
    auto Update() -> bool override;
};

struct ReturnToUserCommand : Command
{
    auto Update() -> bool override;
};

struct ScaleCommand : Command
{
    std::int32_t nStartScale{};
    std::int32_t nEndScale{};
    auto Update() -> bool override;
};

struct ScaleAbsMoveCommand : Command
{
    Point2D ptDest;
    std::int32_t nStartScale{};
    std::int32_t nEndScale{};
    auto Update() -> bool override;
};

struct ScaleRelMoveCommand : Command
{
    Point2D ptOffset;
    std::int32_t nStartScale{};
    std::int32_t nEndScale{};
    auto Update() -> bool override;
};

struct FloatCommand : Command
{
    auto Update() -> bool override;
};

struct FreeFromUserCommand : Command
{
    auto Update() -> bool override;
};

struct StickToUserCommand : Command
{
    auto Update() -> bool override;
};

// -----------------------------------------------------------------------
// CameraCtrl::Manager
// -----------------------------------------------------------------------

/**
 * @brief Manages queued camera commands.
 *
 * Original: CameraCtrl::Manager : IUpdatable, TSingleton<Manager> (76 bytes).
 *
 * Attaches itself to UpdateManager::m_slUpdates on construction,
 * detaches on destruction (matching original ctor/dtor).
 */
class Manager final : public IUpdatable, public Singleton<Manager>
{
    friend class Singleton<Manager>;

public:
    ~Manager() override;

    // IUpdatable
    void Update() override;

    [[nodiscard]] auto IsWorking() const noexcept -> bool
    {
        return m_pCmd != nullptr;
    }

    void QueueCommand(std::shared_ptr<Command> pCmd);

    /// Called on field/stage transition. Clears current command and queue.
    void OnSetField();

private:
    Manager();

    std::shared_ptr<Command> m_pCmd;
    std::deque<std::shared_ptr<Command>> m_ScaleCmds;
    std::deque<std::shared_ptr<Command>> m_queueCmds;
};

} // namespace CameraCtrl

} // namespace ms
