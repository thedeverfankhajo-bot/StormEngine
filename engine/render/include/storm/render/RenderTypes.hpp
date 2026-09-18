#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>
#include <limits>

namespace storm::render {

class Material;

template <typename Tag>
class ResourceHandle final {
public:
    using value_type = std::uint32_t;
    static constexpr value_type invalidId = 0;
    static constexpr value_type invalidGeneration = 0;
    constexpr ResourceHandle() noexcept = default;
    explicit constexpr ResourceHandle(value_type id) noexcept : id_(id), generation_(1) {}
    constexpr ResourceHandle(value_type id, value_type generation) noexcept : id_(id), generation_(generation) {}
    [[nodiscard]] constexpr value_type id() const noexcept { return id_; }
    [[nodiscard]] constexpr value_type generation() const noexcept { return generation_; }
    [[nodiscard]] constexpr bool valid() const noexcept { return id_ != invalidId && generation_ != invalidGeneration; }
    friend constexpr bool operator==(ResourceHandle, ResourceHandle) noexcept = default;
private:
    value_type id_{invalidId};
    value_type generation_{invalidGeneration};
};

struct BufferTag; struct TextureTag; struct ShaderTag; struct MaterialTag; struct MeshTag;
using BufferHandle = ResourceHandle<BufferTag>;
using TextureHandle = ResourceHandle<TextureTag>;
using ShaderHandle = ResourceHandle<ShaderTag>;
using MaterialHandle = ResourceHandle<MaterialTag>;
using MeshHandle = ResourceHandle<MeshTag>;

template <typename Handle>
class ResourceHandleAllocator final {
public:
    [[nodiscard]] Handle allocate() {
        if (!free_.empty()) {
            const auto id = free_.back();
            free_.pop_back();
            return Handle(id, generations_[id - 1]);
        }
        if (generations_.size() >= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
            return {};
        generations_.push_back(1);
        return Handle(static_cast<std::uint32_t>(generations_.size()), 1);
    }

    bool release(Handle handle) noexcept {
        if (!handle.valid() || handle.id() > generations_.size() || generations_[handle.id() - 1] != handle.generation())
            return false;
        auto& generation = generations_[handle.id() - 1];
        if (generation == std::numeric_limits<std::uint32_t>::max()) return false;
        ++generation;
        free_.push_back(handle.id());
        return true;
    }

    [[nodiscard]] bool valid(Handle handle) const noexcept {
        return handle.valid() && handle.id() > 0 && handle.id() <= generations_.size() &&
               generations_[handle.id() - 1] == handle.generation();
    }

private:
    std::vector<std::uint32_t> generations_;
    std::vector<std::uint32_t> free_;
};

enum class BufferUsage : std::uint8_t { Static, Dynamic, Stream };
enum class PrimitiveTopology : std::uint8_t { Points, Lines, Triangles, TriangleStrip };
enum class IndexType : std::uint8_t { UInt16, UInt32 };
enum class ShaderStage : std::uint8_t { Vertex, Fragment };
enum class VertexFormat : std::uint8_t { Float32, Float32x2, Float32x3, Float32x4 };
enum class TextureFormat : std::uint8_t { RGBA8 };

struct BufferDesc final { std::size_t size{0}; BufferUsage usage{BufferUsage::Static}; };
struct TextureDesc final {
    std::uint32_t width{1};
    std::uint32_t height{1};
    std::uint32_t mipLevels{1};
    TextureFormat format{TextureFormat::RGBA8};
};
struct ShaderDesc final { ShaderStage stage{ShaderStage::Vertex}; };

struct VertexAttribute final { std::uint32_t location{0}; VertexFormat format{VertexFormat::Float32}; std::uint32_t offset{0}; };

struct VertexLayout final {
    static constexpr std::uint32_t maxAttributes = 8;
    std::array<VertexAttribute, maxAttributes> attributes{};
    std::uint32_t attributeCount{0};
    std::uint32_t stride{0};
    [[nodiscard]] constexpr bool valid() const noexcept {
        if (attributeCount == 0 || attributeCount > maxAttributes || stride == 0) return false;
        for (std::uint32_t i = 0; i < attributeCount; ++i) {
            std::uint32_t components = 0;
            switch (attributes[i].format) {
            case VertexFormat::Float32: components = 1; break;
            case VertexFormat::Float32x2: components = 2; break;
            case VertexFormat::Float32x3: components = 3; break;
            case VertexFormat::Float32x4: components = 4; break;
            default: return false;
            }
            if (attributes[i].offset > stride) return false;
            const std::uint64_t end = static_cast<std::uint64_t>(attributes[i].offset) + components * sizeof(float);
            if (end > stride) return false;
            for (std::uint32_t j = 0; j < i; ++j)
                if (attributes[j].location == attributes[i].location) return false;
        }
        return true;
    }
};

struct DrawCommand final {
    PrimitiveTopology topology{PrimitiveTopology::Triangles};
    BufferHandle vertexBuffer{}; BufferHandle indexBuffer{};
    std::uint32_t vertexCount{0}; std::uint32_t indexCount{0};
    std::uint32_t firstVertex{0}; std::uint32_t firstIndex{0};
    std::int32_t baseVertex{0}; IndexType indexType{IndexType::UInt32};
    ShaderHandle shader{}; ShaderHandle fragmentShader{}; MaterialHandle material{};
    const Material* materialData{nullptr}; VertexLayout vertexLayout{};
    std::uint32_t viewportWidth{0};
    std::uint32_t viewportHeight{0};
    bool blendEnabled{false};
    bool depthTestEnabled{true};
    bool depthWriteEnabled{true};
    bool cullEnabled{true};
    bool scissorEnabled{false};
    std::int32_t scissorX{0};
    std::int32_t scissorY{0};
    std::uint32_t scissorWidth{0};
    std::uint32_t scissorHeight{0};
    [[nodiscard]] constexpr bool indexed() const noexcept { return indexCount > 0; }
};

static_assert(std::is_trivially_copyable_v<BufferHandle>);
static_assert(std::is_trivially_copyable_v<TextureHandle>);
static_assert(std::is_trivially_copyable_v<ShaderHandle>);
static_assert(std::is_trivially_copyable_v<MaterialHandle>);
static_assert(std::is_trivially_copyable_v<MeshHandle>);
static_assert(std::is_trivially_copyable_v<VertexLayout>);
static_assert(std::is_trivially_copyable_v<DrawCommand>);
static_assert(std::is_trivially_copyable_v<BufferDesc>);
static_assert(std::is_trivially_copyable_v<TextureDesc>);
static_assert(std::is_trivially_copyable_v<ShaderDesc>);

} // namespace storm::render
