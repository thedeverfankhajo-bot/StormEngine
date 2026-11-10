#include "storm/core/transform_system_2d.hpp"
#include "storm/rendering/sprite_batch.hpp"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {
#define EXPECT(condition) do { if (!(condition)) return std::string("Expectation failed: ") + #condition; } while (false)
using Storm::Math::Transform2D;
using Storm::Rendering::Sprite2DComponent;
using Storm::Rendering::SpriteBatch;
using Storm::Rendering::SpriteBatchConfig;
using Storm::Rendering::SpriteSceneCollector2D;
using Storm::Rendering::SpriteColor;
using Storm::Rendering::TextureKey;
[[nodiscard]] bool near(float a,float b,float e=1.0e-4F){return std::fabs(a-b)<=e;}
[[nodiscard]] Sprite2DComponent make_sprite(std::uint64_t id=1U,std::uint32_t gen=1U){
 return Sprite2DComponent{.texture=TextureKey{id,gen},.size={10,4},.pivot={0.5F,0.5F},
 .uv={0.1F,0.2F,0.9F,0.8F},.color=SpriteColor{255,128,64,200}};
}
[[nodiscard]] std::optional<std::string> quad_geometry_and_uv(){
 auto c=SpriteBatch::create();EXPECT(c);auto b=std::move(c).value();
 EXPECT(b.append_local(make_sprite(),Transform2D{.translation={5,7}}));
 auto v=b.build();EXPECT(v);EXPECT(v.value().vertices.size()==4U);EXPECT(v.value().indices.size()==6U);
 EXPECT(near(v.value().vertices[0].position.x,0));EXPECT(near(v.value().vertices[0].position.y,5));
 EXPECT(near(v.value().vertices[2].position.x,10));EXPECT(near(v.value().vertices[2].position.y,9));
 EXPECT(near(v.value().vertices[0].uv.x,0.1F));EXPECT(near(v.value().vertices[0].uv.y,0.8F));
 EXPECT(near(v.value().vertices[2].uv.x,0.9F));EXPECT(near(v.value().vertices[2].uv.y,0.2F));
 constexpr std::array<std::uint32_t,6U> expected{0,1,2,2,3,0};
 for(std::size_t i=0;i<expected.size();++i)EXPECT(v.value().indices[i]==expected[i]);
 EXPECT(v.value().vertices[0].color.a==200U);return {};
}
[[nodiscard]] std::optional<std::string> pivot_and_rotation(){
 auto c=SpriteBatch::create();EXPECT(c);auto b=std::move(c).value();auto s=make_sprite();
 s.size={2,2};s.pivot={0,0};constexpr float half_pi=1.5707963267948966F;
 EXPECT(b.append_local(s,Transform2D{.translation={10,20},.rotation_radians=half_pi}));
 auto v=b.build();EXPECT(v);EXPECT(near(v.value().vertices[0].position.x,10));EXPECT(near(v.value().vertices[0].position.y,20));
 EXPECT(near(v.value().vertices[1].position.x,10));EXPECT(near(v.value().vertices[1].position.y,22));
 EXPECT(near(v.value().vertices[2].position.x,8));EXPECT(near(v.value().vertices[2].position.y,22));
 return {};
}
[[nodiscard]] std::optional<std::string> stable_painter_sort(){
 auto c=SpriteBatch::create(SpriteBatchConfig{.maximum_sprites=8});EXPECT(c);auto b=std::move(c).value();
 auto a=make_sprite(20);a.sorting_layer=1;a.sorting_order=3;
 auto z=make_sprite(10);z.sorting_order=7;
 auto d=make_sprite(30);d.sorting_layer=1;d.sorting_order=3;
 auto q=make_sprite(40);q.sorting_layer=1;q.sorting_order=2;
 EXPECT(b.append_local(a,{}));EXPECT(b.append_local(z,{}));EXPECT(b.append_local(d,{}));EXPECT(b.append_local(q,{}));
 auto v=b.build();EXPECT(v);EXPECT(v.value().ranges.size()==4U);
 EXPECT(v.value().ranges[0].texture.id==10U);EXPECT(v.value().ranges[1].texture.id==40U);
 EXPECT(v.value().ranges[2].texture.id==20U);EXPECT(v.value().ranges[3].texture.id==30U);return {};
}
[[nodiscard]] std::optional<std::string> contiguous_ranges_only(){
 auto c=SpriteBatch::create(SpriteBatchConfig{.maximum_sprites=8});EXPECT(c);auto b=std::move(c).value();
 auto a=make_sprite(5,2);auto d=make_sprite(5,2);auto x=make_sprite(7,1);auto z=make_sprite(5,2);
 a.sorting_order=2;d.sorting_order=2;x.sorting_order=2;z.sorting_order=2;
 EXPECT(b.append_local(a,{}));EXPECT(b.append_local(d,{}));EXPECT(b.append_local(x,{}));EXPECT(b.append_local(z,{}));
 auto v=b.build();EXPECT(v);EXPECT(v.value().ranges.size()==3U);
 EXPECT(v.value().ranges[0].index_count==12U);EXPECT(v.value().ranges[1].texture.id==7U);
 EXPECT(v.value().ranges[2].first_index==18U);return {};
}
[[nodiscard]] std::optional<std::string> invalid_values_and_capacity(){
 auto c=SpriteBatch::create(SpriteBatchConfig{.maximum_sprites=1});EXPECT(c);auto b=std::move(c).value();
 EXPECT(!b.append_local(make_sprite(0),{}));auto uv=make_sprite();uv.uv.max_u=std::numeric_limits<float>::quiet_NaN();EXPECT(!b.append_local(uv,{}));
 auto size=make_sprite();size.size.x=0;EXPECT(!b.append_local(size,{}));auto pivot=make_sprite();pivot.pivot.y=std::numeric_limits<float>::infinity();EXPECT(!b.append_local(pivot,{}));
 EXPECT(b.pending_sprite_count()==0U);EXPECT(b.append_local(make_sprite(),{}));
 auto ex=b.append_local(make_sprite(2),{});EXPECT(!ex);EXPECT(ex.error().code==Storm::ErrorCode::ResourceExhausted);
 EXPECT(b.build().value().sprite_count==1U);return {};
}
[[nodiscard]] std::optional<std::string> failed_build_hides_partial_geometry(){
 auto c=SpriteBatch::create();EXPECT(c);auto b=std::move(c).value();
 auto s=make_sprite();s.size={std::numeric_limits<float>::max(),std::numeric_limits<float>::max()};
 auto t=Transform2D{.scale={std::numeric_limits<float>::max(),std::numeric_limits<float>::max()}}.to_matrix();EXPECT(t);
 EXPECT(b.append(s,t.value()));EXPECT(!b.build());EXPECT(!b.view());return {};
}
[[nodiscard]] std::optional<std::string> scene_hierarchy_collection(){
 auto s=Storm::Scene::create("Sprites");EXPECT(s);auto scene=std::move(s).value();
 auto p=scene.create_entity("parent"),c=scene.create_entity("child");EXPECT(p);EXPECT(c);EXPECT(scene.set_parent(c.value(),p.value()));
 EXPECT(Storm::TransformSystem2D::set_local_transform(scene,p.value(),Transform2D{.translation={10,0}}));
 EXPECT(Storm::TransformSystem2D::set_local_transform(scene,c.value(),Transform2D{.translation={0,2}}));
 auto sprite=make_sprite(77);sprite.size={2,2};EXPECT(scene.add_component<Sprite2DComponent>(c.value(),sprite));
 auto b=SpriteBatch::create();EXPECT(b);auto batch=std::move(b).value();auto v=SpriteSceneCollector2D::build(scene,batch);EXPECT(v);
 EXPECT(v.value().sprite_count==1U);EXPECT(near(v.value().vertices[0].position.x,9));EXPECT(near(v.value().vertices[0].position.y,1));
 EXPECT(near(v.value().vertices[2].position.x,11));EXPECT(near(v.value().vertices[2].position.y,3));return {};
}
[[nodiscard]] std::optional<std::string> scene_collection_is_transactional(){
 auto s=Storm::Scene::create("Bad");EXPECT(s);auto scene=std::move(s).value();auto e=scene.create_entity();EXPECT(e);
 EXPECT(scene.add_component<Sprite2DComponent>(e.value(),Sprite2DComponent{}));
 auto b=SpriteBatch::create();EXPECT(b);auto batch=std::move(b).value();EXPECT(batch.append_local(make_sprite(),{}));
 EXPECT(!SpriteSceneCollector2D::build(scene,batch));EXPECT(batch.pending_sprite_count()==0U);EXPECT(!batch.view());return {};
}
[[nodiscard]] std::optional<std::string> view_invalidation_and_moves(){
 auto c=SpriteBatch::create();EXPECT(c);auto b=std::move(c).value();EXPECT(b.append_local(make_sprite(),{}));EXPECT(b.build());EXPECT(b.view());
 EXPECT(b.append_local(make_sprite(2),{}));EXPECT(!b.view());SpriteBatch moved=std::move(b);
 EXPECT(moved.capacity()==SpriteBatchConfig{}.maximum_sprites);EXPECT(!b.append_local(make_sprite(),{}));EXPECT(moved.build());return {};
}
[[nodiscard]] std::optional<std::string> invalid_config(){
 EXPECT(!SpriteBatch::create(SpriteBatchConfig{.maximum_sprites=0U}));
 EXPECT(!SpriteBatch::create(SpriteBatchConfig{.maximum_sprites=SpriteBatch::maximum_supported_sprites+1U}));
 return {};
}
struct TestCase{const char*name;std::optional<std::string>(*run)();};
}
int main(){
 constexpr std::array tests{
 TestCase{"quad_geometry_and_uv",quad_geometry_and_uv},
 TestCase{"pivot_and_rotation",pivot_and_rotation},
 TestCase{"stable_painter_sort",stable_painter_sort},
 TestCase{"contiguous_ranges_only",contiguous_ranges_only},
 TestCase{"invalid_values_and_capacity",invalid_values_and_capacity},
 TestCase{"failed_build_hides_partial_geometry",failed_build_hides_partial_geometry},
 TestCase{"scene_hierarchy_collection",scene_hierarchy_collection},
 TestCase{"scene_collection_is_transactional",scene_collection_is_transactional},
 TestCase{"view_invalidation_and_moves",view_invalidation_and_moves},
 TestCase{"invalid_config",invalid_config}};
 std::size_t failures=0;for(const auto&t:tests){auto r=t.run();if(r){++failures;std::cerr<<"[FAIL] "<<t.name<<": "<<*r<<'\n';}else std::cout<<"[PASS] "<<t.name<<'\n';}
 std::cout<<"Executed "<<tests.size()<<" tests; failures: "<<failures<<'\n';return failures==0?EXIT_SUCCESS:EXIT_FAILURE;
}
