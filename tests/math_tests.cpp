#include <cassert>
#include <cmath>

#include "storm/math/Math.hpp"

namespace {

constexpr float kTolerance = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) <= kTolerance;
}

void testVectors() {
    const storm::math::Vec3 a{1.0f, 2.0f, 3.0f};
    const storm::math::Vec3 b{4.0f, 5.0f, 6.0f};
    assert(a.cross(b).x == -3.0f);
    assert(a.cross(b).y == 6.0f);
    assert(a.cross(b).z == -3.0f);
    assert(near(a.normalized().length(), 1.0f));
}

void testTransformMatrix() {
    const storm::math::Mat4 matrix = storm::math::Mat4::translation({2.0f, 3.0f, 4.0f}) *
                                     storm::math::Mat4::scale({2.0f, 2.0f, 2.0f});
    const storm::math::Vec4 result = matrix * storm::math::Vec4{1.0f, 1.0f, 1.0f, 1.0f};
    assert(near(result.x, 4.0f));
    assert(near(result.y, 5.0f));
    assert(near(result.z, 6.0f));
    assert(near(result.w, 1.0f));
}

void testQuaternionRotation() {
    const auto q = storm::math::Quaternion::fromAxisAngle({0.0f, 1.0f, 0.0f}, storm::math::kPi * 0.5f);
    const auto forward = q.rotate(storm::math::Vec3::forward());
    assert(near(forward.x, -1.0f));
    assert(near(forward.y, 0.0f));
    assert(near(forward.z, 0.0f));
}

void testProjection() {
    const auto projection = storm::math::Mat4::perspective(storm::math::radians(90.0f), 1.0f, 0.1f, 100.0f);

    // StormEngine uses right-handed view space with the camera looking down -Z
    // and OpenGL-style NDC depth in [-1, 1]. A point one unit in front of the
    // camera therefore has positive clip-space W and a negative Z before the
    // perspective divide.
    const auto nearPoint = projection * storm::math::Vec4{0.0f, 0.0f, -1.0f, 1.0f};
    assert(nearPoint.w > 0.0f);
    assert(near(nearPoint.x, 0.0f));
    assert(near(nearPoint.y, 0.0f));
    assert(near(nearPoint.z, 0.8018018f));

    const auto farPoint = projection * storm::math::Vec4{0.0f, 0.0f, -100.0f, 1.0f};
    assert(near(farPoint.z / farPoint.w, 1.0f));

    const auto nearPlanePoint = projection * storm::math::Vec4{0.0f, 0.0f, -0.1f, 1.0f};
    assert(near(nearPlanePoint.z / nearPlanePoint.w, -1.0f));
}

void testTranspose() {
    storm::math::Mat4 matrix;
    matrix.m[0][1] = 2.0f;
    matrix.m[3][2] = 7.0f;
    const auto transpose = matrix.transposed();
    assert(transpose.m[1][0] == 2.0f);
    assert(transpose.m[2][3] == 7.0f);
}

} // namespace

int main() {
    testVectors();
    testTransformMatrix();
    testQuaternionRotation();
    testProjection();
    testTranspose();
    return 0;
}
