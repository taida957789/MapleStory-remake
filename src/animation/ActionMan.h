#pragma once

#include "ActionData.h"
#include "ActionKey.h"
#include "CharacterImgEntry.h"
#include "DragonAction.h"
#include "EmployeeAction.h"
#include "FaceLookEntry.h"
#include "MeleeAttackAfterimage.h"
#include "MobAction.h"
#include "MobImgEntry.h"
#include "MorphAction.h"
#include "MoveActionChange.h"
#include "NpcAction.h"
#include "PetAction.h"
#include "ShadowPartnerAction.h"
#include "SkillPetAction.h"
#include "SummonedAction.h"
#include "util/Singleton.h"

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ms
{

class WzProperty;

/// Matches CActionMan from the client (__cppobj : TSingleton<CActionMan>).
class ActionMan final : public Singleton<ActionMan>
{
    friend class Singleton<ActionMan>;

public:
    [[nodiscard]] auto Initialize(bool bCallOnLoadAction = false) -> bool;

    [[nodiscard]] auto GetCharacterImgEntry(std::int32_t nItemID)
        -> std::shared_ptr<CharacterImgEntry>;

    [[nodiscard]] auto GetActionData(std::int32_t nAction) const -> const ActionData*;

    [[nodiscard]] auto GetActionName(std::int32_t nAction) const -> const std::string&;

    [[nodiscard]] auto GetWeaponAfterImage(const std::string& sUOL)
        -> std::shared_ptr<MeleeAttackAfterimage>;

    /// Check if a CharacterImgEntry is loadable for the given item ID.
    [[nodiscard]] auto IsGettableImgEntry(std::int32_t nItemID) -> bool;

    /// Load face look canvases for the given face/emotion/accessory combination.
    void LoadFaceLook(
        std::int32_t nSkin,
        std::int32_t nFace,
        std::int32_t nEmotion,
        std::int32_t nFaceAcc,
        std::list<std::shared_ptr<WzGr2DCanvas>>& lpEmotion,
        std::int32_t nJob,
        bool bIgnoreInvisibleFace);

private:
    ActionMan();
    ~ActionMan() override = default;

    [[nodiscard]] auto GetActionCode(const std::string& sName) const -> std::int32_t;

    [[nodiscard]] static auto IsInvisibleFace(
        const std::shared_ptr<CharacterImgEntry>& pAccEntry,
        std::int32_t nJob) -> bool;

    void LoadRandomMoveActionChange();
    void LoadRandomMoveActionChangeInfo(std::int32_t nAction,
                                        const std::shared_ptr<WzProperty>& pRandomProp);

    // Character
    std::list<std::shared_ptr<CharacterImgEntry>> m_lCharacterImgEntry;
    std::map<std::int32_t, std::shared_ptr<CharacterImgEntry>> m_mCharacterImgEntry;

    // FaceLook
    std::list<std::shared_ptr<FaceLookEntry>> m_lFaceLook;
    std::map<FaceLookCodes, std::shared_ptr<FaceLookEntry>> m_mFaceLook;

    // Character UOL
    std::map<std::int32_t, std::string> m_msCharacterUOL;

    // Morph
    std::list<std::shared_ptr<MorphImgEntry>> m_lMorphImgEntry;
    std::map<std::int32_t, std::shared_ptr<MorphImgEntry>> m_mMorphImgEntry;
    std::list<std::shared_ptr<MorphActionEntry>> m_lMorphAction;
    std::map<std::uint32_t, std::shared_ptr<MorphActionEntry>> m_mMorphAction;

    // Mob
    std::list<std::shared_ptr<MobImgEntry>> m_lMobImgEntry;
    std::map<std::int32_t, std::shared_ptr<MobImgEntry>> m_mMobImgEntry;
    std::list<std::shared_ptr<MobActionEntry>> m_lMobAction;
    std::map<std::uint32_t, std::shared_ptr<MobActionEntry>> m_mMobAction;

    // Afterimage
    std::map<std::string, std::shared_ptr<MeleeAttackAfterimage>> m_mAfterimage;

    // NPC
    std::list<std::shared_ptr<NpcImgEntry>> m_lNpcImgEntry;
    std::map<std::int32_t, std::shared_ptr<NpcImgEntry>> m_mNpcImgEntry;
    std::list<std::shared_ptr<NpcActionEntry>> m_lNpcAction;
    std::map<std::int64_t, std::shared_ptr<NpcActionEntry>> m_mNpcAction;

    // Pet
    std::list<std::shared_ptr<PetImgEntry>> m_lPetImgEntry;
    std::map<std::int32_t, std::shared_ptr<PetImgEntry>> m_mPetImgEntry;
    std::list<std::shared_ptr<PetActionEntry>> m_lPetAction;
    std::map<std::uint32_t, std::shared_ptr<PetActionEntry>> m_mPetAction;

    // Employee
    std::list<std::shared_ptr<EmployeeImgEntry>> m_lEmployeeImgEntry;
    std::map<std::int32_t, std::shared_ptr<EmployeeImgEntry>> m_mEmployeeImgEntry;
    std::list<std::shared_ptr<EmployeeActionEntry>> m_lEmployeeAction;
    std::map<std::uint32_t, std::shared_ptr<EmployeeActionEntry>> m_mEmployeeAction;

    // Summoned
    std::list<std::shared_ptr<SummonedActionEntry>> m_lSummonedAction;
    std::map<ActionKey, std::shared_ptr<SummonedActionEntry>> m_mSummonedAction;

    // Shadow Partner
    std::list<std::shared_ptr<ShadowPartnerActionEntry>> m_lShadowPartnerAction;
    std::map<std::uint32_t, std::shared_ptr<ShadowPartnerActionEntry>> m_mShadowPartnerAction;

    // Dragon
    std::map<std::int32_t, std::map<std::int32_t, std::shared_ptr<DragonActionEntry>>> m_mDragonAction;

    // Skill Pet
    std::list<std::shared_ptr<SkillPetImgEntry>> m_lSkillPetImgEntry;
    std::map<std::int32_t, std::shared_ptr<SkillPetImgEntry>> m_mSkillPetImgEntry;
    std::list<std::shared_ptr<SkillPetActionEntry>> m_lSkillPetAction;
    std::map<std::uint32_t, std::shared_ptr<SkillPetActionEntry>> m_mSkillPetAction;

    // Cache sweep
    std::int32_t m_tLastSweepCache{0};
    std::uint32_t m_tLastMobSweepCache{0};

    // Move action change
    std::map<std::int32_t, std::vector<MoveActionChange>> m_mMoveActionChange;

    // Internal lookup
    std::unordered_map<std::string, std::int32_t> m_nameToCode;

    static const std::string s_sEmpty;
};

} // namespace ms
