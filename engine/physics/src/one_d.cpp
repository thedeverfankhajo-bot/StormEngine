#include "storm/physics/one_d.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <new>
#include <optional>
#include <utility>

namespace Storm::Physics::OneD {
namespace {

std::atomic<std::uint64_t> next_world_id{1U};

[[nodiscard]] std::uint64_t allocate_world_id() noexcept {
    std::uint64_t candidate = next_world_id.load(std::memory_order_relaxed);
    while (candidate != 0U && candidate != std::numeric_limits<std::uint64_t>::max()) {
        if (next_world_id.compare_exchange_weak(
                candidate,
                candidate + 1U,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            return candidate;
        }
    }
    return 0U;
}

[[nodiscard]] bool is_finite(double value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] Error invalid_handle_error() {
    return Error::make(
        ErrorCode::InvalidState,
        "The body handle is stale, foreign, or does not identify a live body");
}

[[nodiscard]] Error invalid_step_error(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}

[[nodiscard]] bool is_valid_body_type(BodyType1D type) noexcept {
    switch (type) {
    case BodyType1D::Static:
    case BodyType1D::Kinematic:
    case BodyType1D::Dynamic:
        return true;
    }
    return false;
}

[[nodiscard]] std::optional<Error> validate_definition(const BodyDefinition1D& definition) {
    if (!is_valid_body_type(definition.type)) {
        return Error::make(ErrorCode::InvalidArgument, "Body type is not supported");
    }
    if (!is_finite(definition.position) ||
        !is_finite(definition.velocity) ||
        !is_finite(definition.half_extent) ||
        !is_finite(definition.mass) ||
        !is_finite(definition.restitution)) {
        return Error::make(
            ErrorCode::InvalidArgument,
            "Body position, velocity, extent, mass, and restitution must be finite");
    }
    if (definition.half_extent <= 0.0) {
        return Error::make(
            ErrorCode::InvalidArgument,
            "A 1D body half-extent must be greater than zero");
    }
    if (definition.restitution < 0.0 || definition.restitution > 1.0) {
        return Error::make(
            ErrorCode::InvalidArgument,
            "Restitution must be within the inclusive range [0, 1]");
    }
    if (definition.type == BodyType1D::Dynamic) {
        if (definition.mass <= 0.0 || !is_finite(1.0 / definition.mass)) {
            return Error::make(
                ErrorCode::InvalidArgument,
                "A dynamic body mass must be positive, finite, and have a representable inverse");
        }
    }
    if (definition.type == BodyType1D::Static && definition.velocity != 0.0) {
        return Error::make(
            ErrorCode::InvalidArgument,
            "A static body cannot have a non-zero velocity");
    }
    return std::nullopt;
}

} // namespace

PhysicsWorld1D::PhysicsWorld1D(
    PhysicsWorld1DConfig config,
    std::uint64_t world_id) noexcept
    : config_(config),
      world_id_(world_id) {}

PhysicsWorld1D::PhysicsWorld1D(PhysicsWorld1D&& other) noexcept
    : config_(other.config_),
      world_id_(std::exchange(other.world_id_, 0U)),
      body_count_(std::exchange(other.body_count_, 0U)),
      slots_(std::move(other.slots_)),
      free_slots_(std::move(other.free_slots_)) {}

Result<PhysicsWorld1D> PhysicsWorld1D::create(PhysicsWorld1DConfig config) {
    if (config.maximum_bodies == 0U ||
        config.maximum_bodies > std::numeric_limits<std::uint32_t>::max() ||
        config.maximum_contacts_per_step == 0U ||
        !is_finite(config.maximum_step_seconds) ||
        config.maximum_step_seconds <= 0.0) {
        return Result<PhysicsWorld1D>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "PhysicsWorld1D requires positive body/contact limits and a finite positive maximum step"));
    }

    const std::uint64_t id = allocate_world_id();
    if (id == 0U) {
        return Result<PhysicsWorld1D>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "The process exhausted unique PhysicsWorld1D identities"));
    }

    return Result<PhysicsWorld1D>::success(PhysicsWorld1D(config, id));
}

BodyHandle1D PhysicsWorld1D::make_handle(
    std::uint32_t slot_index,
    const Slot& slot) const noexcept {
    return BodyHandle1D{world_id_, slot_index, slot.generation};
}

PhysicsWorld1D::Slot* PhysicsWorld1D::find_slot(BodyHandle1D handle) noexcept {
    if (world_id_ == 0U ||
        handle.world_id != world_id_ ||
        handle.slot_index >= slots_.size()) {
        return nullptr;
    }

    Slot& slot = slots_[handle.slot_index];
    if (!slot.alive || slot.generation != handle.generation) {
        return nullptr;
    }
    return &slot;
}

const PhysicsWorld1D::Slot* PhysicsWorld1D::find_slot(
    BodyHandle1D handle) const noexcept {
    if (world_id_ == 0U ||
        handle.world_id != world_id_ ||
        handle.slot_index >= slots_.size()) {
        return nullptr;
    }

    const Slot& slot = slots_[handle.slot_index];
    if (!slot.alive || slot.generation != handle.generation) {
        return nullptr;
    }
    return &slot;
}

Result<BodyHandle1D> PhysicsWorld1D::create_body(
    const BodyDefinition1D& definition) {
    if (world_id_ == 0U) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::InvalidState,
            "A moved-from PhysicsWorld1D cannot create bodies"));
    }

    const std::optional<Error> validation_error = validate_definition(definition);
    if (validation_error.has_value()) {
        return Result<BodyHandle1D>::failure(*validation_error);
    }
    if (body_count_ >= config_.maximum_bodies) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "PhysicsWorld1D reached its configured body limit"));
    }

    try {
        std::uint32_t index = 0U;
        if (!free_slots_.empty()) {
            index = free_slots_.back();
        } else {
            if (slots_.size() >= config_.maximum_bodies ||
                slots_.size() >= std::numeric_limits<std::uint32_t>::max()) {
                return Result<BodyHandle1D>::failure(Error::make(
                    ErrorCode::ResourceExhausted,
                    "PhysicsWorld1D has no reusable body slots"));
            }
            slots_.emplace_back();
            index = static_cast<std::uint32_t>(slots_.size() - 1U);
        }

        Slot& slot = slots_[index];
        slot.definition = definition;
        slot.position = definition.position;
        slot.velocity = definition.velocity;
        slot.accumulated_force = 0.0;
        slot.alive = true;
            if (!free_slots_.empty() && free_slots_.back() == index) {
            free_slots_.pop_back();
        }
        ++body_count_;
        return Result<BodyHandle1D>::success(make_handle(index, slot));
    } catch (const std::bad_alloc&) {
        return Result<BodyHandle1D>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Could not allocate storage for a 1D physics body"));
    }
}

Result<void> PhysicsWorld1D::destroy_body(BodyHandle1D handle) {
    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }

    if (slot->generation != std::numeric_limits<std::uint32_t>::max()) {
        try {
            free_slots_.push_back(handle.slot_index);
        } catch (const std::bad_alloc&) {
            return Result<void>::failure(Error::make(
                ErrorCode::ResourceExhausted,
                "Could not allocate a reusable body slot"));
        }
        ++slot->generation;
    }

    slot->alive = false;
    slot->accumulated_force = 0.0;
    --body_count_;
    return Result<void>::success();
}

Result<void> PhysicsWorld1D::set_position(BodyHandle1D handle, double position) {
    if (!is_finite(position)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "Body position must be finite"));
    }

    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }
    slot->position = position;
    return Result<void>::success();
}

Result<void> PhysicsWorld1D::set_velocity(BodyHandle1D handle, double velocity) {
    if (!is_finite(velocity)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "Body velocity must be finite"));
    }

    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }
    if (slot->definition.type == BodyType1D::Static && velocity != 0.0) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState,
            "A static body cannot have a non-zero velocity"));
    }
    slot->velocity = velocity;
    return Result<void>::success();
}

Result<void> PhysicsWorld1D::apply_force(BodyHandle1D handle, double force) {
    if (!is_finite(force)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "Applied force must be finite"));
    }

    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }
    if (slot->definition.type != BodyType1D::Dynamic) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState,
            "Forces can only be accumulated on dynamic bodies"));
    }

    const double combined_force = slot->accumulated_force + force;
    if (!is_finite(combined_force)) {
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Accumulated force exceeds the supported numeric range"));
    }
    slot->accumulated_force = combined_force;
    return Result<void>::success();
}

Result<void> PhysicsWorld1D::apply_impulse(BodyHandle1D handle, double impulse) {
    if (!is_finite(impulse)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "Applied impulse must be finite"));
    }

    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }
    if (slot->definition.type != BodyType1D::Dynamic) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState,
            "Impulses can only be applied to dynamic bodies"));
    }

    const double new_velocity =
        slot->velocity + (impulse / slot->definition.mass);
    if (!is_finite(new_velocity)) {
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Impulse would produce a non-finite body velocity"));
    }
    slot->velocity = new_velocity;
    return Result<void>::success();
}

Result<BodySnapshot1D> PhysicsWorld1D::body_snapshot(BodyHandle1D handle) const {
    const Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<BodySnapshot1D>::failure(invalid_handle_error());
    }

    return Result<BodySnapshot1D>::success(BodySnapshot1D{
        .handle = handle,
        .type = slot->definition.type,
        .position = slot->position,
        .velocity = slot->velocity,
        .half_extent = slot->definition.half_extent,
        .mass = slot->definition.mass,
        .restitution = slot->definition.restitution,
        .accumulated_force = slot->accumulated_force,
        .is_trigger = slot->definition.is_trigger,
        .collision_layer = slot->definition.collision_layer,
        .collision_mask = slot->definition.collision_mask,
    });
}

Result<std::vector<BodyHandle1D>> PhysicsWorld1D::overlap_query(
    const OverlapQuery1D& query) const {
    if (world_id_ == 0U) {
        return Result<std::vector<BodyHandle1D>>::failure(Error::make(
            ErrorCode::InvalidState,
            "A moved-from PhysicsWorld1D cannot execute overlap queries"));
    }
    if (!is_finite(query.position) ||
        !is_finite(query.half_extent) ||
        query.half_extent < 0.0) {
        return Result<std::vector<BodyHandle1D>>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "An overlap query requires a finite position and non-negative finite half-extent"));
    }

    const double query_min = query.position - query.half_extent;
    const double query_max = query.position + query.half_extent;
    if (!is_finite(query_min) || !is_finite(query_max)) {
        return Result<std::vector<BodyHandle1D>>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "Overlap query bounds exceed the supported numeric range"));
    }

    std::vector<BodyHandle1D> matches;
    if (query.collision_mask == 0U || body_count_ == 0U) {
        return Result<std::vector<BodyHandle1D>>::success(std::move(matches));
    }

    try {
        matches.reserve(body_count_);
        for (std::size_t index = 0U; index < slots_.size(); ++index) {
            const Slot& slot = slots_[index];
            if (!slot.alive ||
                (!query.include_triggers && slot.definition.is_trigger) ||
                (query.collision_mask & slot.definition.collision_layer) == 0U) {
                continue;
            }

            const double body_min = slot.position - slot.definition.half_extent;
            const double body_max = slot.position + slot.definition.half_extent;
            if (!is_finite(body_min) || !is_finite(body_max)) {
                return Result<std::vector<BodyHandle1D>>::failure(Error::make(
                    ErrorCode::ResourceExhausted,
                    "A stored 1D collider bound exceeds the supported numeric range"));
            }
            if (query_max < body_min || body_max < query_min) {
                continue;
            }

            matches.push_back(make_handle(
                static_cast<std::uint32_t>(index),
                slot));
        }
        return Result<std::vector<BodyHandle1D>>::success(std::move(matches));
    } catch (const std::bad_alloc&) {
        return Result<std::vector<BodyHandle1D>>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Could not allocate results for a 1D overlap query"));
    }
}

Result<StepResult1D> PhysicsWorld1D::step(
    double delta_seconds,
    double gravity) {
    if (world_id_ == 0U) {
        return Result<StepResult1D>::failure(Error::make(
            ErrorCode::InvalidState,
            "A moved-from PhysicsWorld1D cannot be stepped"));
    }
    if (!is_finite(delta_seconds) ||
        delta_seconds <= 0.0 ||
        delta_seconds > config_.maximum_step_seconds ||
        !is_finite(gravity)) {
        return Result<StepResult1D>::failure(invalid_step_error(
            "Physics step requires a finite positive delta within the configured limit and finite gravity"));
    }

    try {
        std::vector<WorkingBody> working;
        working.resize(slots_.size());

        for (std::size_t index = 0U; index < slots_.size(); ++index) {
            const Slot& slot = slots_[index];
            if (!slot.alive) {
                continue;
            }

            WorkingBody& body = working[index];
            body.active = true;
            body.definition = slot.definition;
            body.position = slot.position;
            body.velocity = slot.velocity;
            body.accumulated_force = slot.accumulated_force;

            if (body.definition.type == BodyType1D::Dynamic) {
                const double acceleration =
                    gravity + (body.accumulated_force / body.definition.mass);
                body.velocity += acceleration * delta_seconds;
                body.position += body.velocity * delta_seconds;
            } else if (body.definition.type == BodyType1D::Kinematic) {
                body.position += body.velocity * delta_seconds;
            }
            body.accumulated_force = 0.0;

            if (!is_finite(body.position) || !is_finite(body.velocity)) {
                return Result<StepResult1D>::failure(Error::make(
                    ErrorCode::ResourceExhausted,
                    "A 1D physics integration step produced a non-finite state"));
            }
        }

        StepResult1D result;
        result.delta_seconds = delta_seconds;
        result.contacts.reserve(std::min(config_.maximum_contacts_per_step, body_count_));

        for (std::size_t first_index = 0U; first_index < working.size(); ++first_index) {
            WorkingBody& first = working[first_index];
            if (!first.active) {
                continue;
            }

            for (std::size_t second_index = first_index + 1U;
                 second_index < working.size();
                 ++second_index) {
                WorkingBody& second = working[second_index];
                if (!second.active) {
                    continue;
                }

                // Earlier pair resolution may have moved this body; refresh its bounds.
                const double first_min = first.position - first.definition.half_extent;
                const double first_max = first.position + first.definition.half_extent;
                if (!is_finite(first_min) || !is_finite(first_max)) {
                    return Result<StepResult1D>::failure(Error::make(
                        ErrorCode::ResourceExhausted,
                        "A 1D collider bound exceeded the supported numeric range"));
                }
                if ((first.definition.collision_layer & second.definition.collision_mask) == 0U ||
                    (second.definition.collision_layer & first.definition.collision_mask) == 0U) {
                    continue;
                }

                const double second_min = second.position - second.definition.half_extent;
                const double second_max = second.position + second.definition.half_extent;
                if (!is_finite(second_min) || !is_finite(second_max)) {
                    return Result<StepResult1D>::failure(Error::make(
                        ErrorCode::ResourceExhausted,
                        "A 1D collider bound exceeded the supported numeric range"));
                }

                const double overlap_min = std::max(first_min, second_min);
                const double overlap_max = std::min(first_max, second_max);
                const double penetration = overlap_max - overlap_min;
                if (!is_finite(penetration)) {
                    return Result<StepResult1D>::failure(Error::make(
                        ErrorCode::ResourceExhausted,
                        "A 1D collision depth exceeded the supported numeric range"));
                }
                if (penetration < 0.0) {
                    continue;
                }

                if (result.contacts.size() >= config_.maximum_contacts_per_step) {
                    return Result<StepResult1D>::failure(Error::make(
                        ErrorCode::ResourceExhausted,
                        "Physics step exceeded its configured contact limit"));
                }

                const double normal = second.position >= first.position ? 1.0 : -1.0;
                const bool trigger =
                    first.definition.is_trigger || second.definition.is_trigger;
                const double contact_point = overlap_min + (penetration * 0.5);
                if (!is_finite(contact_point)) {
                    return Result<StepResult1D>::failure(Error::make(
                        ErrorCode::ResourceExhausted,
                        "A 1D contact point exceeded the supported numeric range"));
                }

                const auto first_slot = static_cast<std::uint32_t>(first_index);
                const auto second_slot = static_cast<std::uint32_t>(second_index);
                result.contacts.push_back(Contact1D{
                    .first = make_handle(first_slot, slots_[first_index]),
                    .second = make_handle(second_slot, slots_[second_index]),
                    .normal = normal,
                    .point = contact_point,
                    .penetration_depth = penetration,
                    .is_trigger = trigger,
                });

                if (trigger) {
                    continue;
                }

                const double first_inverse_mass =
                    first.definition.type == BodyType1D::Dynamic
                        ? 1.0 / first.definition.mass
                        : 0.0;
                const double second_inverse_mass =
                    second.definition.type == BodyType1D::Dynamic
                        ? 1.0 / second.definition.mass
                        : 0.0;
                const double inverse_mass_sum =
                    first_inverse_mass + second_inverse_mass;
                if (!is_finite(inverse_mass_sum)) {
                    return Result<StepResult1D>::failure(Error::make(
                        ErrorCode::ResourceExhausted,
                        "Combined inverse body mass exceeded the supported numeric range"));
                }
                if (inverse_mass_sum <= 0.0) {
                    continue;
                }

                if (penetration > 0.0) {
                    const double correction = penetration / inverse_mass_sum;
                    if (!is_finite(correction)) {
                        return Result<StepResult1D>::failure(Error::make(
                            ErrorCode::ResourceExhausted,
                            "Collision correction exceeded the supported numeric range"));
                    }
                    first.position -= normal * correction * first_inverse_mass;
                    second.position += normal * correction * second_inverse_mass;
                }

                const double relative_normal_velocity =
                    (second.velocity - first.velocity) * normal;
                if (relative_normal_velocity < 0.0) {
                    const double restitution = std::min(
                        first.definition.restitution,
                        second.definition.restitution);
                    const double impulse_magnitude =
                        (-(1.0 + restitution) * relative_normal_velocity) /
                        inverse_mass_sum;
                    const double impulse = impulse_magnitude * normal;
                    first.velocity -= impulse * first_inverse_mass;
                    second.velocity += impulse * second_inverse_mass;
                }
            }
        }

        for (const WorkingBody& body : working) {
            if (body.active &&
                (!is_finite(body.position) || !is_finite(body.velocity))) {
                return Result<StepResult1D>::failure(Error::make(
                    ErrorCode::ResourceExhausted,
                    "Collision resolution produced a non-finite 1D body state"));
            }
        }

        // Commit only after integration, collision detection, and all limits succeed.
        for (std::size_t index = 0U; index < slots_.size(); ++index) {
            if (!slots_[index].alive) {
                continue;
            }
            slots_[index].position = working[index].position;
            slots_[index].velocity = working[index].velocity;
            slots_[index].accumulated_force = working[index].accumulated_force;
        }

        return Result<StepResult1D>::success(std::move(result));
    } catch (const std::bad_alloc&) {
        return Result<StepResult1D>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Could not allocate temporary state for a transactional 1D physics step"));
    }
}

std::size_t PhysicsWorld1D::body_count() const noexcept {
    return body_count_;
}

std::uint64_t PhysicsWorld1D::world_id() const noexcept {
    return world_id_;
}

} // namespace Storm::Physics::OneD
