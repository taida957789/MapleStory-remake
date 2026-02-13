#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace ms
{

class WzProperty;

/// Matches CHARACTERIMGENTRY from the client (__cppobj : ZRefCounted).
class CharacterImgEntry
{
public:
    /// Root WZ property (e.g. Character/Weapon/01302000.img)
    std::shared_ptr<WzProperty> m_pImg;

    /// Item slot string (from info/islot)
    std::string m_sISlot;

    /// Visual slot string (from info/vslot)
    std::string m_sVSlot;

    /// Weapon type code (from get_weapon_type(nItemID))
    std::int32_t m_nWeapon{0};

    /// Weapon afterimage effect name (from info/afterImage)
    std::string m_sWeaponAfterimage;

    /// Attack speed modifier (from info/attackSpeed)
    std::int32_t m_nAttackSpeed{0};

    /// Walk speed modifier (from info/walk)
    std::int32_t m_nWalk{0};

    /// Stand speed modifier (from info/stand)
    std::int32_t m_nStand{0};

    /// Attack modifier (from info/attack)
    std::int32_t m_nAttack{0};

    /// Sound effect reference (from info/sfx)
    std::string m_sSfx;

    /// Whether this item has weekly rotation variants (from info/weekly)
    bool m_bWeekly{false};

    /// Weekly rotation WZ node (loaded per day of week if m_bWeekly)
    std::shared_ptr<WzProperty> m_pWeeklyImg;

    /// Whether to hide face when this item is equipped (from info/invisibleFace)
    bool m_bInvisibleFace{false};

    /// Whether this item supports extended frame data (from info/extendFrame)
    bool m_bExtendFrame{false};

    /// Vehicle default frame data (from info/vehicleDefaultFrame)
    std::shared_ptr<WzProperty> m_pVehicleDefaultFrame;
};

} // namespace ms
