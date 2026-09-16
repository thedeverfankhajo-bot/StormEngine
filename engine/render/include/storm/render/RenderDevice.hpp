#pragma once

#include "RenderTypes.hpp"
#include <cstddef>
#include <cstdint>

namespace storm::render {

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;

    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual void destroyTexture(TextureHandle handle) = 0;

    virtual void beginFrame() = 0;
    virtual bool submit(const DrawCommand& command) = 0;
    virtual void endFrame() = 0;

    [[nodiscard]] virtual std::size_t liveBufferCount() const noexcept = 0;
    [[nodiscard]] virtual std::size_t liveTextureCount() const noexcept = 0;
    [[nodiscard]] virtual std::size_t submittedDrawCount() const noexcept = 0;
};

} // namespace storm::render
