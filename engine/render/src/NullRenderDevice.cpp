#include "storm/render/NullRenderDevice.hpp"
#include <algorithm>
#include <limits>

namespace storm::render {
namespace {
std::uint32_t maxMipLevels(std::uint32_t width, std::uint32_t height) noexcept {
    std::uint32_t levels = 1;
    while (width > 1 || height > 1) {
        width = std::max(1u, width / 2u);
        height = std::max(1u, height / 2u);
        ++levels;
    }
    return levels;
}
}

BufferHandle NullRenderDevice::createBuffer(const BufferDesc& desc) {
    if (desc.size == 0) return {};
    const auto handle = bufferHandles_.allocate();
    if (!handle.valid()) return {};
    buffers_.emplace(handle.id(), desc.size);
    return handle;
}
void NullRenderDevice::destroyBuffer(BufferHandle handle) {
    if (!bufferHandles_.valid(handle)) return;
    const auto it = buffers_.find(handle.id());
    if (it == buffers_.end()) return;
    buffers_.erase(it);
    bufferHandles_.release(handle);
}
bool NullRenderDevice::updateBuffer(BufferHandle handle, const void* data, std::size_t size, std::size_t offset) {
    if (!bufferHandles_.valid(handle) || data == nullptr || size == 0) return false;
    const auto it = buffers_.find(handle.id()); if (it == buffers_.end()) return false;
    const auto updateSize = static_cast<std::uint64_t>(size), updateOffset = static_cast<std::uint64_t>(offset);
    return updateOffset <= it->second && updateSize <= it->second - updateOffset;
}
TextureHandle NullRenderDevice::createTexture(const TextureDesc& desc) {
    if (desc.width == 0 || desc.height == 0 || desc.mipLevels == 0 || desc.format != TextureFormat::RGBA8 ||
        desc.mipLevels > maxMipLevels(desc.width, desc.height)) return {};
    const auto handle = textureHandles_.allocate(); if (!handle.valid()) return {};
    textures_.emplace(handle.id(), desc); return handle;
}
bool NullRenderDevice::updateTexture(TextureHandle handle, const void* pixels, std::size_t size, std::uint32_t mipLevel) {
    if (!textureHandles_.valid(handle) || pixels == nullptr || size == 0) return false;
    const auto it = textures_.find(handle.id()); if (it == textures_.end() || it->second.format != TextureFormat::RGBA8 || mipLevel >= it->second.mipLevels) return false;
    std::uint64_t width = it->second.width, height = it->second.height;
    for (std::uint32_t level = 0; level < mipLevel; ++level) { width = std::max<std::uint64_t>(1, width / 2); height = std::max<std::uint64_t>(1, height / 2); }
    if (width > std::numeric_limits<std::uint64_t>::max() / height) return false;
    const std::uint64_t pixelsCount = width * height;
    if (pixelsCount > std::numeric_limits<std::uint64_t>::max() / 4u) return false;
    const std::uint64_t expected = pixelsCount * 4u;
    return static_cast<std::uint64_t>(size) == expected;
}
void NullRenderDevice::destroyTexture(TextureHandle handle) {
    if (!textureHandles_.valid(handle)) return;
    const auto it = textures_.find(handle.id());
    if (it == textures_.end()) return;
    textures_.erase(it);
    textureHandles_.release(handle);
}
ShaderHandle NullRenderDevice::createShader(const ShaderDesc& desc, const std::string& source) {
    if (source.empty()) return {};
    const auto handle = shaderHandles_.allocate(); if (!handle.valid()) return {};
    shaders_.emplace(handle.id(), desc.stage); return handle;
}
void NullRenderDevice::destroyShader(ShaderHandle handle) {
    if (!shaderHandles_.valid(handle)) return;
    const auto it = shaders_.find(handle.id());
    if (it == shaders_.end()) return;
    shaders_.erase(it);
    shaderHandles_.release(handle);
}
void NullRenderDevice::beginFrame() { frameActive_ = true; submittedDraws_ = 0; }
bool NullRenderDevice::submit(const DrawCommand& command) {
    if (!frameActive_ || !command.vertexBuffer.valid() || command.vertexCount == 0 || !command.vertexLayout.valid()) return false;
    if (!bufferHandles_.valid(command.vertexBuffer)) return false;
    const auto vertexIt = buffers_.find(command.vertexBuffer.id()); if (vertexIt == buffers_.end()) return false;
    const std::uint64_t vertexEnd = static_cast<std::uint64_t>(command.firstVertex) + command.vertexCount;
    if (vertexEnd < command.firstVertex || vertexEnd > std::numeric_limits<std::uint64_t>::max() / command.vertexLayout.stride) return false;
    if (vertexEnd * command.vertexLayout.stride > vertexIt->second) return false;
    if (command.indexed()) {
        if (!command.indexBuffer.valid()) return false;
        if (!bufferHandles_.valid(command.indexBuffer)) return false;
        const auto indexIt = buffers_.find(command.indexBuffer.id()); if (indexIt == buffers_.end()) return false;
        const std::uint64_t indexSize = command.indexType == IndexType::UInt16 ? 2u : 4u;
        if (command.firstIndex > std::numeric_limits<std::uint64_t>::max() / indexSize ||
            command.indexCount > std::numeric_limits<std::uint64_t>::max() / indexSize) return false;
        const std::uint64_t indexOffset = static_cast<std::uint64_t>(command.firstIndex) * indexSize;
        const std::uint64_t indexBytes = static_cast<std::uint64_t>(command.indexCount) * indexSize;
        if (indexOffset > indexIt->second || indexBytes > indexIt->second - indexOffset) return false;
    } else if (command.indexBuffer.valid()) return false;
    if (!command.shader.valid() || !command.fragmentShader.valid()) return false;
    if (command.viewportWidth > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ||
        command.viewportHeight > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) return false;
    if (!shaderHandles_.valid(command.shader) || !shaderHandles_.valid(command.fragmentShader)) return false;
    const auto vertexShader = shaders_.find(command.shader.id());
    const auto fragmentShader = shaders_.find(command.fragmentShader.id());
    if (vertexShader == shaders_.end() || fragmentShader == shaders_.end() ||
        vertexShader->second != ShaderStage::Vertex || fragmentShader->second != ShaderStage::Fragment) return false;
    ++submittedDraws_; return true;
}
void NullRenderDevice::endFrame() { frameActive_ = false; }

} // namespace storm::render
