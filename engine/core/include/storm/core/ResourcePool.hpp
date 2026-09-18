#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace storm::core {

template <typename Tag>
class GenerationalHandle final {
public:
    using Index = std::uint32_t;
    using Generation = std::uint32_t;
    static constexpr Index invalidIndex = std::numeric_limits<Index>::max();
    static constexpr Generation invalidGeneration = 0;

    constexpr GenerationalHandle() noexcept = default;
    constexpr GenerationalHandle(Index index, Generation generation) noexcept
        : index_(index), generation_(generation) {}

    [[nodiscard]] constexpr Index index() const noexcept { return index_; }
    [[nodiscard]] constexpr Generation generation() const noexcept { return generation_; }
    [[nodiscard]] constexpr bool valid() const noexcept {
        return index_ != invalidIndex && generation_ != invalidGeneration;
    }

    friend constexpr bool operator==(GenerationalHandle, GenerationalHandle) noexcept = default;

private:
    Index index_{invalidIndex};
    Generation generation_{invalidGeneration};
};

template <typename T, typename Tag>
class ResourcePool final {
public:
    using Handle = GenerationalHandle<Tag>;
    using Index = typename Handle::Index;
    using Generation = typename Handle::Generation;

    ResourcePool() = default;
    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;

    template <typename... Args>
    [[nodiscard]] Handle emplace(Args&&... args) {
        if (!free_.empty()) {
            const Index index = free_.back();
            free_.pop_back();
            Slot& slot = slots_[index];
            slot.value.emplace(std::forward<Args>(args)...);
            ++liveCount_;
            return Handle(index, slot.generation);
        }

        if (slots_.size() >= static_cast<std::size_t>(Handle::invalidIndex))
            return {};

        Slot slot{};
        slot.generation = 1;
        slot.value.emplace(std::forward<Args>(args)...);
        slots_.push_back(std::move(slot));
        ++liveCount_;
        return Handle(static_cast<Index>(slots_.size() - 1), 1);
    }

    bool destroy(Handle handle) noexcept {
        Slot* slot = findSlot(handle);
        if (!slot) return false;

        slot->value.reset();
        --liveCount_;
        if (slot->generation != std::numeric_limits<Generation>::max()) {
            ++slot->generation;
            free_.push_back(handle.index());
        } else {
            // A generation that can no longer be incremented is retired
            // instead of allowing an old handle to become valid again.
            slot->generation = Handle::invalidGeneration;
        }
        return true;
    }

    [[nodiscard]] T* get(Handle handle) noexcept {
        Slot* slot = findSlot(handle);
        return slot ? std::addressof(*slot->value) : nullptr;
    }

    [[nodiscard]] const T* get(Handle handle) const noexcept {
        const Slot* slot = findSlot(handle);
        return slot ? std::addressof(*slot->value) : nullptr;
    }

    [[nodiscard]] bool valid(Handle handle) const noexcept {
        return findSlot(handle) != nullptr;
    }

    [[nodiscard]] std::size_t liveCount() const noexcept { return liveCount_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return slots_.size(); }

    void clear() noexcept {
        slots_.clear();
        free_.clear();
        liveCount_ = 0;
    }

private:
    struct Slot final {
        std::optional<T> value;
        Generation generation{1};
    };

    Slot* findSlot(Handle handle) noexcept {
        if (!handle.valid() || handle.index() >= slots_.size()) return nullptr;
        Slot& slot = slots_[handle.index()];
        if (!slot.value || slot.generation != handle.generation()) return nullptr;
        return &slot;
    }

    const Slot* findSlot(Handle handle) const noexcept {
        if (!handle.valid() || handle.index() >= slots_.size()) return nullptr;
        const Slot& slot = slots_[handle.index()];
        if (!slot.value || slot.generation != handle.generation()) return nullptr;
        return &slot;
    }

    std::vector<Slot> slots_;
    std::vector<Index> free_;
    std::size_t liveCount_{0};
};

} // namespace storm::core
