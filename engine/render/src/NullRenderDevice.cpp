#include "storm/render/NullRenderDevice.hpp"

namespace storm::render {

BufferHandle NullRenderDevice::createBuffer(const BufferDesc& desc) {
    if (desc.size == 0) return {};
    const auto id = nextBufferId_++;
    if (id == BufferHandle::invalidId) return {};
    buffers_.insert(id);
    return BufferHandle(id);
}

void NullRenderDevice::destroyBuffer(BufferHandle handle) { if (handle.valid()) buffers_.erase(handle.id()); }

bool NullRenderDevice::updateBuffer(BufferHandle handle, const void* data, std::size_t size, std::size_t) {
    return handle.valid() && data != nullptr && size != 0 && buffers_.find(handle.id()) != buffers_.end();
}

TextureHandle NullRenderDevice::createTexture(const TextureDesc& desc) {
    if (desc.width == 0 || desc.height == 0 || desc.mipLevels == 0) return {};
    const auto id = nextTextureId_++;
    if (id == TextureHandle::invalidId) return {};
    textures_.insert(id);
    return TextureHandle(id);
}

void NullRenderDevice::destroyTexture(TextureHandle handle) { if (handle.valid()) textures_.erase(handle.id()); }

ShaderHandle NullRenderDevice::createShader(const ShaderDesc&, const std::string& source) {
    if (source.empty()) return {};
    const auto id = nextShaderId_++;
    if (id == ShaderHandle::invalidId) return {};
    shaders_.insert(id);
    return ShaderHandle(id);
}

void NullRenderDevice::destroyShader(ShaderHandle handle) { if (handle.valid()) shaders_.erase(handle.id()); }

void NullRenderDevice::beginFrame() { frameActive_ = true; submittedDraws_ = 0; }

bool NullRenderDevice::submit(const DrawCommand& command) {
    if (!frameActive_ || !command.vertexBuffer.valid() || command.vertexCount == 0) return false;
    if (buffers_.find(command.vertexBuffer.id()) == buffers_.end()) return false;
    if (command.indexCount > 0) {
        if (!command.indexBuffer.valid() || buffers_.find(command.indexBuffer.id()) == buffers_.end()) return false;
    } else if (command.indexBuffer.valid()) return false;
    if (!command.shader.valid() || !command.fragmentShader.valid()) return false;
    if (shaders_.find(command.shader.id()) == shaders_.end() ||
        shaders_.find(command.fragmentShader.id()) == shaders_.end()) return false;
    ++submittedDraws_;
    return true;
}

void NullRenderDevice::endFrame() { frameActive_ = false; }

} // namespace storm::render
