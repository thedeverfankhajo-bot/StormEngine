#include "storm/render/NullRenderDevice.hpp"
#include <limits>

namespace storm::render {

BufferHandle NullRenderDevice::createBuffer(const BufferDesc& desc) {
    if (desc.size == 0) return {};
    const auto id = ++nextBufferId_;
    if (id == BufferHandle::invalidId) return {};
    buffers_.emplace(id, desc.size);
    return BufferHandle(id);
}
void NullRenderDevice::destroyBuffer(BufferHandle handle) { if (handle.valid()) buffers_.erase(handle.id()); }
bool NullRenderDevice::updateBuffer(BufferHandle handle, const void* data, std::size_t size, std::size_t offset) {
    if (!handle.valid() || data == nullptr || size == 0) return false;
    const auto it = buffers_.find(handle.id()); if (it == buffers_.end()) return false;
    const auto updateSize = static_cast<std::uint64_t>(size), updateOffset = static_cast<std::uint64_t>(offset);
    return updateOffset <= it->second && updateSize <= it->second - updateOffset;
}
TextureHandle NullRenderDevice::createTexture(const TextureDesc& desc) {
    if (desc.width == 0 || desc.height == 0 || desc.mipLevels == 0) return {};
    const auto id = ++nextTextureId_; if (id == TextureHandle::invalidId) return {};
    textures_.emplace(id, desc); return TextureHandle(id);
}
bool NullRenderDevice::updateTexture(TextureHandle handle, const void* pixels, std::size_t size, std::uint32_t mipLevel) {
    if (!handle.valid() || pixels == nullptr || size == 0) return false;
    const auto it = textures_.find(handle.id()); if (it == textures_.end() || mipLevel >= it->second.mipLevels) return false;
    std::uint64_t width = it->second.width, height = it->second.height;
    for (std::uint32_t level = 0; level < mipLevel; ++level) { width = std::max<std::uint64_t>(1, width / 2); height = std::max<std::uint64_t>(1, height / 2); }
    const std::uint64_t expected = width * height * 4u;
    return static_cast<std::uint64_t>(size) == expected;
}
void NullRenderDevice::destroyTexture(TextureHandle handle) { if (handle.valid()) textures_.erase(handle.id()); }
ShaderHandle NullRenderDevice::createShader(const ShaderDesc&, const std::string& source) {
    if (source.empty()) return {};
    const auto id = ++nextShaderId_; if (id == ShaderHandle::invalidId) return {};
    shaders_.insert(id); return ShaderHandle(id);
}
void NullRenderDevice::destroyShader(ShaderHandle handle) { if (handle.valid()) shaders_.erase(handle.id()); }
void NullRenderDevice::beginFrame() { frameActive_ = true; submittedDraws_ = 0; }
bool NullRenderDevice::submit(const DrawCommand& command) {
    if (!frameActive_ || !command.vertexBuffer.valid() || command.vertexCount == 0 || !command.vertexLayout.valid()) return false;
    const auto vertexIt = buffers_.find(command.vertexBuffer.id()); if (vertexIt == buffers_.end()) return false;
    const std::uint64_t vertexEnd = static_cast<std::uint64_t>(command.firstVertex) + command.vertexCount;
    if (vertexEnd < command.firstVertex || vertexEnd > std::numeric_limits<std::uint64_t>::max() / command.vertexLayout.stride) return false;
    if (vertexEnd * command.vertexLayout.stride > vertexIt->second) return false;
    if (command.indexed()) {
        if (!command.indexBuffer.valid()) return false;
        const auto indexIt = buffers_.find(command.indexBuffer.id()); if (indexIt == buffers_.end()) return false;
        const std::uint64_t indexSize = command.indexType == IndexType::UInt16 ? 2u : 4u;
        const std::uint64_t indexOffset = static_cast<std::uint64_t>(command.firstIndex) * indexSize;
        const std::uint64_t indexBytes = static_cast<std::uint64_t>(command.indexCount) * indexSize;
        if (indexOffset > indexIt->second || indexBytes > indexIt->second - indexOffset) return false;
    } else if (command.indexBuffer.valid()) return false;
    if (!command.shader.valid() || !command.fragmentShader.valid()) return false;
    if (shaders_.find(command.shader.id()) == shaders_.end() || shaders_.find(command.fragmentShader.id()) == shaders_.end()) return false;
    ++submittedDraws_; return true;
}
void NullRenderDevice::endFrame() { frameActive_ = false; }

} // namespace storm::render
