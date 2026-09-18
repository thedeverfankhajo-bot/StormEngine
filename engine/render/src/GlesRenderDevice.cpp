#include "storm/render/GlesRenderDevice.hpp"

#if defined(__ANDROID__)
#include "storm/render/Material.hpp"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstring>

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
GLenum toIndexType(IndexType type) noexcept { return type == IndexType::UInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT; }
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
    if (shader == 0) return 0;
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1) {
            std::vector<char> log(static_cast<std::size_t>(logLength), '\0');
            GLsizei written = 0;
            glGetShaderInfoLog(shader, logLength, &written, log.data());
            __android_log_print(ANDROID_LOG_ERROR, "StormEngine", "GLSL compile failed: %.*s",
                                 static_cast<int>(written), log.data());
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
constexpr std::uint64_t maxGlSize = static_cast<std::uint64_t>(std::numeric_limits<GLsizeiptr>::max());
constexpr std::uint64_t maxGlCount = static_cast<std::uint64_t>(std::numeric_limits<GLsizei>::max());
constexpr std::uint64_t maxGlInt = static_cast<std::uint64_t>(std::numeric_limits<GLint>::max());

bool applyMaterial(const Material& material,
                   GLuint program,
                   std::unordered_map<std::uint64_t, std::unordered_map<std::string, std::int32_t>>& uniformLocations,
                   const std::unordered_map<std::uint32_t, GlesRenderDevice::TextureRecord>& textures,
                   const ResourceHandleAllocator<TextureHandle>& textureHandles,
                   std::int32_t maxTextureUnits) noexcept {
    std::uint32_t textureUnit = 0;
    const std::uint32_t maxUnits = maxTextureUnits > 0 ? static_cast<std::uint32_t>(maxTextureUnits) : 1u;
    for (const auto& [name, value] : material.parameters()) {
        auto& locations = uniformLocations[static_cast<std::uint64_t>(program)];
        const auto cached = locations.find(name);
        const GLint location = cached != locations.end()
            ? static_cast<GLint>(cached->second)
            : (locations.emplace(name, glGetUniformLocation(program, name.c_str())).first->second);
        if (location < 0) continue;

        bool supported = true;
        if (const auto* scalar = std::get_if<MaterialScalar>(&value)) {
            glUniform1f(location, *scalar);
        } else if (const auto* vec2 = std::get_if<MaterialVec2>(&value)) {
            glUniform2fv(location, 1, vec2->data());
        } else if (const auto* vec3 = std::get_if<MaterialVec3>(&value)) {
            glUniform3fv(location, 1, vec3->data());
        } else if (const auto* vec4 = std::get_if<MaterialVec4>(&value)) {
            glUniform4fv(location, 1, vec4->data());
        } else if (const auto* mat4 = std::get_if<MaterialMat4>(&value)) {
            glUniformMatrix4fv(location, 1, GL_FALSE, mat4->data());
        } else if (const auto* texture = std::get_if<TextureHandle>(&value)) {
            if (!textureHandles.valid(*texture) || textures.find(texture->id()) == textures.end() || textureUnit >= maxUnits) {
                return false;
            }
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textures.at(texture->id()).glId));
            glUniform1i(location, static_cast<GLint>(textureUnit));
            ++textureUnit;
        } else {
            supported = false;
        }
        if (!supported || glGetError() != GL_NO_ERROR) return false;
    }
    return true;
}
} // namespace

GlesRenderDevice::~GlesRenderDevice() {
    if (!gpuResourcesValid_) return;
    for (const auto& [key, program] : programs_) { (void)key; if (program != 0) glDeleteProgram(static_cast<GLuint>(program)); }
    if (vao_ != 0) {
        const GLuint id = static_cast<GLuint>(vao_);
        glDeleteVertexArrays(1, &id);
    }
    for (const auto& [handle, shader] : shaders_) { (void)handle; if (shader.glId != 0) glDeleteShader(static_cast<GLuint>(shader.glId)); }
    for (const auto& [handle, record] : buffers_) { (void)handle; const GLuint id = static_cast<GLuint>(record.glId); if (id != 0) glDeleteBuffers(1, &id); }
    for (const auto& [handle, record] : textures_) { (void)handle; const GLuint id = static_cast<GLuint>(record.glId); if (id != 0) glDeleteTextures(1, &id); }
}

void GlesRenderDevice::onContextLost() noexcept {
    // Do not issue GL deletion calls here: this hook is also invoked after
    // EGL reports EGL_CONTEXT_LOST, where the old context is no longer safe
    // to use. The context teardown owns the old GL objects.
    for (auto& [key, program] : programs_) { (void)key; program = 0; }
    vao_ = 0;
    for (auto& [handle, shader] : shaders_) { (void)handle; shader.glId = 0; }
    for (auto& [handle, record] : buffers_) { (void)handle; record.glId = 0; }
    for (auto& [handle, record] : textures_) { (void)handle; record.glId = 0; }
    programs_.clear();
    uniformLocations_.clear();
    program_ = 0;
    maxTextureUnits_ = 0;
    stateCache_.invalidate();
    frameActive_ = false;
    gpuResourcesValid_ = false;
}
bool GlesRenderDevice::onContextRestored() noexcept {
    if (gpuResourcesValid_) return true;

    for (auto& [handle, record] : buffers_) {
        (void)handle;
        GLuint id = 0;
        glGenBuffers(1, &id);
        if (id == 0) { onContextLost(); return false; }
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(record.size),
                     record.cpuData.empty() ? nullptr : record.cpuData.data(), toUsage(record.usage));
        if (glGetError() != GL_NO_ERROR) { glDeleteBuffers(1, &id); onContextLost(); return false; }
        record.glId = id;
    }

    for (auto& [handle, record] : textures_) {
        (void)handle;
        GLuint id = 0;
        glGenTextures(1, &id);
        if (id == 0) { onContextLost(); return false; }
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, record.desc.mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        for (std::uint32_t level = 0; level < record.desc.mipLevels; ++level) {
            const std::uint32_t width = std::max(1u, record.desc.width >> std::min(level, 31u));
            const std::uint32_t height = std::max(1u, record.desc.height >> std::min(level, 31u));
            const std::size_t offset = mipByteOffset(record.desc, level);
            glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), GL_RGBA8,
                         static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                         GL_RGBA, GL_UNSIGNED_BYTE,
                         record.cpuData.empty() ? nullptr : record.cpuData.data() + offset);
        }
        if (record.desc.mipLevels > 1) glGenerateMipmap(GL_TEXTURE_2D);
        if (glGetError() != GL_NO_ERROR) { glDeleteTextures(1, &id); onContextLost(); return false; }
        record.glId = id;
    }

    for (auto& [handle, record] : shaders_) {
        (void)handle;
        const GLenum type = record.stage == ShaderStage::Vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
        const GLuint id = compileShader(type, record.source.c_str());
        if (id == 0) { onContextLost(); return false; }
        record.glId = id;
    }

    program_ = 0;
    vao_ = 0;
    maxTextureUnits_ = 0;
    uniformLocations_.clear();
    stateCache_.invalidate();
    frameActive_ = false;
    gpuResourcesValid_ = true;
    return true;
}

std::uint32_t maxMipLevels(std::uint32_t width, std::uint32_t height) noexcept {
    std::uint32_t levels = 1;
    while (width > 1 || height > 1) {
        width = std::max(1u, width / 2u);
        height = std::max(1u, height / 2u);
        ++levels;
    }
    return levels;
}

std::size_t mipByteOffset(const TextureDesc& desc, std::uint32_t mipLevel) noexcept {
    std::size_t offset = 0;
    for (std::uint32_t level = 0; level < mipLevel; ++level) {
        const std::uint32_t width = std::max(1u, desc.width >> std::min(level, 31u));
        const std::uint32_t height = std::max(1u, desc.height >> std::min(level, 31u));
        const std::uint64_t bytes = static_cast<std::uint64_t>(width) * height * 4u;
        if (bytes > std::numeric_limits<std::size_t>::max() - offset) return 0;
        offset += static_cast<std::size_t>(bytes);
    }
    return offset;
}

std::size_t textureByteSize(const TextureDesc& desc) noexcept {
    const std::size_t offset = mipByteOffset(desc, desc.mipLevels);
    return offset;
}

BufferHandle GlesRenderDevice::createBuffer(const BufferDesc& desc) {
    if (desc.size == 0 || desc.size > maxGlSize) return {};
    GLuint glId = 0;
    glGenBuffers(1, &glId);
    if (glId == 0) return {};
    glBindBuffer(GL_ARRAY_BUFFER, glId);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(desc.size), nullptr, toUsage(desc.usage));
    if (glGetError() != GL_NO_ERROR) { glDeleteBuffers(1, &glId); return {}; }
    const auto handle = bufferHandles_.allocate();
    if (!handle.valid()) { glDeleteBuffers(1, &glId); return {}; }
    BufferRecord record{};
    record.size = desc.size;
    record.glId = glId;
    record.usage = desc.usage;
    record.cpuData.resize(desc.size);
    buffers_.emplace(handle.id(), std::move(record));
    return handle;
}
void GlesRenderDevice::destroyBuffer(BufferHandle handle) {
    if (!bufferHandles_.valid(handle)) return;
    const auto it = buffers_.find(handle.id());
    if (it == buffers_.end()) return;
    const GLuint id = static_cast<GLuint>(it->second.glId);
    buffers_.erase(it);
    bufferHandles_.release(handle);
    if (id != 0) glDeleteBuffers(1, &id);
}
bool GlesRenderDevice::updateBuffer(BufferHandle handle, const void* data, std::size_t size, std::size_t offset) {
    if (!bufferHandles_.valid(handle) || data == nullptr || size == 0) return false;
    const auto it = buffers_.find(handle.id());
    if (it == buffers_.end() || offset > it->second.size || size > it->second.size - offset ||
        size > maxGlSize || offset > maxGlSize) return false;
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(it->second.glId));
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    if (glGetError() != GL_NO_ERROR) return false;
    std::memcpy(it->second.cpuData.data() + offset, data, size);
    return true;
}
TextureHandle GlesRenderDevice::createTexture(const TextureDesc& desc) {
    if (desc.width == 0 || desc.height == 0 || desc.mipLevels == 0 ||
        desc.mipLevels > maxMipLevels(desc.width, desc.height) || desc.format != TextureFormat::RGBA8 ||
        desc.width > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max()) ||
        desc.height > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max())) return {};
    GLuint glId = 0;
    glGenTextures(1, &glId);
    if (glId == 0) return {};
    glBindTexture(GL_TEXTURE_2D, glId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc.mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(desc.width), static_cast<GLsizei>(desc.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    if (desc.mipLevels > 1) glGenerateMipmap(GL_TEXTURE_2D);
    if (glGetError() != GL_NO_ERROR) { glDeleteTextures(1, &glId); return {}; }
    const std::size_t cpuBytes = textureByteSize(desc);
    if (cpuBytes == 0) { glDeleteTextures(1, &glId); return {}; }
    const auto handle = textureHandles_.allocate();
    if (!handle.valid()) { glDeleteTextures(1, &glId); return {}; }
    TextureRecord record{};
    record.desc = desc;
    record.glId = glId;
    record.cpuData.resize(cpuBytes);
    textures_.emplace(handle.id(), std::move(record));
    return handle;
}
bool GlesRenderDevice::updateTexture(TextureHandle handle, const void* data, std::size_t size, std::uint32_t mipLevel) {
    if (!textureHandles_.valid(handle) || data == nullptr) return false;
    const auto it = textures_.find(handle.id());
    if (it == textures_.end() || it->second.desc.format != TextureFormat::RGBA8 || mipLevel >= it->second.desc.mipLevels) return false;
    const std::uint32_t width = std::max(1u, it->second.desc.width >> std::min(mipLevel, 31u));
    const std::uint32_t height = std::max(1u, it->second.desc.height >> std::min(mipLevel, 31u));
    const std::uint64_t expected = static_cast<std::uint64_t>(width) * height * 4u;
    if (expected > std::numeric_limits<std::size_t>::max() || size != static_cast<std::size_t>(expected)) return false;
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(it->second.glId));
    glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(mipLevel), 0, 0,
                    static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, data);
    if (glGetError() != GL_NO_ERROR) return false;
    const std::size_t offset = mipByteOffset(it->second.desc, mipLevel);
    std::memcpy(it->second.cpuData.data() + offset, data, size);
    return true;
}
void GlesRenderDevice::destroyTexture(TextureHandle handle) {
    if (!textureHandles_.valid(handle)) return;
    const auto it = textures_.find(handle.id());
    if (it == textures_.end()) return;
    const GLuint id = static_cast<GLuint>(it->second.glId);
    textures_.erase(it);
    textureHandles_.release(handle);
    if (id != 0) glDeleteTextures(1, &id);
}
ShaderHandle GlesRenderDevice::createShader(const ShaderDesc& desc, const std::string& source) {
    if (source.empty()) return {};
    const GLenum type = desc.stage == ShaderStage::Vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    const GLuint shader = compileShader(type, source.c_str());
    if (shader == 0) return {};
    const auto handle = shaderHandles_.allocate();
    if (!handle.valid()) { glDeleteShader(shader); return {}; }
    shaders_.emplace(handle.id(), ShaderRecord{shader, desc.stage, source});
    return handle;
}
void GlesRenderDevice::destroyShader(ShaderHandle handle) {
    if (!shaderHandles_.valid(handle)) return;
    const auto it = shaders_.find(handle.id());
    if (it == shaders_.end()) return;
    for (auto programIt = programs_.begin(); programIt != programs_.end();) {
        const std::uint32_t vertex = static_cast<std::uint32_t>(programIt->first >> 32u);
        const std::uint32_t fragment = static_cast<std::uint32_t>(programIt->first & 0xffffffffu);
        if (vertex == handle.id() || fragment == handle.id()) {
            glDeleteProgram(static_cast<GLuint>(programIt->second));
            if (program_ == programIt->second) program_ = 0;
            uniformLocations_.erase(static_cast<std::uint64_t>(programIt->second));
            programIt = programs_.erase(programIt);
        } else ++programIt;
    }
    glDeleteShader(static_cast<GLuint>(it->second.glId));
    shaders_.erase(it);
    shaderHandles_.release(handle);
}

void GlesRenderDevice::beginFrame() { frameActive_ = true; submittedDraws_ = 0; }

bool GlesRenderDevice::submit(const DrawCommand& command) {
    if (!gpuResourcesValid_ || !frameActive_ || !bufferHandles_.valid(command.vertexBuffer) || command.vertexCount == 0 || !command.vertexLayout.valid()) return false;
    if (!shaderHandles_.valid(command.shader) || !shaderHandles_.valid(command.fragmentShader)) return false;
    const auto vertexBufferIt = buffers_.find(command.vertexBuffer.id());
    if (vertexBufferIt == buffers_.end() || command.baseVertex != 0) return false;
    if (!command.shader.valid() || !command.fragmentShader.valid()) return false;
    const auto vertexIt = shaders_.find(command.shader.id());
    const auto fragmentIt = shaders_.find(command.fragmentShader.id());
    if (vertexIt == shaders_.end() || fragmentIt == shaders_.end() ||
        vertexIt->second.stage != ShaderStage::Vertex || fragmentIt->second.stage != ShaderStage::Fragment) return false;

    const std::uint64_t vertexEnd = static_cast<std::uint64_t>(command.firstVertex) + command.vertexCount;
    if (vertexEnd < command.firstVertex || vertexEnd > maxGlCount ||
        vertexEnd > std::numeric_limits<std::uint64_t>::max() / command.vertexLayout.stride) return false;
    const std::uint64_t vertexBytes = vertexEnd * command.vertexLayout.stride;
    if (vertexBytes > vertexBufferIt->second.size) return false;

    for (std::uint32_t i = 0; i < command.vertexLayout.attributeCount; ++i) {
        const auto& attribute = command.vertexLayout.attributes[i];
        const GLint components = componentCount(attribute.format);
        if (components == 0) return false;
        const std::uint64_t attributeBytes = static_cast<std::uint64_t>(components) * sizeof(float);
        const std::uint64_t attributeEnd = static_cast<std::uint64_t>(attribute.offset) + attributeBytes;
        if (attributeEnd < attribute.offset || attributeEnd > command.vertexLayout.stride) return false;
    }

    const std::uint64_t programKey = (static_cast<std::uint64_t>(command.shader.id()) << 32u) | command.fragmentShader.id();
    auto programIt = programs_.find(programKey);
    if (programIt == programs_.end()) {
        const GLuint program = glCreateProgram();
        if (program == 0) return false;
        glAttachShader(program, static_cast<GLuint>(vertexIt->second.glId));
        glAttachShader(program, static_cast<GLuint>(fragmentIt->second.glId));
        glLinkProgram(program);
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            if (logLength > 1) {
                std::vector<char> log(static_cast<std::size_t>(logLength), '\0');
                GLsizei written = 0;
                glGetProgramInfoLog(program, logLength, &written, log.data());
                __android_log_print(ANDROID_LOG_ERROR, "StormEngine", "GLSL link failed: %.*s",
                                     static_cast<int>(written), log.data());
            }
            glDeleteProgram(program);
            return false;
        }
        if (glGetError() != GL_NO_ERROR) {
            glDeleteProgram(program);
            return false;
        }
        programIt = programs_.emplace(programKey, program).first;
    }
    program_ = programIt->second;

    RenderState desiredState{};
    desiredState.shader = command.shader;
    desiredState.fragmentShader = command.fragmentShader;
    desiredState.topology = command.topology;
    if (stateCache_.needsApply(desiredState)) {
        glUseProgram(static_cast<GLuint>(program_));
        if (desiredState.depthTestEnabled) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        if (desiredState.depthWriteEnabled) glDepthMask(GL_TRUE);
        else glDepthMask(GL_FALSE);
        if (desiredState.cullEnabled) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        if (desiredState.blendEnabled) glEnable(GL_BLEND);
        else glDisable(GL_BLEND);
        if (desiredState.scissorEnabled) glEnable(GL_SCISSOR_TEST);
        else glDisable(GL_SCISSOR_TEST);
        stateCache_.markApplied(desiredState);
    }

    if (vao_ == 0) {
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        vao_ = static_cast<std::uint32_t>(vao);
    }
    if (vao_ == 0) return false;
    glBindVertexArray(static_cast<GLuint>(vao_));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(vertexBufferIt->second.glId));
    for (std::uint32_t i = 0; i < command.vertexLayout.attributeCount; ++i) {
        const auto& attribute = command.vertexLayout.attributes[i];
        const GLint components = componentCount(attribute.format);
        glEnableVertexAttribArray(attribute.location);
        glVertexAttribPointer(attribute.location, components, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(command.vertexLayout.stride),
                              reinterpret_cast<const void*>(static_cast<std::uintptr_t>(attribute.offset)));
    }

    if (maxTextureUnits_ == 0) glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits_);
    if (maxTextureUnits_ <= 0) return false;

    if (command.materialData != nullptr && !applyMaterial(*command.materialData,
                                                            static_cast<GLuint>(program_), uniformLocations_, textures_, textureHandles_, maxTextureUnits_)) return false;

    if (command.indexed()) {
        if (!bufferHandles_.valid(command.indexBuffer)) return false;
        const auto indexIt = buffers_.find(command.indexBuffer.id());
        if (indexIt == buffers_.end()) return false;
        const std::uint64_t indexSize = command.indexType == IndexType::UInt16 ? 2u : 4u;
        if (command.firstIndex > std::numeric_limits<std::uint64_t>::max() / indexSize ||
            command.indexCount > maxGlCount ||
            static_cast<std::uint64_t>(command.indexCount) > std::numeric_limits<std::uint64_t>::max() / indexSize) return false;
        const std::uint64_t indexOffset = static_cast<std::uint64_t>(command.firstIndex) * indexSize;
        const std::uint64_t indexBytes = static_cast<std::uint64_t>(command.indexCount) * indexSize;
        if (indexOffset > indexIt->second.size || indexBytes > indexIt->second.size - indexOffset) return false;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(indexIt->second.glId));
        glDrawElements(toTopology(command.topology), static_cast<GLsizei>(command.indexCount), toIndexType(command.indexType),
                       reinterpret_cast<const void*>(static_cast<std::uintptr_t>(indexOffset)));
    } else {
        if (command.firstVertex > maxGlInt) return false;
        glDrawArrays(toTopology(command.topology), static_cast<GLint>(command.firstVertex), static_cast<GLsizei>(command.vertexCount));
    }
    if (glGetError() != GL_NO_ERROR) return false;
    ++submittedDraws_;
    return true;
}
void GlesRenderDevice::endFrame() { frameActive_ = false; }
std::size_t GlesRenderDevice::liveBufferCount() const noexcept { return buffers_.size(); }
std::size_t GlesRenderDevice::liveTextureCount() const noexcept { return textures_.size(); }
std::size_t GlesRenderDevice::liveShaderCount() const noexcept { return shaders_.size(); }

} // namespace storm::render
#else
namespace storm::render {
GlesRenderDevice::~GlesRenderDevice() = default;
void GlesRenderDevice::onContextLost() noexcept { gpuResourcesValid_ = false; }
bool GlesRenderDevice::onContextRestored() noexcept { return true; }
BufferHandle GlesRenderDevice::createBuffer(const BufferDesc&) { return {}; }
void GlesRenderDevice::destroyBuffer(BufferHandle) {}
bool GlesRenderDevice::updateBuffer(BufferHandle, const void*, std::size_t, std::size_t) { return false; }
TextureHandle GlesRenderDevice::createTexture(const TextureDesc&) { return {}; }
bool GlesRenderDevice::updateTexture(TextureHandle, const void*, std::size_t, std::uint32_t) { return false; }
void GlesRenderDevice::destroyTexture(TextureHandle) {}
ShaderHandle GlesRenderDevice::createShader(const ShaderDesc&, const std::string&) { return {}; }
void GlesRenderDevice::destroyShader(ShaderHandle) {}
void GlesRenderDevice::beginFrame() { frameActive_ = true; submittedDraws_ = 0; }
bool GlesRenderDevice::submit(const DrawCommand&) { return false; }
void GlesRenderDevice::endFrame() { frameActive_ = false; }
std::size_t GlesRenderDevice::liveBufferCount() const noexcept { return 0; }
std::size_t GlesRenderDevice::liveTextureCount() const noexcept { return 0; }
std::size_t GlesRenderDevice::liveShaderCount() const noexcept { return 0; }
std::uint32_t GlesRenderDevice::allocateHandle(std::uint32_t& next) { return next++; }
} // namespace storm::render
#endif
