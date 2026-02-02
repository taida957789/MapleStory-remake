#pragma once

#include "MapLoadable.h"
#include "ui/UIManager.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class WzCanvas;
class WzGr2DLayer;
class WzProperty;
class UIButton;
class UIEdit;

// Forward declarations for UI classes
class UILoginStart;
class UITitle;
class UIWorldSelect;
class UISelectChar;
class UINewCharRaceSelect;
class FadeWnd;
class Avatar;

/**
 * @brief Login stage - handles login, world/channel selection, and character selection
 *
 * Based on CLogin from the original MapleStory client (v1029).
 * Inherits from CMapLoadable for layer management.
 *
 * CLogin is the main stage that handles the entire login process:
 * - Title screen (step 0)
 * - World/channel selection (step 1)
 * - Character selection (step 2)
 * - Character creation - race select (step 3)
 * - Character creation - appearance (step 4)
 *
 * Login Steps (m_nLoginStep):
 * 0 - Initial state (title screen with login/quit buttons)
 * 1 - World selection (world list, channel selection)
 * 2 - Character selection (character list, play/delete buttons)
 * 3 - Character creation - race select
 * 4 - Character creation - appearance customization
 *
 * Sub Steps (m_nSubStep) for character creation (step 3/4):
 * 0 - Job selection
 * 1 - Gender selection
 * 2 - Frame/equipment selection
 * 3 - Avatar customization
 * 4 - Name input
 *
 * Step transitions use fade effects:
 * - ChangeStep() triggers fade out and schedules step change
 * - ChangeStepImmediate() performs the actual step change
 * - OnStepChanged() is called when step transition completes
 */
class Login final : public MapLoadable
{
public:
    /**
     * @brief Character equipment info for new character creation
     */
    struct NewEquip
    {
        std::int32_t nItemID{};
        std::int32_t nPart{};   // Equipment slot type
        std::int32_t nFrame{};  // Frame index
    };

    /**
     * @brief World item info
     */
    struct WorldItem
    {
        std::int32_t nWorldID{};
        std::string sName;
        std::int32_t nWorldState{};    // 0=normal, 1=event, 2=new, 3=hot
        std::string sEventDesc;
        std::int32_t nEventEXP{};
        std::int32_t nEventDrop{};
        std::int32_t nBlockCharCreation{};
        std::vector<std::int32_t> aChannelLoad;
    };

    /**
     * @brief Rank display info
     */
    struct Rank
    {
        std::int32_t nWorldRank{};
        std::int32_t nWorldRankMove{};
        std::int32_t nJobRank{};
        std::int32_t nJobRankMove{};
    };

    /**
     * @brief Character card info
     */
    struct CharacterCard
    {
        std::int32_t nCharacterID{};
        std::int32_t nLevel{};
        std::int32_t nJobCode{};
    };

    Login();
    ~Login() override;

    // Stage interface
    void Init(void* param) override;
    void Update() override;
    void Draw() override;
    void Close() override;

    // Input handling (from IUIMsgHandler)
    void OnMouseMove(std::int32_t x, std::int32_t y) override;
    void OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button) override;
    void OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button) override;
    void OnKeyDown(std::int32_t keyCode) override;
    void OnTextInput(const std::string& text) override;

    // Login step management
    [[nodiscard]] auto GetLoginStep() const noexcept -> std::int32_t { return m_nLoginStep; }
    void ChangeStep(std::int32_t nStep);

    [[nodiscard]] auto GetSubStep() const noexcept -> std::int32_t { return m_nSubStep; }
    void SetSubStep(std::int32_t nSubStep);

    [[nodiscard]] auto IsStepChanging() const noexcept -> bool { return m_tStepChanging != 0; }
    [[nodiscard]] auto IsRequestSent() const noexcept -> bool { return m_bRequestSent; }

    // Character selection
    [[nodiscard]] auto GetCharSelected() const noexcept -> std::int32_t { return m_nCharSelected; }
    void SetCharSelected(std::int32_t index) noexcept { m_nCharSelected = index; }
    [[nodiscard]] auto GetCharCount() const noexcept -> std::int32_t { return m_nCharCount; }
    [[nodiscard]] auto GetSlotCount() const noexcept -> std::int32_t { return m_nSlotCount; }

    // Race/job selection for new character
    [[nodiscard]] auto GetSelectedRace() const noexcept -> std::int32_t { return m_nCurSelectedRace; }
    void SetSelectedRace(std::int32_t race) noexcept { m_nCurSelectedRace = race; }

    [[nodiscard]] auto GetSelectedSubJob() const noexcept -> std::int16_t { return m_nCurSelectedSubJob; }
    void SetSelectedSubJob(std::int16_t subJob) noexcept { m_nCurSelectedSubJob = subJob; }

    // Frame/gender selection
    [[nodiscard]] auto GetFrameCount() const noexcept -> std::int32_t { return m_nChoosableFrame; }
    [[nodiscard]] auto GetCurFrame() const noexcept -> std::int32_t { return m_nCurFrame; }
    void SetCurFrame(std::int32_t frame) noexcept { m_nCurFrame = frame; }

    [[nodiscard]] auto IsGenderChoosable() const noexcept -> bool { return m_bChoosableGender; }
    [[nodiscard]] auto GetAccountGender() const noexcept -> std::int32_t { return m_nAccountGender; }

    // Shining star (special world events)
    [[nodiscard]] auto GetShiningStarCount() const noexcept -> std::int32_t { return m_nShiningStarCount; }
    [[nodiscard]] auto GetStarPlanetWorldId() const noexcept -> std::int32_t { return m_nStarPlanetWorldId; }
    void SetStarPlanetWorldId(std::int32_t worldId) noexcept { m_nStarPlanetWorldId = worldId; }

    // Login options
    [[nodiscard]] auto GetLoginOpt() const noexcept -> std::uint8_t { return m_bLoginOpt; }
    void SetLoginOpt(std::uint8_t opt) noexcept { m_bLoginOpt = opt; }

    // World info
    void SendWorldInfoRequest();
    void OnWorldInfoReceived();
    [[nodiscard]] auto GetWorldItemFinal() const noexcept -> const std::vector<WorldItem>& { return m_vWorldItemFinal; }
    [[nodiscard]] auto GetWorldItems() const noexcept -> const std::vector<WorldItem>& { return m_vWorldItem; }

    /**
     * @brief Go back to world selection from character selection
     *
     * Based on CLogin::GotoWorldSelect at 0xb66a10.
     * Sends world info request packet (opcode 117), clears world items,
     * and changes to step 1.
     */
    void GotoWorldSelect();

    // New character creation
    void InitNewAvatar(std::int32_t nGender, std::int32_t nFrame);
    void InitNewCharEquip(std::int32_t nRace);
    void ShiftNewCharEquip(std::int32_t nPart, std::int32_t nDirection, bool bIgnoreFrame);
    void ShiftNewCharSkin(std::int32_t nDirection);

    // UI callbacks
    void OnNewCharJobSel();
    void OnNewCharGenderSel();
    void OnNewCharFrameSel();
    void OnNewCharAvatarSel();
    void OnNewCharNameSel(const std::string& sName);
    void OnNewCharCanceled();

    // Static members (based on CLogin statics)
    static std::int32_t m_nBaseStep;             // Base login step (0=normal, 1=web login)
    static bool m_bOpenedNotActiveAccountDlg;    // Account activation dialog opened

private:
    // Step transition
    void ChangeStepImmediate();
    void OnStepChanged();
    void ChangeStepBGM();

    // World info
    void SendWorldInfoForShiningRequest();
    void InitWorldItemFinal();

    // UI setup for each step
    void SetupStep0UI();   // Title screen
    void SetupStep1UI();   // World selection
    void SetupStep2UI();   // Character selection
    void SetupStep3UI();   // Race selection
    void SetupStep4UI();   // Character creation

    void ClearStepUI();
    void DestroyUICharNameSelectAll();
    void CloseLoginDescWnd(FadeWnd* pExcept);

    // Resource loading
    void LoadLoginResources();
    void LoadGenderAndFrameChoosable();
    void LoadNewCharInfo(std::int32_t nRace);
    void LoadNewDummyCharInfo();
    void LoadSkinList();

    // Fade effect
    void FadeOverFrame(bool bFadeIn);

    // Utility
    [[nodiscard]] auto ConvertSelectedRaceToUIRace() const -> std::int32_t;
    void UpdateBanner();
    void CheckGoToStarPlanet();
    void CreatePlaceholderBackground();

    // Network packets (stubs for offline mode, real implementation sends to server)
    // Based on decompiled CLogin packet handling

    /**
     * @brief Send world info request packet (opcode 104)
     * Used on login to request world list from server.
     * Format: opcode(104) + isWebLogin(1 byte) + [webCookie(string) if isWebLogin]
     */
    void SendWorldInfoRequestPacket();

    /**
     * @brief Send select character packet (opcode 107)
     * Format: opcode(107) + SPW(string) + characterID(4 bytes) + offlineMode(1 byte)
     */
    void SendSelectCharacterPacket(std::uint32_t characterId);

    /**
     * @brief Send world info request for Goto (opcode 117)
     * Simple world info request used when going back to world select.
     * Format: opcode(117) only
     */
    void SendWorldInfoRequestForGotoPacket();

    /**
     * @brief Send client loading time packet (opcode 108)
     * Reports client loading performance to server.
     */
    void SendClientLoadingTimePacket();

    /**
     * @brief Send SPW verification packet (opcode 937)
     * Used during new character creation to verify secondary password.
     * Format: opcode(937) + SPW(string)
     */
    void SendSPWVerificationPacket(const std::string& spw);

    // Button callbacks
    void OnLoginButtonClick();
    void OnQuitButtonClick();
    void OnServerButtonClick();


private:
    // UI Manager
    UIManager m_uiManager;

    // Login state
    std::int32_t m_nLoginStep{};              // Current login step (0-4)
    std::int32_t m_nSubStep{};                // Sub-step for character creation
    bool m_bSubStepChanged{false};            // Flag indicating sub-step changed

    // Step transition timing
    std::uint64_t m_tStepChanging{};          // Time when step change was initiated
    std::uint64_t m_tStartFadeOut{};          // Time when fade out started
    std::int32_t m_nFadeOutLoginStep{-1};     // Step we're fading out from

    // World info request
    bool m_bWorldInfoRequest{false};          // World info request sent
    std::uint64_t m_tWorldInfoRequest{};      // Time of last world info request

    // Request state
    bool m_bRequestSent{false};               // Login/select request sent to server

    // Slot/character info
    std::int32_t m_nSlotCount{};              // Character slot count
    std::int32_t m_nBuyCharCount{};           // Purchased character slots
    std::int32_t m_nCharCount{};              // Number of characters
    std::int32_t m_nCharSelected{-1};         // Selected character index

    // Character creation state
    std::int32_t m_nCurSelectedRace{};        // Selected race for new character
    std::int16_t m_nCurSelectedSubJob{};      // Selected sub-job for new character
    bool m_bChoosableGender{false};           // Gender can be chosen
    std::int32_t m_nAccountGender{};          // Account gender
    std::int32_t m_nChoosableFrame{};         // Number of choosable frames
    std::int32_t m_nCurFrame{};               // Current frame selection
    std::int32_t m_nCurSelectedSkinIdx{};     // Current skin selection

    // Event character creation
    bool m_bEventNewChar{false};              // Event character creation mode
    std::int32_t m_nEventNewCharJob{-1};      // Event character job

    // Character sale (special promotion)
    bool m_bCharSale{false};
    std::int32_t m_nCharSaleJob{1};

    // Login options
    std::uint8_t m_bLoginOpt{0xFF};           // Login option flags

    // Termination flag
    bool m_bTerminate{false};

    // Go to world select flag
    bool m_bGotoWorldSelect{false};

    // Shining star count (for world selection UI)
    std::int32_t m_nShiningStarCount{-1};

    // Star Planet world
    std::int32_t m_nStarPlanetWorldId{-1};
    bool m_bGoToStarPlanet{false};
    bool m_bGoToStarPlanetForUpdate{false};
    std::string m_sGoToStarPlanetSPW;

    // Shining star mode
    std::int32_t m_nMakeShiningStar{};

    // Latest connected world
    std::int32_t m_nLatestConnectedWorldID{254};

    // Password (stored temporarily)
    std::string m_sSPW;
    std::string m_sCheckedName;

    // Offline mode
    bool m_bOfflineMode{false};

    // Enter type
    std::int32_t m_nEnterType{};

    // Beginner user flag
    bool m_bIsBeginingUser{false};

    // Not active account dialog focus
    bool m_bNotActiveAccountDlgFocus{false};

    // Can open UI flag
    bool m_bCanOpenUI{true};

    // Character card saved time
    std::uint64_t m_tCharCardSaved{};

    // Alba state bypass
    bool m_bAlbaStateBypass{false};

    // Rename character count
    std::int32_t m_nRenameCount{};
    std::string m_sOldName;

    // Race select order
    std::int32_t m_nRaceSelectOrder{};

    // Recommend world message
    bool m_bRecommendWorldMsgLoaded{false};

    // Balloon count
    std::int32_t m_nBalloonCount{};

    // Hair customization
    std::int32_t m_nHairItemID{};
    std::int32_t m_nHairType{-1};

    // Character select list edited flag
    bool m_bEditedCharSelectList{false};

    // Banner index
    std::int32_t m_nCurBannerIdx{-1};

public:
    // Accessors for UIWorldSelect
    [[nodiscard]] auto GetBalloonCount() const noexcept -> std::int32_t { return m_nBalloonCount; }

private:

    // World items
    std::vector<WorldItem> m_vWorldItem;
    std::vector<WorldItem> m_vWorldItemFinal;
    std::vector<WorldItem> m_vWorldItemFinalReboot;

    // Rank info
    std::vector<Rank> m_aRank;

    // New equipment list (for character creation)
    std::vector<NewEquip> m_lNewEquip;
    std::vector<NewEquip> m_lNewDummyEquip;

    // Skin list
    std::vector<std::int32_t> m_aSkin;

    // Equipment selection index per part (part -> currentIndex)
    std::map<std::int32_t, std::int32_t> m_mEquipSelIdx;

    // Gender/frame choosable maps (race -> choosable)
    std::map<std::int32_t, std::int32_t> m_mGenderChoosable;
    std::map<std::int32_t, std::int32_t> m_mFrameChoosable;
    std::map<std::int32_t, std::int32_t> m_mBasicAvatar;

    // Disabled race check
    bool m_aDisabledRaceCheck[19]{};
    std::int32_t m_aDisabledRaceReason[19]{};

    // Reserved delete character map (charID -> deleteTime)
    std::map<std::uint32_t, std::uint32_t> m_mReservedDeleteCharacter;

    // Character select list order
    std::vector<std::uint32_t> m_aCharacterSelectList;

    // Login-specific layers (in addition to MapLoadable layers)
    std::shared_ptr<WzGr2DLayer> m_pLayerBook;
    std::shared_ptr<WzGr2DLayer> m_pLayerLight;
    std::shared_ptr<WzGr2DLayer> m_pLayerDust;
    std::shared_ptr<WzGr2DLayer> m_pLayerFadeOverFrame;

    // UI elements
    std::shared_ptr<UILoginStart> m_pLoginStart;
    std::shared_ptr<FadeWnd> m_pLoginDesc0;
    std::shared_ptr<FadeWnd> m_pLoginDesc1;
    std::shared_ptr<Avatar> m_pNewAvatar;
    std::shared_ptr<Avatar> m_pNewDummyAvatar;

    // UI instances - owned via unique_ptr, use Create/Destroy pattern
    std::unique_ptr<UIWorldSelect> m_worldSelectUI;
    std::unique_ptr<UISelectChar> m_selectCharUI;
    std::unique_ptr<UINewCharRaceSelect> m_raceSelectUI;

    // UI Buttons (based on CUITitle::OnCreate from v1029)
    std::shared_ptr<UIButton> m_pBtnLogin;      // (178, 41) - Login button
    std::shared_ptr<UIButton> m_pBtnQuit;       // (159, 117) - Quit button
    std::shared_ptr<UIButton> m_pBtnEmailSave;  // (27, 97) - Save email checkbox
    std::shared_ptr<WzGr2DLayer> m_pLayerEmailCheck;  // Checkmark layer for email save
    std::shared_ptr<WzCanvas> m_pCanvasCheck0;        // Unchecked state (check/0)
    std::shared_ptr<WzCanvas> m_pCanvasCheck1;        // Checked state (check/1)
    std::shared_ptr<UIButton> m_pBtnEmailLost;  // (99, 97) - Lost email button
    std::shared_ptr<UIButton> m_pBtnPasswdLost; // (171, 97) - Lost password button
    std::shared_ptr<UIButton> m_pBtnNew;        // (15, 117) - New account button
    std::shared_ptr<UIButton> m_pBtnHomePage;   // (87, 117) - Homepage button

    // UI Edit fields (based on CCtrlEdit from v1029)
    std::shared_ptr<UIEdit> m_pEditID;          // (14, 43) - ID input field
    std::shared_ptr<UIEdit> m_pEditPasswd;      // (14, 69) - Password input field

    // Cached WZ properties for UI
    std::shared_ptr<WzProperty> m_pPropChangeStepBGM;
    std::string m_sLastChangeStepBGM;
    std::shared_ptr<WzProperty> m_pLoginImgProp;

    // Character card
    CharacterCard m_characterCard;

    // Event character ID
    std::string m_sEventCharacterID;
};

} // namespace ms
