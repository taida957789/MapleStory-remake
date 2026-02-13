#pragma once

namespace ms
{

/**
 * @brief Simplified Box2D 2D vector stub
 *
 * Replaces b2Vec2 from Box2D. Used by CVecCtrl for Box2D-integrated
 * physics calculations. Placeholder until full Box2D integration.
 */
struct b2Vec2
{
    float x{};
    float y{};
};

/**
 * @brief Simplified Box2D body stub
 *
 * Replaces b2Body from Box2D. Used by CVecCtrl as an opaque pointer
 * for Box2D world interaction. Placeholder until full Box2D integration.
 */
class b2Body
{
};

} // namespace ms
