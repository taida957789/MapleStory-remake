#include "Loading.h"
#include "Login.h"
#include "app/Application.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "graphics/WzGr2DCanvas.h"
#include "util/Logger.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <cstdlib>
#include <ctime>

namespace ms
{

constexpr std::uint64_t kMinLoadingDisplayMs = 10000; // Minimum loading screen display time

Loading::Loading() = default;

Loading::~Loading() = default;

void Loading::Init(void* param)
{
    Stage::Init(param);

    InitLoading();
    StartLoadingMode();

    LOG_INFO("Loading stage initialized");
}

auto Loading::LoadLogoFrames(const std::shared_ptr<WzProperty>& prop)
    -> std::vector<std::shared_ptr<WzGr2DCanvas>>
{
    std::vector<std::shared_ptr<WzGr2DCanvas>> frames;

    if (!prop)
    {
        return frames;
    }

    // Try numbered children (0, 1, 2, etc.)
    int i = 0;
    while (auto child = prop->GetChild(std::to_string(i)))
    {
        ++i;

        auto wzCanvas = child->GetCanvas();
        auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas, child) : nullptr;
        if (canvas)
        {
            frames.push_back(canvas);
            continue;
        }

        // Check sub-properties
        for (const auto& [subName, subChild] : child->GetChildren())
        {
            wzCanvas = subChild->GetCanvas();
            canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas, subChild) : nullptr;
            if (canvas)
            {
                frames.push_back(canvas);
                break;
            }
        }
    }

    // If no numbered frames, check all children
    if (frames.empty())
    {
        for (const auto& [name, child] : prop->GetChildren())
        {
            auto wzCanvas = child->GetCanvas();
            auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas, child) : nullptr;
            if (canvas)
            {
                frames.push_back(canvas);
                continue;
            }

            for (const auto& [subName, subChild] : child->GetChildren())
            {
                auto subWzCanvas = subChild->GetCanvas();
                canvas = subWzCanvas ? std::make_shared<WzGr2DCanvas>(subWzCanvas, subChild) : nullptr;
                if (canvas)
                {
                    frames.push_back(canvas);
                    break;
                }
            }
        }
    }

    return frames;
}

void Loading::InitLoading()
{
    auto& resMan = WzResMan::GetInstance();
    auto loadingProp = resMan.GetProperty("UI/Logo.img/Loading");

    if (!loadingProp)
    {
        LOG_WARN("UI/Logo.img/Loading not found - loading screen disabled");
        return;
    }

    // Seed random number generator
    static bool randomSeeded = false;
    if (!randomSeeded)
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        randomSeeded = true;
    }

    // 1. Load random background
    auto randomBgProp = loadingProp->GetChild("randomBackgrd");
    if (randomBgProp && randomBgProp->HasChildren())
    {
        std::vector<std::string> loadImgNames;
        for (const auto& [name, child] : randomBgProp->GetChildren())
        {
            if (name.find("LoadImg") == 0)
            {
                loadImgNames.push_back(name);
            }
        }

        if (!loadImgNames.empty())
        {
            auto randomIndex = rand() % loadImgNames.size();
            const auto& selectedName = loadImgNames[randomIndex];
            auto loadImgProp = randomBgProp->GetChild(selectedName);

            if (loadImgProp)
            {
                auto bgFrameProp = loadImgProp->GetChild("0");
                if (bgFrameProp)
                {
                    auto bgProp = bgFrameProp->GetChild("backgrd");
                    auto bg1Prop = bgFrameProp->GetChild("backgrd1");

                    if (bgProp)
                    {
                        auto wzCanvas = bgProp->GetCanvas();
                        auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas, bgProp) : nullptr;
                        if (canvas)
                        {
                            m_loadingBgCanvases.push_back(canvas);
                        }
                    }

                    if (bg1Prop)
                    {
                        auto wzCanvas = bg1Prop->GetCanvas();
                        auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas, bg1Prop) : nullptr;
                        if (canvas)
                        {
                            m_loadingBgCanvases.push_back(canvas);
                        }
                    }

                    LOG_INFO("Selected random background: {} ({} canvases)",
                            selectedName, m_loadingBgCanvases.size());
                }
            }
        }
    }

    // 2. Load repeat animations
    auto repeatProp = loadingProp->GetChild("repeat");
    if (repeatProp && repeatProp->HasChildren())
    {
        int n = 0;
        while (auto repeatN = repeatProp->GetChild(std::to_string(n)))
        {
            ++n;

            auto frames = LoadLogoFrames(repeatN);
            if (!frames.empty())
            {
                m_repeatAnims.push_back(frames);
            }
        }
        LOG_INFO("Loaded {} repeat animations", m_repeatAnims.size());
    }

    // 3. Load step progress indicators — one layer per step for cumulative display
    auto stepProp = loadingProp->GetChild("step");
    auto& gr = get_gr();

    if (stepProp && stepProp->HasChildren())
    {
        auto stepFrames = LoadLogoFrames(stepProp);
        m_nLoadingStepCount = static_cast<std::int32_t>(stepFrames.size());

        for (std::int32_t s = 0; s < m_nLoadingStepCount; ++s)
        {
            // z-order: 12, 13, 14, ... one per step
            auto layer = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 12 + s);
            if (layer)
            {
                layer->InsertCanvas(stepFrames[static_cast<std::size_t>(s)]);
                layer->SetVisible(false);
            }
            m_stepLayers.push_back(layer);
        }
        LOG_INFO("Loaded {} loading steps (one layer each)", m_nLoadingStepCount);
    }

    // 4. Load grade frames (for overlay, currently TODO)
    auto gradeProp = resMan.GetProperty("UI/Logo.img/Grade");
    m_gradeFrames = LoadLogoFrames(gradeProp);

    // 5. Create loading layers (initially hidden)
    m_pLayerLoadingBg = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 10);
    if (m_pLayerLoadingBg)
    {
        m_pLayerLoadingBg->SetVisible(false);
    }

    m_pLayerLoadingAnim = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 11);
    if (m_pLayerLoadingAnim)
    {
        m_pLayerLoadingAnim->SetVisible(false);
    }
}

void Loading::StartLoadingMode()
{
    LOG_INFO("=== Starting loading mode ===");

    m_nLoadingStep = -1;
    m_loadingAlpha = 255;
    m_dwLoadingStartTick = Application::GetTick();
    m_nCurrentRepeat = 0;
    m_nCurrentRepeatFrame = 0;

    // Reset WZ loading state
    m_wzLoadingComplete = false;
    m_currentWzFileIndex = 0;

    // Setup background layer
    if (m_pLayerLoadingBg && !m_loadingBgCanvases.empty())
    {
        m_pLayerLoadingBg->RemoveAllCanvases();
        for (const auto& bgCanvas : m_loadingBgCanvases)
        {
            m_pLayerLoadingBg->InsertCanvas(bgCanvas);
        }
        m_pLayerLoadingBg->SetColor(0xFFFFFFFF);
        m_pLayerLoadingBg->SetVisible(true);
    }

    // Setup animation layer with repeat animation
    if (m_pLayerLoadingAnim && !m_repeatAnims.empty())
    {
        m_pLayerLoadingAnim->RemoveAllCanvases();

        auto& resMan = WzResMan::GetInstance();
        auto& firstRepeat = m_repeatAnims[0];
        auto repeatProp = resMan.GetProperty("UI/Logo.img/Loading/repeat/0");

        for (std::size_t i = 0; i < firstRepeat.size(); ++i)
        {
            int delay = 100;
            if (repeatProp)
            {
                auto frameProp = repeatProp->GetChild(std::to_string(i));
                if (frameProp)
                {
                    auto delayProp = frameProp->GetChild("delay");
                    if (delayProp)
                    {
                        delay = delayProp->GetInt(100);
                    }
                }
            }
            m_pLayerLoadingAnim->InsertCanvas(firstRepeat[i], delay, 255, 255);
        }

        // Start looping animation
        m_pLayerLoadingAnim->Animate(Gr2DAnimationType::Loop, 1000, -1);
        m_pLayerLoadingAnim->SetColor(0xFFFFFFFF);
        m_pLayerLoadingAnim->SetVisible(true);
        LOG_INFO("Animation layer: {} frames, looping", firstRepeat.size());
    }

    // Show initial step
    SetLoadingProgress(0);

    LOG_INFO("=== Loading mode started ===");
}

void Loading::SetLoadingProgress(std::int32_t step)
{
    if (step <= m_nLoadingStep)
    {
        return;
    }

    if (step >= m_nLoadingStepCount)
    {
        return;
    }

    // Cumulative: make layers visible from current+1 up to step
    auto start = std::max(0, m_nLoadingStep + 1);
    for (auto i = start; i <= step; ++i)
    {
        if (auto& layer = m_stepLayers[static_cast<std::size_t>(i)])
        {
            layer->SetVisible(true);
        }
    }

    m_nLoadingStep = step;

    if (step >= m_nLoadingStepCount - 1)
    {
        LOG_INFO("Loading complete - starting fade out");
        FadeOutLoading();
    }
}

void Loading::FadeOutLoading()
{
    if (m_loadingAlpha == 255)
    {
        m_loadingAlpha = 254;
    }
}

void Loading::LoadWzFilesProgressively()
{
    static const std::vector<std::string> wzFiles = {
        "Character", "Mob", "Skill", "Reactor", "Npc",
        "Quest", "Item", "Effect", "String",
        "Etc", "Morph", "TamingMob", "Sound", "Map"
    };

    if (m_wzLoadingComplete)
    {
        return;
    }

    if (m_currentWzFileIndex >= wzFiles.size())
    {
        m_wzLoadingComplete = true;
        SetLoadingProgress(m_nLoadingStepCount - 1);
        LOG_INFO("All WZ files loaded");
        return;
    }

    const auto& filename = wzFiles[m_currentWzFileIndex];
    auto& resMan = WzResMan::GetInstance();

    LOG_INFO("Loading WZ file: {}.wz ({}/{})", filename, m_currentWzFileIndex + 1, wzFiles.size());

    if (resMan.LoadWzFile(filename))
    {
        LOG_INFO("Loaded WZ file: {}.wz", filename);
    }
    else
    {
        LOG_WARN("Failed to load WZ file: {}.wz", filename);
    }

    m_currentWzFileIndex++;

    if (m_nLoadingStepCount > 0)
    {
        auto progress = static_cast<std::int32_t>(
            m_currentWzFileIndex * (m_nLoadingStepCount - 1) / wzFiles.size());
        SetLoadingProgress(progress);
    }
}

void Loading::Update()
{
    if (!m_wzLoadingComplete)
    {
        LoadWzFilesProgressively();
    }

    // Don't start fading until minimum display time has elapsed
    if (m_loadingAlpha < 255 && Application::GetTick() - m_dwLoadingStartTick < kMinLoadingDisplayMs)
    {
        return;
    }

    if (m_loadingAlpha < 255)
    {
        auto alphaInt = static_cast<std::int32_t>(m_loadingAlpha);
        alphaInt = std::max(0, alphaInt - 5);
        m_loadingAlpha = static_cast<std::uint8_t>(alphaInt);

        auto color = (static_cast<std::uint32_t>(m_loadingAlpha) << 24) | 0x00FFFFFF;
        if (m_pLayerLoadingBg) m_pLayerLoadingBg->SetColor(color);
        if (m_pLayerLoadingAnim) m_pLayerLoadingAnim->SetColor(color);
        for (auto& layer : m_stepLayers)
        {
            if (layer) layer->SetColor(color);
        }

        if (m_loadingAlpha == 0)
        {
            LOG_INFO("Fade out complete - transitioning to Login");
            GoToLogin();
        }

        return;
    }
}

void Loading::Draw()
{
    // Rendering is handled by WzGr2D::RenderFrame through the layer system
}

void Loading::Close()
{
    Stage::Close();

    auto& gr = get_gr();

    if (m_pLayerLoadingBg)
    {
        gr.RemoveLayer(m_pLayerLoadingBg);
        m_pLayerLoadingBg.reset();
    }

    if (m_pLayerLoadingAnim)
    {
        gr.RemoveLayer(m_pLayerLoadingAnim);
        m_pLayerLoadingAnim.reset();
    }

    for (auto& layer : m_stepLayers)
    {
        if (layer) gr.RemoveLayer(layer);
    }
    m_stepLayers.clear();

    m_loadingBgCanvases.clear();
    m_repeatAnims.clear();
    m_gradeFrames.clear();

    LOG_DEBUG("Loading stage closed");
}

void Loading::GoToLogin()
{
    auto login = std::make_shared<Login>();
    Application::GetInstance().SetStage(login);
}

} // namespace ms
