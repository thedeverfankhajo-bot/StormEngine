#pragma once

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

using BufferHandle = ResourceHandle<BufferTag>;
using TextureHandle = ResourceHandle<TextureTag>;
using ShaderHandle = ResourceHandle<ShaderTag>;
using MaterialHandle = ResourceHandle<MaterialTag>;

enum class BufferUsage {
    Static,
    Dynamic,
    Stream
};

enum class PrimitiveTopology {
    Points,
    Lines,
    Triangles,
    TriangleStrip
};

enum class IndexType {
    UInt16,
    UInt32
};

struct BufferDesc {
    std::uint64_t size{0};
    BufferUsage usage{BufferUsage::Static};
};

struct TextureDesc {
    std::uint32_t width{1};
    std::uint32_t height{1};
    std::uint32_t mipLevels{1};
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

    constexpr bool indexed() const noexcept { return indexBuffer.valid() && indexCount > 0; }
};

static_assert(std::is_trivially_copyable_v<BufferHandle>);
static_assert(std::is_trivially_copyable_v<TextureHandle>);
static_assert(std::is_trivially_copyable_v<ShaderHandle>);
static_assert(std::is_trivially_copyable_v<MaterialHandle>);
static_assert(sizeof(BufferHandle) == sizeof(BufferHandle::Id));

} // namespace storm::render
