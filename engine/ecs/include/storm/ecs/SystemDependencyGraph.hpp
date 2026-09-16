#pragma once

#include <cstddef>
#include <limits>
#include <string_view>
#include <vector>

namespace storm::ecs {

class SystemDependencyGraph final {
public:
    using SystemIndex = std::size_t;
    static constexpr SystemIndex kInvalidIndex = std::numeric_limits<SystemIndex>::max();

    bool addSystem(std::string_view name) {
        if (indexOf(name) != kInvalidIndex) return false;
        names_.emplace_back(name);
        edges_.emplace_back();
        return true;
    }

    bool addDependency(std::string_view system, std::string_view dependency) {
        const SystemIndex systemIndex = indexOf(system);
        const SystemIndex dependencyIndex = indexOf(dependency);
        if (systemIndex == kInvalidIndex || dependencyIndex == kInvalidIndex ||
            systemIndex == dependencyIndex) {
            return false;
        }

        auto& dependencies = edges_[systemIndex];
        for (const SystemIndex existing : dependencies) {
            if (existing == dependencyIndex) return true;
        }
        dependencies.push_back(dependencyIndex);
        return true;
    }

    bool buildOrder(std::vector<SystemIndex>& order) const {
        order.clear();
        order.reserve(names_.size());

        std::vector<std::size_t> remainingDependencies(names_.size(), 0);
        std::vector<std::vector<SystemIndex>> dependents(names_.size());

        for (SystemIndex system = 0; system < names_.size(); ++system) {
            remainingDependencies[system] = edges_[system].size();
            for (const SystemIndex dependency : edges_[system])
                dependents[dependency].push_back(system);
        }

        std::vector<SystemIndex> ready;
        for (SystemIndex system = 0; system < names_.size(); ++system) {
            if (remainingDependencies[system] == 0)
                ready.push_back(system);
        }

        while (!ready.empty()) {
            const SystemIndex current = ready.front();
            ready.erase(ready.begin());
            order.push_back(current);

            for (const SystemIndex dependent : dependents[current]) {
                if (--remainingDependencies[dependent] == 0)
                    ready.push_back(dependent);
            }
        }

        return order.size() == names_.size();
    }

    std::size_t size() const noexcept { return names_.size(); }

    std::string_view name(SystemIndex index) const noexcept {
        return index < names_.size() ? names_[index] : std::string_view{};
    }

private:
    SystemIndex indexOf(std::string_view name) const noexcept {
        for (SystemIndex i = 0; i < names_.size(); ++i) {
            if (names_[i] == name) return i;
        }
        return kInvalidIndex;
    }

    std::vector<std::string_view> names_;
    std::vector<std::vector<SystemIndex>> edges_;
};

} // namespace storm::ecs
