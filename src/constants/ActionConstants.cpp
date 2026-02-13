#include "ActionConstants.h"

namespace ms
{

CharacterAction get_replaced_stand_action(
    std::int32_t nReplacedStandActionType, std::int32_t nStandType)
{
    // nStandType == Floating(1) → Stand1 variant (offset 0)
    // nStandType != Floating(1) → Stand2 variant (offset 1)
    auto offset = (nStandType != static_cast<std::int32_t>(ReplacedStandAction::Floating)) ? 1 : 0;

    if (nReplacedStandActionType == static_cast<std::int32_t>(ReplacedStandAction::Floating))
        return static_cast<CharacterAction>(
            static_cast<std::int32_t>(CharacterAction::Stand1Floating) + offset);

    if (nReplacedStandActionType == static_cast<std::int32_t>(ReplacedStandAction::Floating2))
        return static_cast<CharacterAction>(
            static_cast<std::int32_t>(CharacterAction::Stand1Floating2) + offset);

    return static_cast<CharacterAction>(
        static_cast<std::int32_t>(CharacterAction::Stand1) + offset);
}

CharacterAction get_replaced_action_by_pose(
    std::int32_t /*nPose*/, std::int32_t /*nWeaponType*/)
{
    // TODO: implement pose-based action replacement
    return CharacterAction::Walk1;
}

} // namespace ms
