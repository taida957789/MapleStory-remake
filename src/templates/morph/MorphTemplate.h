#pragma once

#include <cstdint>

namespace ms
{

class MorphTemplate
{
public:
    /// Check if morph template is IceKnight type.
    [[nodiscard]] static bool IsIceKnight(std::uint32_t dwMorphTemplateID);

    /// Check if morph template is HideMorphed type.
    [[nodiscard]] static bool IsHideMorphed(std::uint32_t dwMorphTemplateID);

    /// Check if morph template is SuperMan type.
    [[nodiscard]] static bool IsSuperMan(std::uint32_t dwMorphTemplateID);

    /// Check if morph template is KaiserDragon type.
    [[nodiscard]] static bool IsKaiserDragon(std::uint32_t dwMorphTemplateID);
};

} // namespace ms
