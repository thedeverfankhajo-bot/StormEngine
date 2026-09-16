#pragma once

#include "System.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace storm::ecs {

class SystemScheduler final {
public:
    SystemScheduler() = default;
    SystemScheduler(const SystemScheduler&) = delete;
    SystemScheduler& operator=(const SystemScheduler&) = delete;

    template <typename T, typename... Args>
    T& add(Args&&... args) {
        static_assert(std::is_base_of_v<System, T>, "T must derive from System");
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& reference = *system;
        systems_.push_back(std::move(system));
        dirty_ = true;
        return reference;
    }

    bool empty() const noexcept { return systems_.empty(); }
    std::size_t size() const noexcept { return systems_.size(); }

    System& at(std::size_t index) noexcept {
        assert(index < systems_.size());
        return *systems_[index];
    }

    const System& at(std::size_t index) const noexcept {
        assert(index < systems_.size());
        return *systems_[index];
    }

    void update(Registry& registry, float deltaTime) {
        rebuildOrderIfNeeded();
        SystemContext context{registry};
        for (const std::size_t index : executionOrder_)
            systems_[index]->update(context, deltaTime);
    }

    bool contains(std::string_view systemName) const noexcept {
        return std::any_of(systems_.begin(), systems_.end(), [systemName](const auto& system) {
            return system->name() == systemName;
        });
    }

private:
    void rebuildOrderIfNeeded() {
        if (!dirty_) return;

        executionOrder_.resize(systems_.size());
        for (std::size_t i = 0; i < executionOrder_.size(); ++i)
            executionOrder_[i] = i;

        std::stable_sort(executionOrder_.begin(), executionOrder_.end(), [this](std::size_t a, std::size_t b) {
            return static_cast<std::uint8_t>(systems_[a]->phase()) <
                   static_cast<std::uint8_t>(systems_[b]->phase());
        });

        dirty_ = false;
    }

    std::vector<std::unique_ptr<System>> systems_;
    std::vector<std::size_t> executionOrder_;
    bool dirty_{true};
};

} // namespace storm::ecs
