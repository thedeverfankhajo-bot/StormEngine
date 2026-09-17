#include <cassert>
#include <cmath>
#include "storm/math/Vec1.hpp"
#include "storm/math/Quaternion.hpp"
#include "storm/math/Transform3D.hpp"
#include "storm/physics/Aabb1.hpp"
#include "storm/physics/Aabb2.hpp"
#include "storm/physics/Aabb3.hpp"

int main() {
    using namespace storm::math;
    using namespace storm::physics;

    const Vec1 one{4.0f};
    assert(one.length() == 4.0f);
    assert(one.normalized().x == 1.0f);
    assert(Vec1{}.normalized().x == 0.0f);

    const Quaternion q = Quaternion::fromAxisAngle(Vec3::up(), 3.14159265358979323846f * 0.5f);
    const Vec3 rotated = q.rotate(Vec3::right());
    assert(std::fabs(rotated.x) < 1e-5f);
    assert(std::fabs(rotated.z + 1.0f) < 1e-5f);
    const Quaternion badAxis = Quaternion::fromAxisAngle(Vec3::zero(), 1.0f);
    assert(std::fabs(badAxis.w - 1.0f) < 1e-6f);

    Transform3D transform;
    assert(transform.finite());
    transform.position = {1.0f, 2.0f, 3.0f};
    transform.scale = {2.0f, 3.0f, 4.0f};
    const Mat4 matrix = transform.matrix();
    assert(matrix.m[0][3] == 1.0f && matrix.m[1][3] == 2.0f && matrix.m[2][3] == 3.0f);

    assert(collide(Aabb1{0.0f, 2.0f}, Aabb1{1.0f, 3.0f}).hit);
    assert(!collide(Aabb1{0.0f, 1.0f}, Aabb1{1.0f, 2.0f}).hit);
    assert(collide(Aabb2{{0,0}, {2,2}}, Aabb2{{1,1}, {3,3}}).hit);
    assert(collide(Aabb3{{0,0,0}, {2,2,2}}, Aabb3{{1,1,1}, {3,3,3}}).hit);
    const Aabb3 invalidAabb{{NAN, 0, 0}, {1, 1, 1}};
    assert(!invalidAabb.valid());
    return 0;
}
