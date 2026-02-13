#pragma once

#include <array>
#include <cstdint>

namespace ms
{

class InPacket;

/**
 * @brief Character appearance data
 *
 * Based on AvatarLook (__cppobj : ZRefCounted) from the original MapleStory client.
 * We use value semantics instead of ZRefCounted/ZRef smart pointers.
 *
 * Contains all visual information needed to render a character:
 * gender, skin, face, equipped items, pet IDs, hair color mixing, etc.
 */
class AvatarLook
{
public:
    static constexpr int kMaxHairEquip = 32;
    static constexpr int kMaxUnseenEquip = 32;
    static constexpr int kMaxPets = 3;

    void Initialize();

    [[nodiscard]] auto IsZeroBetaLook() const noexcept -> bool { return bIsZeroBetaLook; }

    void Decode(InPacket& iPacket);

    // --- Fields ---
    std::uint8_t nGender{};
    std::int32_t nSkin{};
    std::int32_t nFace{};
    std::int32_t nWeaponStickerID{};
    std::int32_t nWeaponID{};
    std::int32_t nSubWeaponID{};
    std::array<std::int32_t, kMaxHairEquip> anHairEquip{};
    std::array<std::int32_t, kMaxUnseenEquip> anUnseenEquip{};
    std::array<std::int32_t, kMaxPets> anPetID{};
    std::int32_t nJob{};
    bool bDrawElfEar{};
    std::int32_t nDemonSlayerDefFaceAcc{};
    std::int32_t nXenonDefFaceAcc{};
    bool bIsZeroBetaLook{};
    std::int32_t nMixedHairColor{};
    std::int32_t nMixHairPercent{};
};

} // namespace ms
