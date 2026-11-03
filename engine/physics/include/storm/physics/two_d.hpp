#pragma once

#include "storm/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace Storm::Physics::TwoD {

struct Vector2 final {
    double x{0.0};
    double y{0.0};

    friend bool operator==(const Vector2&, const Vector2&) = default;
};

enum class BodyType2D : std::uint8_t {
    Static,
    Kinematic,
    Dynamic,
};

struct CircleCollider2D final {
    double radius{0.5};
};

struct BoxCollider2D final {
    Vector2 half_extents{0.5, 0.5};
};

// Boxes are axis-aligned in this first physics slice; rotation is not stored.
using ColliderShape2D = std::variant<CircleCollider2D, BoxCollider2D>;

struct BodyHandle2D final {
    std::uint64_t world_id{0U};
    std::uint32_t slot_index{0U};
    std::uint32_t generation{0U};

    friend bool operator==(const BodyHandle2D&, const BodyHandle2D&) = default;
};

struct BodyDefinition2D final {
    BodyType2D type{BodyType2D::Dynamic};
    Vector2 position{};
    Vector2 velocity{};
    ColliderShape2D collider{BoxCollider2D{}};
    double mass{1.0};
    double restitution{0.0};
    double friction{0.5};
    bool is_trigger{false};
    std::uint32_t collision_layer{1U};
    std::uint32_t collision_mask{0xFFFFFFFFU};
};

struct BodySnapshot2D final {
    BodyHandle2D handle{};
    BodyType2D type{BodyType2D::Dynamic};
    Vector2 position{};
    Vector2 velocity{};
    ColliderShape2D collider{BoxCollider2D{}};
    double mass{1.0};
    double restitution{0.0};
    double friction{0.5};
    Vector2 accumulated_force{};
    bool is_trigger{false};
    std::uint32_t collision_layer{1U};
    std::uint32_t collision_mask{0xFFFFFFFFU};
};

struct Contact2D final {
    BodyHandle2D first{};
    BodyHandle2D second{};
    // Unit normal directed from the first body toward the second body.
    Vector2 normal{1.0, 0.0};
    Vector2 point{};
    double penetration_depth{0.0};
    bool is_trigger{false};
};

struct StepResult2D final {
    double delta_seconds{0.0};
    // Narrow-phase contacts in deterministic ascending body-slot pair order.
    std::vector<Contact2D> contacts;
    // Unique candidate pairs passed to the narrow phase after spatial hashing.
    std::size_t broadphase_candidate_pairs{0U};
};

struct PhysicsWorld2DConfig final {
    std::size_t maximum_bodies{512U};
    std::size_t maximum_contacts_per_step{4096U};
    std::size_t maximum_broadphase_entries{16384U};
    std::size_t maximum_candidate_pairs{32768U};
    std::size_t maximum_cells_per_body{256U};
    double broadphase_cell_size{4.0};
    double maximum_step_seconds{0.25};
    double maximum_absolute_coordinate{1.0e9};
};

class PhysicsWorld2D final {
public:
    [[nodiscard]] static Result<PhysicsWorld2D> create(
        PhysicsWorld2DConfig config = {});

    PhysicsWorld2D(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D& operator=(PhysicsWorld2D&&) = delete;
    PhysicsWorld2D(PhysicsWorld2D&& other) noexcept;

    [[nodiscard]] Result<BodyHandle2D> create_body(
        const BodyDefinition2D& definition);
    [[nodiscard]] Result<void> destroy_body(BodyHandle2D handle);
    [[nodiscard]] Result<void> set_position(BodyHandle2D handle, Vector2 position);
    [[nodiscard]] Result<void> set_velocity(BodyHandle2D handle, Vector2 velocity);
    [[nodiscard]] Result<void> apply_force(BodyHandle2D handle, Vector2 force);
    [[nodiscard]] Result<void> apply_impulse(BodyHandle2D handle, Vector2 impulse);
    [[nodiscard]] Result<BodySnapshot2D> body_snapshot(BodyHandle2D handle) const;

    // Call from a fixed-update phase. Forces are consumed only by a successful
    // step; any validation, allocation, or configured-budget failure is atomic.
    [[nodiscard]] Result<StepResult2D> step(
        double delta_seconds,
        Vector2 gravity = {});

    [[nodiscard]] std::size_t body_count() const noexcept;
    [[nodiscard]] std::uint64_t world_id() const noexcept;

private:
    struct Slot final {
        bool alive{false};
        std::uint32_t generation{1U};
        BodyDefinition2D definition{};
        Vector2 position{};
        Vector2 velocity{};
        Vector2 accumulated_force{};
    };

    struct WorkingBody final {
        bool active{false};
        BodyDefinition2D definition{};
        Vector2 position{};
        Vector2 velocity{};
        Vector2 accumulated_force{};
    };

    PhysicsWorld2D(PhysicsWorld2DConfig config, std::uint64_t world_id) noexcept;

    [[nodiscard]] Slot* find_slot(BodyHandle2D handle) noexcept;
    [[nodiscard]] const Slot* find_slot(BodyHandle2D handle) const noexcept;
    [[nodiscard]] BodyHandle2D make_handle(
        std::uint32_t slot_index,
        const Slot& slot) const noexcept;
    [[nodiscard]] bool within_coordinate_limit(Vector2 value) const noexcept;

    PhysicsWorld2DConfig config_{};
    std::uint64_t world_id_{0U};
    std::size_t body_count_{0U};
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_slots_;
};

} // namespace Storm::Physics::TwoD
