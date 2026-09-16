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

GLint componentCount(VertexFormat format) noexcept {
    switch (format) {
    case VertexFormat::Float32: return 1;
    case VertexFormat::Float32x2: return 2;
    case VertexFormat::Float32x3: return 3;
    case VertexFormat::Float32x4: return 4;
    }
    return 0;
}

GLuint compileShader(GLenum type, const char* source) noexcept {
    const GLuint shader = glCreateShader(type);
    if (shader == 0)
        return 0;
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint createTriangleProgram() noexcept {
    constexpr const char* vertexSource = R"glsl(
#version 300 es
layout(location = 0) in vec3 aPosition;
void main() {
    gl_Position = vec4(aPosition, 1.0);
}
)glsl";
    constexpr const char* fragmentSource = R"glsl(
#version 300 es
precision mediump float;
out vec4 outColor;
void main() {
    outColor = vec4(0.95, 0.25, 0.08, 1.0);
}
)glsl";

    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertex == 0 || fragment == 0) {
        if (vertex != 0) glDeleteShader(vertex);
        if (fragment != 0) glDeleteShader(fragment);
        return 0;
    }

    const GLuint program = glCreateProgram();
    if (program == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        glDeleteProgram(program);
        return 0;
    }
    return program;
}
} // namespace

GlesRenderDevice::~GlesRenderDevice() {
    if (vao_ != 0)
        glDeleteVertexArrays(1, reinterpret_cast<const GLuint*>(&vao_));
    if (program_ != 0)
        glDeleteProgram(static_cast<GLuint>(program_));
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

bool GlesRenderDevice::updateBuffer(BufferHandle handle, const void* data, std::size_t size,
                                    std::size_t offset) {
    if (!handle.valid() || data == nullptr || size == 0)
        return false;
    if (buffers_.find(handle.id()) == buffers_.end())
        return false;

    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(handle.id()));
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    return glGetError() == GL_NO_ERROR;
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
    if (!frameActive_ || !command.vertexBuffer.valid() || command.vertexCount == 0 ||
        !command.vertexLayout.valid())
        return false;
    if (buffers_.find(command.vertexBuffer.id()) == buffers_.end())
        return false;
    if (command.baseVertex != 0)
        return false;

    if (!pipelineReady_) {
        program_ = createTriangleProgram();
        if (program_ == 0)
            return false;

        glGenVertexArrays(1, reinterpret_cast<GLuint*>(&vao_));
        if (vao_ == 0) {
            glDeleteProgram(static_cast<GLuint>(program_));
            program_ = 0;
            return false;
        }
        pipelineReady_ = true;
    }

    glUseProgram(static_cast<GLuint>(program_));
    glBindVertexArray(static_cast<GLuint>(vao_));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(command.vertexBuffer.id()));

    for (std::uint32_t i = 0; i < command.vertexLayout.attributeCount; ++i) {
        const auto& attribute = command.vertexLayout.attributes[i];
        const GLint components = componentCount(attribute.format);
        if (components == 0)
            return false;
        glEnableVertexAttribArray(attribute.location);
        glVertexAttribPointer(attribute.location, components, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(command.vertexLayout.stride),
                              reinterpret_cast<const void*>(static_cast<std::uintptr_t>(attribute.offset)));
    }

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
bool GlesRenderDevice::updateBuffer(BufferHandle, const void*, std::size_t, std::size_t) { return false; }
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
