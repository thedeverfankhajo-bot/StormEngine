#pragma once

#include "RenderDevice.hpp"
#include <cstddef>
#include <cstdint>

#if defined(__ANDROID__)
#include <unordered_set>
#endif

namespace storm::render {

// OpenGL ES 3.x backend. The caller must make a valid GLES context current
// before using the device. The backend owns GL buffer/texture lifetime.
class GlesRenderDevice final : public RenderDevice {
public:
    GlesRenderDevice() = default;
    ~GlesRenderDevice() override;

    GlesRenderDevice(const GlesRenderDevice&) = delete;
    GlesRenderDevice& operator=(const GlesRenderDevice&) = delete;

    BufferHandle createBuffer(const BufferDesc& desc) override;
    void destroyBuffer(BufferHandle handle) override;
    bool updateBuffer(BufferHandle handle, const void* data, std::size_t size,
                      std::size_t offset = 0) override;

    TextureHandle createTexture(const TextureDesc& desc) override;
    void destroyTexture(TextureHandle handle) override;

    void beginFrame() override;
    bool submit(const DrawCommand& command) override;
    void endFrame() override;

    [[nodiscard]] std::size_t liveBufferCount() const noexcept override;
    [[nodiscard]] std::size_t liveTextureCount() const noexcept override;
    [[nodiscard]] std::size_t submittedDrawCount() const noexcept override { return submittedDraws_; }

private:
    static std::uint32_t allocateHandle(std::uint32_t& next);

    std::uint32_t nextBufferId_{1};
    std::uint32_t nextTextureId_{1};
    std::size_t submittedDraws_{0};
    bool frameActive_{false};

#if defined(__ANDROID__)
    std::unordered_set<std::uint32_t> buffers_;
    std::unordered_set<std::uint32_t> textures_;
    std::uint32_t program_{0};
    std::uint32_t vao_{0};
    bool pipelineReady_{false};
#endif
};

} // namespace storm::render
