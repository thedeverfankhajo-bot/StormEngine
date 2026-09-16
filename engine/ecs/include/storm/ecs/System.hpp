#pragma once

#include "SystemPhase.hpp"
#include <string_view>

namespace storm::ecs {

class Registry;

class SystemContext {
public:
    explicit SystemContext(Registry& registry) noexcept : registry_(registry) {}

    Registry& registry() noexcept { return registry_; }
    const Registry& registry() const noexcept { return registry_; }

private:
    Registry& registry_;
};

class System {
public:
    virtual ~System() = default;

    virtual std::string_view name() const noexcept = 0;
    virtual SystemPhase phase() const noexcept = 0;
    virtual void update(SystemContext& context, float deltaTime) = 0;
};

} // namespace storm::ecs
