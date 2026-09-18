#pragma once

#include "RenderDevice.hpp"
#include "RenderStateCache.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include <unordered_map>
#include <string>
#endif

namespace storm::render {

class GlesRenderDevice final : public RenderDevice {
public:
    struct TextureRecord { TextureDesc desc{}; std::uint32_t glId{0}; };
    GlesRenderDevice() = default;
    ~GlesRenderDevice() override;
    GlesRenderDevice(const GlesRenderDevice&) = delete;
    GlesRenderDevice& operator=(const GlesRenderDevice&) = delete;
    BufferHandle createBuffer(const BufferDesc& desc) override;
    void destroyBuffer(BufferHandle handle) override;
    bool updateBuffer(BufferHandle handle, const void* data, std::size_t size, std::size_t offset = 0) override;
    TextureHandle createTexture(const TextureDesc& desc) override;
    bool updateTexture(TextureHandle handle, const void* data, std::size_t size, std::uint32_t mipLevel = 0) override;
    void destroyTexture(TextureHandle handle) override;
    ShaderHandle createShader(const ShaderDesc& desc, const std::string& source) override;
    void destroyShader(ShaderHandle handle) override;
    void beginFrame() override;
    bool submit(const DrawCommand& command) override;
    void endFrame() override;
    // Called with no current GL context when the EGL context is lost.
    // Logical handles and CPU-side resource data remain valid.
    void invalidateGpuResources() noexcept;
    // Must be called with a current GLES context to rebuild GPU objects.
    bool restoreGpuResources() noexcept;
    [[nodiscard]] std::size_t liveBufferCount() const noexcept override;
    [[nodiscard]] std::size_t liveTextureCount() const noexcept override;
    [[nodiscard]] std::size_t liveShaderCount() const noexcept override;
    [[nodiscard]] std::size_t submittedDrawCount() const noexcept override { return submittedDraws_; }
private:
    struct BufferRecord {
        std::uint64_t size{0};
        std::uint32_t glId{0};
        std::vector<std::uint8_t> data;
    };
    struct ShaderRecord {
        std::uint32_t glId{0};
        ShaderStage stage{ShaderStage::Vertex};
        std::string source;
    };
    ResourceHandleAllocator<BufferHandle> bufferHandles_;
    ResourceHandleAllocator<TextureHandle> textureHandles_;
    ResourceHandleAllocator<ShaderHandle> shaderHandles_;
    std::size_t submittedDraws_{0};
    bool frameActive_{false};
#if defined(__ANDROID__)
    std::unordered_map<std::uint32_t, BufferRecord> buffers_;
    std::unordered_map<std::uint32_t, TextureRecord> textures_;
    std::unordered_map<std::uint32_t, ShaderRecord> shaders_;
    std::unordered_map<std::uint64_t, std::uint32_t> programs_;
    std::uint32_t program_{0};
    std::uint32_t vao_{0};
    std::int32_t maxTextureUnits_{0};
    RenderStateCache stateCache_{};
    std::unordered_map<std::uint64_t, std::unordered_map<std::string, std::int32_t>> uniformLocations_;
#endif
};

} // namespace storm::render
