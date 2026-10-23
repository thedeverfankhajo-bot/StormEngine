#include "storm/core/diagnostic_buffer.hpp"
#include "storm/core/fixed_step_clock.hpp"
#include "storm/core/logging.hpp"
#include "storm/core/runtime_config.hpp"
#include "storm/core/runtime_lifecycle.hpp"
#include "storm/core/stable_id.hpp"
#include "storm/core/stable_id_generator.hpp"
#include "storm/core/stable_id_registry.hpp"
#include "storm/core/status.hpp"
#include "storm/core/version.hpp"
#include "storm/math/vector.hpp"
#include "storm/platform/input_state.hpp"
#include "storm/platform/event_queue.hpp"
#include "storm/scene/dimension.hpp"
#include "storm/scene/scene.hpp"
#include "storm/scene/transform.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, std::string_view expression, int line) {
    if (!condition) {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        ++failures;
    }
}

#define STORM_CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

[[nodiscard]] bool nearly_equal(double left, double right, double tolerance = 1.0e-9) {
    return std::abs(left - right) <= tolerance;
}

struct LogCapture final {
    unsigned int count{};
    storm::core::LogLevel last_level{storm::core::LogLevel::Info};
    std::string last_category{};
    std::string last_message{};
};

void capture_log(void* user_data, const storm::core::LogRecord& record) noexcept {
    if (user_data == nullptr) {
        return;
    }

    auto& capture = *static_cast<LogCapture*>(user_data);
    ++capture.count;
    capture.last_level = record.level;
    capture.last_category.assign(record.category);
    capture.last_message.assign(record.message);
}

struct EntityVisitCapture final {
    std::array<storm::core::StableId, 4U> ids{};
    std::size_t count{};
};

void capture_entity_id(void* user_data, const storm::scene::Entity& entity) noexcept {
    if (user_data == nullptr) {
        return;
    }

    auto& capture = *static_cast<EntityVisitCapture*>(user_data);
    if (capture.count < capture.ids.size()) {
        capture.ids[capture.count] = entity.id();
        ++capture.count;
    }
}

void test_logging_dispatch_and_filtering() {
    using storm::core::LogLevel;
    using storm::core::Logger;
    using storm::core::is_valid_log_level;
    using storm::core::log_level_name;

    STORM_CHECK(log_level_name(LogLevel::Trace) == "Trace");
    STORM_CHECK(log_level_name(LogLevel::Off) == "Off");
    STORM_CHECK(log_level_name(static_cast<LogLevel>(255)) == "Unknown");
    STORM_CHECK(is_valid_log_level(LogLevel::Critical));
    STORM_CHECK(!is_valid_log_level(static_cast<LogLevel>(255)));

    LogCapture capture;
    Logger logger{capture_log, &capture};
    STORM_CHECK(logger.minimum_level() == LogLevel::Info);
    STORM_CHECK(logger.is_enabled(LogLevel::Info));
    STORM_CHECK(!logger.is_enabled(LogLevel::Debug));

    logger.write(LogLevel::Info, "core", "runtime ready");
    STORM_CHECK(capture.count == 1U);
    STORM_CHECK(capture.last_level == LogLevel::Info);
    STORM_CHECK(capture.last_category == "core");
    STORM_CHECK(capture.last_message == "runtime ready");

    STORM_CHECK(logger.set_minimum_level(LogLevel::Warning));
    logger.write(LogLevel::Info, "core", "filtered message");
    logger.write(LogLevel::Warning, "platform", "surface unavailable");
    STORM_CHECK(capture.count == 2U);
    STORM_CHECK(capture.last_level == LogLevel::Warning);
    STORM_CHECK(capture.last_category == "platform");
    STORM_CHECK(capture.last_message == "surface unavailable");

    const auto previous_minimum = logger.minimum_level();
    STORM_CHECK(!logger.set_minimum_level(static_cast<LogLevel>(255)));
    STORM_CHECK(logger.minimum_level() == previous_minimum);

    STORM_CHECK(logger.set_minimum_level(LogLevel::Off));
    logger.write(LogLevel::Critical, "core", "disabled");
    STORM_CHECK(capture.count == 2U);
    STORM_CHECK(!logger.is_enabled(LogLevel::Critical));

    logger.set_sink(nullptr);
    STORM_CHECK(!logger.is_enabled(LogLevel::Critical));
    logger.write(LogLevel::Critical, "core", "no sink");
    STORM_CHECK(capture.count == 2U);

    Logger sinkless_logger;
    STORM_CHECK(!sinkless_logger.is_enabled(LogLevel::Error));
    sinkless_logger.write(LogLevel::Error, "core", "no output sink");
}

void test_stable_id_validation_and_round_trip() {
    using storm::core::StableId;

    constexpr std::string_view canonical = "f81d4fae-7dec-11d0-a765-00a0c91e6bf6";
    const auto parsed = StableId::parse(canonical);
    STORM_CHECK(parsed.has_value());
    if (!parsed.has_value()) {
        return;
    }

    STORM_CHECK(parsed->is_valid());
    STORM_CHECK(static_cast<bool>(*parsed));
    STORM_CHECK(parsed->to_string() == canonical);

    const auto uppercase = StableId::parse("F81D4FAE-7DEC-11D0-A765-00A0C91E6BF6");
    STORM_CHECK(uppercase.has_value());
    if (uppercase.has_value()) {
        STORM_CHECK(*uppercase == *parsed);
        STORM_CHECK(uppercase->to_string() == canonical);
    }

    const auto different = StableId::parse("f81d4fae-7dec-11d0-a765-00a0c91e6bf7");
    STORM_CHECK(different.has_value());
    if (different.has_value()) {
        STORM_CHECK(*different != *parsed);
    }

    const auto from_bytes = StableId::from_bytes(parsed->bytes());
    STORM_CHECK(from_bytes.has_value());
    if (from_bytes.has_value()) {
        STORM_CHECK(*from_bytes == *parsed);
        STORM_CHECK(from_bytes->bytes()[0] == 0xF8U);
        STORM_CHECK(from_bytes->bytes()[15] == 0xF6U);
    }

    const StableId invalid;
    STORM_CHECK(!invalid.is_valid());
    STORM_CHECK(!static_cast<bool>(invalid));
    STORM_CHECK(invalid.to_string().empty());
    STORM_CHECK(!StableId::from_bytes(StableId::Bytes{}).has_value());

    STORM_CHECK(!StableId::parse("").has_value());
    STORM_CHECK(!StableId::parse("f81d4fae7dec-11d0-a765-00a0c91e6bf6").has_value());
    STORM_CHECK(!StableId::parse("f81d4fae-7dec-11d0-a765-00a0c91e6bf").has_value());
    STORM_CHECK(!StableId::parse("f81d4fae-7dec-11d0-a765-00a0c91e6bfz").has_value());
    STORM_CHECK(!StableId::parse("f81d4fae_7dec-11d0-a765-00a0c91e6bf6").has_value());
    STORM_CHECK(!StableId::parse("00000000-0000-0000-0000-000000000000").has_value());
}

void test_runtime_config_validation() {
    using storm::core::ErrorCode;
    using storm::core::LogLevel;
    using storm::core::RuntimeConfig;

    RuntimeConfig config;
    STORM_CHECK(config.validate().ok());
    STORM_CHECK(config.minimum_log_level == LogLevel::Info);
    STORM_CHECK(config.simulation.max_steps_per_frame == 8U);

    config.minimum_log_level = LogLevel::Off;
    STORM_CHECK(config.validate().ok());

    config.minimum_log_level = static_cast<LogLevel>(255);
    const auto invalid_log_level = config.validate();
    STORM_CHECK(!invalid_log_level.ok());
    STORM_CHECK(invalid_log_level.code() == ErrorCode::InvalidArgument);

    config.minimum_log_level = LogLevel::Warning;
    config.simulation.fixed_delta_seconds = 0.0;
    const auto invalid_fixed_delta = config.validate();
    STORM_CHECK(!invalid_fixed_delta.ok());
    STORM_CHECK(invalid_fixed_delta.code() == ErrorCode::InvalidArgument);

    config.simulation.fixed_delta_seconds = 1.0 / 60.0;
    config.simulation.max_frame_delta_seconds = 0.001;
    STORM_CHECK(config.validate().code() == ErrorCode::InvalidArgument);

    config.simulation.max_frame_delta_seconds = 0.25;
    config.simulation.max_steps_per_frame = 0U;
    STORM_CHECK(config.validate().code() == ErrorCode::InvalidArgument);

    config.simulation.max_steps_per_frame = 8U;
    config.minimum_log_level = LogLevel::Debug;
    STORM_CHECK(config.validate().ok());
}

void test_stable_id_registry_duplicate_detection() {
    using storm::core::ErrorCode;
    using storm::core::StableId;
    using storm::core::StableIdRegistry;

    StableIdRegistry registry;
    STORM_CHECK(registry.empty());
    STORM_CHECK(registry.size() == 0U);

    const auto first = StableId::parse("f81d4fae-7dec-11d0-a765-00a0c91e6bf6");
    const auto second = StableId::parse("f81d4fae-7dec-11d0-a765-00a0c91e6bf7");
    STORM_CHECK(first.has_value());
    STORM_CHECK(second.has_value());
    if (!first.has_value() || !second.has_value()) {
        return;
    }

    STORM_CHECK(registry.register_id(*first).ok());
    STORM_CHECK(registry.contains(*first));
    STORM_CHECK(!registry.contains(*second));
    STORM_CHECK(registry.size() == 1U);

    const auto duplicate = registry.register_id(*first);
    STORM_CHECK(!duplicate.ok());
    STORM_CHECK(duplicate.code() == ErrorCode::AlreadyExists);
    STORM_CHECK(registry.size() == 1U);

    const auto missing = registry.unregister_id(*second);
    STORM_CHECK(!missing.ok());
    STORM_CHECK(missing.code() == ErrorCode::NotFound);

    const StableId invalid;
    STORM_CHECK(registry.register_id(invalid).code() == ErrorCode::InvalidArgument);
    STORM_CHECK(registry.unregister_id(invalid).code() == ErrorCode::InvalidArgument);
    STORM_CHECK(registry.size() == 1U);

    STORM_CHECK(registry.register_id(*second).ok());
    STORM_CHECK(registry.size() == 2U);
    STORM_CHECK(registry.unregister_id(*first).ok());
    STORM_CHECK(!registry.contains(*first));
    STORM_CHECK(registry.contains(*second));

    registry.clear();
    STORM_CHECK(registry.empty());
    STORM_CHECK(registry.size() == 0U);
}

void test_secure_stable_id_generation() {
    using storm::core::StableId;
    using storm::core::StableIdGenerator;
    using storm::core::StableIdRegistry;

    StableIdRegistry registry;
    for (std::size_t index = 0U; index < 8U; ++index) {
        StableId generated;
        const auto status = StableIdGenerator::generate_v4(generated);
        STORM_CHECK(status.ok());
        if (!status.ok()) {
            return;
        }

        STORM_CHECK(generated.is_valid());
        const auto& bytes = generated.bytes();
        STORM_CHECK((bytes[6] & 0xF0U) == 0x40U);
        STORM_CHECK((bytes[8] & 0xC0U) == 0x80U);

        const std::string text = generated.to_string();
        STORM_CHECK(text.size() == StableId::text_length);
        const auto reparsed = StableId::parse(text);
        STORM_CHECK(reparsed.has_value());
        if (reparsed.has_value()) {
            STORM_CHECK(*reparsed == generated);
        }

        STORM_CHECK(registry.register_id(generated).ok());
    }

    STORM_CHECK(registry.size() == 8U);
}

void test_diagnostic_buffer_bounded_storage_and_counters() {
    using storm::core::DiagnosticBuffer;
    using storm::core::LogLevel;
    using storm::core::Logger;
    using storm::core::LogRecord;

    using SmallBuffer = DiagnosticBuffer<2U, 4U, 8U>;
    SmallBuffer buffer;
    STORM_CHECK(buffer.empty());
    STORM_CHECK(buffer.max_size() == 2U);
    STORM_CHECK(buffer.size() == 0U);

    Logger logger{SmallBuffer::capture_sink, &buffer};
    STORM_CHECK(logger.set_minimum_level(LogLevel::Trace));
    logger.write(LogLevel::Info, "core", "startup");
    logger.write(LogLevel::Error, "render", "device-failed");
    STORM_CHECK(buffer.size() == 2U);
    STORM_CHECK(buffer.total_record_count() == 2U);
    STORM_CHECK(buffer.dropped_count() == 0U);
    STORM_CHECK(buffer.count_for_level(LogLevel::Info) == 1U);
    STORM_CHECK(buffer.count_for_level(LogLevel::Error) == 1U);

    const auto oldest_before_overwrite = buffer.at(0U);
    STORM_CHECK(oldest_before_overwrite.has_value());
    if (oldest_before_overwrite.has_value()) {
        STORM_CHECK(oldest_before_overwrite->category == "core");
        STORM_CHECK(oldest_before_overwrite->message == "startup");
        STORM_CHECK(!oldest_before_overwrite->category_truncated);
        STORM_CHECK(!oldest_before_overwrite->message_truncated);
    }

    STORM_CHECK(buffer.append(LogRecord{LogLevel::Warning, "platform-long", "message-123456"}));
    STORM_CHECK(buffer.size() == 2U);
    STORM_CHECK(buffer.total_record_count() == 3U);
    STORM_CHECK(buffer.dropped_count() == 1U);
    STORM_CHECK(buffer.count_for_level(LogLevel::Info) == 1U);
    STORM_CHECK(buffer.count_for_level(LogLevel::Error) == 1U);
    STORM_CHECK(buffer.count_for_level(LogLevel::Warning) == 1U);

    const auto now_oldest = buffer.at(0U);
    STORM_CHECK(now_oldest.has_value());
    if (now_oldest.has_value()) {
        STORM_CHECK(now_oldest->level == LogLevel::Error);
        STORM_CHECK(now_oldest->category == "rend");
        STORM_CHECK(now_oldest->category_truncated);
        STORM_CHECK(now_oldest->message == "device-f");
        STORM_CHECK(now_oldest->message_truncated);
    }

    const auto newest = buffer.at(1U);
    STORM_CHECK(newest.has_value());
    if (newest.has_value()) {
        STORM_CHECK(newest->level == LogLevel::Warning);
        STORM_CHECK(newest->category == "plat");
        STORM_CHECK(newest->category_truncated);
        STORM_CHECK(newest->message == "message-");
        STORM_CHECK(newest->message_truncated);
    }
    STORM_CHECK(!buffer.at(2U).has_value());

    const auto total_before_invalid = buffer.total_record_count();
    STORM_CHECK(!buffer.append(LogRecord{LogLevel::Off, "test", "ignored"}));
    STORM_CHECK(!buffer.append(LogRecord{static_cast<LogLevel>(255), "test", "invalid"}));
    STORM_CHECK(buffer.total_record_count() == total_before_invalid);
    STORM_CHECK(buffer.count_for_level(LogLevel::Off) == 0U);
    STORM_CHECK(buffer.count_for_level(static_cast<LogLevel>(255)) == 0U);

    logger.write(LogLevel::Critical, "runtime", "shutdown");
    STORM_CHECK(buffer.size() == 2U);
    STORM_CHECK(buffer.dropped_count() == 2U);
    STORM_CHECK(buffer.count_for_level(LogLevel::Critical) == 1U);

    buffer.clear();
    STORM_CHECK(buffer.empty());
    STORM_CHECK(buffer.size() == 0U);
    STORM_CHECK(buffer.total_record_count() == 0U);
    STORM_CHECK(buffer.dropped_count() == 0U);
    STORM_CHECK(buffer.count_for_level(LogLevel::Info) == 0U);
    STORM_CHECK(!buffer.at(0U).has_value());
}

void test_dimension_aware_transforms() {
    using storm::scene::Dimension;
    using storm::scene::Transform;
    using storm::scene::Transform1D;
    using storm::scene::Transform2D;
    using storm::scene::Transform3D;

    const auto one = Transform::for_dimension(Dimension::One);
    const auto two = Transform::for_dimension(Dimension::Two);
    const auto three = Transform::for_dimension(Dimension::Three);
    STORM_CHECK(one.has_value());
    STORM_CHECK(two.has_value());
    STORM_CHECK(three.has_value());
    STORM_CHECK(!Transform::for_dimension(static_cast<Dimension>(99)).has_value());

    if (one.has_value()) {
        STORM_CHECK(one->dimension() == Dimension::One);
        STORM_CHECK(one->is_finite());
        STORM_CHECK(std::holds_alternative<Transform1D>(one->data()));
    }
    if (two.has_value()) {
        STORM_CHECK(two->dimension() == Dimension::Two);
        STORM_CHECK(two->is_finite());
        STORM_CHECK(std::holds_alternative<Transform2D>(two->data()));
    }
    if (three.has_value()) {
        STORM_CHECK(three->dimension() == Dimension::Three);
        STORM_CHECK(three->is_finite());
        STORM_CHECK(std::holds_alternative<Transform3D>(three->data()));
    }

    const float not_a_number = std::numeric_limits<float>::quiet_NaN();
    STORM_CHECK((!Transform{Transform1D{not_a_number, 1.0F}}.is_finite()));
    STORM_CHECK((!Transform{Transform2D{storm::math::Vec2{0.0F, 0.0F},
                                       std::numeric_limits<float>::infinity(),
                                       storm::math::Vec2{1.0F, 1.0F}}}
                      .is_finite()));
    STORM_CHECK((!Transform{Transform3D{storm::math::Vec3{0.0F, not_a_number, 0.0F},
                                       storm::math::Vec3{},
                                       storm::math::Vec3{1.0F, 1.0F, 1.0F}}}
                      .is_finite()));
}

void test_scene_entity_lifecycle_and_validation() {
    using storm::core::ErrorCode;
    using storm::core::StableId;
    using storm::scene::Dimension;
    using storm::scene::Entity;
    using storm::scene::Scene;
    using storm::scene::Transform;
    using storm::scene::Transform2D;
    using storm::scene::Transform3D;

    const auto sentinel = StableId::parse("f81d4fae-7dec-11d0-a765-00a0c91e6bf6");
    STORM_CHECK(sentinel.has_value());
    if (!sentinel.has_value()) {
        return;
    }

    Scene scene{Dimension::Two, 2U};
    STORM_CHECK(scene.validate().ok());
    STORM_CHECK(scene.dimension() == Dimension::Two);
    STORM_CHECK(scene.max_entities() == 2U);
    STORM_CHECK(scene.entity_count() == 0U);

    StableId output = *sentinel;
    STORM_CHECK(scene.create_entity("", output).code() == ErrorCode::InvalidArgument);
    STORM_CHECK(output == *sentinel);

    const std::string too_long_name(Entity::max_name_bytes + 1U, 'x');
    STORM_CHECK(scene.create_entity(too_long_name, output).code() == ErrorCode::InvalidArgument);
    STORM_CHECK(output == *sentinel);

    const std::string embedded_nul{"bad\0name", 8U};
    STORM_CHECK(scene.create_entity(embedded_nul, output).code() == ErrorCode::InvalidArgument);
    STORM_CHECK(output == *sentinel);

    STORM_CHECK(scene.create_entity("Player", output).ok());
    const StableId player_id = output;
    STORM_CHECK(player_id != *sentinel);
    STORM_CHECK(scene.entity_count() == 1U);

    Entity* player = scene.find_entity(player_id);
    STORM_CHECK(player != nullptr);
    if (player == nullptr) {
        return;
    }
    STORM_CHECK(player->id() == player_id);
    STORM_CHECK(player->name() == "Player");
    STORM_CHECK(player->set_name("Hero").ok());
    STORM_CHECK(player->name() == "Hero");
    STORM_CHECK(player->set_name("").code() == ErrorCode::InvalidArgument);
    STORM_CHECK(player->set_name(too_long_name).code() == ErrorCode::InvalidArgument);
    STORM_CHECK(player->set_name(embedded_nul).code() == ErrorCode::InvalidArgument);
    STORM_CHECK(player->name() == "Hero");
    STORM_CHECK(player->set_name("Player").ok());
    STORM_CHECK(player->name() == "Player");
    STORM_CHECK(player->transform().dimension() == Dimension::Two);
    STORM_CHECK(std::holds_alternative<Transform2D>(player->transform().data()));

    const Transform original_transform = player->transform();
    const auto wrong_dimension = player->set_transform(Transform{Transform3D{}});
    STORM_CHECK(wrong_dimension.code() == ErrorCode::InvalidArgument);
    STORM_CHECK(player->transform().dimension() == original_transform.dimension());
    STORM_CHECK(std::holds_alternative<Transform2D>(player->transform().data()));

    const float not_a_number = std::numeric_limits<float>::quiet_NaN();
    const Transform non_finite{Transform2D{storm::math::Vec2{not_a_number, 0.0F}, 0.0F,
                                           storm::math::Vec2{1.0F, 1.0F}}};
    STORM_CHECK(player->set_transform(non_finite).code() == ErrorCode::InvalidArgument);
    STORM_CHECK(player->transform().is_finite());

    const Transform moved{Transform2D{storm::math::Vec2{3.0F, 4.0F}, 1.25F,
                                      storm::math::Vec2{2.0F, 3.0F}}};
    STORM_CHECK(player->set_transform(moved).ok());
    const auto* moved_value = std::get_if<Transform2D>(&player->transform().data());
    STORM_CHECK(moved_value != nullptr);
    if (moved_value != nullptr) {
        STORM_CHECK((moved_value->position == storm::math::Vec2{3.0F, 4.0F}));
        STORM_CHECK(moved_value->rotation_radians == 1.25F);
        STORM_CHECK((moved_value->scale == storm::math::Vec2{2.0F, 3.0F}));
    }

    STORM_CHECK(scene.create_entity("Enemy", output).ok());
    const StableId enemy_id = output;
    STORM_CHECK(scene.entity_count() == 2U);
    STORM_CHECK(scene.find_entity(enemy_id) != nullptr);

    EntityVisitCapture visit_capture;
    STORM_CHECK(scene.visit_entities(&visit_capture, &capture_entity_id) == 2U);
    STORM_CHECK(visit_capture.count == 2U);
    STORM_CHECK(scene.visit_entities(nullptr, nullptr) == 0U);
    if (visit_capture.count == 2U) {
        STORM_CHECK(visit_capture.ids[0] != visit_capture.ids[1]);
        STORM_CHECK(storm::core::StableIdLess{}(visit_capture.ids[0], visit_capture.ids[1]));
    }

    const StableId last_successful_output = output;
    STORM_CHECK(scene.create_entity("Overflow", output).code() == ErrorCode::ResourceExhausted);
    STORM_CHECK(output == last_successful_output);
    STORM_CHECK(scene.entity_count() == 2U);

    STORM_CHECK(scene.destroy_entity(player_id).ok());
    STORM_CHECK(scene.find_entity(player_id) == nullptr);
    STORM_CHECK(scene.entity_count() == 1U);
    STORM_CHECK(scene.destroy_entity(player_id).code() == ErrorCode::NotFound);
    STORM_CHECK(scene.destroy_entity(StableId{}).code() == ErrorCode::InvalidArgument);

    STORM_CHECK(scene.create_entity("Replacement", output).ok());
    STORM_CHECK(scene.entity_count() == 2U);
    STORM_CHECK(scene.find_entity(output) != nullptr);

    Scene invalid_dimension{static_cast<Dimension>(99U), 2U};
    STORM_CHECK(invalid_dimension.validate().code() == ErrorCode::InvalidArgument);
    const StableId unchanged = output;
    STORM_CHECK(invalid_dimension.create_entity("Invalid", output).code() ==
                ErrorCode::InvalidArgument);
    STORM_CHECK(output == unchanged);

    Scene invalid_capacity{Dimension::One, 0U};
    STORM_CHECK(invalid_capacity.validate().code() == ErrorCode::InvalidArgument);
    Scene excessive_capacity{Dimension::One, Scene::maximum_allowed_entities + 1U};
    STORM_CHECK(excessive_capacity.validate().code() == ErrorCode::InvalidArgument);

    constexpr std::array dimensions{Dimension::One, Dimension::Two, Dimension::Three};
    for (const Dimension dimension : dimensions) {
        Scene dimension_scene{dimension, 1U};
        StableId dimension_id;
        STORM_CHECK(dimension_scene.create_entity("Dimensional", dimension_id).ok());
        const Entity* entity = dimension_scene.find_entity(dimension_id);
        STORM_CHECK(entity != nullptr);
        if (entity != nullptr) {
            STORM_CHECK(entity->transform().dimension() == dimension);
        }
    }
}

void test_version() {
    STORM_CHECK(storm::version() == storm::kVersion);
    STORM_CHECK(storm::version() == (storm::Version{0, 1, 0}));
    STORM_CHECK(storm::version_string() == "0.1.0");
}

void test_status_values() {
    using storm::core::ErrorCode;
    using storm::core::Status;

    const auto success = Status::success();
    STORM_CHECK(success.ok());
    STORM_CHECK(static_cast<bool>(success));
    STORM_CHECK(success.code() == ErrorCode::None);
    STORM_CHECK(success.message().empty());

    const auto failure = Status::failure(ErrorCode::Parse, "invalid scene header");
    STORM_CHECK(!failure.ok());
    STORM_CHECK(!static_cast<bool>(failure));
    STORM_CHECK(failure.code() == ErrorCode::Parse);
    STORM_CHECK(failure.message() == "invalid scene header");

    // Guard against accidentally constructing a failed operation with the success code.
    const auto normalized_failure = Status::failure(ErrorCode::None, "invalid error code");
    STORM_CHECK(!normalized_failure.ok());
    STORM_CHECK(normalized_failure.code() == ErrorCode::Internal);
    STORM_CHECK(normalized_failure.message() == "invalid error code");
}

void test_runtime_lifecycle_normal_path() {
    using storm::core::ErrorCode;
    using storm::core::RuntimeLifecycle;
    using storm::core::RuntimeState;
    using storm::core::runtime_state_name;

    RuntimeLifecycle lifecycle;
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);
    STORM_CHECK(!lifecycle.is_running());
    STORM_CHECK(runtime_state_name(RuntimeState::Uninitialized) == "Uninitialized");
    STORM_CHECK(runtime_state_name(RuntimeState::Running) == "Running");
    STORM_CHECK(runtime_state_name(static_cast<RuntimeState>(255)) == "Unknown");

    const auto start_too_early = lifecycle.start();
    STORM_CHECK(!start_too_early.ok());
    STORM_CHECK(start_too_early.code() == ErrorCode::InvalidState);
    STORM_CHECK(start_too_early.message().find("Uninitialized") != std::string::npos);
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);

    STORM_CHECK(lifecycle.initialize().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Initialized);
    STORM_CHECK(lifecycle.initialize().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.state() == RuntimeState::Initialized);

    STORM_CHECK(lifecycle.start().ok());
    STORM_CHECK(lifecycle.is_running());
    STORM_CHECK(lifecycle.pause().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Paused);
    STORM_CHECK(!lifecycle.is_running());
    STORM_CHECK(lifecycle.resume().ok());
    STORM_CHECK(lifecycle.is_running());

    STORM_CHECK(lifecycle.request_stop().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Stopping);
    STORM_CHECK(!lifecycle.is_running());
    STORM_CHECK(lifecycle.finish_stop().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Stopped);
    STORM_CHECK(lifecycle.finish_stop().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.state() == RuntimeState::Stopped);

    STORM_CHECK(lifecycle.reset().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);
    STORM_CHECK(!lifecycle.failure_reason().has_value());
}

void test_runtime_lifecycle_invalid_transitions() {
    using storm::core::ErrorCode;
    using storm::core::RuntimeLifecycle;
    using storm::core::RuntimeState;

    RuntimeLifecycle lifecycle;
    STORM_CHECK(lifecycle.pause().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.resume().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.request_stop().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.finish_stop().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.reset().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);

    STORM_CHECK(lifecycle.initialize().ok());
    STORM_CHECK(lifecycle.request_stop().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Stopping);
    STORM_CHECK(lifecycle.start().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.pause().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.reset().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.finish_stop().ok());
    STORM_CHECK(lifecycle.reset().ok());
}

void test_runtime_lifecycle_failure_and_recovery() {
    using storm::core::ErrorCode;
    using storm::core::RuntimeLifecycle;
    using storm::core::RuntimeState;

    RuntimeLifecycle lifecycle;
    STORM_CHECK(lifecycle.initialize().ok());
    STORM_CHECK(lifecycle.start().ok());

    const auto fatal = lifecycle.fail(ErrorCode::Io, "platform initialization failed");
    STORM_CHECK(!fatal.ok());
    STORM_CHECK(fatal.code() == ErrorCode::Io);
    STORM_CHECK(fatal.message() == "platform initialization failed");
    STORM_CHECK(lifecycle.state() == RuntimeState::Failed);
    STORM_CHECK(lifecycle.failure_reason().has_value());
    if (lifecycle.failure_reason().has_value()) {
        STORM_CHECK(lifecycle.failure_reason()->code() == ErrorCode::Io);
        STORM_CHECK(lifecycle.failure_reason()->message() == "platform initialization failed");
    }

    STORM_CHECK(lifecycle.start().code() == ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.fail(ErrorCode::Internal, "second failure").code() ==
                ErrorCode::InvalidState);
    STORM_CHECK(lifecycle.failure_reason()->message() == "platform initialization failed");

    STORM_CHECK(lifecycle.reset().ok());
    STORM_CHECK(lifecycle.state() == RuntimeState::Uninitialized);
    STORM_CHECK(!lifecycle.failure_reason().has_value());
    STORM_CHECK(lifecycle.initialize().ok());

    const auto missing_code = lifecycle.fail(ErrorCode::None, "failure without code");
    STORM_CHECK(missing_code.code() == ErrorCode::Internal);
    STORM_CHECK(lifecycle.state() == RuntimeState::Failed);
}

void test_vector_arithmetic() {
    using storm::math::Vec1;
    using storm::math::Vec2;
    using storm::math::Vec3;

    STORM_CHECK(((Vec1{2.0F} + Vec1{3.0F}) == Vec1{5.0F}));
    STORM_CHECK(((Vec2{1.0F, 2.0F} + Vec2{3.0F, 4.0F}) == Vec2{4.0F, 6.0F}));
    STORM_CHECK(((Vec3{3.0F, 4.0F, 5.0F} - Vec3{1.0F, 2.0F, 3.0F}) ==
                 Vec3{2.0F, 2.0F, 2.0F}));
    STORM_CHECK((Vec2{3.0F, 4.0F}.dot(Vec2{3.0F, 4.0F}) == 25.0F));
    STORM_CHECK(((Vec3{1.0F, 2.0F, 3.0F} * 2.0F) == Vec3{2.0F, 4.0F, 6.0F}));
    STORM_CHECK(((2.0F * Vec2{2.0F, 5.0F}) == Vec2{4.0F, 10.0F}));

    auto accumulated = Vec2{1.0F, 2.0F};
    accumulated += Vec2{2.0F, 3.0F};
    STORM_CHECK((accumulated == Vec2{3.0F, 5.0F}));
    accumulated -= Vec2{1.0F, 1.0F};
    STORM_CHECK((accumulated == Vec2{2.0F, 4.0F}));
}

void test_vector_normalization_and_validation() {
    using storm::math::Vec2;

    const auto normalized = Vec2{3.0F, 4.0F}.normalized();
    STORM_CHECK(normalized.has_value());
    if (normalized.has_value()) {
        STORM_CHECK(std::abs(normalized->length() - 1.0F) < 1.0e-5F);
        STORM_CHECK(std::abs(normalized->operator[](0) - 0.6F) < 1.0e-5F);
        STORM_CHECK(std::abs(normalized->operator[](1) - 0.8F) < 1.0e-5F);
    }

    STORM_CHECK((!Vec2{0.0F, 0.0F}.normalized().has_value()));
    STORM_CHECK((!Vec2{1.0F, 0.0F}.normalized(-1.0F).has_value()));
    STORM_CHECK((!Vec2{1.0F, 0.0F}.normalized(
                     std::numeric_limits<float>::quiet_NaN()).has_value()));
    STORM_CHECK((!Vec2{std::numeric_limits<float>::infinity(), 1.0F}.is_finite()));
    STORM_CHECK((Vec2{1.0F, 1.0F}.is_finite()));
}

void test_dimensions() {
    using storm::scene::Dimension;
    using storm::scene::component_count;
    using storm::scene::dimension_name;
    using storm::scene::is_valid_dimension;

    STORM_CHECK(is_valid_dimension(Dimension::One));
    STORM_CHECK(is_valid_dimension(Dimension::Two));
    STORM_CHECK(is_valid_dimension(Dimension::Three));
    STORM_CHECK(!is_valid_dimension(static_cast<Dimension>(0)));
    STORM_CHECK(!is_valid_dimension(static_cast<Dimension>(4)));
    STORM_CHECK(component_count(Dimension::One) == 1U);
    STORM_CHECK(component_count(Dimension::Two) == 2U);
    STORM_CHECK(component_count(Dimension::Three) == 3U);
    STORM_CHECK(component_count(static_cast<Dimension>(255)) == 0U);
    STORM_CHECK(dimension_name(Dimension::One) == "1D");
    STORM_CHECK(dimension_name(Dimension::Two) == "2D");
    STORM_CHECK(dimension_name(Dimension::Three) == "3D");
    STORM_CHECK(dimension_name(static_cast<Dimension>(99)) == "Unknown");
}

void test_fixed_step_clock_basic_timing() {
    using storm::core::FixedStepClock;

    FixedStepClock clock;
    const auto first = clock.advance(1.0 / 60.0);
    STORM_CHECK(first.accepted);
    STORM_CHECK(first.steps == 1U);
    STORM_CHECK(nearly_equal(first.simulated_seconds, 1.0 / 60.0));
    STORM_CHECK(nearly_equal(first.interpolation_alpha, 0.0));

    const auto half_frame = clock.advance(1.0 / 120.0);
    STORM_CHECK(half_frame.accepted);
    STORM_CHECK(half_frame.steps == 0U);
    STORM_CHECK(nearly_equal(half_frame.interpolation_alpha, 0.5));

    const auto accumulated = clock.advance(1.0 / 120.0);
    STORM_CHECK(accumulated.steps == 1U);
    STORM_CHECK(nearly_equal(accumulated.interpolation_alpha, 0.0));
}

void test_fixed_step_clock_rounding_and_catch_up() {
    using storm::core::FixedStepClock;
    using storm::core::FixedStepClockConfig;

    FixedStepClock clock;
    STORM_CHECK((clock.configure(FixedStepClockConfig{0.1, 0.5, 8U})));

    const auto three_ticks = clock.advance(0.3);
    STORM_CHECK(three_ticks.accepted);
    STORM_CHECK(three_ticks.steps == 3U);
    STORM_CHECK(nearly_equal(three_ticks.simulated_seconds, 0.3));
    STORM_CHECK(nearly_equal(three_ticks.discarded_seconds, 0.0));
    STORM_CHECK(nearly_equal(three_ticks.interpolation_alpha, 0.0));

    STORM_CHECK((clock.configure(FixedStepClockConfig{0.1, 0.25, 8U})));
    const auto clamped = clock.advance(1.0);
    STORM_CHECK(clamped.accepted);
    STORM_CHECK(clamped.steps == 2U);
    STORM_CHECK(nearly_equal(clamped.simulated_seconds, 0.2));
    STORM_CHECK(nearly_equal(clamped.discarded_seconds, 0.75));
    STORM_CHECK(nearly_equal(clamped.interpolation_alpha, 0.5));
}

void test_fixed_step_clock_step_budget() {
    using storm::core::FixedStepClock;
    using storm::core::FixedStepClockConfig;

    FixedStepClock clock;
    STORM_CHECK((clock.configure(FixedStepClockConfig{0.1, 0.8, 2U})));

    const auto bounded = clock.advance(0.65);
    STORM_CHECK(bounded.accepted);
    STORM_CHECK(bounded.steps == 2U);
    STORM_CHECK(nearly_equal(bounded.simulated_seconds, 0.2));
    STORM_CHECK(nearly_equal(bounded.discarded_seconds, 0.4));
    STORM_CHECK(nearly_equal(bounded.interpolation_alpha, 0.5));
    STORM_CHECK(clock.accumulator_seconds() >= 0.0);
    STORM_CHECK(clock.accumulator_seconds() < clock.config().fixed_delta_seconds);
}

void test_fixed_step_clock_rejects_invalid_time_and_configuration() {
    using storm::core::FixedStepClock;
    using storm::core::FixedStepClockConfig;

    FixedStepClock clock;
    STORM_CHECK((clock.configure(FixedStepClockConfig{0.1, 0.5, 8U})));
    const auto partial = clock.advance(0.05);
    STORM_CHECK(partial.accepted);
    const double saved_accumulator = clock.accumulator_seconds();
    const auto saved_config = clock.config();

    const auto negative = clock.advance(-1.0);
    const auto nan = clock.advance(std::numeric_limits<double>::quiet_NaN());
    const auto infinity = clock.advance(std::numeric_limits<double>::infinity());
    STORM_CHECK(!negative.accepted);
    STORM_CHECK(!nan.accepted);
    STORM_CHECK(!infinity.accepted);
    STORM_CHECK(nearly_equal(clock.accumulator_seconds(), saved_accumulator));
    STORM_CHECK(nearly_equal(negative.interpolation_alpha, 0.5));

    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.0, 0.5, 8U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.1, 0.05, 8U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.1, 0.5, 0U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.1, 0.5, 1025U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{
                     std::numeric_limits<double>::quiet_NaN(), 0.5, 8U})));
    STORM_CHECK((!clock.configure(FixedStepClockConfig{0.1, 61.0, 8U})));

    STORM_CHECK(clock.config().fixed_delta_seconds == saved_config.fixed_delta_seconds);
    STORM_CHECK(clock.config().max_frame_delta_seconds == saved_config.max_frame_delta_seconds);
    STORM_CHECK(clock.config().max_steps_per_frame == saved_config.max_steps_per_frame);
    STORM_CHECK(nearly_equal(clock.accumulator_seconds(), saved_accumulator));

    STORM_CHECK((clock.configure(FixedStepClockConfig{0.2, 0.4, 4U})));
    STORM_CHECK(nearly_equal(clock.accumulator_seconds(), 0.0));
    const auto zero = clock.advance(0.0);
    STORM_CHECK(zero.accepted);
    STORM_CHECK(zero.steps == 0U);
    STORM_CHECK(nearly_equal(zero.interpolation_alpha, 0.0));
    clock.reset();
    STORM_CHECK(nearly_equal(clock.accumulator_seconds(), 0.0));
}

void test_input_state_frame_edges() {
    using storm::platform::InputState;

    InputState input;
    STORM_CHECK(InputState::key_code_count == 256U);
    STORM_CHECK(InputState::pointer_button_count == 8U);
    STORM_CHECK(!input.key_is_down(4U));
    STORM_CHECK(!input.key_was_pressed(4U));
    STORM_CHECK(!input.key_was_released(4U));

    STORM_CHECK(input.set_key_down(4U, true));
    STORM_CHECK(input.key_is_down(4U));
    STORM_CHECK(input.key_was_pressed(4U));
    STORM_CHECK(!input.key_was_released(4U));

    // A repeated native key-down (for example, keyboard repeat) must not create a new edge.
    STORM_CHECK(input.set_key_down(4U, true));
    STORM_CHECK(input.key_was_pressed(4U));

    input.begin_frame();
    STORM_CHECK(input.key_is_down(4U));
    STORM_CHECK(!input.key_was_pressed(4U));
    STORM_CHECK(!input.key_was_released(4U));

    STORM_CHECK(input.set_key_down(4U, false));
    STORM_CHECK(!input.key_is_down(4U));
    STORM_CHECK(input.key_was_released(4U));
    STORM_CHECK(input.set_key_down(4U, true));
    STORM_CHECK(input.key_is_down(4U));
    // Both edges are retained when a control is pressed and released in the same frame.
    STORM_CHECK(input.key_was_pressed(4U));
    STORM_CHECK(input.key_was_released(4U));

    STORM_CHECK(input.set_pointer_button_down(0U, true));
    STORM_CHECK(input.pointer_button_is_down(0U));
    STORM_CHECK(input.pointer_button_was_pressed(0U));
    STORM_CHECK(input.set_pointer_button_down(0U, false));
    STORM_CHECK(!input.pointer_button_is_down(0U));
    STORM_CHECK(input.pointer_button_was_released(0U));

    input.begin_frame();
    STORM_CHECK(!input.key_was_pressed(4U));
    STORM_CHECK(!input.key_was_released(4U));
    STORM_CHECK(!input.pointer_button_was_pressed(0U));
    STORM_CHECK(!input.pointer_button_was_released(0U));
    STORM_CHECK(input.key_is_down(4U));
}

void test_input_state_rejects_out_of_range_slots_and_resets() {
    using storm::platform::InputState;

    InputState input;
    STORM_CHECK(input.set_key_down(255U, true));
    STORM_CHECK(input.key_is_down(255U));
    STORM_CHECK(!input.set_key_down(256U, true));
    STORM_CHECK(!input.set_key_down(static_cast<std::size_t>(-1), true));
    STORM_CHECK(!input.key_is_down(256U));
    STORM_CHECK(!input.key_was_pressed(256U));

    STORM_CHECK(input.set_pointer_button_down(7U, true));
    STORM_CHECK(input.pointer_button_is_down(7U));
    STORM_CHECK(!input.set_pointer_button_down(8U, true));
    STORM_CHECK(!input.pointer_button_is_down(8U));
    STORM_CHECK(!input.pointer_button_was_pressed(8U));

    input.reset();
    STORM_CHECK(!input.key_is_down(255U));
    STORM_CHECK(!input.key_was_pressed(255U));
    STORM_CHECK(!input.key_was_released(255U));
    STORM_CHECK(!input.pointer_button_is_down(7U));
    STORM_CHECK(!input.pointer_button_was_pressed(7U));
    STORM_CHECK(!input.pointer_button_was_released(7U));
}

void test_input_state_releases_controls_on_focus_loss() {
    using storm::platform::InputState;

    InputState input;
    STORM_CHECK(input.set_key_down(8U, true));
    STORM_CHECK(input.set_key_down(240U, true));
    STORM_CHECK(input.set_pointer_button_down(0U, true));
    STORM_CHECK(input.set_pointer_button_down(6U, true));

    input.begin_frame();
    input.release_all();

    STORM_CHECK(!input.key_is_down(8U));
    STORM_CHECK(!input.key_is_down(240U));
    STORM_CHECK(input.key_was_released(8U));
    STORM_CHECK(input.key_was_released(240U));
    STORM_CHECK(!input.key_was_pressed(8U));
    STORM_CHECK(!input.pointer_button_is_down(0U));
    STORM_CHECK(!input.pointer_button_is_down(6U));
    STORM_CHECK(input.pointer_button_was_released(0U));
    STORM_CHECK(input.pointer_button_was_released(6U));

    // Releasing twice is harmless and does not invent additional held-state transitions.
    input.release_all();
    STORM_CHECK(input.key_was_released(8U));
    STORM_CHECK(input.pointer_button_was_released(0U));

    input.begin_frame();
    STORM_CHECK(!input.key_was_released(8U));
    STORM_CHECK(!input.pointer_button_was_released(0U));
}

void test_platform_event_queue_fifo_overflow_and_wraparound() {
    using namespace storm::platform;
    using Queue = PlatformEventQueue<3U>;

    Queue queue;
    STORM_CHECK(Queue::capacity == 3U);
    STORM_CHECK(queue.max_size() == 3U);
    STORM_CHECK(queue.empty());
    STORM_CHECK(!queue.pop().has_value());

    STORM_CHECK(queue.push(QuitRequestedEvent{}));
    STORM_CHECK(queue.push(KeyStateChangedEvent{42U, true}));
    STORM_CHECK(queue.push(WindowResizedEvent{1280U, 720U}));
    STORM_CHECK(queue.size() == 3U);

    // The newest event is rejected when full; previously accepted events are never overwritten.
    STORM_CHECK(!queue.push(PointerMovedEvent{12.0, 8.0}));
    STORM_CHECK(queue.dropped_count() == 1U);
    STORM_CHECK(queue.rejected_count() == 0U);
    STORM_CHECK(queue.size() == 3U);

    const auto quit = queue.pop();
    STORM_CHECK(quit.has_value());
    if (quit.has_value()) {
        STORM_CHECK(std::holds_alternative<QuitRequestedEvent>(*quit));
    }

    const auto key = queue.pop();
    STORM_CHECK(key.has_value());
    if (key.has_value()) {
        const auto* value = std::get_if<KeyStateChangedEvent>(&*key);
        STORM_CHECK(value != nullptr);
        if (value != nullptr) {
            STORM_CHECK(value->key_code == 42U);
            STORM_CHECK(value->is_down);
        }
    }

    // Reuse the wrapped slot after popping and preserve FIFO order across the ring boundary.
    STORM_CHECK(queue.push(PointerMovedEvent{12.0, 8.0}));
    STORM_CHECK(queue.size() == 2U);
    const auto resized = queue.pop();
    STORM_CHECK(resized.has_value());
    if (resized.has_value()) {
        const auto* value = std::get_if<WindowResizedEvent>(&*resized);
        STORM_CHECK(value != nullptr);
        if (value != nullptr) {
            STORM_CHECK(value->width == 1280U);
            STORM_CHECK(value->height == 720U);
        }
    }

    const auto moved = queue.pop();
    STORM_CHECK(moved.has_value());
    if (moved.has_value()) {
        const auto* value = std::get_if<PointerMovedEvent>(&*moved);
        STORM_CHECK(value != nullptr);
        if (value != nullptr) {
            STORM_CHECK(value->x == 12.0);
            STORM_CHECK(value->y == 8.0);
        }
    }
    STORM_CHECK(queue.empty());
    STORM_CHECK(!queue.pop().has_value());
    STORM_CHECK(queue.dropped_count() == 1U);
}

void test_platform_event_queue_rejects_invalid_payloads() {
    using namespace storm::platform;
    PlatformEventQueue<4U> queue;

    STORM_CHECK(!queue.push(KeyStateChangedEvent{256U, true}));
    STORM_CHECK(!queue.push(PointerButtonStateChangedEvent{8U, true}));
    STORM_CHECK(!queue.push(PointerMovedEvent{std::numeric_limits<double>::quiet_NaN(), 0.0}));
    STORM_CHECK(!queue.push(PointerWheelScrolledEvent{
        0.0, std::numeric_limits<double>::infinity()}));
    STORM_CHECK(queue.empty());
    STORM_CHECK(queue.size() == 0U);
    STORM_CHECK(queue.rejected_count() == 4U);
    STORM_CHECK(queue.dropped_count() == 0U);

    // Zero-sized surfaces are a valid minimized-window state, not a malformed event.
    STORM_CHECK(queue.push(WindowResizedEvent{0U, 0U}));
    STORM_CHECK(queue.push(WindowFocusChangedEvent{false}));
    STORM_CHECK(queue.size() == 2U);

    queue.clear();
    STORM_CHECK(queue.empty());
    STORM_CHECK(queue.size() == 0U);
    STORM_CHECK(queue.rejected_count() == 0U);
    STORM_CHECK(queue.dropped_count() == 0U);
}

void test_platform_event_queue_integrates_with_input_state() {
    using namespace storm::platform;
    PlatformEventQueue<8U> queue;
    InputState input;

    STORM_CHECK(queue.push(KeyStateChangedEvent{8U, true}));
    STORM_CHECK(queue.push(PointerButtonStateChangedEvent{2U, true}));
    STORM_CHECK(queue.push(WindowFocusChangedEvent{false}));

    while (const auto event = queue.pop()) {
        if (const auto* key = std::get_if<KeyStateChangedEvent>(&*event)) {
            STORM_CHECK(input.set_key_down(key->key_code, key->is_down));
        } else if (const auto* button =
                       std::get_if<PointerButtonStateChangedEvent>(&*event)) {
            STORM_CHECK(input.set_pointer_button_down(button->button, button->is_down));
        } else if (const auto* focus = std::get_if<WindowFocusChangedEvent>(&*event)) {
            if (!focus->focused) {
                input.release_all();
            }
        }
    }

    STORM_CHECK(queue.empty());
    STORM_CHECK(!input.key_is_down(8U));
    STORM_CHECK(input.key_was_pressed(8U));
    STORM_CHECK(input.key_was_released(8U));
    STORM_CHECK(!input.pointer_button_is_down(2U));
    STORM_CHECK(input.pointer_button_was_pressed(2U));
    STORM_CHECK(input.pointer_button_was_released(2U));
}

} // namespace

int main() {
    test_version();
    test_status_values();
    test_stable_id_validation_and_round_trip();
    test_diagnostic_buffer_bounded_storage_and_counters();
    test_secure_stable_id_generation();
    test_stable_id_registry_duplicate_detection();
    test_dimension_aware_transforms();
    test_scene_entity_lifecycle_and_validation();
    test_runtime_config_validation();
    test_logging_dispatch_and_filtering();
    test_runtime_lifecycle_normal_path();
    test_runtime_lifecycle_invalid_transitions();
    test_runtime_lifecycle_failure_and_recovery();
    test_vector_arithmetic();
    test_vector_normalization_and_validation();
    test_dimensions();
    test_fixed_step_clock_basic_timing();
    test_fixed_step_clock_rounding_and_catch_up();
    test_fixed_step_clock_step_budget();
    test_fixed_step_clock_rejects_invalid_time_and_configuration();
    test_input_state_frame_edges();
    test_input_state_rejects_out_of_range_slots_and_resets();
    test_input_state_releases_controls_on_focus_loss();
    test_platform_event_queue_fifo_overflow_and_wraparound();
    test_platform_event_queue_rejects_invalid_payloads();
    test_platform_event_queue_integrates_with_input_state();

    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed.\n";
        return 1;
    }

    std::cout << "All StormEngine core tests passed.\n";
    return 0;
}
