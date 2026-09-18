#pragma once

#include "RenderDevice.hpp"
#include "RenderTypes.hpp"
#include <cstddef>
#include <vector>

namespace storm::render {

class RenderQueue final {
public:
    void beginFrame() noexcept { commands_.clear(); }

    bool submit(const DrawCommand& command) {
        if (!command.vertexBuffer.valid() || command.vertexCount == 0 || !command.vertexLayout.valid()) return false;
        if (command.viewportWidth > 0 && command.viewportHeight == 0) return false;
        if (command.viewportHeight > 0 && command.viewportWidth == 0) return false;
        if (command.scissorEnabled && (command.scissorWidth == 0 || command.scissorHeight == 0)) return false;
        if (command.indexed()) {
            if (!command.indexBuffer.valid()) return false;
        } else if (command.indexBuffer.valid()) {
            return false;
        }
        commands_.push_back(command);
        return true;
    }

    [[nodiscard]] bool execute(RenderDevice& device) const {
        if (commands_.empty()) return true;
        device.beginFrame();
        bool success = true;
        for (const auto& command : commands_) {
            if (!device.submit(command)) success = false;
        }
        device.endFrame();
        return success;
    }

    [[nodiscard]] bool empty() const noexcept { return commands_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return commands_.size(); }
    [[nodiscard]] const DrawCommand& at(std::size_t index) const { return commands_.at(index); }
    [[nodiscard]] const std::vector<DrawCommand>& commands() const noexcept { return commands_; }

private:
    std::vector<DrawCommand> commands_;
};

} // namespace storm::render
