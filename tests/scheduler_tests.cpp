#include <cassert>
#include <string>
#include <vector>

#include "storm/ecs/Registry.hpp"
#include "storm/ecs/SystemScheduler.hpp"

namespace {

class TestSystem final : public storm::ecs::System {
public:
    TestSystem(std::string systemName, storm::ecs::SystemPhase systemPhase, std::vector<std::string>& log)
        : name_(std::move(systemName)), phase_(systemPhase), log_(log) {}

    std::string_view name() const noexcept override { return name_; }
    storm::ecs::SystemPhase phase() const noexcept override { return phase_; }
    void update(storm::ecs::SystemContext&, float) override { log_.push_back(name_); }

private:
    std::string name_;
    storm::ecs::SystemPhase phase_;
    std::vector<std::string>& log_;
};

void testPhaseOrderingIsRegistrationIndependent() {
    storm::ecs::SystemScheduler scheduler;
    std::vector<std::string> log;
    scheduler.add<TestSystem>("render", storm::ecs::SystemPhase::Render, log);
    scheduler.add<TestSystem>("input", storm::ecs::SystemPhase::Input, log);
    scheduler.add<TestSystem>("physics", storm::ecs::SystemPhase::Physics, log);

    storm::ecs::Registry registry;
    scheduler.update(registry, 1.0f / 60.0f);
    assert((log == std::vector<std::string>{"input", "physics", "render"}));
}

void testExplicitDependencySurvivesRebuild() {
    storm::ecs::SystemScheduler scheduler;
    std::vector<std::string> log;
    scheduler.add<TestSystem>("a", storm::ecs::SystemPhase::Gameplay, log);
    scheduler.add<TestSystem>("b", storm::ecs::SystemPhase::Gameplay, log);
    assert(scheduler.dependsOn("b", "a"));
    scheduler.add<TestSystem>("c", storm::ecs::SystemPhase::Gameplay, log);
    assert(scheduler.hasValidOrder());

    storm::ecs::Registry registry;
    scheduler.update(registry, 0.016f);
    const auto a = std::find(log.begin(), log.end(), "a");
    const auto b = std::find(log.begin(), log.end(), "b");
    assert(a < b);
}

void testUnknownDependencyRejected() {
    storm::ecs::SystemScheduler scheduler;
    std::vector<std::string> log;
    scheduler.add<TestSystem>("a", storm::ecs::SystemPhase::Gameplay, log);
    assert(!scheduler.dependsOn("missing", "a"));
    assert(!scheduler.dependsOn("a", "missing"));
    assert(!scheduler.dependsOn("a", "a"));
    assert(scheduler.hasValidOrder());
}

void testDuplicateNamesInvalidateSchedule() {
    storm::ecs::SystemScheduler scheduler;
    std::vector<std::string> log;
    scheduler.add<TestSystem>("same", storm::ecs::SystemPhase::Gameplay, log);
    scheduler.add<TestSystem>("same", storm::ecs::SystemPhase::Physics, log);
    assert(!scheduler.hasValidOrder());
}

} // namespace

int main() {
    testPhaseOrderingIsRegistrationIndependent();
    testExplicitDependencySurvivesRebuild();
    testUnknownDependencyRejected();
    testDuplicateNamesInvalidateSchedule();
    return 0;
}
