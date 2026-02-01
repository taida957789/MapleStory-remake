# CMapLoadable Decompiled Analysis

Based on reverse engineering of MapleStory v1029 client using IDA Pro.

## Overview

CMapLoadable is an intermediate base class between CStage and specific stages like CLogin, CField, etc. It provides layer management, BGM handling, camera control, and map loading functionality.

## Class Hierarchy

```
CMapLoadable : CStage : IGObj, IUIMsgHandler, INetMsgHandler, ZRefCounted
```

## Key Functions

| Address | Name | Size | Description |
|---------|------|------|-------------|
| 0xbf6f00 | `??0CMapLoadable@@QAE@XZ` | 0x604 | Constructor |
| 0xbf7560 | `??1CMapLoadable@@UAE@XZ` | 0x5c9 | Destructor |
| 0xbe1f70 | `?Init@CMapLoadable@@UAEXPAX@Z` | 0x235 | Virtual Init |
| 0xbdc910 | `?Close@CMapLoadable@@UAEXXZ` | 0x24a | Virtual Close |
| 0xbfbb00 | `?Update@CMapLoadable@@UAEXXZ` | 0x1a9 | Virtual Update |
| 0xbfff50 | `?LoadMap@CMapLoadable@@UAEXXZ` | 0xc6d | Virtual LoadMap |
| 0xbea1d0 | `?RestoreTile@CMapLoadable@@IAEXXZ` | 0x6f0 | Restore tiles |
| 0xbe6ff0 | `?RestoreViewRange@CMapLoadable@@QAEXXZ` | 0x41b | Restore view range |
| 0xbfbcb0 | `?RestoreBack@CMapLoadable@@IAEX_N@Z` | 0x348 | Restore backgrounds |
| 0xbff8e0 | `?RestoreObj@CMapLoadable@@IAEX_N@Z` | 0x3e0 | Restore objects |
| 0xbeada0 | `?RestoreWeather@CMapLoadable@@IAEXXZ` | 0xf3 | Restore weather |
| 0xbeb920 | `?RestoreLetterBox@CMapLoadable@@IAEXXZ` | 0x360 | Restore letterbox |
| 0xbeb070 | `?RestoreBGM@CMapLoadable@@IAEXH@Z` | 0x197 | Restore BGM |
| 0xbeaea0 | `?PlayBGMFromMapInfo@CMapLoadable@@IAEXXZ` | 0x1cb | Play BGM from map info |
| 0xbda610 | `?UpdateCameraMoveEffect@CMapLoadable@@IAEXJ@Z` | 0x438 | Update camera effect |
| 0xbd3bb0 | `?SetCameraMoveInfo@CMapLoadable@@QAEXJUtagPOINT@@000JH@Z` | 0x98 | Set camera info |
| 0xbd5030 | `??0CameraMoveInfo@CMapLoadable@@QAE@XZ` | 0x2e | CameraMoveInfo ctor |
| 0xbd3b80 | `?Clear@CameraMoveInfo@CMapLoadable@@QAEXXZ` | 0x2c | Clear camera info |
| 0x538040 | `?GetViewRangeRect@CMapLoadable@@QBEABUtagRECT@@XZ` | 0x7 | Get view range |
| 0x538050 | `?GetMinScaleForZoomOut@CMapLoadable@@QBEJXZ` | 0x7 | Get min scale |
| 0x635ac0 | `?IsSameChangeBGM@CMapLoadable@@QAE_NV?$ZXString@G@@@Z` | 0x3f | Check same BGM |
| 0x6fa4c0 | `?SetSysTrembleOpt@CMapLoadable@@QAEXH@Z` | 0xd | Set tremble opt |
| 0x899d10 | `?IsQuarterViewMap@CMapLoadable@@QBE_NXZ` | 0x7 | Is quarter view |
| 0xbdabf0 | `?SetObjectAnimation@CMapLoadable@@QAEXABV?$ZXString@D@@W4GR2D_ANITYPE@@@Z` | 0x181 | Set object animation |
| 0xbd9850 | `?TransientLayer_Clear@CMapLoadable@@IAEXXZ` | 0x25f | Clear transient layers |
| 0xbd6010 | `?AnimateObjLayer@CMapLoadable@@KAXV...@Z` | 0x293 | Animate object layer |

## Member Variables (from Constructor)

### BGM Related
```cpp
ZXString<wchar_t> m_sChangedBgmUOL;      // Current BGM UOL
ZXString<wchar_t> m_sFieldCustomBgmUOL;  // Custom field BGM
int m_nJukeBoxItemID{0};                 // Jukebox item ID
bool m_bJukeBoxPlaying{false};           // Is jukebox playing
unsigned int m_unWeatherSoundCookie{0};  // Weather sound cookie
```

### WZ Properties
```cpp
_com_ptr_t<IWzProperty> m_pPropFieldInfo;     // Field info property
_com_ptr_t<IWzProperty> m_pPropField;         // Field property
_com_ptr_t<IWzProperty> m_pPropFieldRefBack;  // Field reference back
```

### Physical Space
```cpp
ZRef<CWvsPhysicalSpace2D> m_pSpace2D;  // Physical space for collisions
```

### Layer Lists
```cpp
ZList<IWzGr2DLayer*> m_lpLayerGen;       // General layers
ZList<IWzGr2DLayer*> m_lpLayerObj;       // Object layers
ZList<IWzGr2DLayer*> m_lpLayerTransient; // Transient/temporary layers
ZList<IWzGr2DLayer*> m_lpLayerLetterBox; // Letterbox layers
```

### Layer Maps
```cpp
ZMap<ZXString<char>, IWzGr2DLayer*> m_mpLayerObj;   // Named object layers
ZMap<ZXString<char>, IWzGr2DLayer*> m_mTaggedLayer; // Tagged layers
ZMap<const char*, CHANGING_OBJECT> m_mNamedObj;     // Named changing objects
ZMap<const char*, ZRef<ZList<IWzGr2DLayer*>>> m_mTagedObj;   // Tagged objects
ZMap<const char*, ZRef<ZList<IWzGr2DLayer*>>> m_mTagedBack;  // Tagged backgrounds
ZMap<long, ZRef<ZList<IWzGr2DLayer*>>> m_mlLayerBack;        // Background layers by page
```

### Obstacles
```cpp
ZList<ZRef<OBSTACLE>> m_lpObstacle;        // Obstacle list
ZArray<OBSTACLE_INFO> m_aObstacleInfo;     // Obstacle info array
```

### Reflection
```cpp
ZList<ZRef<REFLECTION_INFO>> m_lpRefInfo;  // Reflection info
```

### Quest Visibility
```cpp
ZList<VISIBLE_BY_QUEST> m_lVisibleByQuest;  // Quest-based visibility
```

### Awesomium
```cpp
ZList<AWESOMIUM_INFO> m_lAwesomiumInfo;  // Embedded browser info
```

### Skeleton Animation (Spine)
```cpp
ZList<ZRef<spine::SkeletonAnimation>> m_lSkeletonObject;  // Skeleton objects
ZMap<long, ZRef<ZList<spine::SkeletonAnimation>>> m_mSkeletonBack;  // Skeleton backgrounds
ZMap<ZXString<char>, ZRef<ZList<spine::SkeletonAnimation>>> m_mTaggedSkeletonObject;  // Tagged skeleton objects
ZMap<ZXString<char>, ZRef<ZList<spine::SkeletonAnimation>>> m_mTaggedSkeletonBack;    // Tagged skeleton backgrounds
ZRef<SkeletonContactManager> m_pSkeletonContactManager;
```

### Mag Levels (Zoom/Scale)
```cpp
int m_nMagLevel_Obj{0};          // Object magnification level
int m_nMagLevel_Back{0};         // Background magnification level
int m_nMagLevel_SkillEffect{0};  // Skill effect magnification
int m_nMinZoomOutScale{1000};    // Minimum zoom out scale
bool m_bMagLevelModifying{false};
```

### Scale
```cpp
int m_nScaleField{1000};  // Field scale (1000 = 100%)
```

### Weather
```cpp
int m_nWeatherFadeInTime{0};
int m_tForceFadeOutTime{0};
```

### Map Properties
```cpp
bool m_bNeedZoomOutMap{false};
bool m_bNoFollowCharacter{false};
bool m_bStandAlone{false};
bool m_bPartyStandAlone{false};
bool m_bQuarterView{false};
bool m_bBGMVolumeOnly{false};
```

### BGM Restore
```cpp
int m_tRestoreBgmVolume{0};
int m_nRestoreBgmVolume{0};
```

### Camera Movement
```cpp
struct CameraMoveInfo
{
    bool bOn{false};
    bool bClipInViewRange{true};
    int tStart{0};
    int tEnd{0};
    POINT ptVelocity_First{0, 0};
    POINT ptAccelation{0, 0};         // Note: typo in original
    POINT ptVelocity_AdjustRate{0, 0};
    POINT ptAccelation_AdjustRate{0, 0};
} m_cameraMoveInfo;
```

### Zone Data Maps
```cpp
ZMap<ZXString<char>, ZRef<RectEventData>> m_mpRectEventData;
ZMap<ZXString<char>, ZRef<FadeData>> m_mpFadeData;
ZMap<ZXString<char>, ZRef<BgmZoneData>> m_mpBgmZoneData;
ZMap<ZXString<char>, ZRef<AmbienceZoneData>> m_mpAmbienceZoneData;
ZMap<ZXString<char>, ZRef<FootStepZoneData>> m_mpFootstepZoneData;
ZMap<ZXString<char>, ZRef<EffectZoneData>> m_mpEffectZoneData;
ZMap<ZXString<char>, ZRef<ScriptRunZoneData>> m_mpScriptRunZoneData;
ZMap<ZXString<char>, ZRef<SpineEventZone>> m_mpSpineEventZoneData;
ZMap<ZXString<char>, ZRef<CameraCtrlZone>> m_mpCameraCtrlZoneData;
```

### Sub BGM
```cpp
std::map<long, ZXString<char>> m_mSubBgm;  // STL map for sub BGM
```

### Footstep Sound
```cpp
ZXString<wchar_t> m_wsFootStepSound;
int m_nFootStepSoundCount{0};
_com_ptr_t<IWzProperty> m_pFootStepSoundProp;
```

### Color Flow
```cpp
ZXString<char> m_sColorFlowName;
```

### Delay Invisible Layers
```cpp
std::vector<DELAY_INVISIBLE_LAYER> m_aDelayInvisibleLayer;
```

---

## Constructor (0xbf6f00)

The constructor initializes all vtables and member variables:

```cpp
CMapLoadable::CMapLoadable()
{
    // Set vtables for inherited interfaces
    this->IGObj::__vftable = &CMapLoadable::`vftable'{for `IGObj'};
    this->IUIMsgHandler::__vftable = &CMapLoadable::`vftable'{for `IUIMsgHandler'};
    this->INetMsgHandler::__vftable = &CMapLoadable::`vftable'{for `INetMsgHandler'};
    this->ZRefCounted::__vftable = &CMapLoadable::`vftable'{for `ZRefCounted'};

    // Initialize BGM strings (empty)
    m_sChangedBgmUOL = L"";
    m_sFieldCustomBgmUOL = L"";

    // Initialize COM pointers
    m_pPropFieldInfo = nullptr;
    m_pPropField = nullptr;
    m_pPropFieldRefBack = nullptr;
    m_pSpace2D = nullptr;

    // Initialize lists
    m_lpLayerGen = ZList<>();
    m_lpLayerObj = ZList<>();
    m_lpLayerTransient = ZList<>();
    // ... etc for all ZList/ZMap members

    // Initialize default values
    m_nJukeBoxItemID = 0;
    m_bJukeBoxPlaying = false;
    m_unWeatherSoundCookie = 0;
    m_nMagLevel_Obj = 0;
    m_nMagLevel_Back = 0;
    m_nMagLevel_SkillEffect = 0;
    m_nMinZoomOutScale = 1000;
    m_bMagLevelModifying = false;
    m_nScaleField = 1000;
    m_nWeatherFadeInTime = 0;
    m_tForceFadeOutTime = 0;
    m_bNeedZoomOutMap = false;
    m_bNoFollowCharacter = false;
    m_bStandAlone = false;
    m_bPartyStandAlone = false;
    m_bQuarterView = false;
    m_bBGMVolumeOnly = false;

    // Initialize camera move info
    m_cameraMoveInfo.bOn = false;
    m_cameraMoveInfo.bClipInViewRange = true;
    m_cameraMoveInfo.tStart = 0;
    m_cameraMoveInfo.tEnd = 0;
    m_cameraMoveInfo.ptVelocity_First = {0, 0};
    m_cameraMoveInfo.ptAccelation = {0, 0};
    m_cameraMoveInfo.ptVelocity_AdjustRate = {0, 0};
    m_cameraMoveInfo.ptAccelation_AdjustRate = {0, 0};

    // Initialize zone data maps
    // ... initialize all ZMap members

    // Initialize STL map
    m_mSubBgm._Myhead = std::_Tree::_Buynode();
    m_mSubBgm._Myhead->_Isnil = 1;
    // ... standard STL map init

    // Initialize footstep sound
    m_wsFootStepSound = L"";
    m_nFootStepSoundCount = 0;
    m_pFootStepSoundProp = nullptr;

    // Initialize color flow name
    m_sColorFlowName = "";

    // Initialize delay invisible layer vector
    m_aDelayInvisibleLayer.clear();

    // Allocate skeleton contact manager
    ZRef<SkeletonContactManager>::_Alloc(&m_pSkeletonContactManager);
}
```

---

## Init (0xbe1f70)

```cpp
void CMapLoadable::Init(void* pParam)
{
    // Get video mag levels from config
    CONFIG_OPTION::GetVideo_MagLevel(
        CConfig::GetInstance()->video,
        &m_nMagLevel_Obj,
        &m_nMagLevel_Back);

    // Get skill effect mag level from config
    int nSkillOpt = CConfig::GetInstance()->nSkillEffect;
    if (nSkillOpt == 0)
        m_nMagLevel_SkillEffect = 3;  // High
    else if (nSkillOpt == 1)
        m_nMagLevel_SkillEffect = 2;  // Medium
    else if (nSkillOpt == 2)
        m_nMagLevel_SkillEffect = 1;  // Low
    else
        m_nMagLevel_SkillEffect = 0;  // Off

    // Set frame skip on graphics engine
    g_gr->SetFrameSkip();

    // Reset graphics center origin
    g_gr->center->put_origin(vtEmpty);
    g_gr->center->Move(0, 0);

    // Get and potentially reset screen scale
    int nScale = 1000;
    g_gr->GetScreenScale(&nScale);
    if (nScale != 1000)
        g_gr->SetScreenScale(1000);

    // Allocate physical space
    ZRef<CWvsPhysicalSpace2D>::_Alloc(&m_pSpace2D);

    // Show cursor
    CInputSystem::ShowCursor(1);

    // Clear back grayscale flag
    if (CWvsContext::GetInstance())
        CWvsContext::GetInstance()->m_bBackGrayScale = false;
}
```

---

## Close (0xbdc910)

```cpp
void CMapLoadable::Close()
{
    // Fade out with force fade out time
    CStage::FadeOut(m_tForceFadeOutTime);

    // Reset screen scale to 1000 (100%)
    g_gr->SetScreenScale(1000);

    // Reset graphics center
    g_gr->center->put_origin(vtEmpty);
    g_gr->center->Move(0, 0);

    // Release field properties
    if (m_pPropField)
    {
        m_pPropField->Release();
        m_pPropField = nullptr;
    }
    if (m_pPropFieldRefBack)
    {
        m_pPropFieldRefBack->Release();
        m_pPropFieldRefBack = nullptr;
    }

    // Clear all layer lists
    m_lpLayerGen.RemoveAll();
    m_lpLayerObj.RemoveAll();
    m_mpLayerObj.RemoveAll();
    m_mlLayerBack.RemoveAll();
    m_lBackEffect.RemoveAll();
    m_lpLayerTransient.RemoveAll();
    m_lpObstacle.RemoveAll();
    m_mTagedObj.RemoveAll();
    m_mTagedBack.RemoveAll();
    m_lpLayerLetterBox.RemoveAll();

    // Clear skeleton lists
    m_lSkeletonObject.RemoveAll();
    m_mSkeletonBack.RemoveAll();
    m_mTaggedSkeletonObject.RemoveAll();
    m_mTaggedSkeletonBack.RemoveAll();

    // Release physical space
    if (m_pSpace2D)
    {
        m_pSpace2D.Release();
        m_pSpace2D = nullptr;
    }

    // Apply system options
    CConfig::ApplySysOpt(0, 0);
}
```

---

## Update (0xbfbb00)

```cpp
void CMapLoadable::Update()
{
    static int nCounter = 0;

    // Every 4 frames, clean up transient layers with 0 alpha
    if (!(++nCounter % 4))
    {
        __POSITION* pos = m_lpLayerTransient.GetHeadPosition();
        while (pos)
        {
            __POSITION* posSave = pos;
            IWzGr2DLayer* pLayer = m_lpLayerTransient.GetNext(pos);

            int alpha = 0;
            pLayer->alpha->get_x(&alpha);

            if (alpha == 0)
            {
                m_lpLayerTransient.RemoveAt(posSave);
            }
        }
    }

    unsigned int now = timeGetTime();

    // Play next music if jukebox is active
    if (!CWvsContext::GetInstance()->m_bDirectionMode &&
        m_nJukeBoxItemID != 0 &&
        (now - m_tNextMusic) > 0)
    {
        PlayNextMusic();
    }

    // Restore BGM volume if needed
    if (m_nRestoreBgmVolume != 0 && (now - m_tRestoreBgmVolume) < 0)
    {
        CSoundMan::SetBGMVolume(m_nRestoreBgmVolume, 500);
        m_tRestoreBgmVolume = 0;
        m_nRestoreBgmVolume = 0;
    }

    // Update layer visibility
    UpdateLayerInvisible();
}
```

---

## LoadMap (0xbfff50)

```cpp
void CMapLoadable::LoadMap()
{
    // Get "foothold" property from field
    IWzProperty* pPropFoothold = m_pPropField->GetItem(
        StringPool::GetBSTR(0x9D0));  // "foothold"

    // Get "ladderRope" property
    IWzProperty* pLadderRope = m_pPropField->GetItem(
        StringPool::GetBSTR(0x9D1));  // "ladderRope"

    // Load physical space with foothold and ladder info
    m_pSpace2D->Load(pPropFoothold, pLadderRope, m_pPropFieldInfo);

    // Read map info properties
    if (m_pPropFieldInfo)
    {
        // Quarter view
        m_bQuarterView = m_pPropFieldInfo->GetItem(L"quarterView")->GetInt32(0) != 0;

        // Color flow name
        m_sColorFlowName = m_pPropFieldInfo->GetItem(L"colorFlow")->GetBSTR("");

        // Scale
        int enterScale = m_pPropFieldInfo->GetItem(L"enterScale")->GetInt32(1000);
        if (enterScale == 1000)
        {
            m_nScaleField = m_pPropFieldInfo->GetItem(L"scale")->GetInt32(1000);
        }
        else
        {
            m_nScaleField = enterScale;
            g_gr->SetScreenScale(enterScale);
        }

        // Zoom out map flag
        m_bNeedZoomOutMap = m_pPropFieldInfo->GetItem(L"zoomOutField")->GetInt32(0) != 0;
    }

    // Restore map elements
    RestoreTile();
    RestoreViewRange();
    RestoreObj(true);
    RestoreBack(true);
    RestoreWeather();
    RestoreLetterBox();

    // Load life (NPCs and mobs)
    IWzProperty* pLife = m_pPropField->GetItem(StringPool::GetBSTR(0xC43));  // "life"
    if (pLife)
    {
        for (int i = 0; ; i++)
        {
            IWzProperty* pEntity = pLife->GetItem(_itow(i));
            if (!pEntity)
                break;

            ZXString<wchar_t> sType = pEntity->GetItem(L"type")->GetBSTR(L"");
            int dwID = pEntity->GetItem(L"id")->GetInt32(0);
            ZXString<char> sGroupName = pEntity->GetItem(L"groupName")->GetBSTR("");
            int nSideType = pEntity->GetItem(L"sideType")->GetInt32(0);

            if (wcscmp(sType, L"n") == 0)
            {
                // NPC
                CNpcTemplate::GetNpcTemplate(dwID);
                if (sGroupName && *sGroupName)
                {
                    SetFadeData(&sGroupName, nSideType, 3, dwID);
                }
            }
            else if (wcscmp(sType, L"m") == 0)
            {
                // Mob
                CMobTemplate::GetMobTemplate(dwID);
            }
        }
    }

    // Load reactors
    IWzProperty* pReactor = m_pPropField->GetItem(StringPool::GetBSTR(0x125B));  // "reactor"
    if (pReactor)
    {
        for (int j = 0; ; j++)
        {
            IWzProperty* pR = pReactor->GetItem(_itow(j));
            if (!pR)
                break;

            int dwID = pR->GetItem(L"id")->GetInt32(0);
            CReactorTemplate::GetReactorTemplate(dwID);
        }
    }

    // Load rect event data
    LoadRectEventData();

    // Set default footstep sound
    SetFootStepSound(L"");
}
```

---

## RestoreViewRange (0xbe6ff0)

```cpp
void CMapLoadable::RestoreViewRange()
{
    // Get current screen scale
    int nScale = 1000;
    g_gr->GetScreenScale(&nScale);

    // Calculate half screen dimensions
    int nHalfWidth = get_screen_width() / 2;
    int nHalfHeight = get_screen_height() / 2;

    // Scale by current zoom level
    float fScale = nScale * 0.001f;
    int nScaledHalfWidth = (int)(nHalfWidth / fScale);
    int nScaledHalfHeight = (int)(nHalfHeight / fScale);

    // Get physical space bounds (MBR = minimum bounding rectangle)
    RECT* mbr = CWvsPhysicalSpace2D::GetMBR();

    // Read view range from field info, with defaults based on MBR
    m_rcViewRange.left = m_pPropFieldInfo->GetItem(L"VRLeft")->GetInt32(mbr->left - 20);
    m_rcViewRange.top = m_pPropFieldInfo->GetItem(L"VRTop")->GetInt32(mbr->top - 60);
    m_rcViewRange.right = m_pPropFieldInfo->GetItem(L"VRRight")->GetInt32(mbr->right + 20);
    m_rcViewRange.bottom = m_pPropFieldInfo->GetItem(L"VRBottom")->GetInt32(mbr->bottom + 190);

    // Calculate min zoom out scale
    int nMarginX = (m_rcViewRange.left + m_rcViewRange.right) / 2 - m_rcViewRange.left;
    int nMarginY = (m_rcViewRange.top + m_rcViewRange.bottom) / 2 - m_rcViewRange.top;

    if (nMarginX >= 0 && nMarginY >= 0)
    {
        float scaleX = nHalfWidth * 1000.0f / nMarginX;
        float scaleY = nHalfHeight * 1000.0f / nMarginY;

        m_nMinZoomOutScale = (int)max(scaleX, scaleY);

        if (m_nMinZoomOutScale < 1)
            m_nMinZoomOutScale = 1;
        else if (m_nMinZoomOutScale > 1000)
            m_nMinZoomOutScale = 1000;
    }
    else
    {
        m_nMinZoomOutScale = 1000;
    }

    // Adjust view range by scaled half dimensions
    m_rcViewRange.left += nScaledHalfWidth;
    m_rcViewRange.right -= nScaledHalfWidth;
    m_rcViewRange.top += nScaledHalfHeight;
    m_rcViewRange.bottom -= nScaledHalfHeight;

    // Clamp if dimensions become negative
    if (m_rcViewRange.right - m_rcViewRange.left <= 0)
    {
        int mid = (m_rcViewRange.right + m_rcViewRange.left) / 2;
        m_rcViewRange.left = mid;
        m_rcViewRange.right = mid;
    }
    if (m_rcViewRange.bottom - m_rcViewRange.top <= 0)
    {
        int mid = (m_rcViewRange.bottom + m_rcViewRange.top) / 2;
        m_rcViewRange.top = mid;
        m_rcViewRange.bottom = mid;
    }

    // Apply vertical adjustment
    int adjustY = get_adjust_cy();
    m_rcViewRange.top += adjustY;
    m_rcViewRange.bottom += adjustY;
}
```

---

## RestoreTile (0xbea1d0)

```cpp
void CMapLoadable::RestoreTile()
{
    // Iterate through 8 pages
    for (int nPageIdx = 0; nPageIdx < 8; ++nPageIdx)
    {
        // Get page property (e.g., "0", "1", ...)
        IWzProperty* pPage = m_pPropField->GetItem(_itow(nPageIdx));
        if (!pPage)
            continue;

        // Get info sub-property
        IWzProperty* pInfo = pPage->GetItem(L"info");

        // Get tile sub-property
        IWzProperty* pTile = pPage->GetItem(L"tile");
        if (!pTile)
            continue;

        unsigned int nCount = 0;
        pTile->get_count(&nCount);
        if (nCount <= 0)
            continue;

        // Build UOL path: "Map/Tile/" + tileSetName
        ZXString<wchar_t> sUOL = StringPool::GetStringW(0x9D3);  // "Map/Tile/"
        ZXString<wchar_t> sTileSet = pInfo->GetItem(L"tS")->GetBSTR(L"");
        sUOL += sTileSet;

        // Get tile set property
        IWzProperty* pTileSet = g_rm->GetObject(sUOL);

        // Create tile for each entry
        for (int i = 0; i < nCount; ++i)
        {
            IWzProperty* pPiece = pTile->GetItem(_itow(i));
            if (pPiece)
            {
                MakeTile(nPageIdx, pTileSet, pPiece);
            }
        }
    }
}
```

---

## SetCameraMoveInfo (0xbd3bb0)

```cpp
void CMapLoadable::SetCameraMoveInfo(
    int tStart,
    POINT ptVelocity,
    POINT ptAcceleration,
    POINT ptVelocityAdjust,
    POINT ptAccelAdjust,
    int duration,
    int bClipInViewRange)
{
    m_cameraMoveInfo.bOn = true;
    m_cameraMoveInfo.tStart = tStart;
    m_cameraMoveInfo.tEnd = tStart + duration;
    m_cameraMoveInfo.ptVelocity_First = ptVelocity;
    m_cameraMoveInfo.ptAccelation = ptAcceleration;
    m_cameraMoveInfo.ptVelocity_AdjustRate = ptVelocityAdjust;
    m_cameraMoveInfo.ptAccelation_AdjustRate = ptAccelAdjust;
    m_cameraMoveInfo.bClipInViewRange = bClipInViewRange != 0;
}
```

---

## CameraMoveInfo::Clear (0xbd3b80)

```cpp
void CMapLoadable::CameraMoveInfo::Clear()
{
    bOn = false;
    bClipInViewRange = true;
    tStart = 0;
    tEnd = 0;
    ptVelocity_First = {0, 0};
    ptAccelation = {0, 0};
    ptVelocity_AdjustRate = {0, 0};
    ptAccelation_AdjustRate = {0, 0};
}
```

---

## Nested Structures

### OBSTACLE_INFO
```cpp
struct CMapLoadable::OBSTACLE_INFO
{
    ZRef<OBSTACLE> pObstacle;
    // ... other fields
};
```

### CHANGING_OBJECT
```cpp
struct CMapLoadable::CHANGING_OBJECT
{
    // Object that can change state
    // ... fields
};
```

### VISIBLE_BY_QUEST
```cpp
struct CMapLoadable::VISIBLE_BY_QUEST
{
    // Quest-based visibility data
    // ... fields
};
```

### AWESOMIUM_INFO
```cpp
struct CMapLoadable::AWESOMIUM_INFO
{
    // Embedded browser/web content info
    // ... fields
};
```

### REFLECTION_INFO
```cpp
struct CMapLoadable::REFLECTION_INFO
{
    // Reflection/mirror effect info
    // ... fields
};
```

### OBJECT_STATE
```cpp
struct CMapLoadable::OBJECT_STATE
{
    // Object state data
    // ... fields
};
```

### DELAY_INVISIBLE_LAYER
```cpp
struct CMapLoadable::DELAY_INVISIBLE_LAYER
{
    // Delayed layer visibility
    // ... fields
};
```

---

## Key Behaviors

1. **Initialization**: `Init()` reads config options for magnification levels and initializes the graphics engine state
2. **Map Loading**: `LoadMap()` orchestrates loading of tiles, objects, backgrounds, life entities, and reactors
3. **View Range**: Calculated from field info properties with fallbacks to physical space MBR
4. **Layer Management**: Multiple layer lists for different purposes (general, object, transient, letterbox, background)
5. **Update Loop**: Cleans up faded transient layers every 4 frames, handles jukebox playback and BGM restore
6. **Camera Control**: `CameraMoveInfo` structure allows animated camera movement with velocity and acceleration

---

## String Pool References

| ID | Usage |
|----|-------|
| 0x9D0 | "foothold" |
| 0x9D1 | "ladderRope" |
| 0x9D3 | "Map/Tile/" |
| 0xA0E | "VRLeft" (or similar) |
| 0xA0F | "VRRight" |
| 0xA10 | "VRTop" |
| 0xA11 | "VRBottom" |
| 0xC43 | "life" |
| 0x125B | "reactor" |
| 0x2B35 | "quarterView" |
