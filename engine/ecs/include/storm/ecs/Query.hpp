#pragma once

#include "Registry.hpp"
#include <functional>
#include <tuple>
#include <type_traits>

namespace storm::ecs {

namespace detail {

template <typename T>
using ComponentRef = std::add_lvalue_reference_t<std::remove_const_t<T>>;

template <typename... Components>
class Query {
public:
    explicit Query(Registry& registry) noexcept : registry_(registry) {}

    template <typename Func>
    void each(Func&& func) {
        registry_.each<Components...>(std::forward<Func>(func));
    }

private:
    Registry& registry_;
};

} // namespace detail

template <typename... Components>
using Query = detail::Query<Components...>;

template <typename... Components>
Query<Components...> query(Registry& registry) noexcept {
    return Query<Components...>(registry);
}

} // namespace storm::ecs
