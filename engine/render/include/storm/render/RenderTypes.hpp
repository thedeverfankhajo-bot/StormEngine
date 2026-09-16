#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace storm::render {

template <typename Tag>
class ResourceHandle {
public:
    using Id = std::uint32_t;
    static constexpr Id invalidId = std::numeric_limits<Id>::max();
    constexpr ResourceHandle() noexcept = default;
    explicit constexpr ResourceHandle(Id id) noexcept : id_(id) {}
    constexpr Id id() const noexcept { return id_; }
    constexpr bool valid() const noexcept { return id_ != invalidId; }
    friend constexpr bool operator==(ResourceHandle a, ResourceHandle b) noexcept { return a.id_ == b.id_; }
    friend constexpr bool operator!=(ResourceHandle a, ResourceHandle b) noexcept { return !(a == b); }
private:
    Id id_{invalidId};
};

struct BufferTag;
struct TextureTag;
struct ShaderTag;
struct MaterialTag;
struct MeshTag;
using BufferHandle = ResourceHandle<BufferTag>;
using TextureHandle = ResourceHandle<TextureTag>;
using ShaderHandle = ResourceHandle<ShaderTag>;
using MaterialHandle = ResourceHandle<MaterialTag>;
using MeshHandle = ResourceHandle<MeshTag>;

enum class BufferUsage { Static, Dynamic, Stream };
enum class PrimitiveTopology { Points, Lines, Triangles, TriangleStrip };
enum class IndexType { UInt16, UInt32 };
enum class ShaderStage { Vertex, Fragment };
enum class VertexFormat { Float32, Float32x2, Float32x3, Float32x4 };

struct BufferDesc { std::uint64_t size{0}; BufferUsage usage{BufferUsage::Static}; };
struct TextureDesc { std::uint32_t width{1}; std::uint32_t height{1}; std::uint32_t mipLevels{1}; };
struct ShaderDesc { ShaderStage stage{ShaderStage::Vertex}; };

struct VertexAttribute {
    std::uint32_t location{0};
    VertexFormat format{VertexFormat::Float32x3};
    std::uint32_t offset{0};
};

struct VertexLayout {
    static constexpr std::size_t maxAttributes = 8;
    std::array<VertexAttribute, maxAttributes> attributes{};
    std::uint32_t attributeCount{0};
    std::uint32_t stride{0};
    constexpr bool valid() const noexcept { return attributeCount > 0 && attributeCount <= maxAttributes && stride > 0; }
};

struct MeshDesc {
    BufferHandle vertexBuffer{};
    BufferHandle indexBuffer{};
    std::uint32_t vertexCount{0};
    std::uint32_t indexCount{0};
    IndexType indexType{IndexType::UInt32};
    PrimitiveTopology topology{PrimitiveTopology::Triangles};
    VertexLayout vertexLayout{};
};

struct DrawCommand {
    PrimitiveTopology topology{PrimitiveTopology::Triangles};
    BufferHandle vertexBuffer{};
    BufferHandle indexBuffer{};
    std::uint32_t vertexCount{0};
    std::uint32_t indexCount{0};
    std::uint32_t firstVertex{0};
    std::uint32_t firstIndex{0};
    std::int32_t baseVertex{0};
    IndexType indexType{IndexType::UInt32};
    ShaderHandle shader{};
    ShaderHandle fragmentShader{};
    MaterialHandle material{};
    VertexLayout vertexLayout{};
    constexpr bool indexed() const noexcept { return indexCount > 0; }
};

static_assert(std::is_trivially_copyable_v<BufferHandle>);
static_assert(std::is_trivially_copyable_v<TextureHandle>);
static_assert(std::is_trivially_copyable_v<ShaderHandle>);
static_assert(std::is_trivially_copyable_v<MaterialHandle>);
static_assert(std::is_trivially_copyable_v<MeshHandle>);
static_assert(std::is_trivially_copyable_v<VertexAttribute>);
static_assert(std::is_trivially_copyable_v<VertexLayout>);
static_assert(std::is_trivially_copyable_v<DrawCommand>);
static_assert(sizeof(BufferHandle) == sizeof(BufferHandle::Id));

} // namespace storm::render
