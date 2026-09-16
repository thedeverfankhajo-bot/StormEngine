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
        if (system == dependency) return false;
        const auto duplicate = std::find(explicitDependencies_.begin(), explicitDependencies_.end(),
            std::pair<std::string_view, std::string_view>{system, dependency});
        if (duplicate != explicitDependencies_.end()) return true;
        explicitDependencies_.emplace_back(system, dependency);
        dirty_ = true;
        graphDirty_ = true;
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
        for (const auto& system : systems_) {
            if (!graph_.addSystem(system->name())) {
                validOrder_ = false;
                graphDirty_ = false;
                return;
            }
        }

        for (std::size_t a = 0; a < systems_.size(); ++a) {
            for (std::size_t b = a + 1; b < systems_.size(); ++b) {
                const auto phaseA = static_cast<std::uint8_t>(systems_[a]->phase());
                const auto phaseB = static_cast<std::uint8_t>(systems_[b]->phase());
                if (phaseA < phaseB)
                    graph_.addDependency(systems_[b]->name(), systems_[a]->name());
                else if (phaseB < phaseA)
                    graph_.addDependency(systems_[a]->name(), systems_[b]->name());
            }
        }

        for (const auto& [system, dependency] : explicitDependencies_)
            if (!graph_.addDependency(system, dependency)) {
                validOrder_ = false;
                graphDirty_ = false;
                return;
            }
        graphDirty_ = false;
    }

    void rebuildOrderIfNeeded() {
        if (!dirty_) return;
        rebuildGraphIfNeeded();
        if (validOrder_)
            validOrder_ = graph_.buildOrder(executionOrder_);
        else
            executionOrder_.clear();
        dirty_ = false;
    }

    std::vector<std::unique_ptr<System>> systems_;
    std::vector<std::pair<std::string_view, std::string_view>> explicitDependencies_;
    std::vector<std::size_t> executionOrder_;
    SystemDependencyGraph graph_;
    bool dirty_{true};
    bool graphDirty_{true};
    bool validOrder_{true};
};

} // namespace storm::ecs
