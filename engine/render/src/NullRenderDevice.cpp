#include "storm/render/NullRenderDevice.hpp"

namespace storm::render {

BufferHandle NullRenderDevice::createBuffer(const BufferDesc& desc) {
    if (desc.size == 0)
        return {};

    const auto id = nextBufferId_++;
    if (id == BufferHandle::invalidId)
        return {};

    buffers_.insert(id);
    return BufferHandle(id);
}

void NullRenderDevice::destroyBuffer(BufferHandle handle) {
    if (!handle.valid())
        return;
    buffers_.erase(handle.id());
}

TextureHandle NullRenderDevice::createTexture(const TextureDesc& desc) {
    if (desc.width == 0 || desc.height == 0 || desc.mipLevels == 0)
        return {};

    const auto id = nextTextureId_++;
    if (id == TextureHandle::invalidId)
        return {};

    textures_.insert(id);
    return TextureHandle(id);
}

void NullRenderDevice::destroyTexture(TextureHandle handle) {
    if (!handle.valid())
        return;
    textures_.erase(handle.id());
}

void NullRenderDevice::beginFrame() {
    frameActive_ = true;
    submittedDraws_ = 0;
}

bool NullRenderDevice::submit(const DrawCommand& command) {
    if (!frameActive_ || !command.vertexBuffer.valid() || command.vertexCount == 0)
        return false;
    if (buffers_.find(command.vertexBuffer.id()) == buffers_.end())
        return false;

    if (command.indexCount > 0) {
        if (!command.indexBuffer.valid())
            return false;
        if (buffers_.find(command.indexBuffer.id()) == buffers_.end())
            return false;
    } else if (command.indexBuffer.valid()) {
        return false;
    }

    ++submittedDraws_;
    return true;
}

void NullRenderDevice::endFrame() {
    frameActive_ = false;
}

} // namespace storm::render
