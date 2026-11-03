#include "storm/core/transform_system_2d.hpp"
#include <new>
#include <stdexcept>
#include <vector>
namespace Storm {
namespace {
[[nodiscard]] Error invalid_entity(){return Error::make(ErrorCode::InvalidState,"Entity handle is stale, invalid, or belongs to another Scene");}
[[nodiscard]] Error exhausted(const char* m){return Error::make(ErrorCode::ResourceExhausted,m);}
}
Result<void> TransformSystem2D::set_local_transform(Scene& scene,EntityHandle entity,const Math::Transform2D& value){
 if(!scene.is_alive(entity))return Result<void>::failure(invalid_entity());
 if(!value.is_valid())return Result<void>::failure(Error::make(ErrorCode::InvalidArgument,"Transform2D must contain finite values"));
 if(auto* existing=scene.try_get_component<Math::Transform2D>(entity)){*existing=value;return Result<void>::success();}
 try{auto added=scene.add_component<Math::Transform2D>(entity,value);if(!added)return Result<void>::failure(added.error());}
 catch(const std::bad_alloc&){return Result<void>::failure(exhausted("Unable to allocate Transform2D storage"));}
 catch(const std::length_error&){return Result<void>::failure(exhausted("Transform2D storage limit reached"));}
 return Result<void>::success();
}
Result<Math::Transform2D> TransformSystem2D::local_transform(const Scene& scene,EntityHandle entity){
 if(!scene.is_alive(entity))return Result<Math::Transform2D>::failure(invalid_entity());
 const auto* value=scene.try_get_component<Math::Transform2D>(entity);
 if(value==nullptr)return Result<Math::Transform2D>::success(Math::Transform2D{});
 if(!value->is_valid())return Result<Math::Transform2D>::failure(Error::make(ErrorCode::InvalidState,"Invalid Transform2D component"));
 return Result<Math::Transform2D>::success(*value);
}
Result<Math::Matrix3> TransformSystem2D::world_matrix(const Scene& scene,EntityHandle entity){
 if(!scene.is_alive(entity))return Result<Math::Matrix3>::failure(invalid_entity());
 std::vector<EntityHandle> chain;
 try{
  const std::size_t maximum=scene.entity_count();EntityHandle current=entity;
  for(;;){
   if(chain.size()>=maximum)return Result<Math::Matrix3>::failure(Error::make(ErrorCode::InvalidState,"Scene hierarchy cycle or invalid depth"));
   chain.push_back(current);auto parent=scene.parent_of(current);
   if(!parent)return Result<Math::Matrix3>::failure(parent.error());
    if (!parent.value().has_value()) { break; } else { current = *parent.value(); }
  }
 }catch(const std::bad_alloc&){return Result<Math::Matrix3>::failure(exhausted("Unable to allocate Transform2D ancestry"));}
 catch(const std::length_error&){return Result<Math::Matrix3>::failure(exhausted("Transform2D ancestry exceeds container limits"));}
 Math::Matrix3 world=Math::Matrix3::identity();
 for(auto it=chain.rbegin();it!=chain.rend();++it){
  const auto* local=scene.try_get_component<Math::Transform2D>(*it);
  if(local==nullptr)continue;
  auto matrix=local->to_matrix();if(!matrix)return Result<Math::Matrix3>::failure(matrix.error());
  auto composed=Math::Matrix3::multiply_checked(world,matrix.value());
  if(!composed)return Result<Math::Matrix3>::failure(composed.error());
  world=composed.value();
 }
 return Result<Math::Matrix3>::success(world);
}
} // namespace Storm
