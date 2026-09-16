#pragma once

#include <cstdint>
#include <limits>

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

struct BufferDesc {
    std::uint64_t size{0};
    BufferUsage usage{BufferUsage::Static};
};

struct TextureDesc {
    std::uint32_t width{1};
    std::uint32_t height{1};
    std::uint32_t mipLevels{1};
};

} // namespace storm::render
