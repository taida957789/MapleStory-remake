#pragma once

#include <cstdint>

namespace ms
{

/// Matches ActionKey from the client (__cppobj).
class ActionKey
{
public:
    std::int32_t nSkillID{0};
    std::int32_t nSLV{0};
    std::int32_t nAction{0};

    ActionKey() = default;
    ActionKey(std::int32_t nSkillID, std::int32_t nSLV, std::int32_t nAction)
        : nSkillID(nSkillID), nSLV(nSLV), nAction(nAction) {}

    auto operator==(const ActionKey& o) const -> bool
    {
        return nSkillID == o.nSkillID
            && nSLV == o.nSLV
            && nAction == o.nAction;
    }

    auto operator<(const ActionKey& o) const -> bool
    {
        if (nSkillID != o.nSkillID) return nSkillID < o.nSkillID;
        if (nSLV != o.nSLV) return nSLV < o.nSLV;
        return nAction < o.nAction;
    }
};

inline auto HashKey(const ActionKey& key) -> std::int32_t
{
    return 32 * (key.nSLV + 32 * key.nSkillID) + key.nAction + 1057;
}

} // namespace ms
