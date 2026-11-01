#pragma once

#include "storm/foundation/result.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <typeindex>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Storm {

// Synchronous, type-safe event dispatch for the Core/runtime thread.
// This class is deliberately not internally synchronized.
class EventDispatcher final {
private:
    struct Slot final {
        std::uint64_t id{0U};
        bool active{true};
        std::function<Result<void>(const void*)> invoke;
    };

    struct State final {
        std::unordered_map<std::type_index, std::vector<std::shared_ptr<Slot>>> listeners;
        std::uint64_t next_id{1U};

        void disconnect(std::type_index event_type, std::uint64_t id) noexcept {
            const auto found = listeners.find(event_type);
            if (found == listeners.end()) {
                return;
            }

            for (const std::shared_ptr<Slot>& slot : found->second) {
                if (slot->id == id) {
                    slot->active = false;
                }
            }

            std::erase_if(found->second, [](const std::shared_ptr<Slot>& slot) {
                return !slot->active;
            });
            if (found->second.empty()) {
                listeners.erase(found);
            }
        }

        [[nodiscard]] bool contains(
            std::type_index event_type,
            std::uint64_t id) const noexcept {
            const auto found = listeners.find(event_type);
            if (found == listeners.end()) {
                return false;
            }

            for (const std::shared_ptr<Slot>& slot : found->second) {
                if (slot->id == id && slot->active) {
                    return true;
                }
            }
            return false;
        }
    };

public:
    // A move-only RAII handle. Destroying or disconnecting it removes its listener.
    class Subscription final {
    public:
        Subscription() noexcept = default;

        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;

        Subscription(Subscription&& other) noexcept
            : state_(std::move(other.state_)),
              event_type_(other.event_type_),
              id_(std::exchange(other.id_, 0U)) {}

        Subscription& operator=(Subscription&& other) noexcept {
            if (this != &other) {
                disconnect();
                state_ = std::move(other.state_);
                event_type_ = other.event_type_;
                id_ = std::exchange(other.id_, 0U);
            }
            return *this;
        }

        ~Subscription() {
            disconnect();
        }

        void disconnect() noexcept {
            if (id_ == 0U) {
                return;
            }

            if (const std::shared_ptr<State> state = state_.lock()) {
                state->disconnect(event_type_, id_);
            }

            id_ = 0U;
            state_.reset();
        }

        [[nodiscard]] bool connected() const noexcept {
            if (id_ == 0U) {
                return false;
            }
            const std::shared_ptr<State> state = state_.lock();
            return state != nullptr && state->contains(event_type_, id_);
        }

    private:
        friend class EventDispatcher;

        Subscription(
            const std::shared_ptr<State>& state,
            std::type_index event_type,
            std::uint64_t id) noexcept
            : state_(state), event_type_(event_type), id_(id) {}

        std::weak_ptr<State> state_;
        std::type_index event_type_{typeid(void)};
        std::uint64_t id_{0U};
    };

    EventDispatcher() : state_(std::make_shared<State>()) {}

    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
    EventDispatcher(EventDispatcher&&) = delete;
    EventDispatcher& operator=(EventDispatcher&&) = delete;

    // Handlers may return void or Result<void>. A failed Result stops dispatch
    // and is returned to the caller. Exceptions are not caught.
    template <typename Event, typename Callback>
    [[nodiscard]] Result<Subscription> subscribe(Callback&& callback) {
        using StoredCallback = std::decay_t<Callback>;
        static_assert(
            std::is_invocable_v<StoredCallback&, const Event&>,
            "Event handlers must accept const Event&");

        using CallbackResult = std::invoke_result_t<StoredCallback&, const Event&>;
        static_assert(
            std::is_same_v<CallbackResult, void> ||
                std::is_same_v<CallbackResult, Result<void>>,
            "Event handlers must return void or Storm::Result<void>");

        if constexpr (requires(const StoredCallback& value) {
                          static_cast<bool>(value);
                      }) {
            if (!static_cast<bool>(callback)) {
                return Result<Subscription>::failure(Error::make(
                    ErrorCode::InvalidArgument,
                    "EventDispatcher::subscribe requires a non-empty handler"));
            }
        }

        if (state_->next_id == 0U) {
            return Result<Subscription>::failure(Error::make(
                ErrorCode::ResourceExhausted,
                "EventDispatcher subscription identifiers are exhausted"));
        }

        const std::uint64_t id = state_->next_id;
        auto stored_callback =
            std::make_shared<StoredCallback>(std::forward<Callback>(callback));

        auto slot = std::make_shared<Slot>();
        slot->id = id;
        slot->invoke = [stored_callback](const void* event_data) -> Result<void> {
            const auto& event = *static_cast<const Event*>(event_data);
            if constexpr (std::is_same_v<CallbackResult, void>) {
                std::invoke(*stored_callback, event);
                return Result<void>::success();
            } else {
                return std::invoke(*stored_callback, event);
            }
        };

        const std::type_index event_type(typeid(Event));
        state_->listeners[event_type].push_back(slot);
        if (id == std::numeric_limits<std::uint64_t>::max()) {
            state_->next_id = 0U;
        } else {
            ++state_->next_id;
        }

        return Result<Subscription>::success(
            Subscription{state_, event_type, id});
    }

    // Dispatches one event synchronously in subscription order.
    // The listener set is snapshotted at entry. New handlers begin receiving
    // events on the next dispatch; disconnected handlers are skipped at once.
    template <typename Event>
    [[nodiscard]] Result<std::size_t> dispatch(const Event& event) {
        const std::type_index event_type(typeid(Event));
        const auto found = state_->listeners.find(event_type);
        if (found == state_->listeners.end()) {
            return Result<std::size_t>::success(0U);
        }

        const std::vector<std::shared_ptr<Slot>> snapshot = found->second;
        std::size_t completed_handlers = 0U;
        for (const std::shared_ptr<Slot>& slot : snapshot) {
            if (!slot->active) {
                continue;
            }

            const Result<void> callback_result = slot->invoke(&event);
            if (!callback_result) {
                Error error = callback_result.error();
                error.message =
                    "EventDispatcher::dispatch handler failed: " + error.message;
                return Result<std::size_t>::failure(std::move(error));
            }
            ++completed_handlers;
        }

        return Result<std::size_t>::success(completed_handlers);
    }

    // Disconnects every listener. A dispatch already in progress skips
    // listeners that have not yet run.
    void clear() noexcept {
        for (auto& [event_type, slots] : state_->listeners) {
            (void)event_type;
            for (const std::shared_ptr<Slot>& slot : slots) {
                slot->active = false;
            }
        }
        state_->listeners.clear();
    }

private:
    std::shared_ptr<State> state_;
};

} // namespace Storm
