#pragma once

#include "RenderTypes.hpp"

namespace storm::render {

class Mesh final {
public:
    Mesh() = default;
    explicit Mesh(const MeshDesc& desc) noexcept : desc_(desc) {}

    [[nodiscard]] const MeshDesc& description() const noexcept { return desc_; }
    [[nodiscard]] bool valid() const noexcept {
        return desc_.vertexBuffer.valid() && desc_.vertexCount > 0 &&
               desc_.vertexLayout.valid() &&
               (desc_.indexCount == 0 || desc_.indexBuffer.valid());
    }
    [[nodiscard]] bool indexed() const noexcept { return desc_.indexCount > 0; }

    [[nodiscard]] DrawCommand drawCommand() const noexcept {
        DrawCommand command{};
        command.topology = desc_.topology;
        command.vertexBuffer = desc_.vertexBuffer;
        command.indexBuffer = desc_.indexBuffer;
        command.vertexCount = desc_.vertexCount;
        command.indexCount = desc_.indexCount;
        command.indexType = desc_.indexType;
        command.vertexLayout = desc_.vertexLayout;
        return command;
    }

private:
    MeshDesc desc_{};
};

} // namespace storm::render
