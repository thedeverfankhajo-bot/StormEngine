#include "storm/physics/two_d.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <limits>
#include <new>
#include <optional>
#include <unordered_map>
#include <utility>

namespace Storm::Physics::TwoD {
namespace {

std::atomic<std::uint64_t> next_world_id{1U};

struct Bounds final {
    Vector2 minimum{};
    Vector2 maximum{};
};

struct CellCoordinate final {
    std::int64_t x{0};
    std::int64_t y{0};

    friend bool operator==(const CellCoordinate&, const CellCoordinate&) = default;
};

struct CellCoordinateHash final {
    [[nodiscard]] std::size_t operator()(const CellCoordinate& value) const noexcept {
        const std::size_t first = std::hash<std::int64_t>{}(value.x);
        const std::size_t second = std::hash<std::int64_t>{}(value.y);
        return first ^ (second + static_cast<std::size_t>(0x9e3779b9U) +
                        (first << 6U) + (first >> 2U));
    }
};

struct CellRange final {
    std::int64_t minimum_x{0};
    std::int64_t maximum_x{0};
    std::int64_t minimum_y{0};
    std::int64_t maximum_y{0};
    std::size_t cell_count{0U};
};

struct CandidatePair final {
    std::uint32_t first{0U};
    std::uint32_t second{0U};

    friend bool operator==(const CandidatePair&, const CandidatePair&) = default;
};

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

[[nodiscard]] bool is_finite(Vector2 value) noexcept {
    return is_finite(value.x) && is_finite(value.y);
}

[[nodiscard]] Vector2 add(Vector2 first, Vector2 second) noexcept {
    return {first.x + second.x, first.y + second.y};
}

[[nodiscard]] Vector2 subtract(Vector2 first, Vector2 second) noexcept {
    return {first.x - second.x, first.y - second.y};
}

[[nodiscard]] Vector2 multiply(Vector2 value, double scalar) noexcept {
    return {value.x * scalar, value.y * scalar};
}

[[nodiscard]] double dot(Vector2 first, Vector2 second) noexcept {
    return (first.x * second.x) + (first.y * second.y);
}

[[nodiscard]] double length(Vector2 value) noexcept {
    return std::hypot(value.x, value.y);
}

[[nodiscard]] Vector2 negate(Vector2 value) noexcept {
    return {-value.x, -value.y};
}

[[nodiscard]] bool within_limit(Vector2 value, double limit) noexcept {
    return is_finite(value) &&
        std::abs(value.x) <= limit &&
        std::abs(value.y) <= limit;
}

[[nodiscard]] bool valid_body_type(BodyType2D type) noexcept {
    switch (type) {
        case BodyType2D::Static:
        case BodyType2D::Kinematic:
        case BodyType2D::Dynamic:
            return true;
    }
    return false;
}

[[nodiscard]] std::optional<Vector2> collider_half_extents(
    const ColliderShape2D& collider) noexcept {
    if (const auto* circle = std::get_if<CircleCollider2D>(&collider)) {
        if (!is_finite(circle->radius) || circle->radius <= 0.0) {
            return std::nullopt;
        }
        return Vector2{circle->radius, circle->radius};
    }

    if (const auto* box = std::get_if<BoxCollider2D>(&collider)) {
        if (!within_limit(box->half_extents, std::numeric_limits<double>::max()) ||
            box->half_extents.x <= 0.0 || box->half_extents.y <= 0.0) {
            return std::nullopt;
        }
        return box->half_extents;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<Error> validate_definition(
    const BodyDefinition2D& definition,
    double maximum_absolute_coordinate) {
    if (!valid_body_type(definition.type)) {
        return Error::make(ErrorCode::InvalidArgument, "The 2D body type is not supported");
    }
    if (!within_limit(definition.position, maximum_absolute_coordinate) ||
        !within_limit(definition.velocity, maximum_absolute_coordinate) ||
        !is_finite(definition.mass) ||
        !is_finite(definition.restitution) ||
        !is_finite(definition.friction)) {
        return Error::make(
            ErrorCode::InvalidArgument,
            "2D body state, mass, restitution, and friction must be finite and within configured state limits");
    }

    const std::optional<Vector2> extents = collider_half_extents(definition.collider);
    if (!extents.has_value() ||
        !within_limit(*extents, maximum_absolute_coordinate)) {
        return Error::make(
            ErrorCode::InvalidArgument,
            "A 2D collider requires a valid positive radius or positive finite box half-extents within configured limits");
    }

    if (definition.restitution < 0.0 || definition.restitution > 1.0 ||
        definition.friction < 0.0 || definition.friction > 1.0) {
        return Error::make(
            ErrorCode::InvalidArgument,
            "Restitution and friction must be within the inclusive range [0, 1]");
    }
    if (definition.type == BodyType2D::Dynamic) {
        const double inverse_mass = 1.0 / definition.mass;
        if (definition.mass <= 0.0 ||
            !is_finite(inverse_mass) ||
            inverse_mass > maximum_absolute_coordinate) {
            return Error::make(
                ErrorCode::InvalidArgument,
                "A dynamic 2D body mass must have a finite, bounded positive inverse");
        }
    }
    if (definition.type == BodyType2D::Static &&
        definition.velocity != Vector2{}) {
        return Error::make(
            ErrorCode::InvalidArgument,
            "A static 2D body cannot have non-zero velocity");
    }
    return std::nullopt;
}

[[nodiscard]] bool calculate_bounds(
    const BodyDefinition2D& definition,
    Vector2 position,
    Bounds& bounds) noexcept {
    const std::optional<Vector2> extents = collider_half_extents(definition.collider);
    if (!extents.has_value()) {
        return false;
    }
    bounds.minimum = subtract(position, *extents);
    bounds.maximum = add(position, *extents);
    return is_finite(bounds.minimum) && is_finite(bounds.maximum);
}

[[nodiscard]] bool to_cell_coordinate(
    double coordinate,
    double cell_size,
    std::int64_t& result) noexcept {
    const long double cell = std::floor(
        static_cast<long double>(coordinate) / static_cast<long double>(cell_size));
    if (!std::isfinite(cell) ||
        cell < static_cast<long double>(std::numeric_limits<std::int64_t>::min()) ||
        cell > static_cast<long double>(std::numeric_limits<std::int64_t>::max())) {
        return false;
    }
    result = static_cast<std::int64_t>(cell);
    return true;
}

[[nodiscard]] bool calculate_cell_range(
    const Bounds& bounds,
    double cell_size,
    std::size_t maximum_cells,
    CellRange& range) noexcept {
    if (!to_cell_coordinate(bounds.minimum.x, cell_size, range.minimum_x) ||
        !to_cell_coordinate(bounds.maximum.x, cell_size, range.maximum_x) ||
        !to_cell_coordinate(bounds.minimum.y, cell_size, range.minimum_y) ||
        !to_cell_coordinate(bounds.maximum.y, cell_size, range.maximum_y) ||
        range.maximum_x < range.minimum_x ||
        range.maximum_y < range.minimum_y) {
        return false;
    }

    const long double columns =
        static_cast<long double>(range.maximum_x) -
        static_cast<long double>(range.minimum_x) + 1.0L;
    const long double rows =
        static_cast<long double>(range.maximum_y) -
        static_cast<long double>(range.minimum_y) + 1.0L;
    if (!std::isfinite(columns) ||
        !std::isfinite(rows) ||
        columns <= 0.0L ||
        rows <= 0.0L ||
        columns > static_cast<long double>(maximum_cells) ||
        rows > static_cast<long double>(maximum_cells)) {
        return false;
    }

    const std::size_t column_count = static_cast<std::size_t>(columns);
    const std::size_t row_count = static_cast<std::size_t>(rows);
    if (column_count > maximum_cells / row_count) {
        return false;
    }
    range.cell_count = column_count * row_count;
    return range.cell_count <= maximum_cells;
}

[[nodiscard]] bool circle_circle_contact(
    Vector2 first_position,
    const CircleCollider2D& first,
    Vector2 second_position,
    const CircleCollider2D& second,
    Contact2D& contact) noexcept {
    const Vector2 difference = subtract(second_position, first_position);
    const double distance = length(difference);
    const double radius_sum = first.radius + second.radius;
    if (!is_finite(distance) || distance > radius_sum) {
        return false;
    }

    const Vector2 normal = distance > 1.0e-12
        ? multiply(difference, 1.0 / distance)
        : Vector2{1.0, 0.0};
    const double penetration = radius_sum - distance;
    contact.normal = normal;
    contact.penetration_depth = penetration;
    contact.point = add(
        first_position,
        multiply(normal, first.radius - (penetration * 0.5)));
    return true;
}

[[nodiscard]] bool box_box_contact(
    Vector2 first_position,
    const BoxCollider2D& first,
    Vector2 second_position,
    const BoxCollider2D& second,
    Contact2D& contact) noexcept {
    const Vector2 first_min = subtract(first_position, first.half_extents);
    const Vector2 first_max = add(first_position, first.half_extents);
    const Vector2 second_min = subtract(second_position, second.half_extents);
    const Vector2 second_max = add(second_position, second.half_extents);
    const double overlap_x =
        std::min(first_max.x, second_max.x) - std::max(first_min.x, second_min.x);
    const double overlap_y =
        std::min(first_max.y, second_max.y) - std::max(first_min.y, second_min.y);
    if (overlap_x < 0.0 || overlap_y < 0.0) {
        return false;
    }

    if (overlap_x <= overlap_y) {
        contact.normal = {second_position.x >= first_position.x ? 1.0 : -1.0, 0.0};
        contact.penetration_depth = overlap_x;
    } else {
        contact.normal = {0.0, second_position.y >= first_position.y ? 1.0 : -1.0};
        contact.penetration_depth = overlap_y;
    }

    const double contact_min_x = std::max(first_min.x, second_min.x);
    const double contact_max_x = std::min(first_max.x, second_max.x);
    const double contact_min_y = std::max(first_min.y, second_min.y);
    const double contact_max_y = std::min(first_max.y, second_max.y);
    contact.point = {
        (contact_min_x + contact_max_x) * 0.5,
        (contact_min_y + contact_max_y) * 0.5,
    };
    return true;
}

[[nodiscard]] bool circle_box_contact(
    Vector2 circle_position,
    const CircleCollider2D& circle,
    Vector2 box_position,
    const BoxCollider2D& box,
    Contact2D& contact) noexcept {
    const Vector2 box_min = subtract(box_position, box.half_extents);
    const Vector2 box_max = add(box_position, box.half_extents);
    const Vector2 closest{
        std::clamp(circle_position.x, box_min.x, box_max.x),
        std::clamp(circle_position.y, box_min.y, box_max.y),
    };
    const Vector2 difference = subtract(closest, circle_position);
    const double distance = length(difference);
    if (!is_finite(distance) || distance > circle.radius) {
        return false;
    }

    if (distance > 1.0e-12) {
        contact.normal = multiply(difference, 1.0 / distance);
        contact.penetration_depth = circle.radius - distance;
        contact.point = closest;
        return true;
    }

    // The circle center is inside or on the box. Select the nearest face with
    // a deterministic left, right, bottom, top tie-break order.
    const std::array<double, 4U> face_distances{
        circle_position.x - box_min.x,
        box_max.x - circle_position.x,
        circle_position.y - box_min.y,
        box_max.y - circle_position.y,
    };
    std::size_t nearest_face = 0U;
    for (std::size_t index = 1U; index < face_distances.size(); ++index) {
        if (face_distances[index] < face_distances[nearest_face]) {
            nearest_face = index;
        }
    }

    Vector2 outward{};
    switch (nearest_face) {
        case 0U: outward = {-1.0, 0.0}; break;
        case 1U: outward = {1.0, 0.0}; break;
        case 2U: outward = {0.0, -1.0}; break;
        default: outward = {0.0, 1.0}; break;
    }
    contact.normal = negate(outward);
    contact.penetration_depth = circle.radius + face_distances[nearest_face];
    contact.point = add(
        circle_position,
        multiply(outward, face_distances[nearest_face]));
    return true;
}

[[nodiscard]] bool detect_contact(
    const BodyDefinition2D& first_definition,
    Vector2 first_position,
    const BodyDefinition2D& second_definition,
    Vector2 second_position,
    Contact2D& contact) noexcept {
    const auto* first_circle = std::get_if<CircleCollider2D>(&first_definition.collider);
    const auto* first_box = std::get_if<BoxCollider2D>(&first_definition.collider);
    const auto* second_circle = std::get_if<CircleCollider2D>(&second_definition.collider);
    const auto* second_box = std::get_if<BoxCollider2D>(&second_definition.collider);

    if (first_circle != nullptr && second_circle != nullptr) {
        return circle_circle_contact(
            first_position, *first_circle, second_position, *second_circle, contact);
    }
    if (first_box != nullptr && second_box != nullptr) {
        return box_box_contact(
            first_position, *first_box, second_position, *second_box, contact);
    }
    if (first_circle != nullptr && second_box != nullptr) {
        return circle_box_contact(
            first_position, *first_circle, second_position, *second_box, contact);
    }
    if (first_box != nullptr && second_circle != nullptr) {
        const bool collided = circle_box_contact(
            second_position, *second_circle, first_position, *first_box, contact);
        if (collided) {
            contact.normal = negate(contact.normal);
        }
        return collided;
    }
    return false;
}


struct RayIntersection2D final {
    double distance{0.0};
    Vector2 normal{};
};

[[nodiscard]] bool raycast_circle(
    Vector2 origin,
    Vector2 direction,
    double maximum_distance,
    Vector2 center,
    double radius,
    RayIntersection2D& intersection) noexcept {
    if (!is_finite(origin) || !is_finite(direction) ||
        !is_finite(center) || !is_finite(radius) || radius <= 0.0) {
        return false;
    }

    const long double offset_x =
        static_cast<long double>(origin.x) - static_cast<long double>(center.x);
    const long double offset_y =
        static_cast<long double>(origin.y) - static_cast<long double>(center.y);
    const long double direction_x = static_cast<long double>(direction.x);
    const long double direction_y = static_cast<long double>(direction.y);
    const long double wide_radius = static_cast<long double>(radius);
    const long double c =
        (offset_x * offset_x) + (offset_y * offset_y) -
        (wide_radius * wide_radius);

    if (!std::isfinite(c)) {
        return false;
    }
    if (c <= 0.0L) {
        intersection.distance = 0.0;
        intersection.normal = negate(direction);
        return true;
    }

    const long double b =
        (offset_x * direction_x) + (offset_y * direction_y);
    const long double discriminant = (b * b) - c;
    if (!std::isfinite(discriminant) || discriminant < 0.0L) {
        return false;
    }

    const long double hit_distance = -b - std::sqrt(discriminant);
    if (!std::isfinite(hit_distance) ||
        hit_distance < 0.0L ||
        hit_distance > static_cast<long double>(maximum_distance)) {
        return false;
    }

    const long double hit_x = offset_x + (direction_x * hit_distance);
    const long double hit_y = offset_y + (direction_y * hit_distance);
    const long double normal_length = std::hypot(hit_x, hit_y);
    if (!std::isfinite(normal_length) || normal_length <= 0.0L) {
        return false;
    }

    intersection.distance = static_cast<double>(hit_distance);
    intersection.normal = {
        static_cast<double>(hit_x / normal_length),
        static_cast<double>(hit_y / normal_length),
    };
    return is_finite(intersection.distance) && is_finite(intersection.normal);
}

[[nodiscard]] bool raycast_box(
    Vector2 origin,
    Vector2 direction,
    double maximum_distance,
    Vector2 position,
    const BoxCollider2D& collider,
    RayIntersection2D& intersection) noexcept {
    BodyDefinition2D definition;
    definition.collider = collider;
    Bounds bounds;
    if (!calculate_bounds(definition, position, bounds)) {
        return false;
    }

    const bool origin_inside =
        origin.x >= bounds.minimum.x && origin.x <= bounds.maximum.x &&
        origin.y >= bounds.minimum.y && origin.y <= bounds.maximum.y;
    if (origin_inside) {
        intersection.distance = 0.0;
        intersection.normal = negate(direction);
        return true;
    }

    long double entry = -std::numeric_limits<long double>::infinity();
    long double exit = std::numeric_limits<long double>::infinity();
    Vector2 entry_normal{};

    const auto update_axis = [&entry, &exit, &entry_normal](
        double ray_origin,
        double ray_direction,
        double minimum,
        double maximum,
        Vector2 negative_normal,
        Vector2 positive_normal) noexcept {
        if (ray_direction == 0.0) {
            return ray_origin >= minimum && ray_origin <= maximum;
        }

        const long double first =
            (static_cast<long double>(minimum) -
             static_cast<long double>(ray_origin)) /
            static_cast<long double>(ray_direction);
        const long double second =
            (static_cast<long double>(maximum) -
             static_cast<long double>(ray_origin)) /
            static_cast<long double>(ray_direction);
        long double near_distance = first;
        long double far_distance = second;
        const Vector2 near_normal =
            ray_direction > 0.0 ? negative_normal : positive_normal;
        if (near_distance > far_distance) {
            std::swap(near_distance, far_distance);
        }

        if (near_distance > entry) {
            entry = near_distance;
            entry_normal = near_normal;
        }
        exit = std::min(exit, far_distance);
        return entry <= exit;
    };

    if (!update_axis(
            origin.x, direction.x,
            bounds.minimum.x, bounds.maximum.x,
            Vector2{-1.0, 0.0}, Vector2{1.0, 0.0}) ||
        !update_axis(
            origin.y, direction.y,
            bounds.minimum.y, bounds.maximum.y,
            Vector2{0.0, -1.0}, Vector2{0.0, 1.0})) {
        return false;
    }

    if (!std::isfinite(entry) || !std::isfinite(exit) ||
        exit < 0.0L || entry < 0.0L ||
        entry > static_cast<long double>(maximum_distance)) {
        return false;
    }

    intersection.distance = static_cast<double>(entry);
    intersection.normal = entry_normal;
    return is_finite(intersection.distance) && is_finite(intersection.normal);
}

[[nodiscard]] Error invalid_handle_error() {
    return Error::make(
        ErrorCode::InvalidState,
        "The 2D body handle is stale, foreign, or does not identify a live body");
}

[[nodiscard]] Error budget_error(const char* message) {
    return Error::make(ErrorCode::ResourceExhausted, message);
}

} // namespace

PhysicsWorld2D::PhysicsWorld2D(
    PhysicsWorld2DConfig config,
    std::uint64_t world_id) noexcept
    : config_(config),
      world_id_(world_id) {}

PhysicsWorld2D::PhysicsWorld2D(PhysicsWorld2D&& other) noexcept
    : config_(other.config_),
      world_id_(std::exchange(other.world_id_, 0U)),
      body_count_(std::exchange(other.body_count_, 0U)),
      slots_(std::move(other.slots_)),
      free_slots_(std::move(other.free_slots_)),
      previous_contacts_(std::move(other.previous_contacts_)) {}

Result<PhysicsWorld2D> PhysicsWorld2D::create(PhysicsWorld2DConfig config) {
    if (config.maximum_bodies == 0U ||
        config.maximum_bodies > std::numeric_limits<std::uint32_t>::max() ||
        config.maximum_contacts_per_step == 0U ||
        config.maximum_broadphase_entries == 0U ||
        config.maximum_candidate_pairs == 0U ||
        config.maximum_cells_per_body == 0U ||
        !is_finite(config.broadphase_cell_size) ||
        config.broadphase_cell_size <= 0.0 ||
        !is_finite(config.maximum_step_seconds) ||
        config.maximum_step_seconds <= 0.0 ||
        !is_finite(config.maximum_absolute_coordinate) ||
        config.maximum_absolute_coordinate <= 0.0) {
        return Result<PhysicsWorld2D>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "PhysicsWorld2D requires positive finite simulation and broadphase limits"));
    }

    const std::uint64_t id = allocate_world_id();
    if (id == 0U) {
        return Result<PhysicsWorld2D>::failure(budget_error(
            "The process exhausted unique PhysicsWorld2D identities"));
    }
    return Result<PhysicsWorld2D>::success(PhysicsWorld2D(config, id));
}

BodyHandle2D PhysicsWorld2D::make_handle(
    std::uint32_t slot_index,
    const Slot& slot) const noexcept {
    return BodyHandle2D{world_id_, slot_index, slot.generation};
}

PhysicsWorld2D::Slot* PhysicsWorld2D::find_slot(BodyHandle2D handle) noexcept {
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

const PhysicsWorld2D::Slot* PhysicsWorld2D::find_slot(
    BodyHandle2D handle) const noexcept {
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

bool PhysicsWorld2D::within_coordinate_limit(Vector2 value) const noexcept {
    return within_limit(value, config_.maximum_absolute_coordinate);
}

Result<BodyHandle2D> PhysicsWorld2D::create_body(
    const BodyDefinition2D& definition) {
    if (world_id_ == 0U) {
        return Result<BodyHandle2D>::failure(Error::make(
            ErrorCode::InvalidState,
            "A moved-from PhysicsWorld2D cannot create bodies"));
    }

    const std::optional<Error> validation_error =
        validate_definition(definition, config_.maximum_absolute_coordinate);
    if (validation_error.has_value()) {
        return Result<BodyHandle2D>::failure(*validation_error);
    }
    if (body_count_ >= config_.maximum_bodies) {
        return Result<BodyHandle2D>::failure(budget_error(
            "PhysicsWorld2D reached its configured body limit"));
    }

    try {
        std::uint32_t index = 0U;
        const bool reuse_slot = !free_slots_.empty();
        if (reuse_slot) {
            index = free_slots_.back();
        } else {
            if (slots_.size() >= config_.maximum_bodies ||
                slots_.size() >= std::numeric_limits<std::uint32_t>::max()) {
                return Result<BodyHandle2D>::failure(budget_error(
                    "PhysicsWorld2D has no reusable body slots"));
            }
            slots_.emplace_back();
            index = static_cast<std::uint32_t>(slots_.size() - 1U);
        }

        Slot& slot = slots_[index];
        slot.definition = definition;
        slot.position = definition.position;
        slot.velocity = definition.velocity;
        slot.accumulated_force = {};
        slot.alive = true;
        if (reuse_slot) {
            free_slots_.pop_back();
        }
        ++body_count_;
        return Result<BodyHandle2D>::success(make_handle(index, slot));
    } catch (const std::bad_alloc&) {
        return Result<BodyHandle2D>::failure(budget_error(
            "Could not allocate storage for a 2D physics body"));
    }
}

Result<void> PhysicsWorld2D::destroy_body(BodyHandle2D handle) {
    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }

    if (slot->generation != std::numeric_limits<std::uint32_t>::max()) {
        try {
            free_slots_.push_back(handle.slot_index);
        } catch (const std::bad_alloc&) {
            return Result<void>::failure(budget_error(
                "Could not allocate a reusable 2D body slot"));
        }
        ++slot->generation;
    }

    slot->alive = false;
    slot->accumulated_force = {};
    --body_count_;
    return Result<void>::success();
}

Result<void> PhysicsWorld2D::set_position(
    BodyHandle2D handle,
    Vector2 position) {
    if (!within_coordinate_limit(position)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "A 2D body position must be finite and within the configured coordinate limit"));
    }
    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }
    slot->position = position;
    return Result<void>::success();
}

Result<void> PhysicsWorld2D::set_velocity(
    BodyHandle2D handle,
    Vector2 velocity) {
    if (!within_coordinate_limit(velocity)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "A 2D body velocity must be finite and within the configured state limit"));
    }
    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }
    if (slot->definition.type == BodyType2D::Static && velocity != Vector2{}) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "A static 2D body cannot have non-zero velocity"));
    }
    slot->velocity = velocity;
    return Result<void>::success();
}

Result<void> PhysicsWorld2D::apply_force(BodyHandle2D handle, Vector2 force) {
    if (!is_finite(force)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "An applied 2D force must be finite"));
    }
    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }
    if (slot->definition.type != BodyType2D::Dynamic) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState,
            "Forces can only be accumulated on dynamic 2D bodies"));
    }

    const Vector2 combined = add(slot->accumulated_force, force);
    if (!is_finite(combined)) {
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Accumulated 2D force exceeded the supported numeric range"));
    }
    slot->accumulated_force = combined;
    return Result<void>::success();
}

Result<void> PhysicsWorld2D::apply_impulse(
    BodyHandle2D handle,
    Vector2 impulse) {
    if (!is_finite(impulse)) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "An applied 2D impulse must be finite"));
    }
    Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<void>::failure(invalid_handle_error());
    }
    if (slot->definition.type != BodyType2D::Dynamic) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState,
            "Impulses can only be applied to dynamic 2D bodies"));
    }

    const Vector2 next_velocity = add(
        slot->velocity,
        multiply(impulse, 1.0 / slot->definition.mass));
    if (!within_coordinate_limit(next_velocity)) {
        return Result<void>::failure(budget_error(
            "A 2D impulse produced a velocity outside the configured state limit"));
    }
    slot->velocity = next_velocity;
    return Result<void>::success();
}

Result<BodySnapshot2D> PhysicsWorld2D::body_snapshot(BodyHandle2D handle) const {
    const Slot* slot = find_slot(handle);
    if (slot == nullptr) {
        return Result<BodySnapshot2D>::failure(invalid_handle_error());
    }

    return Result<BodySnapshot2D>::success(BodySnapshot2D{
        .handle = handle,
        .type = slot->definition.type,
        .position = slot->position,
        .velocity = slot->velocity,
        .collider = slot->definition.collider,
        .mass = slot->definition.mass,
        .restitution = slot->definition.restitution,
        .friction = slot->definition.friction,
        .accumulated_force = slot->accumulated_force,
        .is_trigger = slot->definition.is_trigger,
        .collision_layer = slot->definition.collision_layer,
        .collision_mask = slot->definition.collision_mask,
    });
}


Result<std::vector<BodyHandle2D>> PhysicsWorld2D::overlap_query(
    const OverlapQuery2D& query) const {
    if (world_id_ == 0U) {
        return Result<std::vector<BodyHandle2D>>::failure(Error::make(
            ErrorCode::InvalidState,
            "A moved-from PhysicsWorld2D cannot execute overlap queries"));
    }

    const std::optional<Vector2> query_extents =
        collider_half_extents(query.collider);
    if (!within_coordinate_limit(query.position) ||
        !query_extents.has_value() ||
        !within_coordinate_limit(*query_extents)) {
        return Result<std::vector<BodyHandle2D>>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "A 2D overlap query requires a valid collider and finite position within configured limits"));
    }

    BodyDefinition2D query_definition;
    query_definition.collider = query.collider;
    Bounds query_bounds;
    if (!calculate_bounds(query_definition, query.position, query_bounds)) {
        return Result<std::vector<BodyHandle2D>>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "The 2D overlap query bounds exceed the supported numeric range"));
    }

    std::vector<BodyHandle2D> matches;
    if (query.collision_mask == 0U || body_count_ == 0U) {
        return Result<std::vector<BodyHandle2D>>::success(std::move(matches));
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

            Bounds body_bounds;
            if (!calculate_bounds(slot.definition, slot.position, body_bounds)) {
                return Result<std::vector<BodyHandle2D>>::failure(Error::make(
                    ErrorCode::ResourceExhausted,
                    "A stored 2D collider bound exceeds the supported numeric range"));
            }
            if (query_bounds.maximum.x < body_bounds.minimum.x ||
                body_bounds.maximum.x < query_bounds.minimum.x ||
                query_bounds.maximum.y < body_bounds.minimum.y ||
                body_bounds.maximum.y < query_bounds.minimum.y) {
                continue;
            }

            Contact2D ignored_contact;
            if (detect_contact(
                    query_definition, query.position,
                    slot.definition, slot.position,
                    ignored_contact)) {
                matches.push_back(make_handle(
                    static_cast<std::uint32_t>(index), slot));
            }
        }
        return Result<std::vector<BodyHandle2D>>::success(std::move(matches));
    } catch (const std::bad_alloc&) {
        return Result<std::vector<BodyHandle2D>>::failure(budget_error(
            "Could not allocate results for a 2D overlap query"));
    }
}

Result<std::optional<RaycastHit2D>> PhysicsWorld2D::raycast(
    const RaycastQuery2D& query) const {
    if (world_id_ == 0U) {
        return Result<std::optional<RaycastHit2D>>::failure(Error::make(
            ErrorCode::InvalidState,
            "A moved-from PhysicsWorld2D cannot execute raycasts"));
    }
    if (!within_coordinate_limit(query.origin) ||
        !is_finite(query.direction) ||
        !is_finite(query.maximum_distance) ||
        query.maximum_distance < 0.0) {
        return Result<std::optional<RaycastHit2D>>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "A 2D raycast requires a finite origin, direction, and non-negative finite maximum distance"));
    }

    const double direction_length = length(query.direction);
    if (!is_finite(direction_length) || direction_length <= 0.0) {
        return Result<std::optional<RaycastHit2D>>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "A 2D raycast direction must be non-zero and have a finite length"));
    }
    const Vector2 direction = multiply(query.direction, 1.0 / direction_length);
    const Vector2 ray_end = add(
        query.origin,
        multiply(direction, query.maximum_distance));
    if (!is_finite(direction) || !within_coordinate_limit(ray_end)) {
        return Result<std::optional<RaycastHit2D>>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "The 2D ray endpoint exceeds the configured coordinate range"));
    }

    if (query.collision_mask == 0U || body_count_ == 0U) {
        return Result<std::optional<RaycastHit2D>>::success(std::nullopt);
    }

    std::optional<RaycastHit2D> nearest;
    for (std::size_t index = 0U; index < slots_.size(); ++index) {
        const Slot& slot = slots_[index];
        if (!slot.alive ||
            (!query.include_triggers && slot.definition.is_trigger) ||
            (query.collision_mask & slot.definition.collision_layer) == 0U) {
            continue;
        }

        RayIntersection2D intersection;
        bool hit = false;
        if (const auto* circle =
                std::get_if<CircleCollider2D>(&slot.definition.collider)) {
            hit = raycast_circle(
                query.origin, direction, query.maximum_distance,
                slot.position, circle->radius, intersection);
        } else if (const auto* box =
                       std::get_if<BoxCollider2D>(&slot.definition.collider)) {
            hit = raycast_box(
                query.origin, direction, query.maximum_distance,
                slot.position, *box, intersection);
        } else {
            return Result<std::optional<RaycastHit2D>>::failure(Error::make(
                ErrorCode::InvalidState,
                "A stored 2D body has an unsupported collider state"));
        }

        if (!hit ||
            (nearest.has_value() &&
             intersection.distance >= nearest->distance)) {
            continue;
        }

        const Vector2 point = add(
            query.origin,
            multiply(direction, intersection.distance));
        if (!is_finite(point) ||
            !is_finite(intersection.normal) ||
            !is_finite(intersection.distance)) {
            return Result<std::optional<RaycastHit2D>>::failure(budget_error(
                "A 2D raycast hit exceeds the supported numeric range"));
        }

        nearest = RaycastHit2D{
            .body = make_handle(static_cast<std::uint32_t>(index), slot),
            .point = point,
            .normal = intersection.normal,
            .distance = intersection.distance,
            .is_trigger = slot.definition.is_trigger,
        };
    }

    return Result<std::optional<RaycastHit2D>>::success(std::move(nearest));
}

Result<StepResult2D> PhysicsWorld2D::step(
    double delta_seconds,
    Vector2 gravity) {
    if (world_id_ == 0U) {
        return Result<StepResult2D>::failure(Error::make(
            ErrorCode::InvalidState,
            "A moved-from PhysicsWorld2D cannot be stepped"));
    }
    if (!is_finite(delta_seconds) ||
        delta_seconds <= 0.0 ||
        delta_seconds > config_.maximum_step_seconds ||
        !within_coordinate_limit(gravity)) {
        return Result<StepResult2D>::failure(Error::make(
            ErrorCode::InvalidArgument,
            "A 2D physics step requires a finite positive bounded delta and finite bounded gravity"));
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

            if (body.definition.type == BodyType2D::Dynamic) {
                const Vector2 acceleration = add(
                    gravity,
                    multiply(body.accumulated_force, 1.0 / body.definition.mass));
                body.velocity = add(
                    body.velocity,
                    multiply(acceleration, delta_seconds));
                body.position = add(
                    body.position,
                    multiply(body.velocity, delta_seconds));
            } else if (body.definition.type == BodyType2D::Kinematic) {
                body.position = add(
                    body.position,
                    multiply(body.velocity, delta_seconds));
            }
            body.accumulated_force = {};

            if (!within_coordinate_limit(body.position) ||
                !within_coordinate_limit(body.velocity)) {
                return Result<StepResult2D>::failure(budget_error(
                    "2D physics integration exceeded the configured state range"));
            }
        }

        std::unordered_map<
            CellCoordinate,
            std::vector<std::uint32_t>,
            CellCoordinateHash> cells;
        std::size_t broadphase_entries = 0U;

        for (std::size_t index = 0U; index < working.size(); ++index) {
            const WorkingBody& body = working[index];
            if (!body.active) {
                continue;
            }

            Bounds bounds;
            CellRange range;
            if (!calculate_bounds(body.definition, body.position, bounds) ||
                !calculate_cell_range(
                    bounds,
                    config_.broadphase_cell_size,
                    config_.maximum_cells_per_body,
                    range)) {
                return Result<StepResult2D>::failure(budget_error(
                    "A 2D collider exceeds the configured spatial-hash cell range"));
            }
            if (range.cell_count > config_.maximum_broadphase_entries -
                    std::min(broadphase_entries, config_.maximum_broadphase_entries)) {
                return Result<StepResult2D>::failure(budget_error(
                    "2D broadphase exceeded its configured cell-entry budget"));
            }

            for (std::int64_t x = range.minimum_x;;) {
                for (std::int64_t y = range.minimum_y;;) {
                    if (broadphase_entries >= config_.maximum_broadphase_entries) {
                        return Result<StepResult2D>::failure(budget_error(
                            "2D broadphase exceeded its configured cell-entry budget"));
                    }
                    cells[CellCoordinate{x, y}].push_back(
                        static_cast<std::uint32_t>(index));
                    ++broadphase_entries;
                    if (y == range.maximum_y) {
                        break;
                    }
                    ++y;
                }
                if (x == range.maximum_x) {
                    break;
                }
                ++x;
            }
        }

        std::vector<CandidatePair> candidates;
        for (const auto& [cell, occupants] : cells) {
            static_cast<void>(cell);
            for (std::size_t first_index = 0U;
                 first_index < occupants.size();
                 ++first_index) {
                for (std::size_t second_index = first_index + 1U;
                     second_index < occupants.size();
                     ++second_index) {
                    if (candidates.size() >= config_.maximum_candidate_pairs) {
                        return Result<StepResult2D>::failure(budget_error(
                            "2D broadphase exceeded its configured candidate-pair budget"));
                    }
                    candidates.push_back(CandidatePair{
                        .first = occupants[first_index],
                        .second = occupants[second_index],
                    });
                }
            }
        }

        const auto pair_less = [](const CandidatePair& first,
                                  const CandidatePair& second) noexcept {
            if (first.first != second.first) {
                return first.first < second.first;
            }
            return first.second < second.second;
        };
        std::sort(candidates.begin(), candidates.end(), pair_less);
        candidates.erase(
            std::unique(candidates.begin(), candidates.end()),
            candidates.end());

        StepResult2D result;
        result.delta_seconds = delta_seconds;
        result.broadphase_candidate_pairs = candidates.size();
        result.contacts.reserve(std::min(
            config_.maximum_contacts_per_step,
            candidates.size()));

        for (const CandidatePair& pair : candidates) {
            WorkingBody& first = working[pair.first];
            WorkingBody& second = working[pair.second];
            if ((first.definition.collision_layer &
                    second.definition.collision_mask) == 0U ||
                (second.definition.collision_layer &
                    first.definition.collision_mask) == 0U) {
                continue;
            }

            Contact2D contact;
            if (!detect_contact(first.definition, first.position, second.definition, second.position, contact)) {
                continue;
            }
            if (result.contacts.size() >= config_.maximum_contacts_per_step) {
                return Result<StepResult2D>::failure(budget_error(
                    "PhysicsWorld2D exceeded its configured contact limit"));
            }

            contact.first = make_handle(pair.first, slots_[pair.first]);
            contact.second = make_handle(pair.second, slots_[pair.second]);
            contact.is_trigger =
                first.definition.is_trigger || second.definition.is_trigger;
            result.contacts.push_back(contact);

            if (contact.is_trigger) {
                continue;
            }

            const double first_inverse_mass =
                first.definition.type == BodyType2D::Dynamic
                    ? 1.0 / first.definition.mass
                    : 0.0;
            const double second_inverse_mass =
                second.definition.type == BodyType2D::Dynamic
                    ? 1.0 / second.definition.mass
                    : 0.0;
            const double inverse_mass_sum =
                first_inverse_mass + second_inverse_mass;
            if (inverse_mass_sum <= 0.0) {
                continue;
            }

            const Vector2 relative_velocity =
                subtract(second.velocity, first.velocity);
            const double normal_velocity = dot(relative_velocity, contact.normal);
            if (normal_velocity < 0.0) {
                const double restitution = std::min(
                    first.definition.restitution,
                    second.definition.restitution);
                const double impulse_magnitude =
                    (-(1.0 + restitution) * normal_velocity) /
                    inverse_mass_sum;
                const Vector2 impulse = multiply(
                    contact.normal, impulse_magnitude);
                first.velocity = subtract(
                    first.velocity,
                    multiply(impulse, first_inverse_mass));
                second.velocity = add(
                    second.velocity,
                    multiply(impulse, second_inverse_mass));

                Vector2 tangent = subtract(
                    relative_velocity,
                    multiply(contact.normal, normal_velocity));
                const double tangent_length = length(tangent);
                if (tangent_length > 1.0e-12) {
                    tangent = multiply(tangent, 1.0 / tangent_length);
                    double tangent_impulse =
                        -dot(relative_velocity, tangent) / inverse_mass_sum;
                    const double friction = std::sqrt(
                        first.definition.friction * second.definition.friction);
                    const double maximum_tangent_impulse =
                        impulse_magnitude * friction;
                    tangent_impulse = std::clamp(
                        tangent_impulse,
                        -maximum_tangent_impulse,
                        maximum_tangent_impulse);
                    const Vector2 friction_impulse =
                        multiply(tangent, tangent_impulse);
                    first.velocity = subtract(
                        first.velocity,
                        multiply(friction_impulse, first_inverse_mass));
                    second.velocity = add(
                        second.velocity,
                        multiply(friction_impulse, second_inverse_mass));
                }
            }

            constexpr double kPositionSlop = 0.005;
            constexpr double kPositionCorrectionPercent = 0.8;
            const double correction_depth = std::max(
                contact.penetration_depth - kPositionSlop, 0.0);
            if (correction_depth > 0.0) {
                const double correction_magnitude =
                    (correction_depth * kPositionCorrectionPercent) /
                    inverse_mass_sum;
                const Vector2 correction =
                    multiply(contact.normal, correction_magnitude);
                first.position = subtract(
                    first.position,
                    multiply(correction, first_inverse_mass));
                second.position = add(
                    second.position,
                    multiply(correction, second_inverse_mass));
            }

            if (!within_coordinate_limit(first.position) ||
                !within_coordinate_limit(second.position) ||
                !within_coordinate_limit(first.velocity) ||
                !within_coordinate_limit(second.velocity)) {
                return Result<StepResult2D>::failure(budget_error(
                    "2D collision resolution exceeded the configured state range"));
            }
        }

        // Merge the current and previous slot-ordered contact lists. A reused
        // slot with a new generation is an End for the old handle followed by
        // a Begin for the new handle, never a false Stay.
        const auto contact_pair_less = [](const Contact2D& left,
                                  const Contact2D& right) noexcept {
            if (left.first.slot_index != right.first.slot_index) {
                return left.first.slot_index < right.first.slot_index;
            }
            return left.second.slot_index < right.second.slot_index;
        };
        const auto append_event = [&result](
                                      ContactPhase2D phase,
                                      const Contact2D& contact) {
            result.contact_events.push_back(ContactEvent2D{
                .phase = phase,
                .contact = contact,
            });
        };

        const std::size_t maximum_event_count = result.contact_events.max_size();
        if (result.contacts.size() > maximum_event_count ||
            previous_contacts_.size() > maximum_event_count - result.contacts.size()) {
            return Result<StepResult2D>::failure(budget_error(
                "2D contact transition count exceeds the supported result capacity"));
        }
        result.contact_events.reserve(
            result.contacts.size() + previous_contacts_.size());

        std::size_t current_index = 0U;
        std::size_t previous_index = 0U;
        while (current_index < result.contacts.size() &&
               previous_index < previous_contacts_.size()) {
            const Contact2D& current = result.contacts[current_index];
            const Contact2D& previous = previous_contacts_[previous_index];

            if (contact_pair_less(current, previous)) {
                append_event(ContactPhase2D::Begin, current);
                ++current_index;
                continue;
            }
            if (contact_pair_less(previous, current)) {
                append_event(ContactPhase2D::End, previous);
                ++previous_index;
                continue;
            }

            if (current.first == previous.first &&
                current.second == previous.second) {
                append_event(ContactPhase2D::Stay, current);
            } else {
                append_event(ContactPhase2D::End, previous);
                append_event(ContactPhase2D::Begin, current);
            }
            ++current_index;
            ++previous_index;
        }

        while (current_index < result.contacts.size()) {
            append_event(ContactPhase2D::Begin, result.contacts[current_index]);
            ++current_index;
        }
        while (previous_index < previous_contacts_.size()) {
            append_event(ContactPhase2D::End, previous_contacts_[previous_index]);
            ++previous_index;
        }

        // Prepare persistent history before mutating body state. Allocation
        // failure must leave both the simulation and contact history unchanged.
        std::vector<Contact2D> next_previous_contacts = result.contacts;

        // Commit only after simulation, transition generation, and all budgets
        // have succeeded. Swaps are non-throwing, then body state is committed.
        previous_contacts_.swap(next_previous_contacts);
        for (std::size_t index = 0U; index < slots_.size(); ++index) {
            if (!slots_[index].alive) {
                continue;
            }
            slots_[index].position = working[index].position;
            slots_[index].velocity = working[index].velocity;
            slots_[index].accumulated_force = working[index].accumulated_force;
        }

        return Result<StepResult2D>::success(std::move(result));
    } catch (const std::bad_alloc&) {
        return Result<StepResult2D>::failure(budget_error(
            "Could not allocate temporary state for a transactional 2D physics step"));
    }
}

std::size_t PhysicsWorld2D::body_count() const noexcept {
    return body_count_;
}

std::uint64_t PhysicsWorld2D::world_id() const noexcept {
    return world_id_;
}

} // namespace Storm::Physics::TwoD
