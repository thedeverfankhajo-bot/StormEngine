#pragma once

#include "RenderDevice.hpp"
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

    void beginFrame() override;
    bool submit(const DrawCommand& command) override;
    void endFrame() override;

    [[nodiscard]] std::size_t liveBufferCount() const noexcept override { return buffers_.size(); }
    [[nodiscard]] std::size_t liveTextureCount() const noexcept override { return textures_.size(); }
    [[nodiscard]] std::size_t submittedDrawCount() const noexcept override { return submittedDraws_; }

private:
    std::uint32_t nextBufferId_{0};
    std::uint32_t nextTextureId_{0};
    std::unordered_set<std::uint32_t> buffers_;
    std::unordered_set<std::uint32_t> textures_;
    std::size_t submittedDraws_{0};
    bool frameActive_{false};
};

} // namespace storm::render
