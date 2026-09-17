#pragma once

#include "RenderTypes.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

namespace storm::render {

class RenderDevice {
public:
    virtual ~RenderDevice() = default;
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;
    virtual bool updateBuffer(BufferHandle handle, const void* data, std::size_t size, std::size_t offset = 0) = 0;
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    // Backends may override this when CPU pixel upload is supported. Keeping a
    // default failure preserves source compatibility for experimental backends.
    virtual bool updateTexture(TextureHandle, const void*, std::size_t, std::uint32_t = 0) { return false; }
    virtual void destroyTexture(TextureHandle handle) = 0;
    virtual ShaderHandle createShader(const ShaderDesc& desc, const std::string& source) = 0;
    virtual void destroyShader(ShaderHandle handle) = 0;
    virtual void beginFrame() = 0;
    virtual bool submit(const DrawCommand& command) = 0;
    virtual void endFrame() = 0;
    [[nodiscard]] virtual std::size_t liveBufferCount() const noexcept = 0;
    [[nodiscard]] virtual std::size_t liveTextureCount() const noexcept = 0;
    [[nodiscard]] virtual std::size_t liveShaderCount() const noexcept = 0;
    [[nodiscard]] virtual std::size_t submittedDrawCount() const noexcept = 0;
};

} // namespace storm::render
