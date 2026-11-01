#pragma once

#include "storm/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Storm::Physics::OneD {

enum class BodyType1D : std::uint8_t {
    Static,
    Kinematic,
    Dynamic,
};

struct BodyHandle1D final {
    std::uint64_t world_id{0U};
    std::uint32_t slot_index{0U};
    std::uint32_t generation{0U};

    friend bool operator==(const BodyHandle1D&, const BodyHandle1D&) = default;
};

struct BodyDefinition1D final {
    BodyType1D type{BodyType1D::Dynamic};
    double position{0.0};
    double velocity{0.0};
    double half_extent{0.5};
    double mass{1.0};
    double restitution{0.0};
    bool is_trigger{false};
    std::uint32_t collision_layer{1U};
    std::uint32_t collision_mask{0xFFFFFFFFU};
};

struct BodySnapshot1D final {
    BodyHandle1D handle{};
    BodyType1D type{BodyType1D::Dynamic};
    double position{0.0};
    double velocity{0.0};
    double half_extent{0.5};
    double mass{1.0};
    double restitution{0.0};
    double accumulated_force{0.0};
    bool is_trigger{false};
    std::uint32_t collision_layer{1U};
    std::uint32_t collision_mask{0xFFFFFFFFU};
};

struct Contact1D final {
    BodyHandle1D first{};
    BodyHandle1D second{};
    // Unit normal directed from first toward second. In 1D it is always -1 or +1.
    double normal{1.0};
    double point{0.0};
    double penetration_depth{0.0};
    bool is_trigger{false};
};

struct StepResult1D final {
    double delta_seconds{0.0};
    std::vector<Contact1D> contacts;
};

struct PhysicsWorld1DConfig final {
    std::size_t maximum_bodies{256U};
    std::size_t maximum_contacts_per_step{4096U};
    double maximum_step_seconds{0.25};
};

class PhysicsWorld1D final {
public:
    [[nodiscard]] static Result<PhysicsWorld1D> create(
        PhysicsWorld1DConfig config = {});

    PhysicsWorld1D(const PhysicsWorld1D&) = delete;
    PhysicsWorld1D& operator=(const PhysicsWorld1D&) = delete;
    PhysicsWorld1D& operator=(PhysicsWorld1D&&) = delete;
    PhysicsWorld1D(PhysicsWorld1D&& other) noexcept;

    [[nodiscard]] Result<BodyHandle1D> create_body(
        const BodyDefinition1D& definition);
    [[nodiscard]] Result<void> destroy_body(BodyHandle1D handle);
    [[nodiscard]] Result<void> set_position(BodyHandle1D handle, double position);
    [[nodiscard]] Result<void> set_velocity(BodyHandle1D handle, double velocity);
    [[nodiscard]] Result<void> apply_force(BodyHandle1D handle, double force);
    [[nodiscard]] Result<void> apply_impulse(BodyHandle1D handle, double impulse);
    [[nodiscard]] Result<BodySnapshot1D> body_snapshot(BodyHandle1D handle) const;

    // Call from the engine's fixed-update phase. Forces are consumed by one
    // successful step; a failed step leaves body state and forces unchanged.
    [[nodiscard]] Result<StepResult1D> step(
        double delta_seconds,
        double gravity = 0.0);

    [[nodiscard]] std::size_t body_count() const noexcept;
    [[nodiscard]] std::uint64_t world_id() const noexcept;

private:
    struct Slot final {
        bool alive{false};
        bool retired{false};
        std::uint32_t generation{1U};
        BodyDefinition1D definition{};
        double position{0.0};
        double velocity{0.0};
        double accumulated_force{0.0};
    };

    struct WorkingBody final {
        bool active{false};
        BodyDefinition1D definition{};
        double position{0.0};
        double velocity{0.0};
        double accumulated_force{0.0};
    };

    PhysicsWorld1D(PhysicsWorld1DConfig config, std::uint64_t world_id) noexcept;

    [[nodiscard]] Slot* find_slot(BodyHandle1D handle) noexcept;
    [[nodiscard]] const Slot* find_slot(BodyHandle1D handle) const noexcept;
    [[nodiscard]] BodyHandle1D make_handle(
        std::uint32_t slot_index,
        const Slot& slot) const noexcept;

    PhysicsWorld1DConfig config_{};
    std::uint64_t world_id_{0U};
    std::size_t body_count_{0U};
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_slots_;
};

} // namespace Storm::Physics::OneD
