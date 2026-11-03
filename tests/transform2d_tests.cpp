#include "storm/core/transform_system_2d.hpp"
#include "storm/core/transform_system.hpp"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
#define EXPECT(...) do{if(!(__VA_ARGS__))return std::string("Expectation failed: ")+#__VA_ARGS__;}while(false)
bool near(float a,float b,float e=1.0e-4F){return std::fabs(a-b)<=e;}
using Storm::Math::Matrix3;using Storm::Math::Transform2D;using Storm::TransformSystem2D;
std::optional<std::string> matrix_and_rotation(){
 auto id=Matrix3::identity();EXPECT(id.is_finite());for(std::size_t r=0;r<3;++r)for(std::size_t c=0;c<3;++c)EXPECT(near(id.at(r,c),r==c?1:0));
 auto m=Transform2D{.translation={5,-3},.scale={2,4}}.to_matrix();EXPECT(m);EXPECT(near(m.value().at(0,2),5));EXPECT(near(m.value().at(1,2),-3));EXPECT(near(m.value().at(0,0),2));EXPECT(near(m.value().at(1,1),4));
 auto rotated=Transform2D{.rotation_radians=1.57079632679F,.scale={2,1}}.to_matrix();EXPECT(rotated);EXPECT(near(rotated.value().at(1,0),2));EXPECT(near(rotated.value().at(0,1),-1));return {};
}
std::optional<std::string> invalid_values_and_bounds(){
 EXPECT(!Transform2D{.rotation_radians=std::numeric_limits<float>::infinity()}.to_matrix());
 EXPECT(!Transform2D{.translation={std::numeric_limits<float>::quiet_NaN(),0}}.to_matrix());
 EXPECT(Transform2D{.scale={0,-2}}.to_matrix());bool threw=false;try{(void)Matrix3::identity().at(3,0);}catch(const std::out_of_range&){threw=true;}EXPECT(threw);return {};
}
std::optional<std::string> hierarchy_composition(){
 auto s=Storm::Scene::create("Planar");EXPECT(s);auto scene=std::move(s).value();auto p=scene.create_entity("P"),c=scene.create_entity("C");EXPECT(p);EXPECT(c);EXPECT(scene.set_parent(c.value(),p.value()));
 EXPECT(TransformSystem2D::set_local_transform(scene,p.value(),Transform2D{.translation={10,0}}));
 EXPECT(TransformSystem2D::set_local_transform(scene,c.value(),Transform2D{.translation={0,2}}));
 auto w=TransformSystem2D::world_matrix(scene,c.value());EXPECT(w);EXPECT(near(w.value().at(0,2),10));EXPECT(near(w.value().at(1,2),2));return {};
}
std::optional<std::string> inheritance_and_identity_group(){
 auto s=Storm::Scene::create("Groups");EXPECT(s);auto scene=std::move(s).value();auto g=scene.create_entity("G"),p=scene.create_entity("P"),c=scene.create_entity("C");EXPECT(g);EXPECT(p);EXPECT(c);EXPECT(scene.set_parent(p.value(),g.value()));EXPECT(scene.set_parent(c.value(),p.value()));
 EXPECT(TransformSystem2D::set_local_transform(scene,p.value(),Transform2D{.translation={3,0},.rotation_radians=1.57079632679F,.scale={2,2}}));
 EXPECT(TransformSystem2D::set_local_transform(scene,c.value(),Transform2D{.translation={1,0}}));auto w=TransformSystem2D::world_matrix(scene,c.value());EXPECT(w);EXPECT(near(w.value().at(0,2),3));EXPECT(near(w.value().at(1,2),2));return {};
}
std::optional<std::string> stale_foreign_and_corrupt_component(){
 auto a=Storm::Scene::create("A"),b=Storm::Scene::create("B");EXPECT(a);EXPECT(b);auto first=std::move(a).value(),second=std::move(b).value();auto e=first.create_entity("E");EXPECT(e);EXPECT(!TransformSystem2D::world_matrix(second,e.value()));
 EXPECT(first.add_component<Transform2D>(e.value(),Transform2D{.rotation_radians=std::numeric_limits<float>::quiet_NaN()}));EXPECT(!TransformSystem2D::world_matrix(first,e.value()));EXPECT(first.destroy_entity(e.value()));EXPECT(!TransformSystem2D::local_transform(first,e.value()));return {};
}
std::optional<std::string> hierarchy_overflow(){
 auto s=Storm::Scene::create("Overflow");EXPECT(s);auto scene=std::move(s).value();auto p=scene.create_entity("P"),c=scene.create_entity("C");EXPECT(p);EXPECT(c);EXPECT(scene.set_parent(c.value(),p.value()));
 EXPECT(TransformSystem2D::set_local_transform(scene,p.value(),Transform2D{.scale={std::numeric_limits<float>::max(),1}}));EXPECT(TransformSystem2D::set_local_transform(scene,c.value(),Transform2D{.scale={2,1}}));
 auto w=TransformSystem2D::world_matrix(scene,c.value());EXPECT(!w);EXPECT(w.error().code==Storm::ErrorCode::InvalidArgument);return {};
}
std::optional<std::string> deep_chain_and_scene_move(){
 auto s=Storm::Scene::create("Deep");EXPECT(s);auto scene=std::move(s).value();std::vector<Storm::EntityHandle> chain;chain.reserve(128);
 for(std::size_t i=0;i<128;++i){auto e=scene.create_entity("N");EXPECT(e);chain.push_back(e.value());EXPECT(TransformSystem2D::set_local_transform(scene,e.value(),Transform2D{.translation={1,0}}));if(i)EXPECT(scene.set_parent(e.value(),chain[i-1]));}
 Storm::Scene moved=std::move(scene);auto w=TransformSystem2D::world_matrix(moved,chain.back());EXPECT(w);EXPECT(near(w.value().at(0,2),128));EXPECT(!TransformSystem2D::world_matrix(scene,chain.back()));return {};
}
std::optional<std::string> two_d_and_three_d_are_separate(){
 auto s=Storm::Scene::create("Mixed");EXPECT(s);auto scene=std::move(s).value();auto e=scene.create_entity("E");EXPECT(e);
 EXPECT(TransformSystem2D::set_local_transform(scene,e.value(),Transform2D{.translation={2,3}}));
 EXPECT(Storm::TransformSystem::set_local_transform(scene,e.value(),Storm::Math::Transform3D{.translation={0,0,8}}));
 auto p=TransformSystem2D::world_matrix(scene,e.value());auto q=Storm::TransformSystem::world_matrix(scene,e.value());EXPECT(p);EXPECT(q);EXPECT(near(p.value().at(0,2),2));EXPECT(near(p.value().at(1,2),3));EXPECT(near(q.value().at(2,3),8));return {};
}
std::optional<std::string> matrix3_checked_math(){
 auto large=Transform2D{.scale={std::numeric_limits<float>::max(),1}}.to_matrix();
 auto twice=Transform2D{.scale={2,1}}.to_matrix();EXPECT(large);EXPECT(twice);
 auto overflow=Matrix3::multiply_checked(large.value(),twice.value());EXPECT(!overflow);EXPECT(overflow.error().code==Storm::ErrorCode::InvalidArgument);
 auto unchecked=large.value()*twice.value();EXPECT(!unchecked.is_finite());
 EXPECT(!Matrix3::multiply_checked(unchecked,Matrix3::identity()));
 auto point=Matrix3::identity().transform_point({3,-4});EXPECT(point);EXPECT(near(point.value().x,3));EXPECT(near(point.value().y,-4));
 EXPECT(!Matrix3::identity().transform_point({std::numeric_limits<float>::infinity(),0}));
 return {};
}
std::optional<std::string> camera2d_center_edges_and_rotation(){
 using Storm::Math::Camera2D;
 Camera2D camera{.position={10,20},.viewport_size={200,100},.zoom=1};
 auto center=camera.world_to_ndc({10,20});EXPECT(center);EXPECT(near(center.value().x,0));EXPECT(near(center.value().y,0));
 auto upper_right=camera.world_to_ndc({110,70});EXPECT(upper_right);EXPECT(near(upper_right.value().x,1));EXPECT(near(upper_right.value().y,1));
 camera.rotation_radians=1.57079632679F;
 auto rotated_right=camera.world_to_ndc({10,120});EXPECT(rotated_right);EXPECT(near(rotated_right.value().x,1));EXPECT(near(rotated_right.value().y,0));
 auto rotated_center=camera.world_to_ndc({10,20});EXPECT(rotated_center);EXPECT(near(rotated_center.value().x,0));EXPECT(near(rotated_center.value().y,0));
 return {};
}
std::optional<std::string> camera2d_rejects_invalid_and_unrepresentable_state(){
 using Storm::Math::Camera2D;
 Camera2D camera{};EXPECT(camera.view_projection());
 camera.zoom=0;EXPECT(!camera.view_matrix());camera.zoom=1;
 camera.viewport_size.x=0;EXPECT(!camera.projection_matrix());camera.viewport_size={1,1};
 camera.rotation_radians=std::numeric_limits<float>::quiet_NaN();EXPECT(!camera.view_projection());
 camera.rotation_radians=0;camera.viewport_size.x=std::numeric_limits<float>::denorm_min();EXPECT(!camera.projection_matrix());
 return {};
}
struct Test{const char*name;std::optional<std::string>(*fn)();};
}
int main(){
 constexpr std::array tests{
 Test{"matrix_and_rotation",matrix_and_rotation},Test{"invalid_values_and_bounds",invalid_values_and_bounds},
 Test{"hierarchy_composition",hierarchy_composition},Test{"inheritance_and_identity_group",inheritance_and_identity_group},
 Test{"stale_foreign_and_corrupt_component",stale_foreign_and_corrupt_component},Test{"hierarchy_overflow",hierarchy_overflow},
 Test{"deep_chain_and_scene_move",deep_chain_and_scene_move},Test{"2d_and_3d_are_separate",two_d_and_three_d_are_separate},
 Test{"matrix3_checked_math",matrix3_checked_math},Test{"camera2d_center_edges_and_rotation",camera2d_center_edges_and_rotation},
 Test{"camera2d_rejects_invalid_and_unrepresentable_state",camera2d_rejects_invalid_and_unrepresentable_state}};
 std::size_t failed=0;for(const auto&t:tests){auto r=t.fn();if(r){++failed;std::cerr<<"[FAIL] "<<t.name<<": "<<*r<<'\n';}else std::cout<<"[PASS] "<<t.name<<'\n';}
 std::cout<<"Executed "<<tests.size()<<" tests; failures: "<<failed<<'\n';return failed==0?EXIT_SUCCESS:EXIT_FAILURE;
}
