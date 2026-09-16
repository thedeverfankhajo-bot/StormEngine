#include "storm/render/GlesRenderDevice.hpp"

#if defined(__ANDROID__)

#include <GLES3/gl3.h>

namespace storm::render {
namespace {
GLenum toUsage(BufferUsage usage) noexcept {
    switch (usage) {
    case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
    case BufferUsage::Stream: return GL_STREAM_DRAW;
    default: return GL_STATIC_DRAW;
    }
}

GLenum toTopology(PrimitiveTopology topology) noexcept {
    switch (topology) {
    case PrimitiveTopology::Points: return GL_POINTS;
    case PrimitiveTopology::Lines: return GL_LINES;
    case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
    default: return GL_TRIANGLES;
    }
}

GLenum toIndexType(IndexType type) noexcept {
    return type == IndexType::UInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}
} // namespace

GlesRenderDevice::~GlesRenderDevice() {
    for (const auto id : buffers_) {
        const GLuint glId = static_cast<GLuint>(id);
        glDeleteBuffers(1, &glId);
    }
    for (const auto id : textures_) {
        const GLuint glId = static_cast<GLuint>(id);
        glDeleteTextures(1, &glId);
    }
}

std::uint32_t GlesRenderDevice::allocateHandle(std::uint32_t& next) {
    const auto id = next++;
    return id == BufferHandle::invalidId ? 0u : id;
}

BufferHandle GlesRenderDevice::createBuffer(const BufferDesc& desc) {
    if (desc.size == 0 || desc.size > static_cast<std::uint64_t>(static_cast<std::size_t>(-1)))
        return {};

    GLuint glId = 0;
    glGenBuffers(1, &glId);
    if (glId == 0)
        return {};

    glBindBuffer(GL_ARRAY_BUFFER, glId);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(desc.size), nullptr, toUsage(desc.usage));
    if (glGetError() != GL_NO_ERROR) {
        glDeleteBuffers(1, &glId);
        return {};
    }

    buffers_.insert(glId);
    return BufferHandle(glId);
}

void GlesRenderDevice::destroyBuffer(BufferHandle handle) {
    if (!handle.valid())
        return;

    const GLuint glId = static_cast<GLuint>(handle.id());
    if (buffers_.erase(handle.id()) != 0)
        glDeleteBuffers(1, &glId);
}

TextureHandle GlesRenderDevice::createTexture(const TextureDesc& desc) {
    if (desc.width == 0 || desc.height == 0 || desc.mipLevels == 0)
        return {};

    GLuint glId = 0;
    glGenTextures(1, &glId);
    if (glId == 0)
        return {};

    glBindTexture(GL_TEXTURE_2D, glId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    desc.mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(desc.width),
                 static_cast<GLsizei>(desc.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    if (desc.mipLevels > 1)
        glGenerateMipmap(GL_TEXTURE_2D);

    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &glId);
        return {};
    }

    textures_.insert(glId);
    return TextureHandle(glId);
}

void GlesRenderDevice::destroyTexture(TextureHandle handle) {
    if (!handle.valid())
        return;

    const GLuint glId = static_cast<GLuint>(handle.id());
    if (textures_.erase(handle.id()) != 0)
        glDeleteTextures(1, &glId);
}

void GlesRenderDevice::beginFrame() {
    frameActive_ = true;
    submittedDraws_ = 0;
}

bool GlesRenderDevice::submit(const DrawCommand& command) {
    if (!frameActive_ || !command.vertexBuffer.valid() || command.vertexCount == 0)
        return false;
    if (buffers_.find(command.vertexBuffer.id()) == buffers_.end())
        return false;

    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(command.vertexBuffer.id()));

    if (command.indexed()) {
        if (!command.indexBuffer.valid() || buffers_.find(command.indexBuffer.id()) == buffers_.end())
            return false;

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(command.indexBuffer.id()));
        const std::size_t indexSize = command.indexType == IndexType::UInt16
                                          ? sizeof(std::uint16_t)
                                          : sizeof(std::uint32_t);
        const void* offset = reinterpret_cast<const void*>(
            static_cast<std::uintptr_t>(command.firstIndex) * indexSize);
        glDrawElements(toTopology(command.topology), static_cast<GLsizei>(command.indexCount),
                       toIndexType(command.indexType), offset);
    } else {
        glDrawArrays(toTopology(command.topology), static_cast<GLint>(command.firstVertex),
                     static_cast<GLsizei>(command.vertexCount));
    }

    if (glGetError() != GL_NO_ERROR)
        return false;

    ++submittedDraws_;
    return true;
}

void GlesRenderDevice::endFrame() {
    frameActive_ = false;
}

std::size_t GlesRenderDevice::liveBufferCount() const noexcept {
    return buffers_.size();
}

std::size_t GlesRenderDevice::liveTextureCount() const noexcept {
    return textures_.size();
}

} // namespace storm::render

#else

namespace storm::render {
GlesRenderDevice::~GlesRenderDevice() = default;
BufferHandle GlesRenderDevice::createBuffer(const BufferDesc&) { return {}; }
void GlesRenderDevice::destroyBuffer(BufferHandle) {}
TextureHandle GlesRenderDevice::createTexture(const TextureDesc&) { return {}; }
void GlesRenderDevice::destroyTexture(TextureHandle) {}
void GlesRenderDevice::beginFrame() { frameActive_ = true; submittedDraws_ = 0; }
bool GlesRenderDevice::submit(const DrawCommand&) { return false; }
void GlesRenderDevice::endFrame() { frameActive_ = false; }
std::size_t GlesRenderDevice::liveBufferCount() const noexcept { return 0; }
std::size_t GlesRenderDevice::liveTextureCount() const noexcept { return 0; }
std::uint32_t GlesRenderDevice::allocateHandle(std::uint32_t& next) { return next++; }
} // namespace storm::render

#endif
