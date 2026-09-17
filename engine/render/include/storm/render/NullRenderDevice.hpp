#pragma once

#include "RenderDevice.hpp"
#include <unordered_map>
#include <unordered_set>

namespace storm::render {

class NullRenderDevice final : public RenderDevice {
public:
    BufferHandle createBuffer(const BufferDesc& desc) override;
    void destroyBuffer(BufferHandle handle) override;
    bool updateBuffer(BufferHandle handle, const void* data, std::size_t size,
                      std::size_t offset = 0) override;

    TextureHandle createTexture(const TextureDesc& desc) override;
    void destroyTexture(TextureHandle handle) override;

    ShaderHandle createShader(const ShaderDesc& desc, const std::string& source) override;
    void destroyShader(ShaderHandle handle) override;

    void beginFrame() override;
    bool submit(const DrawCommand& command) override;
    void endFrame() override;

    [[nodiscard]] std::size_t liveBufferCount() const noexcept { return buffers_.size(); }
    [[nodiscard]] std::size_t liveTextureCount() const noexcept { return textures_.size(); }
    [[nodiscard]] std::size_t liveShaderCount() const noexcept { return shaders_.size(); }
    [[nodiscard]] std::size_t submittedDrawCount() const noexcept { return submittedDraws_; }

private:
    std::uint32_t nextBufferId_{0};
    std::uint32_t nextTextureId_{0};
    std::uint32_t nextShaderId_{0};
    std::unordered_map<std::uint32_t, std::uint64_t> buffers_;
    std::unordered_set<std::uint32_t> textures_;
    std::unordered_set<std::uint32_t> shaders_;
    std::size_t submittedDraws_{0};
    bool frameActive_{false};
};

} // namespace storm::render
