#include "storm/serialization/scene_serializer.hpp"
#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>
namespace {
#define EXPECT(...) do{if(!(__VA_ARGS__))return std::string("Expectation failed: ")+#__VA_ARGS__;}while(false)
using Storm::Math::Transform2D;using Storm::Serialization::BinaryWriter;using Storm::Serialization::SceneSerializer;
std::optional<std::vector<std::byte>> make_archive(std::uint32_t schema,std::vector<std::byte> payload){
 BinaryWriter w(4096);const std::array<std::byte,4> magic{std::byte{'S'},std::byte{'T'},std::byte{'S'},std::byte{'C'}};
 if(!w.write_bytes(magic)||!w.write_u16(1)||!w.write_u16(0)||!w.write_string("Scene",64)||!w.write_u32(1)||!w.write_u32(0)||!w.write_string("Entity",64)||!w.write_u32(0xFFFFFFFFU)||!w.write_u32(0)||!w.write_u32(1)||!w.write_string("storm.math.transform2d",64)||!w.write_u32(schema)||!w.write_u32(1)||!w.write_u32(0)||!w.write_u32(static_cast<std::uint32_t>(payload.size()))||!w.write_bytes(payload))return {};
 return std::move(w).take();
}
std::optional<std::string> round_trip(){
 auto s=Storm::Scene::create("Level");EXPECT(s);auto scene=std::move(s).value();auto e=scene.create_entity("Player");EXPECT(e);
 const Transform2D expected{.translation={12.5F,-8.25F},.rotation_radians=.75F,.scale={1.25F,.5F}};EXPECT(scene.add_component<Transform2D>(e.value(),expected));
 auto bytes=SceneSerializer::serialize<Transform2D>(scene);EXPECT(bytes);auto result=SceneSerializer::deserialize<Transform2D>(bytes.value());EXPECT(result);
 std::optional<Storm::EntityHandle> h;EXPECT(result.value().for_each_entity([&](Storm::EntityHandle x,std::string_view){h=x;}));EXPECT(h);
 auto*v=result.value().try_get_component<Transform2D>(*h);EXPECT(v);EXPECT(v->translation.x==expected.translation.x);EXPECT(v->translation.y==expected.translation.y);EXPECT(v->rotation_radians==expected.rotation_radians);EXPECT(v->scale.x==expected.scale.x);EXPECT(v->scale.y==expected.scale.y);return {};
}
std::optional<std::string> invalid_component_encode(){
 auto s=Storm::Scene::create("Bad");EXPECT(s);auto scene=std::move(s).value();auto e=scene.create_entity("E");EXPECT(e);
 EXPECT(scene.add_component<Transform2D>(e.value(),Transform2D{.scale={1,std::numeric_limits<float>::quiet_NaN()}}));
 auto bytes=SceneSerializer::serialize<Transform2D>(scene);EXPECT(!bytes);EXPECT(bytes.error().code==Storm::ErrorCode::InvalidArgument);return {};
}
std::optional<std::string> invalid_payload_and_schema(){
 BinaryWriter p(64);EXPECT(p.write_f32(1));EXPECT(p.write_f32(2));EXPECT(p.write_f32(0));EXPECT(p.write_f32(std::numeric_limits<float>::infinity()));EXPECT(p.write_f32(1));
 auto bad=make_archive(1,std::move(p).take());EXPECT(bad);auto invalid=SceneSerializer::deserialize<Transform2D>(*bad);EXPECT(!invalid);EXPECT(invalid.error().code==Storm::ErrorCode::InvalidArgument);
 BinaryWriter q(64);EXPECT(q.write_f32(1));EXPECT(q.write_f32(2));EXPECT(q.write_f32(0));EXPECT(q.write_f32(1));EXPECT(q.write_f32(1));
 auto future=make_archive(2,std::move(q).take());EXPECT(future);auto unsupported=SceneSerializer::deserialize<Transform2D>(*future);EXPECT(!unsupported);EXPECT(unsupported.error().code==Storm::ErrorCode::Unsupported);return {};
}
struct Test{const char*name;std::optional<std::string>(*fn)();};
}
int main(){
 constexpr std::array tests{Test{"round_trip",round_trip},Test{"invalid_component_encode",invalid_component_encode},Test{"invalid_payload_and_schema",invalid_payload_and_schema}};
 std::size_t failed=0;for(const auto&t:tests){auto r=t.fn();if(r){++failed;std::cerr<<"[FAIL] "<<t.name<<": "<<*r<<'\n';}else std::cout<<"[PASS] "<<t.name<<'\n';}
 std::cout<<"Executed "<<tests.size()<<" tests; failures: "<<failed<<'\n';return failed==0?EXIT_SUCCESS:EXIT_FAILURE;
}
