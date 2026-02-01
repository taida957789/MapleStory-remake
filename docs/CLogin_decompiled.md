# CLogin Decompiled Functions (MapleStory v1029)

This document contains decompiled code from IDA Pro for the CLogin class in MapleStory v1029.
These serve as a reference for implementing the Login stage in the SDL3 client.

## Table of Contents

1. [CLogin::CLogin (Constructor)](#cloginclogin-constructor---0xb6d210)
2. [CLogin::Init](#clogininit---0xb6f150)
3. [CLogin::Update](#cloginupdate---0xb6abd0)
4. [CLogin::ChangeStep](#cloginchangestep---0xb65f00)
5. [CLogin::OnStepChanged](#cloginonstepchanged---0xb64f50)
6. [CLogin::Close](#cloginclose---0xb59660)
7. [CLogin::InitWorldItemFinal](#clogininitorlditemfinal---0xb6ef30)
8. [CLogin::GotoWorldSelect](#clogingotoworldselect---0xb66a10)
9. [CLogin::ProgressNewCharStep](#cloginprogressnewcharstep---0xb66010)
10. [CLogin::OnNewCharStep](#cloginonnewcharstep---0xb66c40)

---

## CLogin::CLogin (Constructor) - 0xb6d210

```cpp
CLogin *__thiscall CLogin::CLogin(CLogin *this)
{
  int v2; // ecx
  int *m_aDisabledRaceReason; // edx
  _FILETIME v4; // rax

  CMapLoadable::CMapLoadable(this); /*0xb6d23b*/
  this->CMapLoadable::CStage::IGObj::__vftable = &CLogin::`vftable'{for `IGObj'}; /*0xb6d242*/
  this->CMapLoadable::CStage::IUIMsgHandler::__vftable = &CLogin::`vftable'{for `IUIMsgHandler'}; /*0xb6d248*/
  this->CMapLoadable::CStage::INetMsgHandler::__vftable = &CLogin::`vftable'{for `INetMsgHandler'}; /*0xb6d24f*/
  this->CMapLoadable::CStage::ZRefCounted::__vftable = &CLogin::`vftable'{for `ZRefCounted'}; /*0xb6d256*/

  // Initialize member variables
  this->m_nMakeShiningStar = 0; /*0xb6d25d*/
  this->m_pConnectionDlg.p = 0; /*0xb6d267*/
  this->m_bGoToStarPlanet = 0; /*0xb6d26d*/
  this->m_bGoToStarPlanetForUpdate = 0; /*0xb6d273*/
  this->m_sGoToStarPlanetSPW._m_pStr = 0; /*0xb6d279*/
  this->m_bOfflineMode = 0; /*0xb6d27f*/
  this->m_nEnterType = 0; /*0xb6d285*/
  this->m_pLayerBook.m_pInterface = 0; /*0xb6d28b*/
  this->m_nFadeOutLoginStep = -1; /*0xb6d294*/
  this->m_tStartFadeOut = 0; /*0xb6d29a*/
  this->m_nLoginStep = 0; /*0xb6d2a0*/
  this->m_bWorldInfoRequest = 0; /*0xb6d2a6*/
  this->m_tWorldInfoRequest = 0; /*0xb6d2ac*/
  this->m_tStepChanging = 0; /*0xb6d2b2*/
  this->m_bRequestSent = 0; /*0xb6d2b8*/
  this->m_bLoginOpt = -1; /*0xb6d2be*/
  this->m_nSlotCount = 0; /*0xb6d2c5*/
  this->m_nBuyCharCount = 0; /*0xb6d2cb*/
  this->m_nEventNewCharJob = -1; /*0xb6d2d1*/
  this->m_nCharCount = 0; /*0xb6d2d7*/
  this->m_bGotoWorldSelect = 0; /*0xb6d2dd*/
  this->m_bTerminate = 0; /*0xb6d2e3*/
  this->m_pFocusedUI = 0; /*0xb6d2e9*/
  this->m_WorldItem.a = 0; /*0xb6d2ef*/
  this->m_WorldItemFinal._Myfirst = 0; /*0xb6d2f5*/
  this->m_WorldItemFinal._Mylast = 0; /*0xb6d2fb*/
  this->m_WorldItemFinal._Myend = 0; /*0xb6d301*/
  this->m_WorldItemFinalReboot._Myfirst = 0; /*0xb6d307*/
  this->m_WorldItemFinalReboot._Mylast = 0; /*0xb6d30d*/
  this->m_WorldItemFinalReboot._Myend = 0; /*0xb6d313*/
  this->m_nCharSelected = -1; /*0xb6d319*/
  this->m_aRank.a = 0; /*0xb6d31f*/
  this->m_abOnFamily.a = 0; /*0xb6d325*/

  // Initialize equipment lists
  this->m_lNewEquip.__vftable = &ZList<CLogin::NEWEQUIP>::`vftable'; /*0xb6d330*/
  this->m_lNewEquip._m_uCount = 0; /*0xb6d336*/
  this->m_lNewEquip._m_pHead = 0; /*0xb6d33c*/
  this->m_lNewEquip._m_pTail = 0; /*0xb6d342*/
  this->m_lNewDummyEquip.__vftable = &ZList<CLogin::NEWEQUIP>::`vftable'; /*0xb6d348*/
  this->m_lNewDummyEquip._m_uCount = 0; /*0xb6d34e*/
  this->m_lNewDummyEquip._m_pHead = 0; /*0xb6d354*/
  this->m_lNewDummyEquip._m_pTail = 0; /*0xb6d35a*/

  // Initialize maps
  this->m_mGenderChoosable.__vftable = &ZMap<long,long,long>::`vftable'; /*0xb6d374*/
  this->m_mGenderChoosable._m_apTable = 0; /*0xb6d37a*/
  this->m_mGenderChoosable._m_uTableSize = 31; /*0xb6d380*/
  this->m_mGenderChoosable._m_uCount = 0; /*0xb6d386*/
  this->m_mGenderChoosable._m_uAutoGrowEvery128 = 100; /*0xb6d38c*/
  this->m_mGenderChoosable._m_uAutoGrowLimit = 24; /*0xb6d392*/

  // ... more map initializations ...

  // Race selection state
  this->m_nBalloonCount = 0; /*0xb6d444*/
  this->m_aBalloon.a = 0; /*0xb6d44a*/
  this->m_nLatestConnectedWorldID = 254; /*0xb6d450*/
  this->m_bRecommendWorldMsgLoaded = 0; /*0xb6d45a*/
  this->m_aRecommendWorldMsg.a = 0; /*0xb6d460*/
  this->m_nCurSelectedRace = 0; /*0xb6d468*/
  this->m_nCurSelectedSubJob = 0; /*0xb6d46e*/
  this->m_bSubStepChanged = 0; /*0xb6d475*/
  this->m_sCheckedName._m_pStr = 0; /*0xb6d47b*/

  // Character creation state
  this->m_bCharSale = 0; /*0xb6d498*/
  this->m_nCharSaleJob = 1; /*0xb6d49e*/
  this->m_bEventNewChar = 0; /*0xb6d4a4*/
  this->m_bChoosableGender = 0; /*0xb6d4aa*/
  this->m_nAccountGender = 0; /*0xb6d4b0*/
  this->m_nChoosableFrame = 0; /*0xb6d4b6*/
  this->m_nCurSelectedSkinIdx = 0; /*0xb6d4c2*/

  // Hair
  this->m_nHairItemID = 0; /*0xb6d4f0*/
  this->m_nHairType = -1; /*0xb6d4f6*/
  this->m_sSPW._m_pStr = 0; /*0xb6d4fc*/
  this->m_bIsBeginingUser = 0; /*0xb6d502*/
  this->m_nShiningStarCount = -1; /*0xb6d508*/
  this->m_bNotActiveAccountDlgFocus = 0; /*0xb6d50e*/
  this->m_bCanOpenUI = 1; /*0xb6d51a*/
  this->m_nRenameCount = 0; /*0xb6d52c*/
  this->m_nRaceSelectOrder = 0; /*0xb6d538*/
  this->m_nStarPlanetWorldId = -1; /*0xb6d54a*/
  this->m_nCurBannerIdx = -1; /*0xb6d56e*/

  // Initialize disabled race arrays
  v2 = 0; /*0xb6d585*/
  m_aDisabledRaceReason = this->m_aDisabledRaceReason; /*0xb6d587*/
  do /*0xb6d5a1*/
  {
    this->m_aDisabledRaceCheck[v2] = 0; /*0xb6d590*/
    *m_aDisabledRaceReason = 0; /*0xb6d597*/
    ++v2; /*0xb6d599*/
    ++m_aDisabledRaceReason; /*0xb6d59b*/
  }
  while ( v2 < 19 ); /*0xb6d5a1*/

  // Security client
  if ( TSingleton<CSecurityClient>::ms_pInstance ) /*0xb6d5ab*/
  {
    TSecType<int>::SetData(&TSingleton<CSecurityClient>::ms_pInstance->m_bKeyCryptSession, 1);
    CSecurityClient::StartKeyCrypt(TSingleton<CSecurityClient>::ms_pInstance);
  }

  v4 = Util::FTGetNow(); /*0xb6d5c1*/
  this->m_ftTimeGab = (*&Util::FTGetNow() - *&v4); /*0xb6d5d3*/
  return this; /*0xb6d5e1*/
}
```

### Key Member Variables:
- `m_nLoginStep` - Current login step (0-4)
- `m_nFadeOutLoginStep` - Previous step for fade transition
- `m_tStartFadeOut` - Timestamp for fade out start
- `m_tStepChanging` - Timestamp for step change
- `m_bRequestSent` - Flag indicating if a request was sent
- `m_nCharSelected` - Index of selected character (-1 = none)
- `m_nCurSelectedRace` - Current selected race for new character
- `m_nCurSelectedSubJob` - Current selected sub-job
- `m_bSubStepChanged` - Flag for sub-step changes in step 4
- `m_nSubStep` - Current sub-step (0=job, 1=gender, 2=frame, 3=avatar, 4=name)

---

## CLogin::Init - 0xb6f150

```cpp
void __thiscall CLogin::Init(CLogin *this, void *pParam)
{
  // ... (very long function - key parts below)

  CMapLoadable::Init(this, pParam);

  // Reset magnification levels
  this->m_nMagLevel_Obj = 0;
  this->m_nMagLevel_Back = 0;
  this->m_nMagLevel_SkillEffect = 0;

  // Load WZ properties
  // StringPool::GetBSTR(Instance, &v147, 0x89Bu);  // "Map/Map/Map0/000010000.img"
  // this->m_pPropField = GetObject(...)

  // StringPool 0x2B35 = "info"
  // this->m_pPropFieldInfo = m_pPropField["info"]

  // StringPool 0x2970 = "bgm"
  // this->m_pPropChangeStepBGM = m_pPropFieldInfo["bgm"]

  this->LoadMap(this);

  // Flush cached objects (180000ms timeout)
  g_rm.m_pInterface->raw_FlushCachedObjects(180000);

  // Load book layer (StringPool 0x89C = "Map/Map/Map0/000010100.img")
  // Create layer at Z=110, centered

  // Create fade overlay layer at Z=211

  // Create CUILoginStart
  v109 = ZAllocEx<ZAllocAnonSelector>::Alloc(0x27Cu);
  if (v109)
    v110 = CUILoginStart::CUILoginStart(v109, this);
  this->m_pLoginStart.p = v110;

  // Check for relogin cookie
  v114 = TSingleton<CWvsContext>::ms_pInstance->m_sReloginCookie._m_pStr;
  if (v114 && *v114)
  {
    CLogin::ChangeStep(this, 1);  // Go directly to world select
  }
  else if (TSingleton<CWvsContext>::ms_pInstance->m_nLoginBaseStep == 1)
  {
    // Web login mode - go to step 1
    this->m_nLoginStep = 1;
    CLogin::m_nBaseStep = 1;
    // ... create CUIWorldSelect, send packets

    // Create world select UI
    v119 = ZAllocEx<ZAllocAnonSelector>::Alloc(0xE8u);
    if (v119)
      CUIWorldSelect::CUIWorldSelect(v119, this);

    // Create world select notice
    if (!TSingleton<CUIWorldSelectNotice>::ms_pInstance)
      TSingleton<CUIWorldSelectNotice>::CreateInstance();

    CLogin::RecoverPassport(this);

    // Send world info request packet (opcode 104)
    COutPacket::COutPacket(&oPacket, 104);
    COutPacket::Encode1(&oPacket, CLogin::m_nBaseStep == 1);
    if (CLogin::m_nBaseStep == 1)
    {
      CLogin::GetWebCookie(this, v132);
      COutPacket::EncodeStr(&oPacket, v132[0]);
    }
    CClientSocket::SendPacket(TSingleton<CClientSocket>::ms_pInstance._m_pStr, &oPacket);

    // Send another packet (opcode 114)
    COutPacket::COutPacket(&oPacket, 114);
    CClientSocket::SendPacket(...);

    // Configure LoginStart buttons
    CUILoginStart::SetButton(this->m_pLoginStart.p,
      (this->m_nShiningStarCount != 0 ? 0x20 : 0) |
      (this->m_bLoginOpt != 2 ? 0 : 8) | 2);
  }
  else
  {
    // Normal mode - create title UI
    v121 = ZAllocEx<ZAllocAnonSelector>::Alloc(0x19Cu);
    if (v121)
      CUITitle::CUITitle(v121, this);
  }

  // Move center to calculated position based on login step
  v125 = -8 - 600 * (this->m_nLoginStep + (this->m_nLoginStep != 4 ? 0 : CLogin::ConvertSelectedRaceToUIRace(this)));
  IWzVector2D::RelMove(*v126, 28, v125, ...);

  CLogin::LoadGenderAndFrameChoosable(this);
  CLogin::LoadNewCharInfo(this, 0);
  CLogin::LoadNewDummyCharInfo(this);

  // Notify load thread
  if (TSingleton<CLoadClientDataThread>::ms_pInstance)
    InterlockedExchange(&TSingleton<CLoadClientDataThread>::ms_pInstance->m_nLoadFlag, 14);

  CMapLoadable::PlayBGMFromMapInfo(this);

  // Fade in if not relogin
  if (!v127 || !*v127)
    CStage::FadeIn(this, 0);

  CLogin::InitWorldItemFinal(this);
}
```

### Key Points:
- Initializes from `Map/Map/Map0/000010000.img`
- Creates `CUILoginStart` singleton
- Handles web login vs normal login
- Creates appropriate UI based on base step
- Sends initial network packets (opcodes 104, 114)
- Loads gender/frame choosable data
- Initializes world item final list

---

## CLogin::Update - 0xb6abd0

```cpp
void __thiscall CLogin::Update(CLogin *this)
{
  char *m_pStr = TSingleton<CWvsApp>::ms_pInstance._m_pStr;
  int v3 = *(TSingleton<CWvsApp>::ms_pInstance._m_pStr + 8);  // Current time

  // Handle termination
  if (this->m_bTerminate)
    ZAPI.PostQuitMessage(0);

  // World info request handling (step 1)
  if (this->m_nLoginStep == 1 && !this->m_bWorldInfoRequest)
  {
    int m_tWorldInfoRequest = this->m_tWorldInfoRequest;
    if ((!m_tWorldInfoRequest || Util::IsOverTime(m_tWorldInfoRequest, 3000, v3))
        && this->m_nShiningStarCount < 0)
    {
      this->m_bWorldInfoRequest = 1;
      this->m_tWorldInfoRequest = v3;
      CLogin::SendWorldInfoForShiningRequestPacket(this);
    }
  }

  // Splash screen handling
  CSplashScreen *v5 = TSingleton<CSplashScreen>::ms_pInstance;
  if (v5)
  {
    if (CWvsApp::IsLoadedAllGameData(m_pStr) && !CSplashScreen::IsSetTimeOut(v5))
      CSplashScreen::StartTimeOut(v5, 2);
    if (CSplashScreen::CheckTimeOut(v5) && TSingleton<CSplashScreen>::ms_pInstance)
    {
      CSplashScreen::~CSplashScreen(TSingleton<CSplashScreen>::ms_pInstance);
      ZAllocEx<ZAllocAnonSelector>::Free(...);
    }
  }

  CAnimationDisplayer::NonFieldUpdate(TSingleton<CAnimationDisplayer>::ms_pInstance.m_pInterface, v3);

  // Channel select world entry handling
  if (TSingleton<CUniqueModeless>::ms_pInstance._m_pStr)
  {
    CUIChannelSelect *v7 = ...;
    if (v7->m_bSelectWorld && CWvsApp::IsPossibleWorldSelectState(m_pStr))
    {
      v7->m_bSelectWorld = 0;
      CUIChannelSelect::EnterChannel(v7);
    }
  }

  // Character selection handling (step 2)
  if (CWvsApp::IsLoadedAllGameData(m_pStr) && this->m_nCharSelected >= 0)
  {
    // Check if can proceed with character selection
    char *v9 = this->m_sSPW._m_pStr;
    if (v9 && *v9 && v8 && this->m_nLoginStep == 2)
    {
      // Send select character packet (opcode 107)
      COutPacket::COutPacket(&oPacket, 107);
      ZXString<char>::ZXString<char>(v30, &this->m_sSPW);
      COutPacket::EncodeStr(&oPacket, v30[0]);
      COutPacket::Encode4(&oPacket, SelectedAvatarData->characterStat.dwCharacterID);
      COutPacket::Encode1(&oPacket, this->m_bOfflineMode);
      CClientSocket::SendPacket(...);

      CUISelectChar::RemoveNoticeConnecting(...);
      CUISelectChar::SetEnterTime(..., 2000);

      this->m_bRequestSent = 1;
      ZXString<char>::Empty(&this->m_sSPW);
      ZXString<char>::Empty(&this->m_sGoToStarPlanetSPW);
      this->m_bOfflineMode = 0;

      // Send loading time packet (opcode 108) if enabled
      // ...
    }
  }

  // Handle fade out completion
  int m_tStartFadeOut = this->m_tStartFadeOut;
  if (m_tStartFadeOut && v3 - m_tStartFadeOut > 0)
  {
    CLogin::ChangeStepImmediate(this);
    this->m_nFadeOutLoginStep = -1;
    this->m_tStartFadeOut = 0;
  }

  // Handle step change completion
  int m_tStepChanging = this->m_tStepChanging;
  if (m_tStepChanging && v3 - m_tStepChanging > 0)
  {
    CLogin::OnStepChanged(this);
    this->m_tStepChanging = 0;
  }

  // Handle sub-step changes (step 4 - new character creation)
  if (this->m_bSubStepChanged)
  {
    switch (this->m_nSubStep)
    {
      case 0:  // Job select
        // Destroy previous UIs
        // Create CUINewCharJobSelectNormal for race 1 or 13
        break;
      case 1:  // Gender select
        // Create CUINewCharGenderSelect
        break;
      case 2:  // Frame select
        // Create CUINewCharEquipFrameSelect
        break;
      case 3:  // Avatar select
        // Create CUINewCharAvatarSelectCommon
        break;
      case 4:  // Name select
        // Call CLogin::CreateUICharNameSelect
        break;
    }
    this->m_bSubStepChanged = 0;
  }

  // Cleanup layers if not in step 2
  if (this->m_nLoginStep != 2)
  {
    if (this->m_pLayerLight.m_pInterface)
    {
      this->m_pLayerLight.m_pInterface = 0;
      m_pInterface->Release(m_pInterface);
    }
    if (this->m_pLayerDust.m_pInterface)
    {
      this->m_pLayerDust.m_pInterface = 0;
      v28->Release(v28);
    }
  }

  // Cleanup avatars if not in step 4
  if (this->m_pNewAvatar.p && this->m_nLoginStep != 4)
    ZRef<CAvatar>::operator=(&this->m_pNewAvatar, 0);
  if (this->m_pNewDummyAvatar.p && this->m_nLoginStep != 4)
    ZRef<CAvatar>::operator=(&this->m_pNewDummyAvatar, 0);

  CLogin::CheckGoToStarPlanet(this);
  CLogin::UpdateBanner(this);
}
```

### Sub-step Flow (Step 4 - New Character):
- Sub-step 0: Job selection (creates `CUINewCharJobSelectNormal`)
- Sub-step 1: Gender selection (creates `CUINewCharGenderSelect`)
- Sub-step 2: Frame selection (creates `CUINewCharEquipFrameSelect`)
- Sub-step 3: Avatar selection (creates `CUINewCharAvatarSelectCommon`)
- Sub-step 4: Name selection (calls `CreateUICharNameSelect`)

---

## CLogin::ChangeStep - 0xb65f00

```cpp
void __thiscall CLogin::ChangeStep(CLogin *this, int nStep)
{
  // If already fading, complete immediately
  if (this->m_tStartFadeOut)
    CLogin::ChangeStepImmediate(this);

  int v3 = nStep;
  int m_nLoginStep = this->m_nLoginStep;
  this->m_nFadeOutLoginStep = m_nLoginStep;

  // Handle negative step (cycle to next)
  if (nStep < 0)
    v3 = (m_nLoginStep + 1) % 5;

  this->m_nLoginStep = v3;

  // Special handling for step 3 (race select)
  if (v3 == 3)
  {
    if (this->m_bEventNewChar)
    {
      this->m_nCurSelectedRace = get_race_select_from_event_job(this->m_nEventNewCharJob);
      this->m_nCurSelectedSubJob = 0;
    }
    else if (this->m_nMakeShiningStar == 2)
    {
      this->m_nCurSelectedRace = -1;
      this->m_nCurSelectedSubJob = 0;
    }
    this->m_nLoginStep = 4;  // Skip to step 4 if special case
  }

  // Reset world info if going back to step 0 or 1
  if (this->m_nLoginStep <= 1)
    CWvsContext::ResetWorldInfoOnWorldSelect(TSingleton<CWvsContext>::ms_pInstance);

  // If step actually changed, start fade transition
  if (this->m_nFadeOutLoginStep != this->m_nLoginStep)
  {
    // Register fade animation (200ms fade out, 200ms fade in)
    CAnimationDisplayer::RegisterFadeInOutAnimation(
      TSingleton<CAnimationDisplayer>::ms_pInstance.m_pInterface,
      200,    // fadeOutTime
      0,      // delay
      200,    // fadeInTime
      22,     // type
      255,    // alpha
      0xFF000000);  // color

    // Set transition timestamps
    int v7 = *(TSingleton<CWvsApp>::ms_pInstance._m_pStr + 8) + 200;
    int v8 = v7;
    if (v7 == -200)
      v8 = 1;
    this->m_tStepChanging = v8;
    if (!v7)
      v7 = 1;
    this->m_tStartFadeOut = v7;
  }

  CLogin::ChangeStepBGM(this);
}
```

### Key Points:
- Step transition uses 200ms fade animation
- Negative step (-1) cycles to next step
- Step 3 may skip to step 4 for special cases
- Resets world info when going to step 0 or 1

---

## CLogin::OnStepChanged - 0xb64f50

```cpp
void __thiscall CLogin::OnStepChanged(CLogin *this)
{
  CWvsContext *v2 = TSingleton<CWvsContext>::ms_pInstance;

  // Reset cursor state if it was set to wait
  if (TSingleton<CInputSystem>::ms_pInstance &&
      TSingleton<CInputSystem>::ms_pInstance->m_nCursorState == 18)
    CInputSystem::SetCursorState(TSingleton<CInputSystem>::ms_pInstance, 0, 1);

  switch (this->m_nLoginStep)
  {
    case 0:  // Title screen
      // Destroy all step-specific UIs
      if (TSingleton<CUISelectChar>::ms_pInstance._m_pStr) CWnd::Destroy(...);
      if (TSingleton<CUIRecommendWorld>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUIWorldSelect>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUIWorldSelectNotice>::ms_pInstance) CWnd::Destroy(...);
      if (this->m_pBannerUI.p) CWnd::Destroy(...);
      if (TSingleton<CUniqueModeless>::ms_pInstance._m_pStr) CWnd::Destroy(...);
      if (TSingleton<CUINewCharAvatarSelectCommon>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharRaceSelect_Ex>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharGenderSelect>::ms_pInstance) CWnd::Destroy(...);
      if (this->m_pBanner.p) CWnd::Destroy(...);

      CLogin::DestroyUICharNameSelectAll(this);
      CLogin::CloseLoginDescWnd(this, 0);

      ZRef<CAvatar>::operator=(&this->m_pNewAvatar, 0);
      ZRef<CAvatar>::operator=(&this->m_pNewDummyAvatar, 0);
      this->m_nCurSelectedRace = 0;
      this->m_nCurSelectedSubJob = 0;
      break;

    case 1:  // World/Channel select
      // Destroy character select and title UIs
      if (TSingleton<CUISelectChar>::ms_pInstance._m_pStr) CWnd::Destroy(...);
      if (TSingleton<CUITitle>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharAvatarSelectCommon>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharRaceSelect_Ex>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharGenderSelect>::ms_pInstance) CWnd::Destroy(...);

      CLogin::DestroyUICharNameSelectAll(this);
      CLogin::CloseLoginDescWnd(this, 0);

      // Create login description for step 1
      v5 = ZAllocEx<ZAllocAnonSelector>::Alloc(0x12Cu);
      if (v5)
        v6 = CUILoginDesc::CUILoginDesc(v5, this, 1);
      ZRef<CFadeWnd>::operator=(&this->m_pLoginDesc0, v6);

      ZRef<CAvatar>::operator=(&this->m_pNewAvatar, 0);
      ZRef<CAvatar>::operator=(&this->m_pNewDummyAvatar, 0);
      break;

    case 2:  // Character select
      // Destroy world select and new char UIs
      if (TSingleton<CUIRecommendWorld>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUITitle>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUIWorldSelect>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUIWorldSelectNotice>::ms_pInstance) CWnd::Destroy(...);
      if (this->m_pBannerUI.p) CWnd::Destroy(...);
      if (TSingleton<CUniqueModeless>::ms_pInstance._m_pStr) CWnd::Destroy(...);
      if (TSingleton<CUINewCharAvatarSelectCommon>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharRaceSelect_Ex>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharGenderSelect>::ms_pInstance) CWnd::Destroy(...);
      if (this->m_pBanner.p) CWnd::Destroy(...);

      CLogin::DestroyUICharNameSelectAll(this);
      CLogin::CloseLoginDescWnd(this, this->m_pLoginDesc0.p);

      // Create description UIs if not Star Planet
      if (TSingleton<CWvsContext>::ms_pInstance->m_nStarPlanetWorldID !=
          TSingleton<CWvsContext>::ms_pInstance->m_nWorldID)
      {
        // Create CUILoginDesc type 2
        v9 = ZAllocEx<ZAllocAnonSelector>::Alloc(0x12Cu);
        if (v9)
          v10 = CUILoginDesc::CUILoginDesc(v9, this, 2);
        ZRef<CFadeWnd>::operator=(&this->m_pLoginDesc0, v10);

        if (!this->m_pLoginDesc1.p)
        {
          v11 = ZAllocEx<ZAllocAnonSelector>::Alloc(0x12Cu);
          if (v11)
            v12 = CUILoginDesc::CUILoginDesc(v11, this, -1);
          ZRef<CFadeWnd>::operator=(&this->m_pLoginDesc1, v12);
        }
      }

      ZRef<CAvatar>::operator=(&this->m_pNewAvatar, 0);
      ZRef<CAvatar>::operator=(&this->m_pNewDummyAvatar, 0);

      if (CWvsContext::IsDoingRelogin(v2))
        CStage::FadeIn(this, 0);
      break;

    case 3:  // Race select (CUINewCharRaceSelect_Ex)
      // Destroy all other UIs
      if (TSingleton<CUISelectChar>::ms_pInstance._m_pStr) CWnd::Destroy(...);
      if (TSingleton<CUIRecommendWorld>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUITitle>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUIWorldSelect>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUIWorldSelectNotice>::ms_pInstance) CWnd::Destroy(...);
      if (this->m_pBannerUI.p) CWnd::Destroy(...);
      if (TSingleton<CUniqueModeless>::ms_pInstance._m_pStr) CWnd::Destroy(...);
      if (TSingleton<CUINewCharGenderSelect>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharAvatarSelectCommon>::ms_pInstance) CWnd::Destroy(...);
      if (this->m_pBanner.p) CWnd::Destroy(...);

      CLogin::DestroyUICharNameSelectAll(this);

      // Create login descriptions for step 3
      if (!this->m_pLoginDesc0.p)
      {
        v15 = ZAllocEx<ZAllocAnonSelector>::Alloc(0x12Cu);
        if (v15)
          v16 = CUILoginDesc::CUILoginDesc(v15, this, 3);
        ZRef<CFadeWnd>::operator=(&this->m_pLoginDesc0, v16);
      }

      if (!this->m_pLoginDesc1.p)
      {
        v17 = ZAllocEx<ZAllocAnonSelector>::Alloc(0x12Cu);
        if (v17)
          v18 = CUILoginDesc::CUILoginDesc(v17, this, 4);
        ZRef<CFadeWnd>::operator=(&this->m_pLoginDesc1, v18);
      }

      // Set cursor to wait state
      CInputSystem::SetCursorState(TSingleton<CInputSystem>::ms_pInstance, 18, 0);
      break;

    case 4:  // New character creation
      // Destroy all other UIs
      if (TSingleton<CUISelectChar>::ms_pInstance._m_pStr) CWnd::Destroy(...);
      if (TSingleton<CUIRecommendWorld>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUITitle>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUIWorldSelect>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUIWorldSelectNotice>::ms_pInstance) CWnd::Destroy(...);
      if (this->m_pBannerUI.p) CWnd::Destroy(...);
      if (TSingleton<CUniqueModeless>::ms_pInstance._m_pStr) CWnd::Destroy(...);
      if (TSingleton<CUINewCharRaceSelect_Ex>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharGenderSelect>::ms_pInstance) CWnd::Destroy(...);
      if (TSingleton<CUINewCharAvatarSelectCommon>::ms_pInstance) CWnd::Destroy(...);
      if (this->m_pBanner.p) CWnd::Destroy(...);

      // Create login description
      if (!this->m_pLoginDesc0.p)
      {
        v21 = ZAllocEx<ZAllocAnonSelector>::Alloc(0x12Cu);
        if (v21)
          v22 = CUILoginDesc::CUILoginDesc(v21, this, 3);
        ZRef<CFadeWnd>::operator=(&this->m_pLoginDesc0, v22);
      }

      // Get job from race and determine gender choosability
      int m_nCurSelectedRace = this->m_nCurSelectedRace;
      int beginner_job_from_race = get_beginner_job_from_race(m_nCurSelectedRace);
      int nJob = beginner_job_from_race;

      if (m_nCurSelectedRace == 8 && !beginner_job_from_race)
        nJob = 430;  // Dual Blade fallback

      // Check gender choosability from map
      int m_nGender = TSingleton<CWvsContext>::ms_pInstance->m_nGender;
      int nGenderChoosable = 0;
      if (this->m_mGenderChoosable._m_uCount &&
          ZMap<long,long,long>::GetAt(&this->m_mGenderChoosable, &nJob, &nGenderChoosable))
      {
        switch (nGenderChoosable)
        {
          case 0: this->m_bChoosableGender = 0; break;
          case 1: this->m_bChoosableGender = 1; break;
          case 2:
            this->m_bChoosableGender = 0;
            TSingleton<CWvsContext>::ms_pInstance->m_nGender = 0;  // Force male
            break;
          case 3:
            this->m_bChoosableGender = 0;
            TSingleton<CWvsContext>::ms_pInstance->m_nGender = 1;  // Force female
            break;
        }
      }

      // Check frame choosability
      this->m_nChoosableFrame = 0;
      if (this->m_mFrameChoosable._m_uCount &&
          ZMap<long,long,long>::GetAt(&this->m_mFrameChoosable, &nJob, &nGenderChoosable) &&
          nGenderChoosable > 0)
      {
        this->m_nChoosableFrame = nGenderChoosable;
      }

      CLogin::LoadSkinList(this);

      // If gender changed, reinitialize avatar
      int v26 = TSingleton<CWvsContext>::ms_pInstance->m_nGender;
      if (m_nGender != v26)
      {
        if (this->m_nChoosableFrame <= 0)
          CLogin::InitNewAvatar(this, v26, 0);
        else
          CLogin::InitNewAvatar(this, v26, 1);
      }

      // Determine starting sub-step
      int v27 = 0;
      if (!this->m_bCharSale)
      {
        v27 = !this->m_bChoosableGender ? 3 : 1;  // Skip gender if not choosable
      }
      this->m_nSubStep = v27;
      this->m_bSubStepChanged = 1;
      break;
  }
}
```

### Step Transitions:
- **Step 0 (Title)**: Destroys all UIs, resets race/job selection
- **Step 1 (World Select)**: Creates `CUILoginDesc` type 1, destroys char select
- **Step 2 (Char Select)**: Creates `CUILoginDesc` types 2 and -1, destroys world select
- **Step 3 (Race Select)**: Creates `CUILoginDesc` types 3 and 4, sets wait cursor
- **Step 4 (New Char)**: Determines sub-step based on gender/frame choosability

---

## CLogin::Close - 0xb59660

```cpp
void __thiscall CLogin::Close(CLogin *this)
{
  CMapLoadable::Close(this);

  // Destroy all singleton UIs
  if (TSingleton<CUIRecommendWorld>::ms_pInstance) CWnd::Destroy(...);
  if (TSingleton<CUITitle>::ms_pInstance) CWnd::Destroy(...);
  if (TSingleton<CUIWorldSelect>::ms_pInstance) CWnd::Destroy(...);
  if (TSingleton<CUIWorldSelectNotice>::ms_pInstance) CWnd::Destroy(...);
  if (this->m_pBannerUI.p) CWnd::Destroy(...);
  if (TSingleton<CUniqueModeless>::ms_pInstance._m_pStr) CWnd::Destroy(...);
  if (TSingleton<CUISelectChar>::ms_pInstance._m_pStr) CWnd::Destroy(...);
  if (TSingleton<CUINewCharAvatarSelectCommon>::ms_pInstance) CWnd::Destroy(...);
  if (TSingleton<CUINewCharGenderSelect>::ms_pInstance) CWnd::Destroy(...);
  if (TSingleton<CUINewCharEquipFrameSelect>::ms_pInstance) CWnd::Destroy(...);
  if (this->m_pBanner.p) CWnd::Destroy(...);

  CLogin::DestroyUICharNameSelectAll(this);

  // Close child modal
  if (this->m_pChildModal.p)
    this->m_pChildModal.p->SetRet(0);

  // Destroy login start and descriptions
  if (this->m_pLoginStart.p) CWnd::Destroy(...);
  if (this->m_pLoginDesc0.p) CWnd::Destroy(...);
  if (this->m_pLoginDesc1.p) CWnd::Destroy(...);

  // Stop key encryption
  if (TSingleton<CSecurityClient>::ms_pInstance)
  {
    CSecurityClient::StopKeyCrypt(TSingleton<CSecurityClient>::ms_pInstance);
    TSecType<int>::SetData(&TSingleton<CSecurityClient>::ms_pInstance->m_bKeyCryptSession, 0);
  }

  // Destroy splash screen
  if (TSingleton<CSplashScreen>::ms_pInstance)
  {
    CSplashScreen::~CSplashScreen(TSingleton<CSplashScreen>::ms_pInstance);
    ZAllocEx<ZAllocAnonSelector>::Free(...);
  }

  // Release layers
  if (this->m_pLayerBook.m_pInterface)
  {
    this->m_pLayerBook.m_pInterface = 0;
    m_pInterface->Release(m_pInterface);
  }
  if (this->m_pLayerDust.m_pInterface) { ... Release ... }
  if (this->m_pLayerLight.m_pInterface) { ... Release ... }
  if (this->m_pLayerFadeOverFrame.m_pInterface) { ... Release ... }

  // Flush cached resources
  g_rm.m_pInterface->raw_FlushCachedObjects(0);
}
```

---

## CLogin::InitWorldItemFinal - 0xb6ef30

```cpp
void __thiscall CLogin::InitWorldItemFinal(CLogin *this)
{
  WORLDITEM wi;

  wi.sName._m_pStr = 0;
  wi.sWorldEventDesc._m_pStr = 0;
  wi.ci.a = 0;

  // Add worlds to m_WorldItemFinal
  wi.nWorldID = 0;
  ZXString<char>::Assign(&wi.sName, "Scania", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  wi.nWorldID = 1;
  ZXString<char>::Assign(&wi.sName, "Bera", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  wi.nWorldID = 3;
  ZXString<char>::Assign(&wi.sName, "Broa", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  wi.nWorldID = 4;
  ZXString<char>::Assign(&wi.sName, "Windia", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  wi.nWorldID = 5;
  ZXString<char>::Assign(&wi.sName, "Khaini", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  wi.nWorldID = 10;
  ZXString<char>::Assign(&wi.sName, "Demethos", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  wi.nWorldID = 16;
  ZXString<char>::Assign(&wi.sName, "Galicia", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  wi.nWorldID = 29;
  ZXString<char>::Assign(&wi.sName, "Renegades", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  wi.nWorldID = 43;
  ZXString<char>::Assign(&wi.sName, "Arcania", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  wi.nWorldID = 44;
  ZXString<char>::Assign(&wi.sName, "Zenith", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  // Reboot world goes to separate list
  wi.nWorldID = 45;
  ZXString<char>::Assign(&wi.sName, "Reboot", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinalReboot, &wi);

  wi.nWorldID = 99;
  ZXString<char>::Assign(&wi.sName, "Nova", -1);
  std::vector<WORLDITEM>::push_back(&this->m_WorldItemFinal, &wi);

  WORLDITEM::~WORLDITEM(&wi);
}
```

### Default Worlds:
| ID | Name |
|----|------|
| 0 | Scania |
| 1 | Bera |
| 3 | Broa |
| 4 | Windia |
| 5 | Khaini |
| 10 | Demethos |
| 16 | Galicia |
| 29 | Renegades |
| 43 | Arcania |
| 44 | Zenith |
| 45 | Reboot (separate list) |
| 99 | Nova |

---

## CLogin::GotoWorldSelect - 0xb66a10

```cpp
void __thiscall CLogin::GotoWorldSelect(CLogin *this)
{
  if (this->m_nLoginStep > 1 && !this->m_tStepChanging)
  {
    // Clear relogin cookie
    char *m_pStr = TSingleton<CWvsContext>::ms_pInstance->m_sReloginCookie._m_pStr;
    if (m_pStr)
    {
      InterlockedDecrement(m_pStr - 3);
      // Free string...
    }

    // Clear world items
    ZArray<WORLDITEM>::RemoveAll(&this->m_WorldItem);

    // Send world info request packet (opcode 117)
    COutPacket::COutPacket(&oPacket, 117);
    CClientSocket::SendPacket(TSingleton<CClientSocket>::ms_pInstance._m_pStr, &oPacket);

    this->m_bGotoWorldSelect = 1;
    this->m_bWorldInfoRequest = 0;

    CLogin::ChangeStep(this, 1);
  }
}
```

---

## CLogin::ProgressNewCharStep - 0xb66010

```cpp
void __thiscall CLogin::ProgressNewCharStep(CLogin *this)
{
  if (!this->m_bQuerySSNOnCreateNewCharacter)
  {
    CLogin::ChangeStep(this, -1);  // Just proceed to next step
    return;
  }

  ZXString<char> sSPW;
  sSPW._m_pStr = 0;

  char *m_pStr = this->m_sGoToStarPlanetSPW._m_pStr;
  if (m_pStr && *m_pStr)
  {
    // Use existing SPW
    ZXString<char>::operator=(&sSPW, &this->m_sGoToStarPlanetSPW);
  }
  else
  {
    // Show soft keyboard dialog for SPW entry
    CSoftKeyboardDlg *v4 = ZAllocEx<ZAllocAnonSelector>::Alloc(0xFA4u);
    if (v4)
      CSoftKeyboardDlg::CSoftKeyboardDlg(v4, 4, 16, 0, 1);

    ZRef<CDialog>::operator=(&this->m_pChildModal, v4);

    if (CDialog::DoModal(v4) != 1)
    {
      // User cancelled
      return;
    }

    ZXString<char>::operator=(&sSPW, CSoftKeyboardDlg::GetResult(v4, &result));
  }

  // Send SPW verification packet (opcode 937)
  COutPacket::COutPacket(&oPacket, 937);
  COutPacket::EncodeStr(&oPacket, sSPW);
  CClientSocket::SendPacket(TSingleton<CClientSocket>::ms_pInstance._m_pStr, &oPacket);

  // Cleanup
  if (this->m_pChildModal.p)
    this->m_pChildModal.p = 0;
}
```

---

## CLogin::OnNewCharStep - 0xb66c40

```cpp
void __thiscall CLogin::OnNewCharStep(CLogin *this, int bCharSale, int bEventNewChar)
{
  bool v3 = this->m_nLoginStep == 3;
  this->m_bCharSale = bCharSale;
  this->m_nCharSaleJob = 1;
  this->m_bEventNewChar = bEventNewChar;

  if (!v3)  // Not in race select step
  {
    int m_nBuyCharCount;
    if (bCharSale)
      m_nBuyCharCount = 0;
    else
      m_nBuyCharCount = this->m_nBuyCharCount;

    BOOL v5;
    if (bEventNewChar)
      v5 = 0;
    else
      v5 = this->m_nEventNewCharJob >= 0;

    // Check if slot available
    int slotIdx = this->m_nSlotCount - v5 - m_nBuyCharCount - 1;
    if (slotIdx <= 0)
      slotIdx = 0;

    if (TSingleton<CWvsContext>::ms_pInstance->m_aAvatarData_InWorld.a[slotIdx].characterStat.dwCharacterID)
    {
      // No slot available - show error
      CLoginUtilDlg::Error(9, &this->m_pChildModal);
    }
    else if (TSingleton<CWvsContext>::ms_pInstance->m_bUseSPW)
    {
      // SPW enabled - progress with SPW entry
      CLogin::ProgressNewCharStep(this);
    }
    else
    {
      // No SPW - request to set one
      CLogin::SendSetSPWRequest(this, 1);
    }
  }
}
```

---

## Network Packet Opcodes

| Opcode | Description |
|--------|-------------|
| 104 | World info request (with base step and web cookie) |
| 107 | Select character (with SPW, character ID, offline mode) |
| 108 | Client loading time report |
| 114 | Unknown (sent after world info request) |
| 117 | World info request (simple, for GotoWorldSelect) |
| 937 | SPW verification for new character |

---

## Login Steps Summary

| Step | Name | UI Created |
|------|------|------------|
| 0 | Title | CUITitle |
| 1 | World/Channel Select | CUIWorldSelect, CUIWorldSelectNotice |
| 2 | Character Select | CUISelectChar |
| 3 | Race Select | CUINewCharRaceSelect_Ex (in step 4 for event chars) |
| 4 | New Character | Sub-steps: Job→Gender→Frame→Avatar→Name |

### Step 4 Sub-steps:
| Sub-step | Name | UI Created |
|----------|------|------------|
| 0 | Job Select | CUINewCharJobSelectNormal |
| 1 | Gender Select | CUINewCharGenderSelect |
| 2 | Frame Select | CUINewCharEquipFrameSelect |
| 3 | Avatar Select | CUINewCharAvatarSelectCommon |
| 4 | Name Select | CreateUICharNameSelect |
