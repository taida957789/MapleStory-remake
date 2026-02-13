#include "WzGr2DLayer.h"
#include "WzGr2DCanvas.h"

#ifdef MS_DEBUG_CANVAS
#include "util/Logger.h"
#endif

#include <SDL3/SDL.h>
#include <algorithm>

namespace ms
{

using namespace Gr2DConstants;

// ============================================================
// Static members
// ============================================================

std::int32_t WzGr2DLayer::s_idCounter = 0;

// ============================================================
// ParticleEmitter
// ============================================================

void ParticleEmitter::reset()
{
    activeCount = 0;
    frameAccumulator = 0.0F;
    elapsedTime = 0.0F;
    for (auto& p : particles)
    {
        p.timeRemaining = 0.0F;
    }
}

void ParticleEmitter::update(float deltaTime, float /*param3*/, int /*param4*/, float param5)
{
    float dt = deltaTime * timeScale;
    if (dt <= 0.0F)
    {
        return;
    }

    elapsedTime += dt;

    if (maxLifetime > 0.0F && elapsedTime >= maxLifetime)
    {
        reset();
        return;
    }

    // Emit new particles
    if (emitInterval > 0.0F)
    {
        frameAccumulator += dt;
        while (frameAccumulator >= emitInterval && activeCount < maxParticles)
        {
            int slot = -1;
            for (int i = 0; i < static_cast<int>(particles.size()); ++i)
            {
                if (particles[i].timeRemaining <= 0.0F)
                {
                    slot = i;
                    break;
                }
            }
            if (slot < 0)
            {
                if (static_cast<int>(particles.size()) < maxParticles)
                {
                    particles.emplace_back();
                    slot = static_cast<int>(particles.size()) - 1;
                }
                else
                {
                    break;
                }
            }

            Particle& p = particles[slot];
            p = Particle{};

            if (positionType == 1 && animOrigin != nullptr)
            {
                p.posX = static_cast<float>(animOrigin->GetX() + originX);
                p.posY = static_cast<float>(animOrigin->GetY() + originY);
            }
            else
            {
                p.posX = static_cast<float>(originX);
                p.posY = static_cast<float>(originY);
            }

            p.baseVelX = velocityX;
            p.baseVelY = velocityY;
            p.colorR = p.startR;
            p.colorG = p.startG;
            p.colorB = p.startB;
            p.colorA = p.startA;

            if (p.totalLifetime <= 0.0F)
            {
                p.totalLifetime = 1.0F;
            }
            p.timeRemaining = p.totalLifetime;
            p.forceScaleA = 1.0F;
            p.forceScaleB = 1.0F;

            activeCount++;
            frameAccumulator -= emitInterval;
        }
    }

    // Update alive particles
    int newActiveCount = 0;
    for (auto& p : particles)
    {
        if (p.timeRemaining <= 0.0F)
        {
            continue;
        }

        p.timeRemaining -= dt;
        if (p.timeRemaining <= 0.0F)
        {
            p.timeRemaining = 0.0F;
            continue;
        }

        newActiveCount++;

        float progress = 1.0F - p.timeRemaining / p.totalLifetime;
        p.colorR = p.startR + (p.endR - p.startR) * progress;
        p.colorG = p.startG + (p.endG - p.startG) * progress;
        p.colorB = p.startB + (p.endB - p.startB) * progress;
        p.colorA = p.startA + (p.endA - p.startA) * progress;

        p.sizeCurrent += p.sizeRate * dt;
        p.rotationRate += p.rotationAccel * dt;
        p.angularData[0] += p.rotationRate * dt;

        if (usePhysics)
        {
            float totalForceX = (forceXA + forceXB) * p.forceScaleA;
            float totalForceY = (forceYA + forceYB) * p.forceScaleB;

            p.driftX += totalForceX * dt;
            p.driftY += totalForceY * dt;

            p.posX += (p.baseVelX + p.driftX) * dt;
            p.posY += (p.baseVelY + p.driftY) * dt * static_cast<float>(mirrorDirection);

            if (affectGravity)
            {
                p.baseVelY += 9.8F * dt;
            }
        }
        else
        {
            p.posX += p.baseVelX * dt + param5;
            p.posY += p.baseVelY * dt * static_cast<float>(mirrorDirection);
        }

        p.colorA *= opacityMultiplier;
    }

    activeCount = newActiveCount;
}

// ============================================================
// SDL blend mode helpers
// ============================================================

namespace
{

auto ConvertToSDLBlendMode(std::int32_t blendType) -> SDL_BlendMode
{
    auto baseMode = blendType & 0x3F3;

    if (baseMode & static_cast<std::int32_t>(LayerBlendType::Add))
    {
        return SDL_BLENDMODE_ADD;
    }
    if (baseMode & static_cast<std::int32_t>(LayerBlendType::Multiply))
    {
        return SDL_BLENDMODE_MUL;
    }
    if (baseMode & static_cast<std::int32_t>(LayerBlendType::LinearDodge))
    {
        return SDL_BLENDMODE_ADD;
    }

    return SDL_BLENDMODE_BLEND;
}

} // anonymous namespace

// ============================================================
// Constructor / Destructor
// ============================================================

WzGr2DLayer::WzGr2DLayer()
{
    m_uniqueId = ++s_idCounter;
    m_hashTable.fill(nullptr);
    ensureVectors();
}

WzGr2DLayer::WzGr2DLayer(std::int32_t left, std::int32_t top,
                           std::uint32_t width, std::uint32_t height,
                           std::int32_t z)
    : m_width(static_cast<std::int32_t>(width))
    , m_height(static_cast<std::int32_t>(height))
    , m_zOrder(z)
    , m_nLeft(left)
    , m_nTop(top)
{
    m_uniqueId = ++s_idCounter;
    m_hashTable.fill(nullptr);
    ensureVectors();
    if (m_positionVec)
    {
        m_positionVec->Move(left, top);
    }
}

WzGr2DLayer::~WzGr2DLayer()
{
    clearFrames();
}

WzGr2DLayer::WzGr2DLayer(WzGr2DLayer&&) noexcept = default;
auto WzGr2DLayer::operator=(WzGr2DLayer&&) noexcept -> WzGr2DLayer& = default;

// ============================================================
// Initialization
// ============================================================

void WzGr2DLayer::SetVideoMode(std::int32_t screenW, std::int32_t screenH,
                                std::int32_t viewW, std::int32_t viewH)
{
    m_screenVec.reset();
    m_alphaVec.reset();
    m_colorRedVec.reset();
    m_colorGBVec.reset();
    m_positionVec.reset();
    m_rbVec.reset();

    m_screenVec = std::make_unique<Gr2DVector>(0, 0);
    m_alphaVec = std::make_unique<Gr2DVector>(0, 0);
    m_colorRedVec = std::make_unique<Gr2DVector>(0, 0);
    m_colorGBVec = std::make_unique<Gr2DVector>(0, 0);
    m_positionVec = std::make_unique<Gr2DVector>(0, 0);
    m_rbVec = std::make_unique<Gr2DVector>(0, 0);

    m_screenVec->RelMove(screenW, screenH, 0, 0);

    m_positionVec->PutOrigin(m_screenVec.get());
    m_positionVec->RelMove(viewW, viewH, 0, 0);

    m_rbVec->PutOrigin(m_screenVec.get());
    m_rbVec->RelMove(viewW, viewH, 0, 0);

    m_width = viewW;
    m_height = viewH;

    put_color(0xFFFFFFFF);
}

void WzGr2DLayer::InitAnimation(std::int32_t baseTimestamp)
{
    m_animOriginVec.reset();
    m_animIntermediate.reset();

    m_baseTimestamp = baseTimestamp;

    m_animOriginVec = std::make_unique<Gr2DVector>(0, 0);
    m_animOriginVec->RelMove(0, 0, 0, 0);
}

void WzGr2DLayer::SetAnimOrigin(Gr2DVector* origin)
{
    if (!m_animOriginVec)
    {
        return;
    }

    m_animIntermediate = std::make_unique<Gr2DVector>(0, 0);

    m_animOriginVec->PutOrigin(m_animIntermediate.get());
    m_animIntermediate->PutOrigin(origin);
}

// ============================================================
// Frame hash table
// ============================================================

auto WzGr2DLayer::hashFrameId(std::int32_t id) -> std::uint32_t
{
    auto uid = static_cast<std::uint32_t>(id);
    std::uint32_t ror = (uid >> 5) | (uid << 27);
    return ror % HASH_BUCKETS;
}

void WzGr2DLayer::insertFrameHash(FrameNode* node)
{
    std::uint32_t bucket = hashFrameId(node->frameId);
    node->hashNext = m_hashTable[bucket];
    m_hashTable[bucket] = node;
}

void WzGr2DLayer::removeFrameHash(FrameNode* node)
{
    std::uint32_t bucket = hashFrameId(node->frameId);
    FrameNode** pp = &m_hashTable[bucket];
    while (*pp)
    {
        if (*pp == node)
        {
            *pp = node->hashNext;
            node->hashNext = nullptr;
            return;
        }
        pp = &(*pp)->hashNext;
    }
}

auto WzGr2DLayer::findFrameById(std::int32_t id) const -> FrameNode*
{
    std::uint32_t bucket = hashFrameId(id);
    FrameNode* node = m_hashTable[bucket];
    while (node)
    {
        if (node->frameId == id)
        {
            return node;
        }
        node = node->hashNext;
    }
    return nullptr;
}

auto WzGr2DLayer::findFrameByIndex(int index) const -> FrameNode*
{
    if (index < 0 || index >= m_frameCount)
    {
        return nullptr;
    }
    FrameNode* node = m_frameHead;
    for (int i = 0; i < index && node; ++i)
    {
        node = node->next;
    }
    return node;
}

// ============================================================
// Frame management (source-matching)
// ============================================================

auto WzGr2DLayer::InsertCanvas(ICanvas* canvas, std::int32_t duration,
                                std::int32_t alpha, std::int32_t colorMod,
                                std::int32_t blendSrc, std::int32_t blendDst) -> int
{
    if (!canvas)
    {
        return -1;
    }

    auto* node = new FrameNode();

    do
    {
        node->frameId = m_frameIdCounter++;
    } while (node->frameId == -1);

    node->canvas = canvas;
    node->duration = duration;
    node->alphaA = alpha;
    node->alphaB = colorMod;
    node->blendSrc = blendSrc;
    node->blendDst = blendDst;

    // Append to linked list
    node->prev = m_frameTail;
    node->next = nullptr;
    if (m_frameTail)
    {
        m_frameTail->next = node;
    }
    else
    {
        m_frameHead = node;
    }
    m_frameTail = node;

    insertFrameHash(node);

    m_frameCount++;
    m_totalDuration += duration;

    if (m_frameCount == 1)
    {
        m_currentFrame = node;
    }

    return node->frameId;
}

void WzGr2DLayer::RemoveCanvas(int index)
{
    FrameNode* node = findFrameByIndex(index);
    if (!node)
    {
        return;
    }

    if (node->prev)
    {
        node->prev->next = node->next;
    }
    else
    {
        m_frameHead = node->next;
    }

    if (node->next)
    {
        node->next->prev = node->prev;
    }
    else
    {
        m_frameTail = node->prev;
    }

    removeFrameHash(node);

    m_totalDuration -= node->duration;
    m_frameCount--;

    if (m_currentFrame == node)
    {
        m_currentFrame = m_frameHead;
    }

    delete node;
}

void WzGr2DLayer::InitCanvasOrder()
{
    m_currentFrame = m_frameHead;
    m_animTimer = 0;
}

void WzGr2DLayer::ShiftCanvas(int index)
{
    if (m_frameCount == 0)
    {
        return;
    }

    int effective = index % m_frameCount;
    if (effective < 0)
    {
        effective += m_frameCount;
    }

    FrameNode* node = findFrameByIndex(effective);
    if (node)
    {
        m_currentFrame = node;
    }
}

void WzGr2DLayer::SetFrameCanvas(int index, ICanvas* canvas)
{
    FrameNode* node = findFrameByIndex(index);
    if (node)
    {
        node->canvas = canvas;
    }
}

auto WzGr2DLayer::get_canvasCount() const -> int
{
    return m_frameCount;
}

auto WzGr2DLayer::get_canvas() const -> ICanvas*
{
    return m_currentFrame ? m_currentFrame->canvas : nullptr;
}

void WzGr2DLayer::clearFrames()
{
    FrameNode* node = m_frameHead;
    while (node)
    {
        FrameNode* next = node->next;
        delete node;
        node = next;
    }
    m_frameHead = nullptr;
    m_frameTail = nullptr;
    m_currentFrame = nullptr;
    m_frameCount = 0;
    m_frameIdCounter = 0;
    m_totalDuration = 0;
    m_hashTable.fill(nullptr);
    m_renderCommands.clear();
}

// ============================================================
// Frame management (backward-compatible wrappers)
// ============================================================

auto WzGr2DLayer::InsertCanvas(std::shared_ptr<WzGr2DCanvas> canvas,
                                std::int32_t delay,
                                std::uint8_t alpha0,
                                std::uint8_t alpha1,
                                std::int32_t /*zoom0*/,
                                std::int32_t /*zoom1*/) -> std::size_t
{
    if (!canvas)
    {
        return 0;
    }

    // Keep shared_ptr alive
    m_ownedCanvases.push_back(canvas);

    // Map alpha0 -> alphaA, alpha1 -> alphaB
    std::int32_t frameAlpha = (alpha0 == 255) ? -1 : static_cast<std::int32_t>(alpha0);
    std::int32_t frameColorMod = (alpha1 == 255) ? -1 : static_cast<std::int32_t>(alpha1);

    InsertCanvas(static_cast<ICanvas*>(canvas.get()), delay, frameAlpha, frameColorMod);

    return static_cast<std::size_t>(m_frameCount > 0 ? m_frameCount - 1 : 0);
}

void WzGr2DLayer::RemoveAllCanvases()
{
    clearFrames();
    m_ownedCanvases.clear();
    m_bAnimating = false;
}

auto WzGr2DLayer::GetCanvasCount() const noexcept -> std::size_t
{
    return static_cast<std::size_t>(m_frameCount);
}

auto WzGr2DLayer::GetCanvas(std::size_t index) const -> std::shared_ptr<WzGr2DCanvas>
{
    if (index >= m_ownedCanvases.size())
    {
        return nullptr;
    }
    return m_ownedCanvases[index];
}

auto WzGr2DLayer::GetCurrentCanvas() const -> std::shared_ptr<WzGr2DCanvas>
{
    // Find current frame index and return from owned canvases
    if (!m_currentFrame || m_ownedCanvases.empty())
    {
        return nullptr;
    }

    int idx = 0;
    for (FrameNode* node = m_frameHead; node; node = node->next, ++idx)
    {
        if (node == m_currentFrame)
        {
            if (static_cast<std::size_t>(idx) < m_ownedCanvases.size())
            {
                return m_ownedCanvases[static_cast<std::size_t>(idx)];
            }
            break;
        }
    }
    return nullptr;
}

// ============================================================
// Animation (source-matching)
// ============================================================

auto WzGr2DLayer::computeAlpha(std::int32_t frameAlpha) const -> std::int32_t
{
    if (!m_alphaVec)
    {
        return 255;
    }

    // Note: get_x() in source = GetX() in local, but for standalone vectors
    // without parent chain, GetRX() (relative/local) is more appropriate
    std::int32_t layerAlpha = m_alphaVec->GetX();

    if (frameAlpha < 0)
    {
        return std::clamp(layerAlpha, 0, 255);
    }

    return std::clamp(static_cast<std::int32_t>(
        static_cast<float>(layerAlpha * frameAlpha) / 255.0F + 0.5F), 0, 255);
}

auto WzGr2DLayer::Animate(std::uint32_t flags, std::int32_t timeDelta, int targetFrame) -> int
{
    // 0x10 and 0x20 are mutually exclusive
    if ((flags & 0x30) == 0x30)
    {
        return -1;
    }

    m_renderCommands.clear();

    if (m_frameCount == 0)
    {
        return 0;
    }

    // 0x200 flag: reset animation timer
    if (flags & 0x200)
    {
        m_animTimer = 0;
    }

    // Determine traversal direction
    bool reverse = (flags & 0x40) != 0 && ((flags >> 6) & 1);
    FrameNode* cursor = reverse ? m_frameTail : m_frameHead;

    // Determine target frame
    FrameNode* targetNode = nullptr;
    if (targetFrame >= 0)
    {
        targetNode = findFrameByIndex(targetFrame);
    }
    else
    {
        targetNode = m_currentFrame;
    }

    // Time-based mode: calculate playback position
    std::int32_t totalDur = m_totalDuration;
    std::int32_t timePos = 0;
    if ((flags & 0x20) && totalDur > 0)
    {
        timePos = static_cast<std::int32_t>(
            static_cast<std::int64_t>(timeDelta) * totalDur / 1000);
    }

    // Build render commands for each frame
    std::int32_t accumTime = 0;
    int frameIdx = 0;
    while (cursor)
    {
        RenderCommand cmd{};
        cmd.frameIndex = reverse ? (m_frameCount - 1 - frameIdx) : frameIdx;
        cmd.timestamp = accumTime;
        accumTime += cursor->duration;

        if (cursor == targetNode)
        {
            cmd.currentFrameTime = timePos;
        }
        else
        {
            cmd.currentFrameTime = -1;
        }

        cmd.alpha = computeAlpha(cursor->alphaA);

        if (cursor->alphaB >= 0 && m_alphaVec)
        {
            std::int32_t layerAlpha = m_alphaVec->GetX();
            cmd.colorMod = std::clamp(static_cast<std::int32_t>(
                static_cast<float>(layerAlpha * cursor->alphaB) / 255.0F + 0.5F), 0, 255);
        }
        else
        {
            cmd.colorMod = computeAlpha(-1);
        }

        cmd.blendSrc = cursor->blendSrc;
        cmd.blendDst = cursor->blendDst;

        ICanvas* canv = cursor->canvas;
        if (canv && canv->isReady())
        {
            cmd.textureHandle = canv->getTextureHandle();
            cmd.srcX = canv->getSrcX();
            cmd.srcY = canv->getSrcY();
            cmd.srcW = canv->getSrcW();
            cmd.srcH = canv->getSrcH();
            cmd.dstW = canv->getWidth();
            cmd.dstH = canv->getHeight();
        }

        m_renderCommands.push_back(cmd);

        frameIdx++;
        cursor = reverse ? cursor->prev : cursor->next;
    }

    m_lastUpdateFlags = static_cast<std::int32_t>(flags);
    m_animTimer += timeDelta;

    // Time-based mode: update current frame based on playback position
    if ((flags & 0x20) && totalDur > 0 && m_frameHead)
    {
        std::int32_t wrappedTime = timePos % totalDur;
        if (wrappedTime < 0)
        {
            wrappedTime += totalDur;
        }

        std::int32_t accum = 0;
        FrameNode* f = m_frameHead;
        while (f)
        {
            accum += f->duration;
            if (wrappedTime < accum)
            {
                m_currentFrame = f;
                break;
            }
            f = f->next;
        }
    }

    return static_cast<int>(m_renderCommands.size());
}

auto WzGr2DLayer::get_animationState() const -> int
{
    return m_lastUpdateFlags;
}

auto WzGr2DLayer::get_animationTime() const -> int
{
    return m_animTimer;
}

auto WzGr2DLayer::getRenderCommands() const -> const std::vector<RenderCommand>&
{
    return m_renderCommands;
}

// ============================================================
// Animation (backward-compatible wrappers)
// ============================================================

auto WzGr2DLayer::Animate(Gr2DAnimationType type,
                           std::int32_t delayRate,
                           std::int32_t repeat) -> bool
{
    if (m_frameCount < 2)
    {
        return false;
    }

    auto typeValue = static_cast<std::int32_t>(type);

    // GA_FIRST and GA_REPEAT are mutually exclusive
    if ((typeValue & 0x30) == 0x30)
    {
        return false;
    }

    m_animType = type;
    m_nDelayRate = delayRate;
    m_nRepeatCount = repeat;
    m_nCurrentRepeat = 0;

    if (typeValue == 0 || (typeValue & static_cast<std::int32_t>(Gr2DAnimationType::Wait)))
    {
        m_bAnimating = false;
        return true;
    }

    m_bAnimating = true;

    m_bReverseDirection = (typeValue & static_cast<std::int32_t>(Gr2DAnimationType::Reverse)) != 0;

    if (m_bReverseDirection)
    {
        if (typeValue & static_cast<std::int32_t>(Gr2DAnimationType::First))
        {
            m_currentFrame = m_frameHead;
        }
        else
        {
            m_currentFrame = m_frameTail;
        }
    }
    else
    {
        if (typeValue & static_cast<std::int32_t>(Gr2DAnimationType::First))
        {
            m_currentFrame = m_frameHead;
        }
    }

    m_tLastFrameTime = -1;

    return true;
}

void WzGr2DLayer::StopAnimation()
{
    m_bAnimating = false;
    m_animType = Gr2DAnimationType::None;
}

auto WzGr2DLayer::GetCurrentFrame() const noexcept -> std::size_t
{
    if (!m_currentFrame)
    {
        return 0;
    }

    std::size_t idx = 0;
    for (FrameNode* node = m_frameHead; node; node = node->next, ++idx)
    {
        if (node == m_currentFrame)
        {
            return idx;
        }
    }
    return 0;
}

void WzGr2DLayer::SetCurrentFrame(std::size_t frame)
{
    FrameNode* node = findFrameByIndex(static_cast<int>(frame));
    if (node)
    {
        m_currentFrame = node;
    }
}

auto WzGr2DLayer::GetAnimationState() const noexcept -> AnimationState
{
    if (!m_bAnimating)
    {
        return AnimationState::Idle;
    }
    return m_bReverseDirection ? AnimationState::Backward : AnimationState::Forward;
}

void WzGr2DLayer::advanceFrame()
{
    if (m_frameCount == 0 || !m_currentFrame)
    {
        return;
    }

    auto typeValue = static_cast<std::int32_t>(m_animType);
    bool hasRepeat = (typeValue & static_cast<std::int32_t>(Gr2DAnimationType::Repeat)) != 0;
    bool hasClear = (typeValue & static_cast<std::int32_t>(Gr2DAnimationType::Clear)) != 0;

    if (m_bReverseDirection)
    {
        if (m_currentFrame->prev)
        {
            m_currentFrame = m_currentFrame->prev;
        }
        else
        {
            // Reached beginning
            if (hasRepeat)
            {
                // Check if this is ping-pong (Repeat + Reverse)
                bool hasReverse = (typeValue & static_cast<std::int32_t>(Gr2DAnimationType::Reverse)) != 0;
                if (hasReverse)
                {
                    // Ping-pong: switch direction
                    m_bReverseDirection = false;
                }
                else
                {
                    m_currentFrame = m_frameTail;
                }

                if (m_nRepeatCount > 0)
                {
                    ++m_nCurrentRepeat;
                    if (m_nCurrentRepeat >= m_nRepeatCount)
                    {
                        m_bAnimating = false;
                        if (hasClear)
                        {
                            RemoveAllCanvases();
                        }
                    }
                }
            }
            else
            {
                m_bAnimating = false;
                if (hasClear)
                {
                    RemoveAllCanvases();
                }
            }
        }
    }
    else
    {
        if (m_currentFrame->next)
        {
            m_currentFrame = m_currentFrame->next;
        }
        else
        {
            // Reached end
            if (hasRepeat)
            {
                bool hasReverse = (typeValue & static_cast<std::int32_t>(Gr2DAnimationType::Reverse)) != 0;
                if (hasReverse)
                {
                    // Ping-pong: switch direction
                    m_bReverseDirection = true;
                }
                else
                {
                    m_currentFrame = m_frameHead;
                }

                if (m_nRepeatCount > 0)
                {
                    ++m_nCurrentRepeat;
                    if (m_nCurrentRepeat >= m_nRepeatCount)
                    {
                        m_bAnimating = false;
                        if (hasClear)
                        {
                            RemoveAllCanvases();
                        }
                    }
                }
            }
            else
            {
                m_bAnimating = false;
                if (hasClear)
                {
                    RemoveAllCanvases();
                }
            }
        }
    }
}

// ============================================================
// Position
// ============================================================

void WzGr2DLayer::SetPosition(std::int32_t left, std::int32_t top) noexcept
{
    m_nLeft = left;
    m_nTop = top;
}

// ============================================================
// Boundary vectors
// ============================================================

auto WzGr2DLayer::get_lt() const -> Gr2DVector*
{
    return m_positionVec.get();
}

auto WzGr2DLayer::get_rb() const -> Gr2DVector*
{
    return m_rbVec.get();
}

void WzGr2DLayer::InterlockedOffset(std::int32_t ltX, std::int32_t ltY,
                                     std::int32_t rbX, std::int32_t rbY)
{
    if (m_positionVec)
    {
        m_positionVec->Offset(ltX, ltY);
    }
    if (m_rbVec)
    {
        m_rbVec->Offset(rbX, rbY);
    }
}

// ============================================================
// Position helpers (operate on positionVec origin)
// ============================================================

void WzGr2DLayer::MoveOrigin(std::int32_t x, std::int32_t y)
{
    if (!m_positionVec)
    {
        return;
    }
    IWzVector2D* origin = m_positionVec->GetOrigin();
    if (origin)
    {
        origin->Move(x, y);
    }
}

void WzGr2DLayer::OffsetOrigin(std::int32_t dx, std::int32_t dy)
{
    if (!m_positionVec)
    {
        return;
    }
    IWzVector2D* origin = m_positionVec->GetOrigin();
    if (origin)
    {
        origin->Offset(dx, dy);
    }
}

void WzGr2DLayer::ScaleOrigin(std::int32_t sx, std::int32_t divx,
                                std::int32_t sy, std::int32_t divy,
                                std::int32_t cx, std::int32_t cy)
{
    if (!m_positionVec)
    {
        return;
    }
    IWzVector2D* origin = m_positionVec->GetOrigin();
    if (origin)
    {
        origin->Scale(sx, divx, sy, divy, cx, cy);
    }
}

// ============================================================
// Color (source-matching: 3 Gr2DVector channels)
// ============================================================

void WzGr2DLayer::put_color(std::uint32_t argb)
{
    auto a = static_cast<std::int32_t>((argb >> 24) & 0xFF);
    auto r = static_cast<std::int32_t>((argb >> 16) & 0xFF);
    auto g = static_cast<std::int32_t>((argb >> 8) & 0xFF);
    auto b = static_cast<std::int32_t>(argb & 0xFF);

    ensureVectors();

    if (m_alphaVec)
    {
        m_alphaVec->Move(a, 0);
    }
    if (m_colorRedVec)
    {
        m_colorRedVec->Move(r, 0);
    }
    if (m_colorGBVec)
    {
        m_colorGBVec->Move(g, b);
    }
}

auto WzGr2DLayer::get_color() const -> std::uint32_t
{
    std::int32_t a = 255, r = 255, g = 255, b = 255;

    if (m_alphaVec)
    {
        a = std::clamp(m_alphaVec->GetX(), 0, 255);
    }
    if (m_colorRedVec)
    {
        r = std::clamp(m_colorRedVec->GetX(), 0, 255);
    }
    if (m_colorGBVec)
    {
        // Get both X (green) and Y (blue) from the vector
        std::int32_t gx = 0, gy = 0;
        std::int32_t rx = 0, ry = 0, ox = 0, oy = 0;
        double aa = 0, ra = 0;
        m_colorGBVec->GetSnapshot(&gx, &gy, &rx, &ry, &ox, &oy, &aa, &ra);
        g = std::clamp(gx, 0, 255);
        b = std::clamp(gy, 0, 255);
    }

    return (static_cast<std::uint32_t>(a) << 24) |
           (static_cast<std::uint32_t>(r) << 16) |
           (static_cast<std::uint32_t>(g) << 8) |
           static_cast<std::uint32_t>(b);
}

auto WzGr2DLayer::get_alpha() const -> Gr2DVector*
{
    return m_alphaVec.get();
}

auto WzGr2DLayer::get_redTone() const -> Gr2DVector*
{
    return m_colorRedVec.get();
}

auto WzGr2DLayer::get_greenBlueTone() const -> Gr2DVector*
{
    return m_colorGBVec.get();
}

auto WzGr2DLayer::GetAlpha() const noexcept -> std::uint8_t
{
    if (m_alphaVec)
    {
        return static_cast<std::uint8_t>(std::clamp(m_alphaVec->GetX(), 0, 255));
    }
    return 255;
}

void WzGr2DLayer::SetAlpha(std::uint8_t alpha) noexcept
{
    ensureVectors();
    if (m_alphaVec)
    {
        m_alphaVec->Move(static_cast<std::int32_t>(alpha), 0);
    }
}

// ============================================================
// Non-vtable helpers
// ============================================================

void WzGr2DLayer::setColorKey(std::uint8_t a, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    m_colorKeyEnabled = true;
    m_colorKey = (static_cast<std::uint32_t>(a) << 24) |
                 (static_cast<std::uint32_t>(r) << 16) |
                 (static_cast<std::uint32_t>(g) << 8) |
                 static_cast<std::uint32_t>(b);
}

// ============================================================
// Particle system
// ============================================================

auto WzGr2DLayer::getEmitter() -> ParticleEmitter*
{
    if (!m_emitter)
    {
        m_emitter = std::make_unique<ParticleEmitter>();
    }
    return m_emitter.get();
}

void WzGr2DLayer::UpdateParticles(float deltaTime)
{
    if (m_emitter)
    {
        m_emitter->update(deltaTime, 0.0F, 0, 0.0F);
    }
}

// ============================================================
// Animation origin
// ============================================================

auto WzGr2DLayer::getAnimOriginVector() const -> Gr2DVector*
{
    return m_animOriginVec.get();
}

// ============================================================
// Position animation (backward-compatible)
// ============================================================

void WzGr2DLayer::StartPositionAnimation(std::int32_t offsetX, std::int32_t offsetY,
                                          std::int32_t duration, bool loop)
{
    ensureVectors();
    if (!m_positionVec)
    {
        return;
    }

    m_positionVec->Move(m_nLeft, m_nTop);

    auto tCur = Gr2DTime::GetCurrentTime();
    m_positionVec->RelMove(offsetX, offsetY, tCur, tCur + duration, false, loop);
}

void WzGr2DLayer::StopPositionAnimation()
{
    if (m_positionVec)
    {
        m_positionVec->Move(m_nLeft, m_nTop);
    }
}

auto WzGr2DLayer::IsPositionAnimating() const noexcept -> bool
{
    return m_positionVec != nullptr;
}

// ============================================================
// Internal helpers
// ============================================================

void WzGr2DLayer::ensureVectors()
{
    if (!m_alphaVec)
    {
        m_alphaVec = std::make_unique<Gr2DVector>(255, 0);
        m_colorRedVec = std::make_unique<Gr2DVector>(255, 0);
        m_colorGBVec = std::make_unique<Gr2DVector>(255, 255);
    }
    if (!m_positionVec)
    {
        m_positionVec = std::make_unique<Gr2DVector>(0, 0);
    }
    if (!m_rbVec)
    {
        m_rbVec = std::make_unique<Gr2DVector>(0, 0);
    }
}

// ============================================================
// Update and Render (SDL-specific)
// ============================================================

void WzGr2DLayer::Update(std::int32_t tCur)
{
    Gr2DTime::SetCurrentTime(tCur);

    // Update position from Gr2DVector if present
    if (m_positionVec)
    {
        m_nLeft = m_positionVec->GetX();
        m_nTop = m_positionVec->GetY();
    }

    // Update frame animation
    if (!m_bAnimating || m_frameCount < 2 || !m_currentFrame)
    {
        return;
    }

    // Initialize time on first update
    if (m_tLastFrameTime < 0)
    {
        m_tLastFrameTime = tCur;
        return;
    }

    // Get current frame delay scaled by delay rate
    auto delay = (m_currentFrame->duration * m_nDelayRate) / DelayRateScaleFactor;
    if (delay <= 0)
    {
        delay = 1;
    }

    if (tCur - m_tLastFrameTime >= delay)
    {
        advanceFrame();
        m_tLastFrameTime = tCur;
    }
}

void WzGr2DLayer::Render(SDL_Renderer* renderer, std::int32_t offsetX, std::int32_t offsetY)
{
    if (!m_visible || m_frameCount == 0 || renderer == nullptr)
    {
        return;
    }

    // Get current frame's canvas
    if (!m_currentFrame || !m_currentFrame->canvas)
    {
        return;
    }

    ICanvas* icanvas = m_currentFrame->canvas;

    // Try to cast to WzGr2DCanvas for SDL texture access
    auto* canvas = dynamic_cast<WzGr2DCanvas*>(icanvas);
    if (!canvas)
    {
        return;
    }

    // Get or create SDL texture
    auto* texture = canvas->GetTexture();
    if (!texture)
    {
        texture = canvas->CreateTexture(renderer);
        if (!texture)
        {
            return;
        }
    }

    auto canvasPos = canvas->GetPosition();
    auto canvasOrigin = canvas->GetOrigin();
    auto canvasWidth = static_cast<float>(canvas->GetWidth());
    auto canvasHeight = static_cast<float>(canvas->GetHeight());

    // Calculate base render position with parallax
    float baseX, baseY;

    if (m_nParallaxRx <= 0)
    {
        baseX = static_cast<float>(m_nLeft + offsetX);
    }
    else
    {
        auto parallaxOffsetX = (offsetX * m_nParallaxRx) / ParallaxScaleFactor;
        baseX = static_cast<float>(m_nLeft + parallaxOffsetX);
    }

    if (m_nParallaxRy <= 0)
    {
        baseY = static_cast<float>(m_nTop + offsetY);
    }
    else
    {
        auto parallaxOffsetY = (offsetY * m_nParallaxRy) / ParallaxScaleFactor;
        baseY = static_cast<float>(m_nTop + parallaxOffsetY);
    }

    // Render position: basePos + canvasPos - canvasOrigin
    float renderX = baseX + static_cast<float>(canvasPos.x) - static_cast<float>(canvasOrigin.x);
    float renderY = baseY + static_cast<float>(canvasPos.y) - static_cast<float>(canvasOrigin.y);
    float renderWidth = canvasWidth;
    float renderHeight = canvasHeight;

    // Apply color modulation from Gr2DVector channels
    auto color = get_color();
    auto alpha = static_cast<std::uint8_t>((color >> 24) & 0xFF);
    auto red = static_cast<std::uint8_t>((color >> 16) & 0xFF);
    auto green = static_cast<std::uint8_t>((color >> 8) & 0xFF);
    auto blue = static_cast<std::uint8_t>(color & 0xFF);

    // Combine layer alpha with per-frame alpha
    auto frameAlpha = computeAlpha(m_currentFrame->alphaA);
    alpha = static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(alpha) * frameAlpha / 255, 0, 255));

    SDL_SetTextureColorMod(texture, red, green, blue);
    SDL_SetTextureAlphaMod(texture, alpha);

    // Apply blend mode
    auto sdlBlendMode = ConvertToSDLBlendMode(m_blendMode);
    SDL_SetTextureBlendMode(texture, sdlBlendMode);

    // Handle flip
    SDL_FlipMode flipMode = SDL_FLIP_NONE;
    if (m_flipMode & static_cast<std::int32_t>(LayerFlipState::Horizontal))
    {
        flipMode = static_cast<SDL_FlipMode>(flipMode | SDL_FLIP_HORIZONTAL);
    }
    if (m_flipMode & static_cast<std::int32_t>(LayerFlipState::Vertical))
    {
        flipMode = static_cast<SDL_FlipMode>(flipMode | SDL_FLIP_VERTICAL);
    }

    // Get viewport size for tiling
    int viewportW = 0;
    int viewportH = 0;
    SDL_GetRenderOutputSize(renderer, &viewportW, &viewportH);

    auto tileCx = m_nTileCx > 0 ? static_cast<float>(m_nTileCx) : renderWidth;
    auto tileCy = m_nTileCy > 0 ? static_cast<float>(m_nTileCy) : renderHeight;

    int tilesX = 1;
    int tilesY = 1;
    float startTileX = renderX;
    float startTileY = renderY;

    if (m_nTileCx > 0)
    {
        while (startTileX > 0)
        {
            startTileX -= tileCx;
        }
        tilesX = static_cast<int>((static_cast<float>(viewportW) - startTileX) / tileCx) + 2;
    }

    if (m_nTileCy > 0)
    {
        while (startTileY > 0)
        {
            startTileY -= tileCy;
        }
        tilesY = static_cast<int>((static_cast<float>(viewportH) - startTileY) / tileCy) + 2;
    }

    for (int ty = 0; ty < tilesY; ++ty)
    {
        for (int tx = 0; tx < tilesX; ++tx)
        {
            float tileX = startTileX + static_cast<float>(tx) * tileCx;
            float tileY = startTileY + static_cast<float>(ty) * tileCy;

            if (tileX + renderWidth < 0 || tileX > static_cast<float>(viewportW) ||
                tileY + renderHeight < 0 || tileY > static_cast<float>(viewportH))
            {
                continue;
            }

            SDL_FRect dstRect{
                tileX,
                tileY,
                renderWidth,
                renderHeight
            };

            if (flipMode != SDL_FLIP_NONE || m_fRotation != 0.0F)
            {
                SDL_RenderTextureRotated(renderer, texture, nullptr, &dstRect,
                                          static_cast<double>(m_fRotation), nullptr, flipMode);
            }
            else
            {
                SDL_RenderTexture(renderer, texture, nullptr, &dstRect);
            }
        }
    }
}

// =============================================================================
// WzGr2DLayer — IWzVector2D delegation to m_positionVec
// =============================================================================

auto WzGr2DLayer::GetX() -> std::int32_t
{
    ensureVectors();
    return m_positionVec->GetX();
}

auto WzGr2DLayer::GetY() -> std::int32_t
{
    ensureVectors();
    return m_positionVec->GetY();
}

void WzGr2DLayer::PutX(std::int32_t x)
{
    ensureVectors();
    m_positionVec->PutX(x);
}

void WzGr2DLayer::PutY(std::int32_t y)
{
    ensureVectors();
    m_positionVec->PutY(y);
}

void WzGr2DLayer::Move(std::int32_t x, std::int32_t y)
{
    ensureVectors();
    m_positionVec->Move(x, y);
}

void WzGr2DLayer::Offset(std::int32_t dx, std::int32_t dy)
{
    ensureVectors();
    m_positionVec->Offset(dx, dy);
}

void WzGr2DLayer::Scale(std::int32_t sx, std::int32_t divx,
                         std::int32_t sy, std::int32_t divy,
                         std::int32_t cx, std::int32_t cy)
{
    ensureVectors();
    m_positionVec->Scale(sx, divx, sy, divy, cx, cy);
}

void WzGr2DLayer::Init(std::int32_t x, std::int32_t y)
{
    ensureVectors();
    m_positionVec->Init(x, y);
}

auto WzGr2DLayer::GetCurrentTime() -> std::int32_t
{
    return Gr2DTime::GetCurrentTime();
}

void WzGr2DLayer::PutCurrentTime(std::int32_t t)
{
    Gr2DTime::SetCurrentTime(t);
}

auto WzGr2DLayer::GetOrigin() const -> IWzVector2D*
{
    if (!m_positionVec) return nullptr;
    return m_positionVec->GetOrigin();
}

void WzGr2DLayer::PutOrigin(IWzVector2D* parent)
{
    ensureVectors();
    m_positionVec->PutOrigin(parent);
}

auto WzGr2DLayer::GetRX() -> std::int32_t
{
    ensureVectors();
    return m_positionVec->GetRX();
}

void WzGr2DLayer::PutRX(std::int32_t x)
{
    ensureVectors();
    m_positionVec->PutRX(x);
}

auto WzGr2DLayer::GetRY() -> std::int32_t
{
    ensureVectors();
    return m_positionVec->GetRY();
}

void WzGr2DLayer::PutRY(std::int32_t y)
{
    ensureVectors();
    m_positionVec->PutRY(y);
}

auto WzGr2DLayer::GetA() -> double
{
    ensureVectors();
    return m_positionVec->GetA();
}

auto WzGr2DLayer::GetRA() -> double
{
    ensureVectors();
    return m_positionVec->GetRA();
}

void WzGr2DLayer::PutRA(double a)
{
    ensureVectors();
    m_positionVec->PutRA(a);
}

auto WzGr2DLayer::GetFlipX() -> bool
{
    ensureVectors();
    return m_positionVec->GetFlipX();
}

void WzGr2DLayer::PutFlipX(std::int32_t f)
{
    ensureVectors();
    m_positionVec->PutFlipX(f);
}

void WzGr2DLayer::GetSnapshot(std::int32_t* x, std::int32_t* y,
                               std::int32_t* rx, std::int32_t* ry,
                               std::int32_t* ox, std::int32_t* oy,
                               double* a, double* ra,
                               std::int32_t time)
{
    ensureVectors();
    m_positionVec->GetSnapshot(x, y, rx, ry, ox, oy, a, ra, time);
}

void WzGr2DLayer::RelMove(std::int32_t x, std::int32_t y,
                           std::int32_t startTime, std::int32_t endTime,
                           bool bounce, bool pingpong, bool replace)
{
    ensureVectors();
    m_positionVec->RelMove(x, y, startTime, endTime, bounce, pingpong, replace);
}

void WzGr2DLayer::RelOffset(std::int32_t dx, std::int32_t dy,
                             std::int32_t startTime, std::int32_t endTime)
{
    ensureVectors();
    m_positionVec->RelOffset(dx, dy, startTime, endTime);
}

void WzGr2DLayer::Ratio(IWzVector2D* target,
                         std::int32_t denomX, std::int32_t denomY,
                         std::int32_t scaleX, std::int32_t scaleY)
{
    ensureVectors();
    m_positionVec->Ratio(target, denomX, denomY, scaleX, scaleY);
}

void WzGr2DLayer::WrapClip(IWzVector2D* bounds,
                            std::int32_t x, std::int32_t y,
                            std::int32_t w, std::int32_t h,
                            bool clampMode)
{
    ensureVectors();
    m_positionVec->WrapClip(bounds, x, y, w, h, clampMode);
}

void WzGr2DLayer::Rotate(double angle, std::int32_t period,
                          std::int32_t easeFrames)
{
    ensureVectors();
    m_positionVec->Rotate(angle, period, easeFrames);
}

auto WzGr2DLayer::GetLooseLevel() -> std::int32_t
{
    ensureVectors();
    return m_positionVec->GetLooseLevel();
}

void WzGr2DLayer::PutLooseLevel(std::int32_t level)
{
    ensureVectors();
    m_positionVec->PutLooseLevel(level);
}

void WzGr2DLayer::Fly(const std::vector<FlyKeyframe>& keyframes,
                       IWzVector2D* completionTarget)
{
    ensureVectors();
    m_positionVec->Fly(keyframes, completionTarget);
}

} // namespace ms
