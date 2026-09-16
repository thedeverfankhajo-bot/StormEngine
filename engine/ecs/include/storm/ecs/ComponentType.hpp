#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace storm::ecs {

using ComponentType = std::uint32_t;
constexpr ComponentType kInvalidComponentType = 0xFFFFFFFFu;

namespace detail {

inline ComponentType nextComponentType() noexcept {
    static ComponentType next = 0;
    return next++;
}

template <typename T>
struct ComponentTypeId {
    static ComponentType value() noexcept {
        static const ComponentType id = nextComponentType();
        return id;
    }
};

} // namespace detail

template <typename T>
ComponentType componentType() noexcept {
    static_assert(!std::is_reference_v<T> && !std::is_const_v<T>,
                  "Components must be non-reference, non-const types");
    return detail::ComponentTypeId<T>::value();
}

} // namespace storm::ecs
