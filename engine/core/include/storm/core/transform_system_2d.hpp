#pragma once
#include "storm/core/scene.hpp"
#include "storm/math/transform2d.hpp"
namespace Storm {
class TransformSystem2D final {
public:
 [[nodiscard]] static Result<void> set_local_transform(Scene&,EntityHandle,const Math::Transform2D&);
 [[nodiscard]] static Result<Math::Transform2D> local_transform(const Scene&,EntityHandle);
 [[nodiscard]] static Result<Math::Matrix3> world_matrix(const Scene&,EntityHandle);
};
} // namespace Storm
