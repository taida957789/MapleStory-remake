#include "Gr2DVector.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ms
{

// =============================================================================
// Global Time Management
// =============================================================================

namespace
{
    std::int32_t g_currentTime = 0;

    // Round-to-nearest with symmetric negative handling (matches original binary)
    auto roundToInt(double v) -> std::int32_t
    {
        if (v < 0.0)
            return -static_cast<std::int32_t>(0.499999999 - v);
        return static_cast<std::int32_t>(v + 0.5);
    }

    // Normalize angle to [0, 360)
    auto normalizeAngle(double angle) -> double
    {
        double r = std::fmod(angle, 360.0);
        if (angle <= 0.0)
        {
            double v = -r;
            if (std::fabs(v) >= 1.0e-10)
                return 360.0 - v;
            return 0.0;
        }
        return r;
    }
} // anonymous namespace

namespace Gr2DTime
{
    auto GetCurrentTime() -> std::int32_t
    {
        return g_currentTime;
    }

    void SetCurrentTime(std::int32_t t)
    {
        g_currentTime = t;
    }
}

// =============================================================================
// AnimChain
// =============================================================================

AnimChain::~AnimChain()
{
    clearNodes();
}

void AnimChain::clearNodes()
{
    AnimNode* cur = head;
    while (cur)
    {
        AnimNode* nx = cur->next;
        delete cur;
        cur = nx;
    }
    head = tail = nullptr;
}

void AnimChain::removeNode(AnimNode* node)
{
    if (!node) return;
    if (node->prev) node->prev->next = node->next;
    else            head = node->next;
    if (node->next) node->next->prev = node->prev;
    else            tail = node->prev;
    node->prev = node->next = nullptr;
    delete node;
}

// Insert sorted by phase ascending (lower phase first = closer to head)
void AnimChain::insertNode(AnimNode* node)
{
    auto ph = static_cast<std::uint16_t>(node->phase());

    // Special case: WrapClipNode replaces existing WrapClipNode
    if (node->type() == 0x00140002)
    {
        AnimNode* cur = head;
        while (cur)
        {
            AnimNode* nx = cur->next;
            if (cur->type() == 0x00140002)
            {
                removeNode(cur);
            }
            cur = nx;
        }
    }

    // Find insertion point: insert before the first node with phase > ph
    AnimNode* pos = head;
    while (pos && pos->phase() <= ph)
    {
        pos = pos->next;
    }

    if (!pos)
    {
        // Append to tail
        node->prev = tail;
        node->next = nullptr;
        if (tail) tail->next = node;
        else      head = node;
        tail = node;
    }
    else
    {
        // Insert before pos
        node->next = pos;
        node->prev = pos->prev;
        if (pos->prev) pos->prev->next = node;
        else           head = node;
        pos->prev = node;
    }
}

// Remove all nodes matching a given type, absorbing their deltas
void AnimChain::removeNodesByType(std::uint32_t nodeType)
{
    std::int32_t lx = base_x;
    std::int32_t ly = base_y;
    AnimNode* cur = head;
    while (cur)
    {
        AnimNode* nx = cur->next;
        if (cur->type() == nodeType)
        {
            std::int32_t old_x = lx;
            std::int32_t old_y = ly;
            cur->evaluatePos(lx, ly, Gr2DTime::GetCurrentTime(), false);
            base_x += (lx - old_x);
            base_y += (ly - old_y);
            lx = base_x;
            ly = base_y;
            removeNode(cur);
        }
        cur = nx;
    }
}

// Full reset: clear nodes, reset base position, invalidate cache
void AnimChain::reset(std::int32_t x, std::int32_t y)
{
    clearNodes();
    parent_ref = nullptr;
    base_x = x;
    base_y = y;
    offset_x = 0;
    offset_y = 0;
    base_angle = 0.0;
    flip_accum = 0;
    first_eval = false;
    first_eval_init = false;
    evaluated = false;
    flip_result = 0;
}

// =============================================================================
// AnimChain::evaluate — the main animation pipeline
// Faithfully reimplements sub_1534168F0
// =============================================================================

void AnimChain::evaluate(std::int32_t frame, bool commit)
{
    evaluated = true;
    evaluated_frame = frame;

    // --- Step 1: Query parent ---
    std::int32_t par_x = 0, par_y = 0;
    double par_angle = 0.0;
    std::int32_t par_flip = 0;

    if (parent_ref)
    {
        double dummy_ra = 0.0;
        parent_ref->GetSnapshot(&par_x, &par_y, nullptr, nullptr,
                                nullptr, nullptr, &par_angle, &dummy_ra, frame);
        par_flip = parent_ref->GetFlipX() ? 1 : 0;
    }

    std::int32_t flip = par_flip + flip_accum;

    // --- Step 2: Phase 0 nodes (FlyNode) — transform parent pos ---
    AnimNode* cur = head;
    while (cur && cur->phase() == 0)
    {
        AnimNode* nx = cur->next;
        int ret = cur->evaluatePos(par_x, par_y, frame, commit);
        if (ret == 1 && commit)
        {
            // FlyNode completion: may change parent
            IWzVector2D* newParent = cur->takeCompletion();
            removeNode(cur);
            if (newParent)
            {
                parent_ref = newParent;
                // Re-query parent
                par_angle = 0.0;
                parent_ref->GetSnapshot(&par_x, &par_y, nullptr, nullptr,
                                        nullptr, nullptr, &par_angle, nullptr, frame);
                par_flip = parent_ref->GetFlipX() ? 1 : 0;
                flip = par_flip + flip_accum;
            }
        }
        cur = nx;
    }

    // --- Step 3: Phase 1 nodes — transform local pos ---
    std::int32_t loc_x = base_x, loc_y = base_y;
    while (cur && cur->phase() == 1)
    {
        AnimNode* nx = cur->next;
        std::int32_t old_x = loc_x, old_y = loc_y;
        int ret = cur->evaluatePos(loc_x, loc_y, frame, commit);
        if (ret == 1 && commit)
        {
            base_x += (loc_x - old_x);
            base_y += (loc_y - old_y);
            removeNode(cur);
        }
        cur = nx;
    }

    // --- Step 4: Rotate local pos by parent angle ---
    std::int32_t rot_x = loc_x, rot_y = loc_y;
    if (par_angle != 0.0)
    {
        double rad = M_PI / 180.0 * par_angle;
        double ca = std::cos(rad);
        double sa = std::sin(rad);

        // Forward rotation
        double fx = static_cast<double>(loc_x) * ca - static_cast<double>(loc_y) * sa;
        double fy = static_cast<double>(loc_x) * sa + static_cast<double>(loc_y) * ca;
        std::int32_t rx = roundToInt(fx);
        std::int32_t ry = roundToInt(fy);

        // Negative rotation (for symmetric rounding correction)
        double nx_d = static_cast<double>(-loc_x) * ca - static_cast<double>(-loc_y) * sa;
        double ny_d = static_cast<double>(-loc_x) * sa + static_cast<double>(-loc_y) * ca;
        std::int32_t rnx = roundToInt(nx_d);
        std::int32_t rny = roundToInt(ny_d);

        // Correction: ensure symmetry
        if (rnx + rx) rx = -rnx;
        if (rny + ry) ry = -rny;

        rot_x = rx;
        rot_y = ry;
    }

    // --- Step 5: World pos = parent + rotated_local + offset ---
    std::int32_t wld_x = par_x + rot_x + offset_x;
    std::int32_t wld_y = par_y + rot_y + offset_y;

    // --- Step 6: Phase 2 nodes (WrapClipNode) — transform world pos ---
    while (cur && cur->phase() == 2)
    {
        AnimNode* nx = cur->next;
        std::int32_t old_x = wld_x, old_y = wld_y;
        int ret = cur->evaluatePos(wld_x, wld_y, frame, commit);
        if (ret == 1 && commit)
        {
            offset_x += (wld_x - old_x);
            offset_y += (wld_y - old_y);
            removeNode(cur);
        }
        cur = nx;
    }

    // --- Step 7: Phase 3 nodes (RotateNode) — transform angle ---
    double loc_angle = base_angle;
    while (cur && cur->phase() == 3)
    {
        AnimNode* nx = cur->next;
        double old_a = loc_angle;
        int ret = cur->evaluateAngle(loc_angle, frame, commit);
        if (ret == 1 && commit)
        {
            base_angle += (loc_angle - old_a);
            removeNode(cur);
        }
        cur = nx;
    }

    // --- Step 8: First-eval flip tracking ---
    std::int32_t final_flip = flip;
    if (first_eval)
    {
        if (first_eval_init)
        {
            first_eval_init = false;
        }
        else
        {
            final_flip = (par_x - parent_cache_x) + flip;
        }
    }

    // --- Step 9: Store cached results ---
    local_angle_cache  = loc_angle;
    total_angle_cache  = loc_angle + par_angle;
    parent_angle_cache = par_angle;
    parent_cache_x = par_x;
    parent_cache_y = par_y;
    local_cache_x  = loc_x;
    local_cache_y  = loc_y;
    world_cache_x  = wld_x;
    world_cache_y  = wld_y;
    flip_result    = final_flip;
}

// =============================================================================
// EasingNode::evaluatePos — reimplements sub_153412310
// =============================================================================

auto EasingNode::evaluatePos(std::int32_t& x, std::int32_t& y,
                              std::int32_t frame, bool commit) -> int
{
    std::int32_t ax = accum_x, ay = accum_y;
    std::int32_t st = startTime, et = endTime;
    std::int32_t curDx = dx, curDy = dy;

    // Instant animation
    if (et - st <= 0)
    {
        x += curDx;
        y += curDy;
        return 1;
    }

    std::int32_t loose = 0;

    if (frame >= et)
    {
        // Past end time
        if (!bounce)
        {
            x += curDx;
            y += curDy;
            return 1;
        }

        if (pingpong)
        {
            // Alternate direction each cycle
            do
            {
                if (ax != 0) { ax = 0; }
                else         { ax = curDx; }
                if (ay != 0) { ay = 0; }
                else         { ay = curDy; }

                std::int32_t period = et - st;
                curDx = -curDx;
                curDy = -curDy;
                st = et;
                et += period;
            } while (frame >= et);

            if (commit)
            {
                dx = curDx;
                dy = curDy;
            }
        }
        else
        {
            // Accumulate each cycle
            do
            {
                ax += curDx;
                ay += curDy;
                std::int32_t period = et - st;
                st = et;
                et += period;
            } while (frame >= et);
        }
        // Fall through to interpolation
    }

    if (frame > st)
    {
        // Interpolate current progress
        x += ax;
        y += ay;
        double progress = static_cast<double>(frame - st);
        double total    = static_cast<double>(et - st);
        x += static_cast<std::int32_t>(static_cast<double>(curDx) * progress / total);
        y += static_cast<std::int32_t>(static_cast<double>(curDy) * progress / total);

        if (commit)
        {
            if (frame - looseTimer >= 30)
            {
                loose = looseLevel;
                looseTimer = frame;
            }
            startTime = st;
            endTime   = et - loose;
            accum_x   = ax;
            accum_y   = ay;
        }
    }

    return 0;
}

// =============================================================================
// RatioNode::evaluatePos — reimplements sub_153412570
// =============================================================================

auto RatioNode::evaluatePos(std::int32_t& x, std::int32_t& y,
                             std::int32_t /*frame*/, bool /*commit*/) -> int
{
    if (!target) return 0;
    std::int32_t cur_x = target->GetX();
    std::int32_t cur_y = target->GetY();
    x += scaleX * (cur_x - base_x) / denomX;
    y += scaleY * (cur_y - base_y) / denomY;
    return 0;  // never completes
}

// =============================================================================
// FlyNode::evaluatePos — cubic Hermite spline, reimplements sub_153413920
// =============================================================================

auto FlyNode::evaluatePos(std::int32_t& x, std::int32_t& y,
                           std::int32_t frame, bool /*commit*/) -> int
{
    if (keyframes.empty()) return 1;

    for (std::size_t i = 0; i < keyframes.size(); ++i)
    {
        if (frame < keyframes[i].time)
            break;

        if (i + 1 >= keyframes.size())
        {
            // Past last keyframe — complete
            return 1;
        }

        std::int32_t nextTime = keyframes[i + 1].time;
        if (nextTime > frame)
        {
            // Interpolate between keyframe[i] and keyframe[i+1]
            double t = static_cast<double>(frame - keyframes[i].time)
                     / static_cast<double>(nextTime - keyframes[i].time);

            // Get positions from control point Gr2DVectors
            double p0x = 0, p0y = 0, p1x = 0, p1y = 0;
            if (keyframes[i].point)
            {
                p0x = static_cast<double>(keyframes[i].point->GetX());
                p0y = static_cast<double>(keyframes[i].point->GetY());
            }
            if (keyframes[i + 1].point)
            {
                p1x = static_cast<double>(keyframes[i + 1].point->GetX());
                p1y = static_cast<double>(keyframes[i + 1].point->GetY());
            }

            // Cubic Hermite basis functions
            double t2 = t * t;
            double t3 = t2 * t;
            double h00 =  2.0 * t3 - 3.0 * t2 + 1.0;
            double h10 =        t3 - 2.0 * t2 + t;
            double h01 = -2.0 * t3 + 3.0 * t2;
            double h11 =        t3 -       t2;

            x = static_cast<std::int32_t>(h00 * p0x + h10 * keyframes[i].vel_x
                        + h11 * keyframes[i].accel_x + h01 * p1x);
            y = static_cast<std::int32_t>(h00 * p0y + h10 * keyframes[i].vel_y
                        + h11 * keyframes[i].accel_y + h01 * p1y);
            return 0;
        }
    }
    return 0;
}

auto FlyNode::takeCompletion() -> IWzVector2D*
{
    IWzVector2D* c = completion;
    completion = nullptr;
    return c;
}

// =============================================================================
// WrapClipNode — reimplements sub_153412730
// =============================================================================

auto WrapClipNode::wrapVal(std::int32_t val, std::int32_t start,
                            std::int32_t size) -> std::int32_t
{
    if (size == 0) return start;
    std::int32_t diff = val - start;
    if (diff > 0)
        return diff % size + start;
    std::int32_t neg = (-diff) % size;
    if (neg) return size - neg + start;
    return start;
}

auto WrapClipNode::clampVal(std::int32_t val, std::int32_t start,
                             std::int32_t size) -> std::int32_t
{
    if (val < start) return start;
    if (val >= start + size) return start + size;
    return val;
}

auto WrapClipNode::evaluatePos(std::int32_t& x, std::int32_t& y,
                                std::int32_t /*frame*/, bool /*commit*/) -> int
{
    std::int32_t bx = 0, by = 0;
    if (bounds)
    {
        bx = bounds->GetX();
        by = bounds->GetY();
    }

    std::int32_t w = right - left;
    std::int32_t h = bottom - top;

    if (clampMode)
    {
        x = clampVal(x, left + bx, w);
        y = clampVal(y, top  + by, h);
    }
    else
    {
        x = wrapVal(x, left + bx, w);
        y = wrapVal(y, top  + by, h);
    }

    return 0;  // never completes
}

// =============================================================================
// RotateNode::evaluateAngle — reimplements sub_153412B30
// =============================================================================

auto RotateNode::evaluateAngle(double& angle, std::int32_t frame,
                                bool /*commit*/) -> int
{
    if (std::fabs(totalAngle) < 1.0e-10)
    {
        // Continuous rotation
        if (period == 0) return 1;

        std::int32_t elapsed = frame - startTime;
        double pd = static_cast<double>(period);

        if (easeFrames > 0 && elapsed < easeFrames)
        {
            // Quadratic ease-in
            angle += 360.0 / pd / static_cast<double>(easeFrames)
                   * static_cast<double>(elapsed) * static_cast<double>(elapsed) * 0.5;
            return 0;
        }

        double cyclePos;
        if (easeFrames > 0)
        {
            cyclePos = static_cast<double>(elapsed % period - easeFrames / 2) * 360.0;
        }
        else
        {
            cyclePos = static_cast<double>(elapsed % period) * 360.0;
        }
        angle += cyclePos / pd;
        return 0;
    }

    // Finite rotation
    if (frame >= period)
    {
        angle += totalAngle;
        return 1;
    }

    std::int32_t elapsed = frame - startTime;
    std::int32_t dur     = period - startTime;

    if (easeFrames == 0)
    {
        // Linear
        angle += totalAngle * static_cast<double>(elapsed) / static_cast<double>(dur);
        return 0;
    }

    // 3-phase ease-in / coast / ease-out
    std::int32_t coastDur = dur - 2 * easeFrames;
    double ease_d = static_cast<double>(easeFrames);
    double rate   = totalAngle / static_cast<double>(coastDur + easeFrames);

    if (elapsed < easeFrames)
    {
        // Ease-in: quadratic acceleration
        angle += rate / ease_d * static_cast<double>(elapsed)
               * static_cast<double>(elapsed) * 0.5;
    }
    else if (elapsed < coastDur + easeFrames)
    {
        // Coast: constant speed
        double easeInContrib = ease_d * rate * 0.5;
        angle += rate * static_cast<double>(elapsed - easeFrames) + easeInContrib;
    }
    else
    {
        // Ease-out: quadratic deceleration
        double easeInContrib = ease_d * rate * 0.5;
        double t = static_cast<double>(elapsed - coastDur - easeFrames);
        angle += (-rate / ease_d * t + 2.0 * rate) * t * 0.5
               + static_cast<double>(coastDur) * rate
               + easeInContrib;
    }
    return 0;
}

// =============================================================================
// Gr2DVector — private helpers
// =============================================================================

auto Gr2DVector::ensureChain() -> AnimChain*
{
    if (!m_chain)
    {
        m_chain = std::make_unique<AnimChain>(m_x, m_y);
    }
    return m_chain.get();
}

// =============================================================================
// Gr2DVector — construction
// =============================================================================

Gr2DVector::Gr2DVector(std::int32_t x, std::int32_t y)
    : m_x(x), m_y(y)
{
}

// =============================================================================
// Gr2DVector — property accessors
// =============================================================================

auto Gr2DVector::GetX() -> std::int32_t
{
    if (!m_chain) return m_x;
    AnimChain* c = m_chain.get();
    if (!c->evaluated || c->evaluated_frame != Gr2DTime::GetCurrentTime())
        c->evaluate(Gr2DTime::GetCurrentTime(), true);
    return c->world_cache_x;
}

auto Gr2DVector::GetY() -> std::int32_t
{
    if (!m_chain) return m_y;
    AnimChain* c = m_chain.get();
    if (!c->evaluated || c->evaluated_frame != Gr2DTime::GetCurrentTime())
        c->evaluate(Gr2DTime::GetCurrentTime(), true);
    return c->world_cache_y;
}

auto Gr2DVector::GetRX() -> std::int32_t
{
    if (!m_chain) return m_x;
    AnimChain* c = m_chain.get();
    if (!c->evaluated || c->evaluated_frame != Gr2DTime::GetCurrentTime())
        c->evaluate(Gr2DTime::GetCurrentTime(), true);
    return c->local_cache_x;
}

auto Gr2DVector::GetRY() -> std::int32_t
{
    if (!m_chain) return m_y;
    AnimChain* c = m_chain.get();
    if (!c->evaluated || c->evaluated_frame != Gr2DTime::GetCurrentTime())
        c->evaluate(Gr2DTime::GetCurrentTime(), true);
    return c->local_cache_y;
}

auto Gr2DVector::GetA() -> double
{
    if (!m_chain) return 0.0;
    AnimChain* c = m_chain.get();
    if (!c->evaluated || c->evaluated_frame != Gr2DTime::GetCurrentTime())
        c->evaluate(Gr2DTime::GetCurrentTime(), true);
    return normalizeAngle(c->total_angle_cache);
}

auto Gr2DVector::GetRA() -> double
{
    if (!m_chain) return 0.0;
    AnimChain* c = m_chain.get();
    if (!c->evaluated || c->evaluated_frame != Gr2DTime::GetCurrentTime())
        c->evaluate(Gr2DTime::GetCurrentTime(), true);
    return normalizeAngle(c->local_angle_cache);
}

auto Gr2DVector::GetFlipX() -> bool
{
    if (!m_chain) return false;
    AnimChain* c = m_chain.get();
    if (!c->evaluated || c->evaluated_frame != Gr2DTime::GetCurrentTime())
        c->evaluate(Gr2DTime::GetCurrentTime(), true);
    return c->flip_result != 0;
}

auto Gr2DVector::GetLooseLevel() -> std::int32_t
{
    if (!m_chain) return 0;
    // Find the first EasingNode and return its looseLevel
    for (AnimNode* n = m_chain->head; n; n = n->next)
    {
        if (n->type() == 0x00000001)
        {
            return static_cast<EasingNode*>(n)->looseLevel;
        }
    }
    return 0;
}

void Gr2DVector::PutLooseLevel(std::int32_t level)
{
    if (!m_chain) return;
    // Set looseLevel on all EasingNodes
    for (AnimNode* n = m_chain->head; n; n = n->next)
    {
        if (n->type() == 0x00000001)
        {
            static_cast<EasingNode*>(n)->looseLevel = level;
        }
    }
}

auto Gr2DVector::GetCurrentTime() -> std::int32_t
{
    return Gr2DTime::GetCurrentTime();
}

// =============================================================================
// Gr2DVector — origin
// =============================================================================

void Gr2DVector::PutOrigin(IWzVector2D* parent)
{
    if (parent == this) return;  // can't parent to self

    AnimChain* c = ensureChain();

    // Always evaluate to capture current local position
    c->evaluate(Gr2DTime::GetCurrentTime(), true);
    std::int32_t cur_rx = c->local_cache_x;
    std::int32_t cur_ry = c->local_cache_y;

    // Reset chain (clear all nodes, base to 0,0)
    c->reset(0, 0);
    c->parent_ref = parent;

    // Re-evaluate with new parent, then adjust base so local pos is preserved
    c->evaluate(Gr2DTime::GetCurrentTime(), true);
    c->base_x += (cur_rx - c->local_cache_x);
    c->base_y += (cur_ry - c->local_cache_y);
    c->evaluated = false;
}

auto Gr2DVector::GetOrigin() const -> IWzVector2D*
{
    if (!m_chain) return nullptr;
    return m_chain->parent_ref;
}

// =============================================================================
// Gr2DVector — transforms
// =============================================================================

void Gr2DVector::Move(std::int32_t x, std::int32_t y)
{
    m_x = x;
    m_y = y;
    if (m_chain)
    {
        m_chain->reset(x, y);
    }
}

void Gr2DVector::Offset(std::int32_t dx, std::int32_t dy)
{
    if (m_chain)
    {
        m_chain->offset_x += dx;
        m_chain->offset_y += dy;
        m_chain->evaluated = false;
    }
    else
    {
        m_x += dx;
        m_y += dy;
    }
}

void Gr2DVector::Scale(std::int32_t sx, std::int32_t divx,
                        std::int32_t sy, std::int32_t divy,
                        std::int32_t cx, std::int32_t cy)
{
    if (divx == 0 || divy == 0) return;

    if (m_chain)
    {
        // Resolve current world position
        m_chain->evaluate(Gr2DTime::GetCurrentTime(), true);
        m_x = m_chain->world_cache_x;
        m_y = m_chain->world_cache_y;
        // Apply scale
        m_x = m_x * sx / divx + cx;
        m_y = m_y * sy / divy + cy;
        // Reset chain to new position
        m_chain->reset(m_x, m_y);
    }
    else
    {
        m_x = cx + sx * (m_x - cx) / divx;
        m_y = cy + sy * (m_y - cy) / divy;
    }
}

void Gr2DVector::RelMove(std::int32_t x, std::int32_t y,
                          std::int32_t startTime, std::int32_t endTime,
                          bool bounce, bool pingpong,
                          bool replace)
{
    if (endTime <= startTime && endTime != 0)
    {
        // Instant move (no duration)
        if (m_chain)
        {
            // Adjust base so rx == x, ry == y
            m_chain->evaluate(Gr2DTime::GetCurrentTime(), true);
            m_chain->base_x += (x - m_chain->local_cache_x);
            m_chain->base_y += (y - m_chain->local_cache_y);
            m_chain->evaluated = false;
        }
        else
        {
            m_x = x;
            m_y = y;
        }
        return;
    }

    AnimChain* c = ensureChain();

    // Get current local position
    c->evaluate(Gr2DTime::GetCurrentTime(), true);
    std::int32_t cur_rx = c->local_cache_x;
    std::int32_t cur_ry = c->local_cache_y;

    std::int32_t now = Gr2DTime::GetCurrentTime();

    // Create easing node
    auto* node = new EasingNode();
    node->dx = x - cur_rx;
    node->dy = y - cur_ry;
    node->startTime = (startTime != 0) ? startTime : now;
    node->endTime = (endTime != 0) ? endTime : now;
    node->bounce = bounce;
    node->pingpong = pingpong;
    node->looseTimer = now;

    // Optionally remove existing easing nodes
    if (replace)
    {
        c->removeNodesByType(0x00000001);
    }

    c->insertNode(node);
    c->evaluated = false;
}

void Gr2DVector::RelOffset(std::int32_t dx, std::int32_t dy,
                            std::int32_t startTime, std::int32_t endTime)
{
    if (endTime <= startTime && endTime != 0)
    {
        // Instant offset
        if (m_chain)
        {
            m_chain->base_x += dx;
            m_chain->base_y += dy;
            m_chain->evaluated = false;
        }
        else
        {
            m_x += dx;
            m_y += dy;
        }
        return;
    }

    AnimChain* c = ensureChain();
    std::int32_t now = Gr2DTime::GetCurrentTime();

    auto* node = new EasingNode();
    node->dx = dx;
    node->dy = dy;
    node->startTime = (startTime != 0) ? startTime : now;
    node->endTime = (endTime != 0) ? endTime : now;
    node->looseTimer = now;

    c->insertNode(node);
    c->evaluated = false;
}

void Gr2DVector::Rotate(double angle, std::int32_t period, std::int32_t easeFrames)
{
    AnimChain* c = ensureChain();
    std::int32_t now = Gr2DTime::GetCurrentTime();

    auto* node = new RotateNode();
    node->totalAngle = angle;
    node->startTime  = now;
    node->period     = period;
    node->easeFrames = easeFrames;

    c->insertNode(node);
    c->evaluated = false;
}

void Gr2DVector::Ratio(IWzVector2D* target,
                        std::int32_t denomX, std::int32_t denomY,
                        std::int32_t scaleX, std::int32_t scaleY)
{
    if (!target || denomX == 0 || denomY == 0) return;

    AnimChain* c = ensureChain();

    auto* node = new RatioNode();
    node->target = target;
    node->base_x = target->GetX();
    node->base_y = target->GetY();
    node->denomX = denomX;
    node->denomY = denomY;
    node->scaleX = scaleX;
    node->scaleY = scaleY;

    c->insertNode(node);
    c->evaluated = false;
}

void Gr2DVector::WrapClip(IWzVector2D* bounds,
                           std::int32_t x, std::int32_t y,
                           std::int32_t w, std::int32_t h,
                           bool clampMode)
{
    AnimChain* c = ensureChain();

    auto* node = new WrapClipNode();
    node->bounds    = bounds;
    node->left      = x;
    node->top       = y;
    node->right     = x + w;
    node->bottom    = y + h;
    node->clampMode = clampMode;

    c->insertNode(node);
    c->evaluated = false;
}

void Gr2DVector::Fly(const std::vector<FlyKeyframe>& keyframes,
                      IWzVector2D* completionTarget)
{
    if (keyframes.empty()) return;

    AnimChain* c = ensureChain();

    auto* node = new FlyNode();
    node->keyframes  = keyframes;
    node->completion = completionTarget;

    c->insertNode(node);
    c->evaluated = false;
}

// =============================================================================
// Gr2DVector::GetSnapshot
// =============================================================================

void Gr2DVector::GetSnapshot(std::int32_t* x, std::int32_t* y,
                              std::int32_t* rx, std::int32_t* ry,
                              std::int32_t* ox, std::int32_t* oy,
                              double* a, double* ra,
                              std::int32_t time)
{
    if (!m_chain)
    {
        // No chain: static position, zero angles
        if (x)  *x  = m_x;
        if (y)  *y  = m_y;
        if (rx) *rx = m_x;
        if (ry) *ry = m_y;
        if (ox) *ox = 0;
        if (oy) *oy = 0;
        if (a)  *a  = 0.0;
        if (ra) *ra = 0.0;
        return;
    }

    AnimChain* c = m_chain.get();

    // Determine frame and whether to commit state changes
    std::int32_t frame;
    bool commit;
    if (time < 0)
    {
        frame  = Gr2DTime::GetCurrentTime();
        commit = true;
    }
    else
    {
        frame  = time;
        commit = (time == Gr2DTime::GetCurrentTime());
    }

    // Evaluate if needed
    if (!c->evaluated || c->evaluated_frame != frame)
        c->evaluate(frame, commit);

    if (x)  *x  = c->world_cache_x;
    if (y)  *y  = c->world_cache_y;
    if (rx) *rx = c->local_cache_x;
    if (ry) *ry = c->local_cache_y;
    if (ox) *ox = c->parent_cache_x;
    if (oy) *oy = c->parent_cache_y;
    if (a)  *a  = normalizeAngle(c->total_angle_cache);
    if (ra) *ra = normalizeAngle(c->local_angle_cache);
}

// =============================================================================
// Gr2DVector::Serialize — parse "(x,y)" or "x\ty" format
// =============================================================================

void Gr2DVector::Serialize(const char* data)
{
    if (!data) return;

    std::int32_t px = 0, py = 0;

    // Skip leading whitespace
    const char* p = data;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;

    if (*p == '(')
    {
        // Format: "(x, y)" or "(x,y)"
        ++p;
        char buf[256] = {};
        int i = 0;
        while (*p && *p != ',' && i < 255)
            buf[i++] = *p++;
        buf[i] = 0;
        px = std::atoi(buf);

        if (*p == ',') ++p;
        while (*p == ' ') ++p;

        i = 0;
        std::memset(buf, 0, sizeof(buf));
        while (*p && *p != ')' && i < 255)
            buf[i++] = *p++;
        buf[i] = 0;
        py = std::atoi(buf);
    }
    else if (*p == '\t' || (*p >= '0' && *p <= '9') || *p == '-' || *p == '+')
    {
        // Format: "x\ty" (tab-separated) or just numbers
        if (*p == '\t') ++p;

        char buf[256] = {};
        int i = 0;
        while (*p && *p != ',' && *p != '\t' && i < 255)
            buf[i++] = *p++;
        buf[i] = 0;
        px = std::atoi(buf);

        if (*p == ',' || *p == '\t') ++p;
        while (*p == ' ') ++p;

        i = 0;
        std::memset(buf, 0, sizeof(buf));
        while (*p && *p != '\t' && *p != '\r' && *p != '\n' && i < 255)
            buf[i++] = *p++;
        buf[i] = 0;
        py = std::atoi(buf);
    }

    m_x = px;
    m_y = py;
    if (m_chain)
    {
        m_chain->reset(px, py);
    }
}

// =============================================================================
// Gr2DVector — new IWzShape2D/IWzVector2D methods
// =============================================================================

void Gr2DVector::PutX(std::int32_t x)
{
    m_x = x;
    if (m_chain)
    {
        m_chain->base_x = x;
        m_chain->evaluated = false;
    }
}

void Gr2DVector::PutY(std::int32_t y)
{
    m_y = y;
    if (m_chain)
    {
        m_chain->base_y = y;
        m_chain->evaluated = false;
    }
}

void Gr2DVector::Init(std::int32_t x, std::int32_t y)
{
    Move(x, y);
}

void Gr2DVector::PutCurrentTime(std::int32_t t)
{
    Gr2DTime::SetCurrentTime(t);
}

void Gr2DVector::PutRX(std::int32_t x)
{
    if (m_chain)
    {
        m_chain->base_x = x;
        m_chain->evaluated = false;
    }
    else
    {
        m_x = x;
    }
}

void Gr2DVector::PutRY(std::int32_t y)
{
    if (m_chain)
    {
        m_chain->base_y = y;
        m_chain->evaluated = false;
    }
    else
    {
        m_y = y;
    }
}

void Gr2DVector::PutRA(double a)
{
    if (m_chain)
    {
        m_chain->base_angle = a;
        m_chain->evaluated = false;
    }
}

void Gr2DVector::PutFlipX(std::int32_t f)
{
    if (m_chain)
    {
        m_chain->flip_accum = f;
        m_chain->evaluated = false;
    }
}

} // namespace ms
