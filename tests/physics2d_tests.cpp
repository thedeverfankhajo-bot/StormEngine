#include <cassert>

#include "storm/physics/Aabb2.hpp"

int main() {
    using storm::math::Vec2;
    using storm::physics::Aabb2;
    using storm::physics::collide;

    const Aabb2 player{{0.0f, 0.0f}, {1.0f, 2.0f}};
    const Aabb2 floor{{-5.0f, -0.5f}, {5.0f, 0.0f}};
    const Aabb2 wall{{1.0f, 0.0f}, {1.5f, 3.0f}};

    assert(player.valid());
    assert(player.size().x == 1.0f);
    assert(player.center().y == 1.0f);
    assert(!player.overlaps(floor));
    assert(player.overlaps(wall));

    const auto wallHit = collide(player, wall);
    assert(wallHit.hit);
    assert(wallHit.normal.x < 0.0f);
    assert(wallHit.penetration.x > 0.0f);

    const Aabb2 touching{{1.0f, 0.0f}, {2.0f, 1.0f}};
    assert(!player.overlaps(touching));
    assert(!collide(player, touching).hit);

    const Aabb2 invalid{{2.0f, 0.0f}, {1.0f, 1.0f}};
    assert(!invalid.valid());
    assert(!collide(player, invalid).hit);

    (void)Vec2{};
    return 0;
}
