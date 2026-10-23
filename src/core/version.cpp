#include "storm/core/version.hpp"

namespace storm {

Version version() noexcept {
    return kVersion;
}

std::string_view version_string() noexcept {
    return "0.1.0";
}

} // namespace storm
