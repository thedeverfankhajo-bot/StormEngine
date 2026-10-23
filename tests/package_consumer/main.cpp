#include <storm/core/version.hpp>
#include <storm/math/vector.hpp>

#include <string_view>

int main() {
    const storm::math::Vec2 sample{3.0F, 4.0F};
    return storm::version_string() == std::string_view{"0.1.0"} &&
                   sample.length_squared() == 25.0F
               ? 0
               : 1;
}
