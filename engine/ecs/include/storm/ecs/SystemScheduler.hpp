#pragma once

#include "System.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <string_view>
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
        SystemContext context{registry};
        for (auto& system : systems_)
            system->update(context, deltaTime);
    }

    bool contains(std::string_view systemName) const noexcept {
        return std::any_of(systems_.begin(), systems_.end(), [systemName](const auto& system) {
            return system->name() == systemName;
        });
    }

private:
    std::vector<std::unique_ptr<System>> systems_;
};

} // namespace storm::ecs
