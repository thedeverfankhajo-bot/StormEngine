#pragma once

#include "System.hpp"
#include "SystemDependencyGraph.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
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
        graphDirty_ = true;
        return reference;
    }

    bool dependsOn(std::string_view system, std::string_view dependency) {
        rebuildGraphIfNeeded();
        if (!graph_.addDependency(system, dependency)) return false;
        dirty_ = true;
        return true;
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
        if (!validOrder_) return;

        SystemContext context{registry};
        for (const std::size_t index : executionOrder_)
            systems_[index]->update(context, deltaTime);
    }

    bool contains(std::string_view systemName) const noexcept {
        return std::any_of(systems_.begin(), systems_.end(), [systemName](const auto& system) {
            return system->name() == systemName;
        });
    }

    bool hasValidOrder() {
        rebuildOrderIfNeeded();
        return validOrder_;
    }

private:
    void rebuildGraphIfNeeded() {
        if (!graphDirty_) return;

        graph_ = SystemDependencyGraph{};
        for (const auto& system : systems_)
            graph_.addSystem(system->name());

        // Phase ordering is a dependency rule as well: every earlier phase
        // must complete before a later phase can execute.
        for (std::size_t later = 0; later < systems_.size(); ++later) {
            for (std::size_t earlier = 0; earlier < later; ++earlier) {
                const auto earlierPhase = static_cast<std::uint8_t>(systems_[earlier]->phase());
                const auto laterPhase = static_cast<std::uint8_t>(systems_[later]->phase());
                if (earlierPhase < laterPhase)
                    graph_.addDependency(systems_[later]->name(), systems_[earlier]->name());
            }
        }

        graphDirty_ = false;
    }

    void rebuildOrderIfNeeded() {
        if (!dirty_) return;
        rebuildGraphIfNeeded();
        validOrder_ = graph_.buildOrder(executionOrder_);
        dirty_ = false;
    }

    std::vector<std::unique_ptr<System>> systems_;
    std::vector<std::size_t> executionOrder_;
    SystemDependencyGraph graph_;
    bool dirty_{true};
    bool graphDirty_{true};
    bool validOrder_{true};
};

} // namespace storm::ecs
